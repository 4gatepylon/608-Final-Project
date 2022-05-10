#include <esp_now.h>
#include <stdint.h>
#include <WiFi.h>
#include <SPI.h>    //Used in support of TFT Display
#include <string.h> //used for some string handling and processing.
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

// #define GPS_TX_PIN 18
// #define GPS_RX_PIN 17

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

// Let me know where I ammm
// HardwareSerial gps(2);

// This is the peer's address
uint8_t broadcastAddress[] = {0x7C, 0xDF, 0xA1, 0x15, 0x3A, 0x14};

WireData info;

uint32_t esp_now_timer;
uint32_t wifi_scan_timer;
const int times_in_state_to_switch = 15;
int times_in_state;
float prev_speed;

MPU6050 imu; // imu object called, appropriately, imu

bool MOVING;
bool AUTO;

// const int GPS_BUFFER_LENGTH = 200;        // size of char array we'll use for
// char gps_buffer[GPS_BUFFER_LENGTH] = {0}; // dump chars into the

const int MOVE_BUTTON = 39;
Button move_button(MOVE_BUTTON);

const int AUTO_BUTTON = 45;
Button auto_button(AUTO_BUTTON);

WiFiClientSecure client; // global WiFiClient Secure object
WiFiClient client2;      // global WiFiClient Secure object

char *SERVER = "googleapis.com"; // Server URL
char body[6072];
uint32_t timer;
char request_buffer2[IN_BUFFER_SIZE];   // char array buffer to hold HTTP request
char response_buffer2[OUT_BUFFER_SIZE]; // char array buffer to hold HTTP response

char request_buffer3[500];
char response_buffer3[50]; // char array buffer to hold HTTP request
//char json_body[JSON_BODY_SIZE];



// callback when data is sent on esp_now
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18];
  //Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  //Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

Direction get_direction(float x, float y)
{
  // Works by looking at three cases:
  // 1. You are in a square in the middle: should not move because not enough tilt
  // 2. You are in a cross in which one side is more tilted than the other by a lot
  //    (in this case just move in that direction)
  // 3. You are in one of the remaining quadrants
  //    (in this case pick the larger one, but understand that it'll be a little but more noisy)
  float x_pos = x > 0 ? x : -x;
  float y_pos = y > 0 ? y : -y;
  if (x_pos < MIN_MOVE_TILT_X && y_pos < MIN_MOVE_TILT_Y)
  {
    return NONE;
  }
  else if (x_pos < MIN_MOVE_TILT_X)
  {
    return y > 0 ? UP : DOWN;
  }
  else if (y_pos < MIN_MOVE_TILT_Y)
  {
    return x > 0 ? RIGHT : LEFT;
  }
  else
  {
    if (x > 0 && y > 0)
    {
      // Top right
      return x_pos > y_pos ? RIGHT : UP;
    }
    else if (x > 0 && y < 0)
    {
      // Bottom right
      return x_pos > y_pos ? RIGHT : DOWN;
    }
    else if (x < 0 && y > 0)
    {
      // Top left
      return x_pos > y_pos ? LEFT : UP;
    }
    else
    {
      // Bottom left
      return x_pos > y_pos ? LEFT : DOWN;
    }
  }
}

void print_direction(Direction dir)
{
  // Add the spaces to deal with overwriting bugs
  switch (dir)
  {
  case UP:
    tft.println("UP    ");
    break;
  case DOWN:
    tft.println("DOWN  ");
    break;
  case LEFT:
    tft.println("LEFT  ");
    break;
  case RIGHT:
    tft.println("RIGHT ");
    break;
  case NONE:
    tft.println("NONE  ");
    break;
  }
}

float get_speed(Direction direction, float x, float y)
{
  // This should be dealt with beforehand since it has the case
  // where x or y is less than the MIN_MOVE_TILT
  float x_pos = x > 0 ? x : -x;
  float y_pos = y > 0 ? y : -y;
  if (x_pos < MIN_MOVE_TILT_X && y_pos < MIN_MOVE_TILT_Y)
  {
    return 0;
  }

  // Use simple linear interpolation
  float speed_range = MAX_SPEED - MIN_SPEED;

  // Get the speed by linearly interpolating, then binning
  float speed = 0;
  if (direction == UP || direction == DOWN)
  {
    float tilt_range = MAX_MOVE_TILT_Y - MIN_MOVE_TILT_Y;
    float percent = (y_pos - MIN_MOVE_TILT_Y) / tilt_range;
    speed = percent * speed_range;
  }
  else if (direction == LEFT || direction == RIGHT)
  {
    float tilt_range = MAX_MOVE_TILT_X - MIN_MOVE_TILT_X;
    float percent = (x_pos - MIN_MOVE_TILT_X) / tilt_range;
    speed = percent * speed_range;
  }
  // I guess you could make this faster with binary search...
  for (int i = 0; i < NUM_SPEED_BINS; i++)
  {
    if (speed <= SPEED_BINS[i])
    {
      speed = SPEED_BINS[i];
      break;
    }
  }
  if (speed > MAX_SPEED)
  {
    // This should be unreachable
    speed = MAX_SPEED;
  }
  times_in_state += 1;
  if (speed == prev_speed)
  {
    return speed;
  }
  else
  {
    if (times_in_state < times_in_state_to_switch)
    {
      return prev_speed;
    }
    else
    {
      prev_speed = speed;
      times_in_state = 0;
      return speed;
    }
  }
}

float get_angle(float x, float y)
{
  // I do not understand this magic number
  return atan2(y, x) * 57.2957795;
}

// void displayAllGPS()
// {
//   // Serial.println("******************** GPS ********************");
//   // Serial.printf("GPS Avail: %d\n", gps.available());
//   while (gps.available())
//   {                                                          // If anything comes in Serial1 (pins 0 & 1)
//     gps.readBytesUntil('\n', gps_buffer, GPS_BUFFER_LENGTH); // read it and send it out Serial (USB)
//     // Serial.println(gps_buffer);
//   }
//   // Serial.println("****************** END GPS ******************\n");
// }

// This is where we run the wifi server loop which is really slow
void wifi_server_loop(void *parameter)
{
  for (;;)
  {
    //if (millis() - wifi_scan_timer > DT_WIFI)
    //{
      //   Serial.println("***DOING WIFI SCAN!\n");
      //   int offset = sprintf(json_body, "%s", PREFIX);
      //   // run a new scan. could also modify to use original scan from setup so quicker (though older info)
      //   int n = WiFi.scanNetworks(); // This is SUPER slow
      //   Serial.println("scan done");
      //   if (n == 0)
      //   {
      //     Serial.println("no networks found");
      //   }
      //   else
      //   {
      //     int max_aps = max(min(MAX_APS, n), 1);
      //     for (int i = 0; i < max_aps; ++i)
      //     {                                                                                                                           // for each valid access point
      //       uint8_t *mac = WiFi.BSSID(i);                                                                                             // get the MAC Address
      //       offset += wifi_object_builder(json_body + offset, JSON_BODY_SIZE - offset, WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSID(i)); // generate the query
      //       if (i != max_aps - 1)
      //       {
      //         offset += sprintf(json_body + offset, ","); // add comma between entries except trailing.
      //       }
      //     }
      //     sprintf(json_body + offset, "%s", SUFFIX);
      //     // Serial.println(json_body);
      //     int len = strlen(json_body);
      //     // Make a HTTP request:
      //     Serial.println("SENDING REQUEST");
      //     request[0] = '\0'; // set 0th byte to null
      //     offset = 0;        // reset offset variable for sprintf-ing
      //     offset += sprintf(request + offset, "POST https://www.googleapis.com/geolocation/v1/geolocate?key=%s  HTTP/1.1\r\n", API_KEY);
      //     offset += sprintf(request + offset, "Host: googleapis.com\r\n");
      //     offset += sprintf(request + offset, "Content-Type: application/json\r\n");
      //     offset += sprintf(request + offset, "cache-control: no-cache\r\n");
      //     offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
      //     offset += sprintf(request + offset, "%s\r\n", json_body);

      //     do_https_request(SERVER, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
      //     Serial.println("-----------");
      //     Serial.println(response);
      //     Serial.println("-----------");
      //     unpack_json(response);
      //   }
      // displayAllGPS();
    //}
    int autoFlag = auto_button.update();
    /*
    body[0] = 0;
    sprintf(
        body, "a_x=%f&a_y=%f&v_x=%d&v_y=%d&angle=%f&dir=%d&speed=%f",
        // NOTE this used to be "acceleration"
        info.tilt.x,
        info.tilt.y,
        // NOTE this is ignored
        0,
        0,
        info.angle,
        info.direction,
        info.speed);

    int body_len = strlen(body); // calculate body length (for header reporting)
    sprintf(request_buffer2, "POST http://608dev-2.net/sandbox/sc/team10/server.py HTTP/1.1\r\n");
    strcat(request_buffer2, "Host: 608dev-2.net\r\n");
    strcat(request_buffer2, "Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request_buffer2 + strlen(request_buffer2), "Content-Length: %d\r\n", body_len); // append string formatted to end of request buffer
    strcat(request_buffer2, "\r\n");                                                        // new line from header to body
    strcat(request_buffer2, body);                                                          // body
    strcat(request_buffer2, "\r\n");                                                        // new lin
    do_http_request("608dev-2.net", request_buffer2, response_buffer2, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    Serial.println(response_buffer2);
     

    delay(DT_SERVER);
    */ 
    memset(request_buffer3,0,50);
    sprintf(request_buffer3,"GET http://608dev-2.net/sandbox/sc/team10/server.py?camera=2 HTTP/1.1\r\n");
    strcat(request_buffer3,"Host: 608dev-2.net\r\n");
    strcat(request_buffer3,"\r\n"); //new line 
    // out buffer size is 
    do_http_request("608dev-2.net", request_buffer3, response_buffer3, 50, RESPONSE_TIMEOUT,true);
    Serial.println("RESPONSE BUFFER"); //viewable in Serial Terminal
      
    
    tft.setCursor(0,80,1);
    //tft.println("Detected: ");
    Serial.println(response_buffer3);
    response_buffer3[12] = '\0';

    tft.println(response_buffer3); 

    // IN THE FUTURE THIS IS WHAT WE WOULD IMPLEMENT FOR DIFFERENT OBJECTS TO MAKE IT MORE AUTONOMOUS 
    /*
    if (strcmp(response,"Person")==0){
        tft.println("STOP"); 
        MOVING = !MOVING; 
        // set moving to false   AUTOMAtICALLY         
    } 
    */
    
      // sender gets from server 
    delay(DT_SERVER);
    
  }
}


// This is where we run everything that is not the super slow wifi server loop
void other_things_loop(void *parameters)
{
  for (;;)
  {
    tft.setCursor(0, 10, 1);
    imu.readAccelData(imu.accelCount);
    // Flipped because forward is "up"
    float y = imu.accelCount[0] * imu.aRes;
    float x = imu.accelCount[1] * imu.aRes;

    // Clip to avoid wierd edge cases
    // y is inverted for some reason we found out during tested
    y = -y;
    y = y > MAX_TILT_Y ? MAX_TILT_Y : y;
    y = y < -MAX_TILT_Y ? -MAX_TILT_Y : y;
    x = x > MAX_TILT_X ? MAX_TILT_X : x;
    x = x < -MAX_TILT_X ? -MAX_TILT_X : x;

    float angle = get_angle(x, y);

    int moveFlag = move_button.update();

    Direction direction;
    float speed;
    if (moveFlag > 0)
    {
      MOVING = !MOVING;
      tft.fillScreen(BACKGROUND);
      tft.setCursor(0, 10, 1);
    }
    if (MOVING)
    {
      direction = get_direction(x, y);
      speed = get_speed(direction, x, y);
      print_direction(direction);
      tft.println("Speed below");
      tft.println(speed);
      tft.println("Tilt Below x then y");
      tft.println(x);
      tft.println(y);
    }
    else
    {
      direction = NONE;
      speed = 0;
      tft.println("Not Moving   ");
    }

    // Update information so we can send to the server if necessary
    // And also send to the arduino
    info.tilt.x = x;
    info.tilt.y = y;
    info.angle = angle;
    info.speed = speed;
    info.direction = direction;

    if (millis() - esp_now_timer > DT_ESP_NOW)
    {
      // send the position
      esp_err_t result = esp_now_send(0, (uint8_t *)&info, sizeof(WireData));

      // do some error checking
      if (result == ESP_OK)
      {
        // Serial.println("Sent with success");
      }
      else
      {
        // Serial.println("Error sending the data");
      }
      esp_now_timer = millis();
    }
    delay(10);
  }
}

void setup()
{
  // Setup TFT and Serial
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

  // Setup wifi
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // PRINT OUT WIFI NETWORKS NEARBY
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
  timer = millis();

  // Set up MAC-based esp now
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

  // Set up state
  esp_now_timer = millis();
  wifi_scan_timer = millis();

  pinMode(MOVE_BUTTON, INPUT_PULLUP);
  pinMode(AUTO_BUTTON, INPUT_PULLUP);
  MOVING = false;

  // Set up the bins
  float speed_range = MAX_SPEED - MIN_SPEED;
  for (int i = 0; i < NUM_SPEED_BINS; i++)
  {
    float percent = (i + 1) / (float)NUM_SPEED_BINS;
    float speed = MIN_SPEED + percent * speed_range;
    SPEED_BINS[i] = speed;
  }

  times_in_state = 10;
  prev_speed = 0;

  // Using GPS instead of wifi
  // gps.begin(9600, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);

  // Set up tasks
  // Use pinned to core so that it'll be scheduled on the same core
  // and that we we can run in parallel.
  
  xTaskCreatePinnedToCore(
      wifi_server_loop, /* Function to implement the task */
      "Task0",          /* Name of the task */
      5000,             /* Stack size in words: wifi needs more for dynamic json doc, etc... */
      NULL,             /* Task input parameter */
      0,                /* Priority of the task */
      NULL,             /* Task handle: not used since tasks are independent. */
      0);               /* Core where the task should run */

  xTaskCreatePinnedToCore( 
      other_things_loop, /* Function to implement the task */
      "Task1",           /* Name of the task */
      1000,              /* Stack size in words */
      NULL,              /* Task input parameter */
      1,                 /* Priority of the task */
      NULL,              /* Task handle: not used since tasks are independent. */
      1);                /* Core where the task should run */
  Serial.println("Setup completed.");
  
}

void loop()
{
  /*
  tft.setCursor(0, 10, 1);
    imu.readAccelData(imu.accelCount);
    // Flipped because forward is "up"
    float y = imu.accelCount[0] * imu.aRes;
    float x = imu.accelCount[1] * imu.aRes;

    // Clip to avoid wierd edge cases
    // y is inverted for some reason we found out during tested
    y = -y;
    y = y > MAX_TILT_Y ? MAX_TILT_Y : y;
    y = y < -MAX_TILT_Y ? -MAX_TILT_Y : y;
    x = x > MAX_TILT_X ? MAX_TILT_X : x;
    x = x < -MAX_TILT_X ? -MAX_TILT_X : x;

    float angle = get_angle(x, y);

    int moveFlag = move_button.update();

    Direction direction;
    float speed;
    if (moveFlag > 0)
    {
      MOVING = !MOVING;
      tft.fillScreen(BACKGROUND);
      tft.setCursor(0, 10, 1);
    }
    if (MOVING)
    {
      direction = get_direction(x, y);
      speed = get_speed(direction, x, y);
      print_direction(direction);
      tft.println("Speed below");
      tft.println(speed);
      tft.println("Tilt Below x then y");
      tft.println(x);
      tft.println(y);
    }
    else
    {
      direction = NONE;
      speed = 0;
      tft.println("Not Moving   ");
    }

    // Update information so we can send to the server if necessary
    // And also send to the arduino
    info.tilt.x = x;
    info.tilt.y = y;
    info.angle = angle;
    info.speed = speed;
    info.direction = direction;

    if (millis() - esp_now_timer > DT_ESP_NOW)
    {
      // send the position
      esp_err_t result = esp_now_send(0, (uint8_t *)&info, sizeof(WireData));

      // do some error checking
      if (result == ESP_OK)
      {
        // Serial.println("Sent with success");
      }
      else
      {
        // Serial.println("Error sending the data");
      }
      esp_now_timer = millis();
    }
    delay(10);
    */


}
