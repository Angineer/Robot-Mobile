//H-bridge test
//
//This sketch drives the DC motors in a robot
//using an h-bridge circuit (L293NE)
//
//Programmed by Andy Tracy

//Variables
//High on rightf = forward on right, etc.
int rightf=2;
int rightb=3;
int leftf=4;
int leftb=5;


void setup(){
  for(int i=2; i<6; i++){
  pinMode(i, OUTPUT);} //Set motor control pins to output mode
  driveStop();
}

void loop(){
  //Perform a simple driving task
  driveForward();
  delay(1000);
  driveBackward();
  delay(1000);
  driveStop();
  delay(2000);
}

void driveForward(){ //Forward on both drive motors
  digitalWrite(rightf, HIGH);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, HIGH);
  digitalWrite(leftb, LOW);
  delay(1000);
}

void driveBackward(){ //Backward on both drive motors
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, HIGH);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, HIGH);
  delay(1000);
}

void driveStop(){ //Stop both drive motors
  digitalWrite(rightf, LOW);
  digitalWrite(rightb, LOW);
  digitalWrite(leftf, LOW);
  digitalWrite(leftb, LOW);
  delay(1000);
}
