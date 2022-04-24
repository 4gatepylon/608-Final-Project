int motor1Pin1 = 38; 
int motor1Pin2 = 39; 

int motor2Pin1 = 40; 
int motor2Pin2 = 41; 

int enable1Pin = 37; 
int enable2Pin = 42; 

const uint8_t BUTTON1 = 39; // press for long time and posts to server
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
  pinMode(BUTTON1, INPUT_PULLUP); 
}

/*
MOVE BASED ON DIRECTION
*/
void moveCar(uint8_t direction, uint8_t speed){ 
    // get request?? 
    if (direction==1){
        moveBack();
        motorSpeedA = map(speed, MAX_SPEED, 0, 0, 255); 
        motorSpeedB = map(speed, MAX_SPEED, 0, 0, 255); 
    }
    if (direction==0){
        moveForward();
        motorSpeedA = map(speed, 550, 1023, 0, 255);  // ASK ABOUT PWM CYCLE 
        motorSpeedB = map(speed, 550, 1023, 0, 255);  //ASK ABOUT PWM CYCLE
    }

    if (direction==2){ // RIGHT
        int changex = map(speed, MAX_SPEED, 0, 0, 255); 
        motorSpeedA = motorSpeedA - changex;
        motorSpeedB = motorSpeedB + changex; 
    }

    if (direction==3){ // LEFT 
        int changex = map(speed, 550, 1023, 0, 255); 
        motorSpeedA = motorSpeedA - changex;
        motorSpeedB = motorSpeedB + changex; 
    }
}
/*
BUTTON PRESS STOP 
*/
void stopcar(uint8_t input){ 
  switch(button_state){ 
    case IDLE: 
      if (input ==0) {  
        button_state = DOWN; 
      }
      break;
    case DOWN:  
      if (input==0){  
          button_state = DOWN; 
      } 
      else if (input==1){ 
          // stop both motors
        digitalWrite(motor1Pin1, LOW);
        digitalWrite(motor1Pin2, LOW);
        digitalWrite(motor2Pin1, LOW);
        digitalWrite(motor2Pin2, LOW);
        button_state = UP;  

      }
      break;
    case UP:
      if (input==1){
        button_state = IDLE;
      }
      else if (input==0){
        button_state = DOWN;
      }
      break;

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




void loop() {
  // Move the DC motor forward at maximum speed
  stopcar(BUTTON1);
  moveCar(direction,speed); 

  if (motorSpeedA < 0) {
      motorSpeedA = 0;
    }
    if (motorSpeedB > 255) {
      motorSpeedB = 255;
    }
    analogWrite(enable1Pin, motorSpeedA);
    analogWrite(enable2Pin, motorSpeedB);

    //TEST
  // Move DC motor forward with increasing speed
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  while (dutyCycle <= 255){
    ledcWrite(pwmChannel, dutyCycle);   
    Serial.print("Forward with duty cycle: ");
    Serial.println(dutyCycle);
    dutyCycle = dutyCycle + 5;
    delay(500);
  }
  dutyCycle = 200;
}

