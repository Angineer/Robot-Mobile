/*
  Robot Main

  This sketch drives Robie. Uses the Parallax Ping sensor and the SparkFun
  line follower array, which relies on the line follower array library:

  https://github.com/sparkfun/SparkFun_Line_Follower_Array_Arduino_Library

  Borrows heavily from the AveragingReadBarOnly sketch by Marshall Taylor
  at SparkFun.

  Author: Andy Tracy <adtme11@gmail.com>
*/

#include "Wire.h"
#include "sensorbar.h"
#include <Servo.h>

/*** Configuration ***/
// Motor control
#define RIGHT_FWD_PIN  3
#define RIGHT_BACK_PIN 2
#define LEFT_FWD_PIN   5
#define LEFT_BACK_PIN  4
#define MAX_SPEED 100 // Max speed for forward driving, 0-255

// Eyeball sensor
#define LOOK_PIN 11
#define PING_PIN 7
#define MIN_OBS_DIST 6 // Minimum obstacle distance in inches
#define EYEBALL_MOVE_TIME 25 // Time for servo to reach desired position in ms

// Line sensor
#define MOVING_AVG_WINDOW_SIZE 4
#define CIRCULAR_BUFFER_SIZE 100

/*** Global vars ***/
// The high-level instruction from the raspberry pi about what we should be
// doing. 'h' = halt, 'd' = drive
char directive { 'h' };

// Line sensor variables
SensorBar mySensorBar ( 0x3E ); // SX1509 I2C address (00)
CircularBuffer positionHistory ( CIRCULAR_BUFFER_SIZE );

// Eyeball variables
Servo look;
int angle { 90 }; // Angle of the servo in degrees, 90 is straight forward
bool direction { true }; // Controls if servo sweep moves right or left

void setup(){
    Serial.begin ( 9600 ); // Connect to raspi

    // Motor control
    pinMode ( RIGHT_FWD_PIN, OUTPUT );
    pinMode ( RIGHT_BACK_PIN, OUTPUT );
    pinMode ( LEFT_FWD_PIN, OUTPUT );
    pinMode ( LEFT_BACK_PIN, OUTPUT );
    driveStop(); // Start with all motors off

    // Eyeball sensor
    pinMode ( LOOK_PIN, OUTPUT );
    look.attach ( LOOK_PIN );
    rotateEyeballServo ( angle );

    // Line sensor
    mySensorBar.setBarStrobe(); // Turn on IR only during reads
    //mySensorBar.clearBarStrobe(); // Run IR all the time
    mySensorBar.clearInvertBits(); // Dark line on light
    //mySensorBar.setInvertBits(); // Light line on dark
    mySensorBar.begin();
}

void loop(){
    // Check if there is an update from the raspi
    if ( Serial.available() ) {
        directive = Serial.read();
    }

    // Proceed based on the current directive from the raspi. 
    if ( directive == 'h' ) { // If we are halted, just hang out.
        driveStop();
    } else if ( directive == 'd' ) { // Raspi says we need to get moving
        // Check for obstacles detected by the eyeball sensor
        long obsDist = ping();
        if ( obsDist < MIN_OBS_DIST ) {
            driveStop();
            return;
        }

        // Move the eyeball servo a little every loop so that it does a scan
        // of the area in front of Robie
        if ( direction ) {
            angle += 5;
        } else {
            angle -= 5;
        }

        if ( angle <= 45 ) {
            direction = true;
        } else if ( angle >= 135 ) {
            direction = false;
        }

        rotateEyeballServo ( angle );

        // Check our position against the line
        float linePos = readLineSensor();
        float linear = 1.0;
        float angular = linePos;

        drive ( linear, angular );
    }

    // Don't let the loop run too fast
    delay ( 50 );
}

/* 
 * @brief Read the line sensor array
 * @return The average of the last MOVING_AVG_WINDOW_SIZE position readings
 */
float readLineSensor()
{
    // First, grab data from the line sensor and save it to the circular buffer
    int temp = mySensorBar.getDensity();
    if ( ( temp < 4 ) && ( temp > 0 ) )
    {
        positionHistory.pushElement ( mySensorBar.getPosition() );
    }
    // Get an average of the last n readings
    return float ( positionHistory.averageLast ( MOVING_AVG_WINDOW_SIZE ) );
}

/*
 * @brief Get a single distance reading from the Ping ultrasonic sensor
 * @return The distance measured by the sensor in inches
 */
long ping()
{
  pinMode(PING_PIN, OUTPUT);
  digitalWrite(PING_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(PING_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(PING_PIN, LOW);

  // Pick up the ping
  pinMode ( PING_PIN, INPUT );
  long duration = pulseIn ( PING_PIN, HIGH );

  // Convert to inches
  return duration / 73.746 / 2;
}

/*
 * @brief Set the eyeball servo to a particular angle.
 * @param pos Angle to set the servo to in degrees
 */
void rotateEyeballServo ( int pos )
{
  pos = pos * 10 + 580; // Convert angle to microseconds
  
  if ( pos>1000 && pos<=2100 ) { // If the angle is acceptable
    look.writeMicroseconds ( pos ); // Send it to the servo
    delay ( EYEBALL_MOVE_TIME );
  }
}

/*
 * @brief Drive the robot
 * @param linearSpeed Linear speed of the robot from -1.0 to 1.0. Positive is
 *        forward, negative is backward.
 * @param angularSpeed Angular speed of the robot from -1.0 to 1.0. Positive is
 *        left, negative is right.
 */
void drive ( float linearSpeed, float angularSpeed )
{
    bool sign = linearSpeed >= 0;
    float leftSpeed, rightSpeed;

    if ( angularSpeed >= 0 ) {
        leftSpeed = ( 1.0 - angularSpeed ) * MAX_SPEED;
        rightSpeed = MAX_SPEED;
    } else {
        leftSpeed = MAX_SPEED;
        rightSpeed = ( 1.0 + angularSpeed ) * MAX_SPEED;
    }

    if ( linearSpeed >= 0 ) {
        analogWrite ( LEFT_FWD_PIN, linearSpeed * leftSpeed );
        digitalWrite ( LEFT_BACK_PIN, LOW );
        analogWrite ( RIGHT_FWD_PIN, linearSpeed * rightSpeed );
        digitalWrite ( RIGHT_BACK_PIN, LOW );
    } else {
        digitalWrite ( LEFT_FWD_PIN, LOW );
        analogWrite ( LEFT_BACK_PIN, -linearSpeed * leftSpeed );
        digitalWrite ( RIGHT_FWD_PIN, LOW );
        analogWrite ( RIGHT_BACK_PIN, -linearSpeed * rightSpeed );
    }
}

/*
 * @brief Stop both drive motors
 */
void driveStop()
{
  digitalWrite(RIGHT_FWD_PIN, LOW);
  digitalWrite(RIGHT_BACK_PIN, LOW);
  digitalWrite(LEFT_FWD_PIN, LOW);
  digitalWrite(LEFT_BACK_PIN, LOW);
}

