#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE

const int DT = 40; //milliseconds
const int SCALER = 1000; //how much force to apply to ball
const uint8_t BUTTON = 5; 

struct Vec { //C struct to represent 2D vector (position, accel, vel, etc)
  float x;
  float y;
};

uint32_t primary_timer; //main loop timer

//state variables:
struct Vec position; //position of ball
struct Vec velocity; //velocity of ball
struct Vec acceleration; //acceleration of ball

//physics constants:
const float MASS = 1; //for starters
const int RADIUS = 5; //radius of ball
const float K_FRICTION = 0.15;  //friction coefficient
const float K_SPRING = 0.9;  //spring coefficient

//boundary constants:
const int LEFT_LIMIT = RADIUS; //left side of screen limit
const int RIGHT_LIMIT = 127 - RADIUS; //right side of screen limit
const int TOP_LIMIT = RADIUS; //top of screen limit
const int BOTTOM_LIMIT = 159 - RADIUS; //bottom of screen limit

bool pushed_last_time; //for finding change of button (using bool type...same as uint8_t)


MPU6050 imu; //imu object called, appropriately, imu

void step(float x_force=0, float y_force=0 ){
  float friction_x = -K_FRICTION * velocity.x;
  float friction_y = -K_FRICTION * velocity.y;
  
  float x_force_with_friction = x_force + friction_x;
  float y_force_with_friction = y_force + friction_y;
  acceleration.x = x_force_with_friction/MASS;
  acceleration.y = y_force_with_friction/MASS;
  //integrate to get velocity from current acceleration
  velocity.x = velocity.x + 0.001*DT*acceleration.x; //integrate, 0.001 is conversion from milliseconds to seconds
  velocity.y = velocity.y + 0.001*DT*acceleration.y; //integrate!!
  
  moveBall();
}

void moveBall(){
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
  if (x < LEFT_LIMIT) backward_x = LEFT_LIMIT - x;
  if (x > RIGHT_LIMIT) backward_x = RIGHT_LIMIT - x;
  if (y < TOP_LIMIT) backward_y = TOP_LIMIT - y;
  if (y > BOTTOM_LIMIT) backward_y =  BOTTOM_LIMIT - y;

  // Note that backward is always in the desired direction
  // It gets smaller in absolute value if you are further from
  // the wall (so if you are going left it gets smaller, if you are
  // going right, then it gets larger)
  if (backward_x != 0) {
    float travel_x = x < LEFT_LIMIT ? position.x - LEFT_LIMIT : position.x - RIGHT_LIMIT;
    backward_x *= K_SPRING;
    position.x += backward_x - travel_x;
  } else {
    position.x = x;
  }
  if (backward_y != 0) {
    float travel_y = y < TOP_LIMIT ? position.y - TOP_LIMIT : position.y - BOTTOM_LIMIT;
    backward_y *= K_SPRING;
    position.y += backward_y - travel_y;
  } else {
    position.y = y;
  }

  // You should update velocity even when you just barely touch, but not
  // position in that case
  if (!(LEFT_LIMIT < x && x < RIGHT_LIMIT)) {
    velocity.x = -K_SPRING * velocity.x;
  }
  if (!(TOP_LIMIT < y && y < BOTTOM_LIMIT)) {
    velocity.y = -K_SPRING * velocity.y;
  }
}




void setup() {
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  delay(100);
  Serial.begin(115200); //for debugging if needed.
  pinMode(BUTTON, INPUT_PULLUP);
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  randomSeed(analogRead(0));  //initialize random numbers
  step(random(-SCALER, SCALER), random(-SCALER, SCALER)); //apply initial force to lower right
  pushed_last_time = false;
  primary_timer = millis();
}

void loop() {
  //draw circle in previous location of ball in color background (redraws minimal num of pixels, therefore is quick!)
  tft.fillCircle(position.x, position.y, RADIUS, BACKGROUND);
  imu.readAccelData(imu.accelCount);//read imu
  float y = imu.accelCount[0] * imu.aRes;
  float x = imu.accelCount[1] * imu.aRes;

  step(x * SCALER, y * SCALER); //apply force from imu
  tft.fillCircle(position.x, position.y, RADIUS, BALL_COLOR); //draw new ball location

  while (millis() - primary_timer < DT); //wait for primary timer to increment
  primary_timer = millis();
}
