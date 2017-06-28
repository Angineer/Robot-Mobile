//Robot Main
//
//This sketch drives Robie.
//
//Programmed by Andy Tracy

#include <Servo.h>
//#include <SD.h>

//Drive motor variables
//High on rightf = forward on right, etc.
const int rightf=3;
const int rightb=2;
const int leftf=5;
const int leftb=4;

int baseSpeed=100; //Max speed for forward driving, 0-255

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
  pinMode(lookpin, OUTPUT); // Set servo control pin to output mode
  DriveStop(); // Start with all motors off
  
  look.attach(lookpin); //Attach eyeball servo
  Look(90); //Start servo at 90 degrees
  Serial.println("Starting up...");
}

void loop(){
  //Look around
  Look(45);
  distVector[0]=GetDist();
  Look(135);
  distVector[2]=GetDist();
  Look(90);
  distVector[1]=GetDist();
  
  //Drive accordingly
  DriveAvoid(distVector);

  //Do as FGL does 
  delay(150);
  //delay(5000);
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
  
  if(pos>1000 && pos<=2100){ //If the angle is acceptable
    look.writeMicroseconds(pos); //Send it to the servo
    delay(150);
  }
  //Pause for debugging
  delay(250);
}

void DriveForward(int right, int left){ //Drive forward
  //Inputs are percentage of baseSpeed on respective motor
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

void DriveStop(){ //Stop both drive motors
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, LOW);
}

void DriveAvoid(int vector[]){
  int min=1000;
  int closer=0;
  //for (int i=0; i<=2; i++){
  //  if (vector[i]<min){
  //    min=vector[i];
  //  }
  //}
  if (vector[0]<vector[2]) closer=-1;
  else closer=1;
  
  if (vector[1] < 16 || min(vector[0], vector[2]) < 10){
    Serial.println("Object too close; avoiding...");
    DriveBackward(closer);
    delay(100*random(5, 20));
    DriveForward(100, 110);
    //DriveStop();
  }
  else{
    Serial.println("Driving randomly...");
    DriveForward(100, 110);
    //DriveStop();
  }
}

