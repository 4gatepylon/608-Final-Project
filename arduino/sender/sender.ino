#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>      //Used in support of TFT Display
#include <string.h>   //used for some string handling and processing.
#include <mpu6050_esp32.h>

// Unfortunately this nonsense is necessary because Arduino copies the file contents to some
// other folder.
#define LIB_PATH_ADRIANO
// #define LIB_PATH_SUALEH
// #define LIB_PATH_NATASHA
// #define LIB_PATH_DANIELA

#ifdef LIB_PATH_ADRIANO
#include "/Users/4gate/Documents/git/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_SUALEH
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_NATASHA
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

#ifdef LIB_PATH_DANIELA
#include "/YourFullPathHere/608-Final-Project/arduino/lib.h"
#endif

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

// This is the peer's address
uint8_t broadcastAddress[] = {0x7C, 0xDF, 0xA1, 0x15, 0x3A, 0x14};

WireData info;

uint32_t primary_timer; // main loop timer

MPU6050 imu; // imu object called, appropriately, imu

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
  float y = imu.accelCount[0] * imu.aRes;
  float x = imu.accelCount[1] * imu.aRes;
  tft.println(x);
  tft.println(y);

  // TODO this is buggy
  float angle = atan2(y, x) * 57.2957795;
  Serial.println(angle);
  float speed = 23;

  Direction direction;
  if (angle > 45 && angle < 135)
  {
    direction = UP;
  }
  else if (angle > 225 && angle < 315)
  {
    direction = DOWN;
  }
  else if (angle > 135 && angle < 225)
  {
    direction = LEFT;
  }
  else // if (angle < 45 && angle > 315)
  {
    direction = RIGHT;
  }

  info.tilt.x = x;
  info.tilt.y = y;
  info.angle = angle;
  info.speed = speed;
  info.direction = direction;

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