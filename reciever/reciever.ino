#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>      //Used in support of TFT Display
#include <string.h>   //used for some string handling and processing.

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE

const int DT = 40; // milliseconds

struct Vec
{ // C struct to represent 2D vector (position, accel, vel, etc)
  float x;
  float y;
};

uint32_t primary_timer; // main loop timer

// state variables:
struct Vec position; // position of ball

// physics constants:
const int RADIUS = 5; // radius of ball

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  tft.fillCircle(position.x, position.y, RADIUS, BACKGROUND);
  memcpy(&position, incomingData, sizeof(Vec));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(position.x);
  Serial.print("y: ");
  Serial.println(position.y);
  Serial.println();
  tft.fillCircle(position.x, position.y, RADIUS, BALL_COLOR);
}

void setup()
{
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  delay(100);
  Serial.begin(115200); // for debugging if needed.

  // Set device as a Wi-Fi Station
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

  position.x = 10;
  position.y = 10;
}

void loop()
{
  // Draw based on the position (which should be updated by the sender)
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
  //tft.fillScreen(BACKGROUND);

  // Wait number of unit time per frame: might want to change this on the reciever side
  while (millis() - primary_timer < DT)
    ;
  primary_timer = millis();
}
