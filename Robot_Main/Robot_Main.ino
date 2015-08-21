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
const int servopin=10;

//Eyeball variables
Servo look;
const int lookpin=11;
const int pingPin = 7;
long duration, lastdist = 30;
int distVector[]={30, 30, 30};

void setup(){
  Serial.begin(9600); // For debugging
  pinMode(rightf, OUTPUT); //Set motor control pins to output mode
  pinMode(rightb, OUTPUT);
  pinMode(leftf, OUTPUT);
  pinMode(leftb, OUTPUT);
  pinMode(servopin, OUTPUT); // Set servo control pins to output mode
  pinMode(lookpin, OUTPUT);
  DriveStop(); // Start with all motors off
  
  steer.attach(servopin); //Attach drive servo
  Turn(90); //Start servo at 90 degrees
  look.attach(lookpin); //Attach eyeball servo
  Look(90); //Start servo at 90 degrees
}

void loop(){
  //while(1){}
  //Look around
  Look(55);
  distVector[0]=GetDist();
  Look(130);
  distVector[1]=GetDist();
  Look(90);
  distVector[2]=GetDist();
  
  Serial.print("Distance: ");
  Serial.print(distVector[0]);
  Serial.print(distVector[1]);
  Serial.println(distVector[2]);
  
  //Drive accordingly
  if (distVector[1] < 10
        || distVector[0] < 10
        || distVector[2] < 10){
    DriveBackward();
    Turn(60);
    delay(500);
    DriveForward();
    Turn(90);
  }
  else{
    DriveForward();
    //DriveStop();
  }

  //Drive very cautiously 
  delay(150);
  DriveStop();
  delay(350);
}

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

void Look(int pos){ //Turns the servo to the given angle in degrees
  pos=pos*10+580; //Convert angle to microseconds
  
  if(pos>1100 && pos<=2100){ //If the angle is acceptable
    look.writeMicroseconds(pos); //Send it to the servo
    delay(150);
  }
  //Pause for debugging
  delay(250);
}

void Turn(int pos){ //Turns the servo to the given angle in degrees
  pos=pos*10+580; //Convert angle to microseconds
  Serial.println(pos);
  
  if(pos>1100 && pos<=2100){ //If the angle is acceptable
    Serial.println("Turning!");
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
