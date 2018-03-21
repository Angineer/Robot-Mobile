/******************************************************************************
AveragingReadBarOnly.ino

A sketch for reading sensor data into a circular buffer

Marshall Taylor, SparkFun Engineering

5-27-2015

Library:
https://github.com/sparkfun/SparkFun_Line_Follower_Array_Arduino_Library
Product:
https://github.com/sparkfun/Line_Follower_Array

This sketch shows how to use the circular buffer class to create a history of
sensor bar scans.
The buffer configured with CBUFFER_SIZE to have a length of 100 16bit integers.

Resources:
sensorbar.h

Development environment specifics:
arduino > v1.6.4
hw v1.0

This code is released under the [MIT License](http://opensource.org/licenses/MIT).
Please review the LICENSE.md file included with this example. If you have any questions 
or concerns with licensing, please contact techsupport@sparkfun.com.
Distributed as-is; no warranty is given.
******************************************************************************/
#define CBUFFER_SIZE 100

#include "Wire.h"
#include "sensorbar.h"

const uint8_t SX1509_ADDRESS = 0x3E;  // SX1509 I2C address (00)

SensorBar mySensorBar(SX1509_ADDRESS);

CircularBuffer positionHistory(CBUFFER_SIZE);

//Drive motor variables
//High on rightf = forward on right, etc.
const int rightf=3;
const int rightb=2;
const int leftf=5;
const int leftb=4;
int baseSpeed = 100;


// Position vars
int16_t avePos;
float avePos_float;


void setup()
{
  Serial.begin(115200);  // start serial for output
  Serial.println("Program started.");
  Serial.println();

  pinMode(rightf, OUTPUT); //Set motor control pins to output mode
  pinMode(rightb, OUTPUT);
  pinMode(leftf, OUTPUT);
  pinMode(leftb, OUTPUT);
  DriveStop(); // Start with all motors off
  
  //For this demo, the IR will only be turned on during reads.
  mySensorBar.setBarStrobe();
  //Other option: Command to run all the time
  //mySensorBar.clearBarStrobe();

  //Default dark on light
  mySensorBar.clearInvertBits();
  //Other option: light line on dark
  //mySensorBar.setInvertBits();
  
  uint8_t returnStatus = mySensorBar.begin();
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

void loop()
{
  //Wait 50 ms
  delay(25);


  //Get the data from the bar and save it to the circular buffer positionHistory.
  int temp = mySensorBar.getDensity();
  //if( (temp < 4)&&(temp > 0) )
  if (temp < 4)
  {
    positionHistory.pushElement( mySensorBar.getPosition());
  }

  {

    //Get an average of the last 'n' readings
    avePos = positionHistory.averageLast( 4 );

    if (avePos < -50){
      DriveForward(0, 100);
    }
    else if (avePos > -50 && avePos < 50){
      DriveForward(100, 100);
    }
    else if (avePos > 50){
      DriveForward(100, 0);
    }

    //avePos = (avePos + 127) / 255.0 * 100.0;

    //Serial.println(avePos);

    //DriveForward(avePos, 100 - avePos);
    
  }
}

void DriveForward(int right, int left){ //Drive forward
  //Inputs are percentage of baseSpeed on respective motor
  analogWrite(rightf, baseSpeed*(right/100.0));
  digitalWrite(rightb, LOW);
  analogWrite(leftf, baseSpeed*(left/100.0));
  digitalWrite(leftb, LOW);
}

void DriveStop(){ //Stop both drive motors
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, LOW);
}
