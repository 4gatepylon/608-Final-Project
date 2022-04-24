#include <esp_now.h>
#include <WiFi.h>
#include <string.h>

// Unfortunately this nonsense is necessary because Arduino copies the file contents to some
// other folder.
// # define LIB_PATH_ADRIANO
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
#include "/Users/natashamaniar/Documents/git/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_DANIELA
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

uint32_t primary_timer; // main loop timer

WireData info;

int motor1Pin1 = 38; 
int motor1Pin2 = 39; 

int motor2Pin1 = 40; 
int motor2Pin2 = 41; 

int enable1Pin = 37; 
int enable2Pin = 42; 

const uint8_t IDLE = 0; // press for long time and posts to server
const uint8_t UP = 1; // press for long time and posts to server
const uint8_t DOWN = 2; // press for long time and posts to server
const uint8_t button_state = IDLE;
const uint8_t MAX_SPEED = 23;
int motorSpeedA = 0;
int motorSpeedB = 0;

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

void setup() {
  // sets the pins as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable2Pin, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin, pwmChannel);
  ledcAttachPin(enable2Pin, pwmChannel);


  Serial.begin(115200);

  // testing 
  Serial.print("Testing DC Motor...");
}

/*
MOVE BASED ON DIRECTION
*/
void moveCar(uint8_t direction, uint8_t speed){ 
    if (direction==1){
        
        motorSpeedA = map(speed, MAX_SPEED, 0, 0, 255); 
        motorSpeedB = map(speed, MAX_SPEED, 0, 0, 255); 
        analogWrite(enable1Pin, motorSpeedA);
        analogWrite(enable2Pin, motorSpeedB);
        moveBack();
    }
    if (direction==0){

        motorSpeedA = map(speed, 550, 1023, 0, 255);  // ASK ABOUT PWM CYCLE 
        motorSpeedB = map(speed, 550, 1023, 0, 255);  //ASK ABOUT PWM CYCLE
        analogWrite(enable1Pin, motorSpeedA);
        analogWrite(enable2Pin, motorSpeedB);
        moveForward(); 
    }

    if (direction==2){ // RIGHT
        int changex = map(speed, MAX_SPEED, 0, 0, 255); 
        motorSpeedA = motorSpeedA - changex;
        motorSpeedB = motorSpeedB + changex; 
        analogWrite(enable1Pin, motorSpeedA);
        analogWrite(enable2Pin, motorSpeedB);
        
        digitalWrite(motor1Pin1, LOW);
        digitalWrite(motor1Pin2, LOW); 
        digitalWrite(motor2Pin1, LOW);
        digitalWrite(motor2Pin2, HIGH); 

    }

    if (direction==3){ // LEFT 
        int changex = map(speed, 550, 1023, 0, 255); 
        motorSpeedA = motorSpeedA - changex;
        motorSpeedB = motorSpeedB + changex; 
        analogWrite(enable1Pin, motorSpeedA);
        analogWrite(enable2Pin, motorSpeedB);
        digitalWrite(motor1Pin1, LOW);
        digitalWrite(motor1Pin2, HIGH); 
        digitalWrite(motor2Pin1, LOW);
        digitalWrite(motor2Pin2, LOW); 

    }

    if (motorSpeedA < 0) {
      motorSpeedA = 0;
    }
    if (motorSpeedB > 255) {
      motorSpeedB = 255;
    }
    
}

void moveBack(){
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW); 
    digitalWrite(motor2Pin1, HIGH);
    digitalWrite(motor2Pin2, LOW); 
}


void moveForward(){
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH); 
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, HIGH); 
}


// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&info, incomingData, sizeof(WireData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(info.tilt.x);
  Serial.print("y: ");
  Serial.println(info.tilt.y);
  Serial.println();
}

void setup()
{
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
}

void loop()
{
  // TODO use google api to get the lat and long and our labwork to get the location on the map
  char body[4072];
  sprintf(
      body, "a_x=%f&a_y=%f&v_x=%f&v_y=%f&x_x=%f&x_y=%f&angle=%f&dir=%d&speed=%f",
      // TODO this is not acceleration!
      info.tilt.x,
      info.tilt.y,
      // TODO this needs to be fixed
      0,
      0,
      // TODO this needs to be fixed
      0,
      0,
      info.angle,
      info.direction,
      info.speed);

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

  moveCar(info.direction,info.speed); 
  // Wait number of unit time per frame: might want to change this on the reciever side
  while (millis() - primary_timer < DT)
    ;
  primary_timer = millis();
}
