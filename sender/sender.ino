#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>      //Used in support of TFT Display
#include <string.h>   //used for some string handling and processing.
#include <mpu6050_esp32.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE

const int DT = 40;       // milliseconds
const int SCALER = 1000; // how much force to apply to ball

// This is the peer's address
uint8_t broadcastAddress[] = {0x7C, 0xDF, 0xA1, 0x15, 0x3A, 0x14};

esp_now_peer_info_t peerInfo;

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

WireData sendme;

uint32_t primary_timer; // main loop timer

// state variables:
struct Vec position;     // position of ball
struct Vec velocity;     // velocity of ball
struct Vec acceleration; // acceleration of ball

// physics constants:
const float MASS = 1;          // for starters
const int RADIUS = 5;          // radius of ball
const float K_FRICTION = 0.35; // friction coefficient
const float K_SPRING = 0.8;    // spring coefficient

// boundary constants:
const int LEFT_LIMIT = RADIUS;         // left side of screen limit
const int RIGHT_LIMIT = 127 - RADIUS;  // right side of screen limit
const int TOP_LIMIT = RADIUS;          // top of screen limit
const int BOTTOM_LIMIT = 159 - RADIUS; // bottom of screen limit

MPU6050 imu; // imu object called, appropriately, imu

void step(float x_force = 0, float y_force = 0)
{
  float friction_x = -K_FRICTION * velocity.x;
  float friction_y = -K_FRICTION * velocity.y;

  float x_force_with_friction = x_force + friction_x;
  float y_force_with_friction = y_force + friction_y;
  acceleration.x = x_force_with_friction / MASS;
  acceleration.y = y_force_with_friction / MASS;
  // integrate to get velocity from current acceleration
  velocity.x = velocity.x + 0.001 * DT * acceleration.x; // integrate, 0.001 is conversion from milliseconds to seconds
  velocity.y = velocity.y + 0.001 * DT * acceleration.y; // integrate!!

  moveBall();
}

void moveBall()
{
  float dx = 0.001 * velocity.x * DT;
  float dy = 0.001 * velocity.y * DT;
  float x = position.x + dx;
  float y = position.y + dy;
  float backward_x = 0.0;
  float backward_y = 0.0;

  // Backward is how much you need to move backward
  // It is always in the direction you have to go in
  // Which means i.e. for x:
  // x < LEFT_LIMIT -> positive
  // x > RIGHT_LIMIT -> negative
  if (x < LEFT_LIMIT)
    backward_x = LEFT_LIMIT - x;
  if (x > RIGHT_LIMIT)
    backward_x = RIGHT_LIMIT - x;
  if (y < TOP_LIMIT)
    backward_y = TOP_LIMIT - y;
  if (y > BOTTOM_LIMIT)
    backward_y = BOTTOM_LIMIT - y;

  // Note that backward is always in the desired direction
  // It gets smaller in absolute value if you are further from
  // the wall (so if you are going left it gets smaller, if you are
  // going right, then it gets larger)
  if (backward_x != 0)
  {
    float travel_x = x < LEFT_LIMIT ? position.x - LEFT_LIMIT : position.x - RIGHT_LIMIT;
    backward_x *= K_SPRING;
    position.x += backward_x - travel_x;
  }
  else
  {
    position.x = x;
  }
  if (backward_y != 0)
  {
    float travel_y = y < TOP_LIMIT ? position.y - TOP_LIMIT : position.y - BOTTOM_LIMIT;
    backward_y *= K_SPRING;
    position.y += backward_y - travel_y;
  }
  else
  {
    position.y = y;
  }

  // You should update velocity even when you just barely touch, but not
  // position in that case
  if (!(LEFT_LIMIT < x && x < RIGHT_LIMIT))
  {
    velocity.x = -K_SPRING * velocity.x;
  }
  if (!(TOP_LIMIT < y && y < BOTTOM_LIMIT))
  {
    velocity.y = -K_SPRING * velocity.y;
  }
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
  randomSeed(analogRead(0));                              // initialize random numbers
  step(random(-SCALER, SCALER), random(-SCALER, SCALER)); // apply initial force to lower right
  primary_timer = millis();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

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
}

void loop()
{
  // Draw, read imu and send set the position (global)
  tft.fillCircle(position.x, position.y, RADIUS, BACKGROUND);
  imu.readAccelData(imu.accelCount);
  float y = imu.accelCount[0] * imu.aRes;
  float x = imu.accelCount[1] * imu.aRes;

  step(x * SCALER, y * SCALER);
  tft.fillCircle(position.x, position.y, RADIUS, BALL_COLOR);

  memcpy(&(sendme.acceleration), &acceleration, sizeof(Vec));
  memcpy(&(sendme.velocity), &velocity, sizeof(Vec));
  memcpy(&(sendme.position), &position, sizeof(Vec));

  // send the position
  esp_err_t result = esp_now_send(0, (uint8_t *)&sendme, sizeof(WireData));

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