// ArduCAM Mini demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with ArduCAM ESP32 2MP/5MP camera.
// This demo was made for ArduCAM ESP32 2MP/5MP Camera.
// It can take photo and send to the Web.
// It can take photo continuously as video streaming and send to the Web.
// The demo sketch will do the following tasks:
// 1. Set the camera to JPEG output mode.
// 2. if server receives "GET /capture",it can take photo and send to the Web.
// 3.if server receives "GET /stream",it can take photo continuously as video
// streaming and send to the Web.

// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM ESP32 2MP/5MP camera
// and use Arduino IDE 1.8.1 compiler or above

#include <WiFi.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP32WebServer.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
#include "camera_support.h"
#include <Arduino.h>
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
#include "/Users/natashammaniar/Documents/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_DANIELA
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

#if !(defined ESP32)
#error Please select the ArduCAM ESP32 UNO board in the Tools/Board
#endif
// This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined(OV2640_MINI_2MP) || defined(OV5640_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(ARDUCAM_SHIELD_V2) && (defined(OV2640_CAM) || defined(OV5640_CAM) || defined(OV5642_CAM))))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

// set GPIO17 as the slave select :

uint32_t primary_timer; // main loop timer

WireData info;

int motor1Pin1 = 38;
int motor1Pin2 = 39;

int motor2Pin1 = 40;
int motor2Pin2 = 41;

int enable1Pin = 42;
int enable2Pin = 33;

int motorSpeedA = 0;
int motorSpeedB = 0;

const int freq = 30000;
const int pwmChannel = 0;
const int pwmChannel2 = 1;
const int resolution = 8;
int dutyCycle = 200;

const int CS = 34;
// uint32_t primary_timer; // main loop timer
// char request_buffer2[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
// char response_buffer2[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
// const int RESPONSE_TIMEOUT = 6000;     // ms to wait for response from host

const int CAM_POWER_ON = 10;
int id;

#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
ArduCAM myCAM(OV2640, CS);
#elif defined(OV5640_MINI_5MP_PLUS) || defined(OV5640_CAM)
ArduCAM myCAM(OV5640, CS);
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
ArduCAM myCAM(OV5642, CS);
#endif

// you can change the value of wifiType to select Station or AP mode.
// Default is AP mode.
int wifiType = 1; // 0:Station  1:AP
char holder[9000] = {0};
// AP mode configuration
// Default is arducam_esp8266.If you want,you can change the AP_aaid  to your favorite name
const char *AP_ssid = "arducam_esp32";
// Default is no password.If you want to set password,put your password here
const char *AP_password = NULL;

// Station mode you should put your ssid and password
// const char *ssid = "6s08"; // Put your SSID here
// const char *password = "iesc6s08"; // Put your PASSWORD here

static const size_t bufferSize = 2048;
static uint8_t buffer[bufferSize] = {0xFF};
uint8_t temp = 0, temp_last = 0;
int i = 0;
bool is_header = false;
char body[9000];

// ESP32WebServer server(80);
WiFiClient client; // global WiFiClient Secure object

void start_capture()
{
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  // camCapture(myCAM);
}

const int img_buffer_size = 8000;
char fullimg[img_buffer_size];

void moveCar(uint8_t direction, uint8_t speed)
{
  if (direction == DOWN)
  {
    //Serial.println("DOWNNNN");
    motorSpeedA = speed;
    motorSpeedB = speed;
    ledcWrite(pwmChannel, motorSpeedA);   
    ledcWrite(pwmChannel2, motorSpeedB); 
    //analogWrite(enable1Pin, motorSpeedA);
    //analogWrite(enable2Pin, motorSpeedB);
    moveBack();
  }
  if (direction == UP)
  {

    motorSpeedA = speed;
    motorSpeedB = speed;
    //analogWrite(enable1Pin, motorSpeedA);
    //analogWrite(enable2Pin, motorSpeedB);
    ledcWrite(pwmChannel, motorSpeedA);   
    ledcWrite(pwmChannel2, motorSpeedB); 
    moveForward();
  }

  if (direction == RIGHT)
  { // RIGHT
    int changex = speed;
    motorSpeedA = motorSpeedA - changex;
    motorSpeedB = motorSpeedB + changex;
    //analogWrite(enable1Pin, motorSpeedA);
    //analogWrite(enable2Pin, motorSpeedB);
    ledcWrite(pwmChannel, motorSpeedA);   
    ledcWrite(pwmChannel2, motorSpeedB); 

    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, HIGH);
  }

  if (direction == LEFT)
  { // LEFT
    int changex = speed;
    motorSpeedA = motorSpeedA - changex;
    motorSpeedB = motorSpeedB + changex;
    //analogWrite(enable1Pin, motorSpeedA);
    //analogWrite(enable2Pin, motorSpeedB);
    ledcWrite(pwmChannel, motorSpeedA);   
    ledcWrite(pwmChannel2, motorSpeedB); 
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, LOW);
  }

  if (direction == NONE)
  {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, LOW);
  }

  if (motorSpeedA < 0)
  {
    motorSpeedA = 0;
  }
  if (motorSpeedB > 255)
  {
    motorSpeedB = 255;
  }
}

void moveBack()
{

  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void moveForward()
{
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  //Serial.println("HI");
  memcpy(&info, incomingData, sizeof(WireData));
  
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(info.tilt.x);
  Serial.print("y: ");
  Serial.println(info.tilt.y);
  Serial.println();
  
}

void camCapture(ArduCAM myCAM)
{
  memset(fullimg, 0, img_buffer_size);

  uint32_t fifoLen = myCAM.read_fifo_length();
  if (fifoLen >= MAX_FIFO_SIZE) // 8M
    Serial.println(F("Over size."));
  if (fifoLen == 0) // 0 kb
    Serial.println(F("Size is 0."));

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  // if (!client.connected()) return;

  int realLen = 0; // FFD8 for start and FFD9 for end
  uint8_t prev = SPI.transfer(0x00);
  bool foundStart = false;
  bool complete = false;
  int i = 0;
  while (true)
  {
    uint8_t current = SPI.transfer(0x00);
    if (!foundStart)
    {
      if (prev == 0xFF && current == 0xD8)
      { // found the start of the image!
        Serial.println("found start of image!");
        foundStart = true;
        fullimg[0] = 0xFF;
        fullimg[1] = 0xD8;
        realLen = 2;
      }
    }
    else
    {
      fullimg[realLen++] = current;
      if (realLen >= img_buffer_size)
      {
        // Serial.println("went too far... quitting");
        break;
      }
      if (prev == 0xFF && current == 0xD9)
      { // found the end of the image!
        Serial.println("found end of image!");
        complete = true;
        break;
      }
    }
    prev = current;
    if (i >= fifoLen)
    {
      // Serial.println("breaking because reached end of fifo");
      break;
    }
    i++;
  }
  myCAM.CS_HIGH();
  if (!complete)
  {
    // Serial.println("incomplete image, returning");
    return;
  }
  Serial.printf("Got full image! len = %d\n", realLen);

  // String response = "HTTP/1.1 200 OK\r\n";
  // response += "Content-Type: image/jpeg\r\n";
  // response += "Content-len: " + String(realLen) + "\r\n\r\n";

  // client.write(&fullimg[0], realLen);
  //  base 64 encode fullimg

  base64_encode(holder, fullimg, realLen);

  // POST request
  body[0] = 0;
  sprintf(
      body, "{\"fullimg\":\"data:image/gif;base64,%s\"}", &holder[0]);
  int body_len = strlen(body);
  Serial.println(body);
  sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team10/server.py?camera=1 HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "Content-Type: application/json\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); // append string formatted to end of request buffer
  strcat(request_buffer, "\r\n");                                                       // new line from header to body
  strcat(request_buffer, body);                                                         // body
  strcat(request_buffer, "\r\n");                                                       // new line
  // Serial.println(request_buffer2);
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.println(response_buffer);

  // server.sendContent(response);
  //  decode base 64 (load html from base64 --)
}

void serverCapture()
{
  delay(1000);
  start_capture();
  Serial.println(F("CAM Capturing"));

  int total_time = 0;

  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    ;
  total_time = millis() - total_time;
  Serial.print(F("capture total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);

  total_time = 0;

  Serial.println(F("CAM Capture Done."));
  total_time = millis();
  camCapture(myCAM);
  total_time = millis() - total_time;
  Serial.print(F("send total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);
  Serial.println(F("CAM send Done."));
}

uint8_t vid, pid;

void logMemory()
{
  log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
}

void setup()
{

  

  Serial.begin(115200); // for debugging if needed.

  // Motor pins and enable pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable2Pin, OUTPUT);

  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);

  ledcSetup(pwmChannel2, freq, resolution);
  ledcAttachPin(enable2Pin, pwmChannel2);

  

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

  // if using regular connection use line below:
  WiFi.begin(network, password);
  // if using channel/mac specification for crowded bands use the following:
  // WiFi.begin(network, password, channel, bssid);

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
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str(), WiFi.SSID().c_str());
    delay(500);
  }
  else
  { // if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }

  uint8_t temp;
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  pinMode(CAM_POWER_ON, OUTPUT);
  digitalWrite(CAM_POWER_ON, HIGH);
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println(F("ArduCAM Start!"));

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); // 4MHz

  // Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println(F("SPI1 interface Error!"));
    while (1)
      ;
  }

  // Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println(F("SPI1 interface Error!"));
    while (1)
      ;
  }
#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
  // Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42)))
    Serial.println(F("Can't find OV2640 module!"));
  else
    Serial.println(F("OV2640 detected."));
#elif defined(OV5640_MINI_5MP_PLUS) || defined(OV5640_CAM)
  // Check if the camera module type is OV5640
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x40))
    Serial.println(F("Can't find OV5640 module!"));
  else
    Serial.println(F("OV5640 detected."));
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
  // Check if the camera module type is OV5642
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42))
  {
    Serial.println(F("Can't find OV5642 module!"));
  }
  else
    Serial.println(F("OV5642 detected."));
#endif

  // Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
  myCAM.OV2640_set_JPEG_size(OV2640_160x120);
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
  // Check if the camera module type is OV5642
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42))
  {
    Serial.println(F("Can't find OV5642 module!"));
  }
  else
    Serial.println(F("OV5642 detected."));
#endif

  // Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
#if defined(OV2640_MINI_2MP) || defined(OV2640_CAM)
  myCAM.OV2640_set_JPEG_size(OV2640_160x120);
#elif defined(OV5640_MINI_5MP_PLUS) || defined(OV5640_CAM)
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK); // VSYNC is active HIGH
  myCAM.OV5640_set_JPEG_size(OV5640_320x240);
#elif defined(OV5642_MINI_5MP_PLUS) || defined(OV5642_MINI_5MP) || defined(OV5642_MINI_5MP_BIT_ROTATION_FIXED) || (defined(OV5642_CAM))
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK); // VSYNC is active HIGH
  myCAM.OV5640_set_JPEG_size(OV5642_320x240);
#endif

  myCAM.clear_fifo_flag();
  
}

void loop()
{
  // server.handleClient();

  // Wait number of unit time per frame: might want to change this on the reciever side
  while (millis() - primary_timer < 200)
    ;
  primary_timer = millis();
  Serial.println(digitalRead(enable1Pin));
  esp_now_register_recv_cb(OnDataRecv);
  //moveCar(DOWN,1);
  
  moveCar(info.direction, info.speed );
  //while (millis() - primary_timer < 20000) {// every 20 seconds take new picture
  serverCapture();
  //}
  primary_timer = millis();
}

void do_http_request(char *host, char *request, char *response, uint16_t response_size, uint16_t response_timeout, uint8_t serial)
{
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
