#include <esp_now.h>
#include <stdint.h>
#include <WiFi.h>
#include <SPI.h>      //Used in support of TFT Display
#include <string.h>   //used for some string handling and processing.
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> 

// Unfortunately this nonsense is necessary because Arduino copies the file contents to some
// other folder.
//#define LIB_PATH_ADRIANO
// #define LIB_PATH_SUALEH 
#define LIB_PATH_NATASHA
// #define LIB_PATH_DANIELA

#ifdef LIB_PATH_ADRIANO
#include "/Users/4gate/Documents/git/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_SUALEH
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_NATASHA 
#include "/Users/natashammaniar/Documents/608-Final-Project/arduino/lib.h"
#endif


#ifdef LIB_PATH_DANIELA
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

enum button_state
{
    S0,
    S1,
    S2,
    S3,
    S4
};
class Button
{
public:
    uint32_t S2_start_time;
    uint32_t button_change_time;
    uint32_t debounce_duration;
    uint32_t long_press_duration;
    uint8_t pin;
    uint8_t flag;
    uint8_t button_pressed;
    button_state state;
    Button(int p);
    void read();
    int update();
};

Button::Button(int p)
{
    flag = 0;
    state = S0;
    pin = p;
    S2_start_time = millis();      // init
    button_change_time = millis(); // init
    debounce_duration = 10;
    long_press_duration = 1000;
    button_pressed = 0;
}
void Button::read()
{
    uint8_t button_val = digitalRead(pin);
    button_pressed = !button_val; // invert button
}
int Button::update()
{
    read();
    flag = 0;
    int m = 0;
    switch (state)
    {
    case S0:
        if (button_pressed)
        {
            button_change_time = millis();
            state = S1;
        }
        break;
    case S1:
        m = millis();
        if (button_pressed && m - button_change_time > debounce_duration)
        {
            S2_start_time = m;
            state = S2;
        }
        else if (!button_pressed)
        {
            button_change_time = m;
            state = S0;
        }
        break;
    case S2:
        if (!button_pressed)
        {
            button_change_time = millis();
            state = S4;
        }
        else if (millis() - S2_start_time > long_press_duration)
        {
            state = S3;
        }
        break;
    case S3:
        if (!button_pressed)
        {
            button_change_time = millis();
            state = S4;
        }
        break;
    case S4:
        m = millis();
        if (!button_pressed && m - button_change_time > debounce_duration)
        {
            flag = 1;
            if (button_change_time - S2_start_time > long_press_duration)
            {
                flag = 2;
            }
            state = S0;
        }
        else if (button_pressed)
        {
            if (m - S2_start_time > long_press_duration)
            {
                state = S3;
            }
            else
            {
                state = S2;
            }
            button_change_time = m;
        }
        break;
    }
    return flag;
};


TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

// This is the peer's address
uint8_t broadcastAddress[] = {0x7C, 0xDF, 0xA1, 0x15, 0x3A, 0x14};

WireData info;

uint32_t primary_timer; // main loop timer

MPU6050 imu; // imu object called, appropriately, imu

bool MOVING;

const int MOVE_BUTTON = 39;
Button move_button(MOVE_BUTTON);

WiFiClientSecure client; //global WiFiClient Secure object
WiFiClient client2; //global WiFiClient Secure object 

char*  SERVER = "googleapis.com";  // Server URL
char body[6072]; 
uint32_t timer;
char request_buffer2[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer2[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

char request[IN_BUFFER_SIZE];
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char json_body[JSON_BODY_SIZE];


void unpack_json(char* object_string){
  char* firstchar = strchr(object_string,'{');
  char* lastchar = strrchr(object_string,'}');
  *(lastchar + 1)= '\0';
  DynamicJsonDocument doc(200);
  deserializeJson(doc, firstchar); 
  double lat  = doc["location"]["lat"]; 
  double lng  = doc["location"]["lng"]; 
  
  
  sprintf(request_buffer,"GET http://608dev-2.net/sandbox/sc/team10/server.py?whereami=1&y=%f&x=%f HTTP/1.1\r\n", lat, lng); 
  strcat(request_buffer,"Host: 608dev-2.net\r\n");
  strcat(request_buffer,"\r\n"); //new line from header to body

  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
  Serial.println("RESPONSE BUFFER"); //viewable in Serial Terminal
  Serial.println(response_buffer); //viewable in Serial Terminal
  char* location = response_buffer; 
  
   
  
  info.lat = (float) lat; 
 
  info.lon = (float) lng;

}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup()
{
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  delay(100);
  Serial.begin(115200); // for debugging if needed.
  if (imu.setupIMU(1))
  {
    Serial.println("IMU Connected!");
  }
  else
  {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

  primary_timer = millis();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  //PRINT OUT WIFI NETWORKS NEARBY
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
      uint8_t* cc = WiFi.BSSID(i);
      for (int k = 0; k < 6; k++) {
        Serial.print(*cc, HEX);
        if (k != 5) Serial.print(":");
        cc++;
      }
      Serial.println("");
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);

  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  timer = millis();

  

  esp_now_register_send_cb(OnDataSent);

  Serial.println("DATASENT");
  // register peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // register first peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(MOVE_BUTTON, INPUT_PULLUP);
  MOVING = false;
}

void loop()
{
  
  int offset = sprintf(json_body, "%s", PREFIX);
    int n = WiFi.scanNetworks(); //run a new scan. could also modify to use original scan from setup so quicker (though older info)
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else { 
      int max_aps = max(min(MAX_APS, n), 1);
      for (int i = 0; i < max_aps; ++i) { //for each valid access point
        uint8_t* mac = WiFi.BSSID(i); //get the MAC Address
        offset += wifi_object_builder(json_body + offset, JSON_BODY_SIZE-offset, WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSID(i)); //generate the query
        if(i!=max_aps-1){
          offset +=sprintf(json_body+offset,",");//add comma between entries except trailing.
        }
      }
      sprintf(json_body + offset, "%s", SUFFIX);
      //Serial.println(json_body);
      int len = strlen(json_body);
      // Make a HTTP request:
      Serial.println("SENDING REQUEST");
      request[0] = '\0'; //set 0th byte to null
      offset = 0; //reset offset variable for sprintf-ing
      offset += sprintf(request + offset, "POST https://www.googleapis.com/geolocation/v1/geolocate?key=%s  HTTP/1.1\r\n", API_KEY);
      offset += sprintf(request + offset, "Host: googleapis.com\r\n");
      offset += sprintf(request + offset, "Content-Type: application/json\r\n");
      offset += sprintf(request + offset, "cache-control: no-cache\r\n");
      offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
      offset += sprintf(request + offset, "%s\r\n", json_body);
      
      do_https_request(SERVER, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
      Serial.println("-----------");
      Serial.println(response);
      Serial.println("-----------");
      unpack_json(response);
    }
    
  
  imu.readAccelData(imu.accelCount);
  float y = imu.accelCount[0] * imu.aRes;
  float x = imu.accelCount[1] * imu.aRes;
  
  

  // TODO this is buggy (maybe, maybe not, it is unknown)
  float angle = atan2(y, x) * 57.2957795;
  Serial.println(angle);
  float speed = 100;

  Direction direction;
  if (angle > 45 && angle < 135)
  {
    direction = UP;
   
    tft.println("UP");
  }
  else if (angle > 225 && angle < 315)
  {
    direction = DOWN;
  
    tft.println("DOWN");
  }
  else if (angle > 135 && angle < 225)
  {
    direction = LEFT;
    
    tft.println("LEFT");
  }
  else // if (angle < 45 && angle > 315)
  {
    direction = RIGHT;
    
    tft.println("RIGHT");
  }

  int moveFlag = move_button.update();
  if (moveFlag > 0)
  {
    speed = 0;
    direction = NONE;
    tft.setCursor(0,10,1);
    tft.println("STOP");
  }

  

  info.tilt.x = x;
  info.tilt.y = y;
  info.angle = angle;
  info.speed = speed;
  info.direction = direction;
  body[0] =0;
  sprintf(
      body, "a_x=%f&a_y=%f&v_x=%d&v_y=%d&x_x=%f&x_y=%f&angle=%f&dir=%d&speed=%f",
      // TODO this is not acceleration!
      info.tilt.x,
      info.tilt.y,
      // TODO this needs to be fixed
      0,
      0,
      // TODO this needs to be fixed
      info.lat,
      info.lon,
      info.angle,
      info.direction,
      info.speed);
  Serial.println("BIII");
  
  int body_len = strlen(body); // calculate body length (for header reporting)
  sprintf(request_buffer2, "POST http://608dev-2.net/sandbox/sc/team10/server.py HTTP/1.1\r\n");
  strcat(request_buffer2, "Host: 608dev-2.net\r\n");
  strcat(request_buffer2, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer2 + strlen(request_buffer2), "Content-Length: %d\r\n", body_len); // append string formatted to end of request buffer
  strcat(request_buffer2, "\r\n");                                                       // new line from header to body
  strcat(request_buffer2, body);                                                         // body
  strcat(request_buffer2, "\r\n");                                                       // new line
  Serial.println(request_buffer); 
  do_http_request("608dev-2.net", request_buffer2, response_buffer2, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.println(response_buffer2);
  // send the position
  esp_err_t result = esp_now_send(0, (uint8_t *)&info, sizeof(WireData));

  // do some error checking
  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }


  // Wait number of unit time per frame
  while (millis() - primary_timer < DT)
    ;
  primary_timer = millis();

}
