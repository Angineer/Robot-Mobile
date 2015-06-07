//Robot Main
//
//This sketch drives Robie.
//
//Programmed by Andy Tracy

#include <Servo.h>
#include <SD.h>

//Drive motor variables
//High on rightf = forward on right, etc.
const int rightf=2;
const int rightb=3;
const int leftf=4;
const int leftb=5;

//Steering variables
Servo steer;
const int servopin=9;

//Eyeball variables
Servo look;
const int lookpin=8;
const int pingPin = 7;
long duration, distance, lastdist = 30;

void setup(){
  pinMode(rightf, OUTPUT); //Set motor control pins to output mode
  pinMode(rightb, OUTPUT);
  pinMode(leftf, OUTPUT);
  pinMode(leftb, OUTPUT);
  pinMode(servopin, OUTPUT);
  pinMode(lookpin, OUTPUT);
  DriveStop();
  
  steer.attach(servopin); //Attach servo
  steer.writeMicroseconds(1500); //Start servo at 90 degrees
  look.attach(lookpin); //Attach servo
  look.writeMicroseconds(1500); //Start servo at 90 degrees
}

void loop(){
  //Look around
  Look(70);
  Look(110);
  Look(90);
  
  //Send out a ping
  //duration=SendPing();
  duration=1000;
  
  //Figure out how far we are from an object (in inches)
  distance = duration / 73.746 / 2;
  //if (distance > 60) distance=lastdist;
  
  //Drive accordingly
  if (distance < 18){
    DriveBackward();
    Turn(60);
    delay(1500);
    DriveForward();
    Turn(90);
  }
  else{
    DriveForward();
  }

  //Remember the last distance and wait to take a new reading  
  lastdist=distance;
  delay(500);
}

long SendPing(){
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  
  //Pick up the ping
  pinMode(pingPin, INPUT);
  return pulseIn(pingPin, HIGH);
}

void Look(int pos){ //Turns the servo to the given angle in degrees
  pos=pos*10+580; //Convert angle to microseconds
  
  if(pos>1100 && pos<=2100){ //If the angle is acceptable
    look.writeMicroseconds(pos); //Send it to the servo
    delay(150);
  }
}

void Turn(int pos){ //Turns the servo to the given angle in degrees
  pos=pos*10+685; //Convert angle to microseconds
  
  if(pos>1100 && pos<=2100){ //If the angle is acceptable
    steer.writeMicroseconds(pos); //Send it to the servo
    delay(150);
  }
}

void DriveForward(){ //Forward on both drive motors
  digitalWrite(rightf, HIGH);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, HIGH);
  digitalWrite(leftb, LOW);
}

void DriveBackward(){ //Backward on both drive motors
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, HIGH);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, HIGH);
}

void DriveStop(){ //Stop both drive motors
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, LOW);
}
