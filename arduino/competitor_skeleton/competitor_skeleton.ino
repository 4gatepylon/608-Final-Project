#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include <math.h>

MPU6050 imu; //imu object called, appropriately, imu
TFT_eSPI tft = TFT_eSPI();
// Set up the TFT object

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
      K_FRICTION = 0.15;  //friction coefficient
      K_SPRING = 0.9;  //spring coefficient
      LEFT_LIMIT = left_lim + RADIUS; //left side of screen limit
      RIGHT_LIMIT = right_lim - RADIUS; //right side of screen limit
      TOP_LIMIT = top_lim + RADIUS; //top of screen limit
      BOTTOM_LIMIT = bottom_lim - RADIUS; //bottom of screen limit
      DT = dt; //timing for integration
    }
    void step(float x_force = 0, float y_force = 0 ) {
      //YOUR FINAL CODE FROM LAST WEEK/TWO WEEKS AGO
      local_tft->drawCircle(position.x, position.y, RADIUS, BKGND_CLR);

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
};

// Copied code from my 6.08 solution to the previous exercises
// turn into a bool so you can do if else if
bool collisionDetectBool(Ball player, int player_radius, Vec food_position, int food_half_width) {
    int playerX = player.getX();
    int playerY = player.getY();
    int rad = player_radius;
    
    int foodRight = food_position.x + food_half_width;
    int foodLeft = food_position.x - food_half_width;
    int foodTop = food_position.y - food_half_width;
    int foodBot = food_position.y + food_half_width;
    
    int playerRight = playerX + rad;
    int playerLeft = playerX - rad;
    int playerTop = playerY - rad;
    int playerBot = playerY + rad;
    
    int collidesX = !(playerLeft > foodRight || foodLeft > playerRight);
    int collidesY = !(playerTop > foodBot || foodTop > playerBot);
    
    return collidesX && collidesY;
}

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
    Ball opponent; //make instance of opponent

    Game(TFT_eSPI* tft_to_use, int loop_speed):
      player(tft_to_use, loop_speed, player_radius, 1, TFT_RED, BACKGROUND,
             left_limit, right_limit, top_limit, bottom_limit),
      opponent(tft_to_use, loop_speed, player_radius, 1, TFT_GREEN, BACKGROUND,
               left_limit, right_limit, top_limit, bottom_limit)
    {
      game_tft = tft_to_use;
      score = 0;
      food_position.x = 40; // Initial x pos
      food_position.y = 40; // Initial y pos
      food_half_width = 1;
    }
    void step(float x, float y) {
      player.step(x, y);
      float opp_x_command;
      float opp_y_command;
      ai(&opp_x_command, &opp_y_command); //get x and y command of opponent...based on AI
      opponent.step(difficulty * opp_x_command, difficulty * opp_y_command);
      int new_score = collisionDetect(); //checks if collision occurred (food found/lost)
      if (new_score != score) {//got some nomnoms!  (score can only increase currently so change in score means food found)
        score = new_score;
        //erase old food position
        game_tft->fillRect(food_position.x - food_half_width, food_position.y - food_half_width, 2 * food_half_width, 2 * food_half_width, BACKGROUND);
        food_position.x = random(right_limit - left_limit - 2 * food_half_width)
                          + left_limit + food_half_width;
        food_position.y = random(bottom_limit - top_limit - 2 * food_half_width)
                          + top_limit + food_half_width;
      }
      int top_left_x = food_position.x - food_half_width;
      int top_left_y = food_position.y - food_half_width;
      int side = 2 * food_half_width;
      //draw new food position
      game_tft->fillRect(top_left_x, top_left_y, side, side, TFT_GREEN);
      game_tft->setCursor(0, 0, 1);
      char output[30];
      sprintf(output, "Score: %d    ", score);
      game_tft->print(output);
    }
  private:
    int collisionDetect() {
      if (collisionDetectBool(player, player_radius, food_position, food_half_width)) return score + 1;
      else if (collisionDetectBool(opponent, player_radius, food_position, food_half_width)) return score - 1;
      return score;

    }
    void ai(float* x, float* y) {
      float dx = food_position.x - opponent.getX();
      float dy = food_position.y- opponent.getY();
      // sqrt slow :(
      float norm = sqrt(dx * dx + dy * dy);
      *x = dx / norm;
      *y = dy / norm;
    }

};


Game game(&tft, LOOP_PERIOD);

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

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(0));  //initialize random numbers
  delay(20); //wait 20 milliseconds
  primary_timer = millis();
}

void loop() {
  imu.readAccelData(imu.accelCount);//read imu
  float x = imu.accelCount[0] * imu.aRes;
  float y = imu.accelCount[1] * imu.aRes;
  game.step(y * EXCITEMENT, x * EXCITEMENT); //apply force from imu
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}
