//Drive motor variables
//High on rightf = forward on right, etc.
const int rightf=2;
const int rightb=3;
const int leftf=4;
const int leftb=5;

int speed=100; //0-255

void setup() {
  Serial.begin(9600); // For debugging
  pinMode(rightf, OUTPUT); //Set motor control pins to output mode
  pinMode(rightb, OUTPUT);
  pinMode(leftf, OUTPUT);
  pinMode(leftb, OUTPUT);

  digitalWrite(rightf, LOW);
  analogWrite(rightb, speed);
  digitalWrite(leftf, LOW);
  analogWrite(leftb, speed);

  delay(1000);

  analogWrite(rightb, LOW);
  analogWrite(leftb, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

}
