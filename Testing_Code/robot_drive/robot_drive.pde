//Robot Drive
//
//This sketch drives the DC motors in a robot
//using an h-bridge circuit (L293NE) and operates
//the servo motor for the steering assembly.
//
//Programmed by Andy Tracy

#include <Servo.h>

//Steering servo variables
Servo steer;

//Drive motor variables
//High on rightf = forward on right, etc.
int rightf=2;
int rightb=3;
int leftf=4;
int leftb=5;


void setup(){
  for(int i=2; i<6; i++){
  pinMode(i, OUTPUT);} //Set motor control pins to output mode
  DriveStop();
  
  steer.attach(9); //Attach servo to pin 9
  steer.writeMicroseconds(1500); //Start servo at 90 degrees
}

void loop(){
  //Perform a simple driving task
  Turn(90);
  DriveForward();
  delay(1000);
  Turn(120);
  DriveForward();
  delay(1000);
  Turn(60);
  DriveForward();
  delay(1000);
  Turn(90);
  delay(100);
  DriveStop();
  delay(2000);
}

void Turn(int pos){ //Turns the servo to the given angle in degrees
  pos=pos*10+685; //Convert angle to microseconds
  
  if(pos>1100 && pos<=1900){ //If the angle is acceptable
    steer.writeMicroseconds(pos); //Send it to the servo
    delay(500);
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
