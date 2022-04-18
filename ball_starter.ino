
#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include <math.h>

MPU6050 imu; //imu object called, appropriately, imu
TFT_eSPI tft = TFT_eSPI();
// Set up the TFT object

char network[] = "MIT GUEST";
char password[] = "";
const char user[] = "joe"; 
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 2000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

WiFiClient client; //global WiFiClient Secure object

const int LOOP_PERIOD = 40;
const int EXCITEMENT = 1000; //how much force to apply to ball
unsigned long primary_timer; //main loop timer

#define BACKGROUND TFT_BLACK
#define BALL_COLOR TFT_WHITE

const uint8_t BUTTON_PIN = 45; //CHANGE YOUR WIRING TO PIN 16!!! (FROM 19)

struct Vec { //C struct to represent 2D vector (position, accel, vel, etc)
  float x;
  float y;
};


const uint8_t UP = 0; 
const uint8_t DOWN = 1; 
const uint8_t LEFT = 2; 
const uint8_t RIGHT = 3; 

uint32_t direction = UP;


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
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
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
void do_http_request(const char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}        

class Ball {
    Vec position;
    Vec velocity;
    Vec acceleration;
    TFT_eSPI* local_tft; //tft
    int BALL_CLR;
    int BKGND_CLR;
    float MASS; //for starters
    int RADIUS; //radius of ball
    float K_FRICTION;  //friction coefficient
    float K_SPRING;  //spring coefficient
    int LEFT_LIMIT; //left side of screen limit
    int RIGHT_LIMIT; //right side of screen limit
    int TOP_LIMIT; //top of screen limit
    int BOTTOM_LIMIT; //bottom of screen limit
    int DT; //timing for integration
  public:
    Ball(TFT_eSPI* tft_to_use, int dt, int rad = 4, float ms = 1,
         int ball_color = TFT_WHITE, int background_color = BACKGROUND,
         int left_lim = 0, int right_lim = 127, int top_lim = 0, int bottom_lim = 159) {
      position.x = 64; //x position
      position.y = 80; //y position
      velocity.x = 0; //x velocity
      velocity.y = 0; //y velocity
      acceleration.x = 0; //x acceleration
      acceleration.y = 0; //y acceleration
      local_tft = tft_to_use; //our TFT
      BALL_CLR = ball_color; //ball color
      BKGND_CLR = background_color;
      MASS = ms; //for starters
      RADIUS = rad; //radius of ball
      K_FRICTION = 0;  //friction coefficient
      K_SPRING = 0;  //spring coefficient
      LEFT_LIMIT = left_lim + RADIUS; //left side of screen limit
      RIGHT_LIMIT = right_lim - RADIUS; //right side of screen limit
      TOP_LIMIT = top_lim + RADIUS; //top of screen limit
      BOTTOM_LIMIT = bottom_lim - RADIUS; //bottom of screen limit
      DT = dt; //timing for integration
    }
    void step(float x_force = 0, float y_force = 0 ) {
      //YOUR FINAL CODE FROM LAST WEEK/TWO WEEKS AGO
      Serial.print("HIII"); 
      local_tft->drawCircle(position.x, position.y, RADIUS, BKGND_CLR);

      //float friction_x = -K_FRICTION * velocity.x;
      //float friction_y = -K_FRICTION * velocity.y;
      
      float x_force_with_friction = x_force; 
      float y_force_with_friction = y_force;
      acceleration.x = x_force_with_friction/MASS;
      acceleration.y = y_force_with_friction/MASS;
      //integrate to get velocity from current acceleration

      
      velocity.x = velocity.x + 0.001*DT*acceleration.x; //integrate, 0.001 is conversion from milliseconds to seconds
      velocity.y = velocity.y + 0.001*DT*acceleration.y; //integrate!!
      
      
      moveBall();

      local_tft->drawCircle(position.x, position.y, RADIUS, BALL_CLR);
    }

    void reset(int x = 64, int y = 32) {
      position.x = x;
      position.y = y;
      velocity.x = 0;
      velocity.y = 0;
      acceleration.x = 0;
      acceleration.y = 0;
    }
    int getX() {
      return position.x;
    }
    int getY() {
      return position.y;
    }
  private:
    void moveBall() {
      float dx = 0.001 * velocity.x * DT;
      float dy = 0.001 * velocity.y * DT;
      float x = position.x + dx;
      float y = position.y + dy;
      float backward_x = 0.0;
      float backward_y = 0.0;

      
      if (x >= RIGHT_LIMIT) {
        position.x = RIGHT_LIMIT;
      } else if (x<=LEFT_LIMIT){
        position.x = LEFT_LIMIT;
      }
      else{
        position.x = x;
      }
      if (y >= TOP_LIMIT) { 
        position.y = TOP_LIMIT;
      } else if (y<= BOTTOM_LIMIT){
        position.y = BOTTOM_LIMIT;
      }
      else{
        position.y = y;
      }
      
      float angle = atan2(velocity.y, velocity.x);
      if (angle>45 && angle<135){ 
        direction = 0; // UP 
      }
      else if (angle>225 && angle<315){ 
        direction = 1; // DOWN 
      }
      else if (angle>135 && angle<225){ 
        direction = 2; // LEFT
      }
      else if (angle<45 && angle>315){ 
        direction = 3; // RIGHT 
      }



      
      char body[4072]; 
      sprintf(body,"a_x=%f&a_y=%f&v_x=%f&v_y=%f&x_x=%f&x_y=%f&angle=%f&dir=%d",acceleration.x,acceleration.y,velocity.x, velocity.y, position.x, position.y, angle, direction); // HOW TO GET ARTIST
      int body_len = strlen(body); //calculate body length (for header reporting) 
      sprintf(request_buffer,"POST http://608dev-2.net/sandbox/sc/team10/server.py HTTP/1.1\r\n");
      strcat(request_buffer,"Host: 608dev-2.net\r\n"); 
      strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
      sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
      strcat(request_buffer,"\r\n"); //new line from header to body
      strcat(request_buffer,body); //body 
      strcat(request_buffer,"\r\n"); //new line
      //Serial.println(request_buffer); 
      do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);  

      // You should update velocity even when you just barely touch, but not
      // position in that case
      
    }
};


class Game {
    int score;
    Vec food_position;
    int food_half_width; // Like "radius", except it's a square
    int left_limit = 0; //left side of screen limit
    int right_limit = 127; //right side of screen limit
    int top_limit = 0; //top of screen limit
    int bottom_limit = 159; //bottom of screen limit
    int player_radius = 4;
    int difficulty = 100; //decent difficulty
    TFT_eSPI* game_tft; //object point to TFT (screen)
  public:
    Ball player; //make instance of "you"

    Game(TFT_eSPI* tft_to_use, int loop_speed):
      player(tft_to_use, loop_speed, player_radius, 1, TFT_RED, BACKGROUND,
             left_limit, right_limit, top_limit, bottom_limit)
     
    {
      game_tft = tft_to_use;
     
    }
    void step(float x, float y) {
      player.step(x, y);
     
    }

};



void setup() {
  Serial.begin(115200); //for debugging if needed.
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to green foreground, black background

 Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)
  Wire.begin();
  delay(50); //pause to make sure comms get set up
  
  
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
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(0));  //initialize random numbers
  delay(20); //wait 20 milliseconds
  primary_timer = millis();
}

Game game(&tft, LOOP_PERIOD);

void loop() {
  imu.readAccelData(imu.accelCount);//read imu
  float x = imu.accelCount[0] * imu.aRes;
  float y = imu.accelCount[1] * imu.aRes;
  game.step(y , x ); //apply force from imu
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
  //while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  //primary_timer = millis();
} 



