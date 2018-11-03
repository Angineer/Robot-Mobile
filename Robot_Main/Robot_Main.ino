/*
  Robot Main

  This sketch drives Robie. Uses the Parallax Ping sensor and the SparkFun
  line follower array, which relies on the line follower array library:

  https://github.com/sparkfun/SparkFun_Line_Follower_Array_Arduino_Library

  Borrows heavily from the AveragingReadBarOnly sketch by Marshall Taylor
  at SparkFun.

  Author: Andy Tracy <adtme11@gmail.com>
*/

#define CBUFFER_SIZE 100
#define MOVING_AVG_WINDOW_SIZE 4
#define MIN_OBS_DIST 6 // Minimum obstacle distance in inches

#define DEBUG_MODE 1

#include "Wire.h"
#include "sensorbar.h"
#include <Servo.h>

// Drive motor variables
// High on rightf = forward on right, etc.
const int rightf = 3;
const int rightb = 2;
const int leftf = 5;
const int leftb = 4;
int baseSpeed = 100; // Max speed for forward driving, 0-255

// Line sensor variables
const uint8_t SX1509_ADDRESS = 0x3E;  // SX1509 I2C address (00)
SensorBar mySensorBar ( SX1509_ADDRESS );
CircularBuffer positionHistory ( CBUFFER_SIZE );
int16_t avePos;
float avePos_float;

// Eyeball variables
Servo look;
const int lookpin = 11;
const int pingPin = 7;
long duration, lastdist = 30;

// The left, center, and right distance readings from the eyeball sensor, in
// inches
int sensorArray[] = { 30, 30, 30 };

void setup(){
    if ( DEBUG_MODE ) {
        Serial.begin ( 115200 ); // For debugging
        Serial.println ( "Starting up..." );
    }

    // Motor control
    pinMode ( rightf, OUTPUT );
    pinMode ( rightb, OUTPUT );
    pinMode ( leftf, OUTPUT );
    pinMode ( leftb, OUTPUT );
    pinMode ( lookpin, OUTPUT );
    DriveStop(); // Start with all motors off

    // Eyeball sensor
    look.attach ( lookpin ); //Attach eyeball servo
    Look ( 90 ); //Start servo at 90 degrees

    // Line sensor
    // The IR will only be turned on during reads.
    mySensorBar.setBarStrobe();
    //Other option: Command to run all the time
    //mySensorBar.clearBarStrobe();

    //Default dark on light
    mySensorBar.clearInvertBits();
    //Other option: light line on dark
    //mySensorBar.setInvertBits();

    uint8_t returnStatus = mySensorBar.begin();

    if ( DEBUG_MODE ) {
        if(returnStatus)
        {
            Serial.println("sx1509 IC communication OK");
        }
        else
        {
            Serial.println("sx1509 IC communication FAILED!");
        while(1);
        }
        Serial.println();
    }
}

void loop(){
  // First, grab data from the line sensor and save it to the circular buffer
  int temp = mySensorBar.getDensity();
  //if( (temp < 4)&&(temp > 0) )
  if (temp < 4)
  {
    positionHistory.pushElement ( mySensorBar.getPosition() );
  }
  // Get an average of the last 'n' readings
  avePos = positionHistory.averageLast ( MOVING_AVG_WINDOW_SIZE );

  // Next, grab readings from the eyeball sensor
  Look(45);
  sensorArray[0]=GetDist();
  Look(135);
  sensorArray[2]=GetDist();
  Look(90);
  sensorArray[1]=GetDist();

  // Take the line and eyeball readings into account and drive accordingly
  FollowLine();

  // Do as FGL does 
  delay ( 150 );
}

/*
 * @brief Get a single distance reading from the Ping ultrasonic sensor
 * @return The distance measured by the sensor in inches
 */
long GetDist(){
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  
  //Pick up the ping
  pinMode(pingPin, INPUT);
  duration=pulseIn(pingPin, HIGH);
  
  //Convert to inches
  return duration / 73.746 / 2;
}

/*
 * @brief Set the eyeball servo to a particular angle.
 * @param pos Angle to set the servo to in degrees
 */
void Look(int pos){
  pos=pos*10+580; //Convert angle to microseconds
  
  if(pos>1000 && pos<=2100){ //If the angle is acceptable
    look.writeMicroseconds(pos); //Send it to the servo
    delay(150);
  }
  //Pause for debugging
  delay(250);
}

/*
 * @brief Drive forward
 * @param right Percentage of base speed to set the right wheel to
 * @param left Percentage of base speed to set the left wheel to
 */
void DriveForward(int right, int left){
  analogWrite(rightf, baseSpeed*(right/100.0));
  digitalWrite(rightb, LOW);
  analogWrite(leftf, baseSpeed*(left/100.0));
  digitalWrite(leftb, LOW);
}

void DriveBackward(int angle){ //Drive backward
  digitalWrite(rightf, LOW);
  digitalWrite(leftf, LOW);
  
  if (angle==0){
    digitalWrite(rightb, HIGH);
    digitalWrite(leftb, HIGH);
  }
  else if (angle>0){
    digitalWrite(rightb, HIGH);
    digitalWrite(leftb, LOW);
  }
  else{
    digitalWrite(rightb, LOW);
    digitalWrite(leftb, HIGH);
  }
}

/*
 * @brief Stop both drive motors
 */
void DriveStop()
{
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, LOW);
}

void FollowLine(){
    // Check for obstacles detected by the eyeball sensor
    if ( DEBUG_MODE ) {
        Serial.println ( "Checking for obstacles..." );
    }

    for ( int i = 0; i < 3; ++i ) {
        if ( sensorArray[i] < MIN_OBS_DIST ) {
            DriveStop();
            return;
        }
    }

    if ( DEBUG_MODE ) {
        Serial.println ( "No obstacles detected, driving forward" );
    }

    // If no obstacles were detected, drive along the line
    if (avePos < -50){
        DriveForward(0, 100);
    }
    else if (avePos > -50 && avePos < 50){
        DriveForward(100, 100);
    }
    else if (avePos > 50){
        DriveForward(100, 0);
    }
}

