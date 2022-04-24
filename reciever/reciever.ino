#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>      //Used in support of TFT Display
#include <string.h>   //used for some string handling and processing.
// #include <math.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

char network[] = "MIT GUEST";
char password[] = "";
const char user[] = "";
const int RESPONSE_TIMEOUT = 6000;     // ms to wait for response from host
const int POSTING_PERIOD = 6000;       // periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 2000;  // size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 2000; // size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE];   // char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; // char array buffer to hold HTTP response
float mappedspeed = 0.0;
float speed = 0.0;
float angle = 0.0;
int flag = 0; //start or stop (start=1), stop =0

const uint8_t BUTTON1 = 39; // press for long time and posts to server
const uint8_t IDLE = 0; // press for long time and posts to server
const uint8_t PRESSED = 4; // press for long time and posts to server
const uint8_t UNPRESSED = 5; // press for long time and posts to server
int button_state = IDLE;
const uint8_t MAX_SPEED = 23;

uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

uint8_t freq = 200;
uint8_t resolution = 8;
const uint8_t RED_PWM = 2; 
const  uint8_t GREEN_PWM = 3; 
const uint8_t BLUE_PWM = 4; 

const uint8_t LED2_PWM = 12; 



#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE

const int DT = 40; // milliseconds

struct Vec
{ // C struct to represent 2D vector (position, accel, vel, etc)
  float x;
  float y;
};

struct WireData
{
  Vec position;
  Vec acceleration;
  Vec velocity;
};

WireData recvme;

// Used to send analytics to server
const uint8_t UP = 0;
const uint8_t DOWN = 1;
const uint8_t LEFT = 2;
const uint8_t RIGHT = 3;

uint32_t direction = UP;

uint32_t primary_timer; // main loop timer

// physics constants:
const int RADIUS = 5; // radius of ball

/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c:
 *    uint16_t buff_size: size of buffer buff
 *
 * Return value:
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
uint8_t char_append(char *buff, char c, uint16_t buff_size)
{
  int len = strlen(buff);
  if (len > buff_size)
    return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
 * do_http_request Function:
 * Arguments:
 *    const char* host: null-terminated char-array containing host to connect to
 *    char* request: null-terminated char-arry containing properly formatted HTTP request
 *    char* response: char-array used as output for function to contain response
 *    uint16_t response_size: size of response buffer (in bytes)
 *    uint16_t response_timeout: duration we'll wait (in ms) for a response from server
 *    uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
 * Return value:
 *    void (none)
 */
void do_http_request(const char *host, char *request, char *response, uint16_t response_size, uint16_t response_timeout, uint8_t serial)
{
  WiFiClient client; // instantiate a client object
  if (client.connect(host, 80))
  { // try to connect to host on port 80
    if (serial)
      Serial.print(request); // Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); // Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected())
    { // while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial)
        Serial.println(response);
      if (strcmp(response, "\r") == 0)
      { // found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout)
        break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available())
    { // read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial)
      Serial.println(response);
    client.stop();
    if (serial)
      Serial.println("-----------");
  }
  else
  {
    if (serial)
      Serial.println("connection failed :/");
    if (serial)
      Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  tft.fillCircle(recvme.position.x, recvme.position.y, RADIUS, BACKGROUND);
  memcpy(&recvme, incomingData, sizeof(WireData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(recvme.position.x);
  Serial.print("y: ");
  Serial.println(recvme.position.y);
  Serial.println();
  tft.fillCircle(recvme.position.x, recvme.position.y, RADIUS, BALL_COLOR);
}

void setup()
{
  pinMode(BUTTON1, INPUT_PULLUP); 
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  delay(100);
  Serial.begin(115200); // for debugging if needed.

  // Set device as a Wi-Fi Station (NOT A ACCESS POINT)
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  // Unclear whether this will work (it's from class code, but we are a wifi station, so...)
  // https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
      uint8_t *cc = WiFi.BSSID(i);
      for (int k = 0; k < 6; k++)
      {
        Serial.print(*cc, HEX);
        if (k != 5)
          Serial.print(":");
        cc++;
      }
      Serial.println("");
    }
  }
  delay(100); // wait a bit (100 ms)

  WiFi.begin(network, password);
  uint8_t count = 0; // count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12)
  {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected())
  { // if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  }
  else
  { // if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }

  pinMode(2, OUTPUT);
  ledcSetup(RED_PWM, freq, resolution); 
  ledcAttachPin(2, RED_PWM);
// start = 1 , stop 0 
  pinMode(3, OUTPUT);
  ledcSetup(GREEN_PWM, freq, resolution); 
  ledcAttachPin(3, GREEN_PWM);

  pinMode(4, OUTPUT);
  ledcSetup(BLUE_PWM, freq, resolution); 
  ledcAttachPin(4, BLUE_PWM);

  pinMode(12, OUTPUT);
  ledcSetup(LED2_PWM, freq, resolution); 
  ledcAttachPin(12, LED2_PWM);

}

void stopcar(uint8_t input){ 
  switch(button_state){  
    case IDLE: 
      if (input ==0) {  
        button_state = PRESSED; 
      }
      else{
        if (flag ==1){
          Serial.println("Speed");
          Serial.println(speed);
          Serial.println("mapped speed");
          mappedspeed = map(speed, 470, 0, 0, 255); // in future have use set the max speed
          Serial.println(mappedspeed);
          ledcWrite(LED2_PWM, 255-mappedspeed); // in future have use set the max speed
        }
        
      }
      break;
    case PRESSED:  
      if (input==0){  
          button_state = PRESSED; 
      } 
      else if (input==1){ 
          // stop both motors
        flag = !flag;
        if (flag ==0){
          //stop car then
          ledcWrite(RED_PWM, 255);
          ledcWrite(GREEN_PWM, 255);
          ledcWrite(BLUE_PWM, 255); 
          ledcWrite(LED2_PWM, 255); // STOPS to start again 
        }
        button_state = UNPRESSED;  

      }
      break;
    case UNPRESSED:
      if (input==1){ 

        if (flag ==1){
          Serial.println("Speed");
          Serial.println(speed);
          Serial.println("mapped speed");
          mappedspeed = map(speed, 470, 0, 0, 255); // in future have use set the max speed
          Serial.println(mappedspeed);
          ledcWrite(LED2_PWM, 255-mappedspeed); // in future have use set the max speed
        }
        button_state = IDLE;
      }
      else if (input==0){
        button_state = PRESSED;
      }
      break;

}
}

void calculateSpeed(){
  angle = atan2(recvme.velocity.y, recvme.velocity.x) * 57.2957795;
  Serial.println(angle); 
  speed = sqrt(recvme.velocity.y * recvme.velocity.y + recvme.velocity.x * recvme.velocity.x);
  if (angle > 45 && angle < 135)
  {
    direction = 0; // UP
    ledcWrite(RED_PWM, 0); /// in future have use set the max speed
    ledcWrite(GREEN_PWM, 255); /// in future have use set the max speed
    ledcWrite(BLUE_PWM, 255); // / in future have use set the max speed
    
  }
  else if (angle > 225 && angle < 315)
  {
    direction = 1; // DOWN
    ledcWrite(RED_PWM, 255); /// in future have use set the max speed
    ledcWrite(GREEN_PWM, 0); /// in future have use set the max speed
    ledcWrite(BLUE_PWM, 255); // / in future have use set the max speed
  }
  else if (angle > 135 && angle < 225)
  {
    direction = 2; // LEFT
    ledcWrite(RED_PWM, 255); /// in future have use set the max speed
    ledcWrite(GREEN_PWM, 255); /// in future have use set the max speed
    ledcWrite(BLUE_PWM, 0); // / in future have use set the max speed
  }
  else // if (angle < 45 && angle > 315)
  {
    direction = 3; // RIGHT
    ledcWrite(RED_PWM, 0); /// in future have use set the max speed
    ledcWrite(GREEN_PWM, 0); /// in future have use set the max speed
    ledcWrite(BLUE_PWM, 255); // / in future have use set the max speed
  }

  

  

}
void loop()
{
  stopcar(BUTTON1); 
  calculateSpeed(); 
  /*
  based on speed 
  */ 
  //ledcWrite(GREEN_PWM, 255-green); /// in future have use set the max speed
  //ledcWrite(BLUE_PWM, 255-blue); // / in future have use set the max speed

  char body[4072];
  sprintf(
      body, "a_x=%f&a_y=%f&v_x=%f&v_y=%f&x_x=%f&x_y=%f&angle=%f&dir=%d&speed=%f",
      recvme.acceleration.x,
      recvme.acceleration.y,
      recvme.velocity.x,
      recvme.velocity.y,
      recvme.position.x,
      recvme.position.y,
      angle,
      direction,
      speed);
  /*
  Serial.println("SPEED");
  Serial.println(speed);
  Serial.println("DIRECTION");
  if (direction==0){
    Serial.println("UP");
  }
  if (direction==1){
    Serial.println("DOWN");
  }
  if (direction==2){
    Serial.println("LEFT");
  }
  if (direction==3){
    Serial.println("RIGHT");
  }
  
  
  int body_len = strlen(body); // calculate body length (for header reporting)
  sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team10/server.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); // append string formatted to end of request buffer
  strcat(request_buffer, "\r\n");                                                       // new line from header to body
  strcat(request_buffer, body);                                                         // body
  strcat(request_buffer, "\r\n");                                                       // new line
  // Serial.println(request_buffer);
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  // tft.fillScreen(BACKGROUND);
*/
  // Wait number of unit time per frame: might want to change this on the reciever side
  while (millis() - primary_timer < DT)
    ;
  primary_timer = millis();
  
}
