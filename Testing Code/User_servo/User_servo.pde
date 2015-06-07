//User Servo
//
//This sketch takes user input in the form of an angle (0-180
//degrees) and changes the position of a servo to that angle
//
//Programmed by Andy Tracy

#include <Servo.h>

Servo servotest;  //Servo object

int pos=90;  //Servo position variable

void setup()
{
  Serial.begin(9600);   //Set up serial communication
  
  servotest.attach(9);  //Attach servo to pin 9
  
  servotest.writeMicroseconds(1500);  //Set the servo to start at 90 deg
  
  Serial.println("Enter an angle between 1 and 180");
}

void loop(){
  pos=UserInput();  //Get user input (degrees)
  
  pos=pos*10+600;   //Convert to microseconds
  
  if(pos>600 && pos<=2400){  //If it is an acceptable angle
    Serial.println(pos);
    servotest.writeMicroseconds(pos);  //Send it to the servo
  }
}


int UserInput(){
  int buffer = Serial.available();
  delay(50);  //Allow buffer to fill up
  
  char val[buffer-1];  //Character string that will be read
  
  //Check if data has been sent from the computer
  if (buffer) {    
    //Read the buffer
    for(int i=0; i<buffer; i++){
      val[i] = Serial.read();
    }
    
    //Convert from string to int
    char *thisChar = val;
    int a = atoi(thisChar);

    //Make sure buffer is empty and return the int version
    Serial.flush();
    return a;
  }
}
