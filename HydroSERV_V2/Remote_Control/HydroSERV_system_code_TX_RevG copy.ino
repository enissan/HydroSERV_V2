//Arduino Wireless Communication
//HydroSERV system code: ball valves, thrusters, pump, and stepper motor
//Remote control/Transmitter Code by Megan Smith 4/29/2025
               
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <AccelStepper.h>
#include <Rotary.h> https://github.com/brianlow/Rotary.git

//declare RF pins, address, and variables
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "HSERV";

//declare pins 

byte yPinR = A0; //JOYSTICK - R // y-axis joystick control of the right thruster 
byte yPinL = A1;//JOYSTICK - L // y-axis joystick control of the left thruster 

#define PumpPWMpin A2 //PUMP//
#define PumpswitchPin  9//PUMP//

int SwitchPin_BVA = 5; //BALL VALVE 1//
int SwitchPin_BVB = 6; //BALL VALVE 2 //

Rotary r = Rotary(2,3); //ROTARY ENCODER// CLK & DT pins
#define SW 4 //ROTARY ENCODER // switch/push button pin

//DECLARE GLOBAL VARIABLES 

//Robotics Back End code--pump button- uncomment if testing with button not switch 
// byte lastButtonStatePUMP = LOW;
// unsigned long debounceDurationPUMP = 50; // millis
// unsigned long lastTimeButtonStateChangedPUMP = 0;

//rotary encoder inputs
int counter = 0;
float lastCounter = 0;
String currentDir ="";
int MaxCount = 20;
int MinCount = 0;
int dir = 1;
int change = 0;

//Rotary Encoder button 
byte lastButtonStateRE = LOW;
unsigned long debounceDurationRE = 50; // millis
unsigned long lastTimeButtonStateChangedRE = 0;


struct data_pack
{
  int pwmVal_R = 1500; // THRUSTER-R //initialize pwm val
  int pwmVal_L = 1500; //THRUSTER -L //initialize pwm val
  boolean path_A = HIGH; //BALL VALVE 1 // set default path
  boolean path_B = HIGH; //BALL VALVE 2 // set default path
  byte PumpDir = LOW; //PUMP // set default direction
  int PumpPWM = 0; //PUMP // set default speed
  int StepperDir = 1; //HOSE REEL// stepper motor direction
  int change = 0; //HOSE REEL // rotary encoder change 
  int k = 0; //HOSE REEL// SW check for release
};

data_pack data;


// Set up code to run once
void setup() {
  
  //JOYSTICKS//
  pinMode(yPinR, INPUT); //configures yPinR to behave as an input 
  pinMode(yPinL, INPUT); //configures yPinL to behave as an input

  //BALL VALVES//
  pinMode(SwitchPin_BVA, INPUT); //configures buttonPin_A to behave as an input
  pinMode(SwitchPin_BVB, INPUT); //configures buttonPin_B to behave as an input

  //PUMP//
  pinMode(PumpPWMpin, INPUT);
  pinMode(PumpswitchPin, INPUT);

  //ROTARY ENCODER//
	pinMode(SW, INPUT_PULLUP);

  Serial.begin(9600);
  radio.begin();  //initialize radio object
  radio.openWritingPipe(address); //set the address of the receiver 
  radio.setPALevel(RF24_PA_MIN); //uncomment to set to minimum range if the modules are close to each other
  //radio.setPALevel(RF24_PA_MAX); //uncomment to set to max range
  radio.stopListening(); //fxn as transmitter

  //Finds states of clk and DT --- Helps with debouncing and midpoint values
  r.begin();
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  delay(7000); //gives time for thrusters to initialize
}

// ROTARY ENCODER DIRECTION AND COUNTER
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_NONE) {
    // do nothing
  }
  else if (result == DIR_CW) { //actually CCW
    counter --;
    dir = 1;
    // Serial.print("Counter:  "); //uncomment with line 112 to see the counter values in the serial monitor
    // Serial.println(counter);
  }
  else if (result == DIR_CCW) { //actually CW
    counter ++;
    dir = 0;
    // Serial.print("Counter:  "); //uncomment with line 118 to see the counter values in the serial monitor
    // Serial.println(counter);
  }
}

void loop() {
// put your main code here, to run repeatedly:

  //JOYSTICKS// It is recommended that the right and left thrusters have the same map values.
  int yValR = analogRead(yPinR); //read the incoming value from the joystick and stores it 
  data.pwmVal_R = map(yValR, 0, 1023, 1400, 1600); //map incoming pwm values to those understood by the thruster /// uncomment for less thrust
  // data.pwmVal_R = map(yValR, 0, 1023, 1100, 1900); // Uncomment for maximum thrust
  

  int yValL = analogRead(yPinL); //read the potentiometer value and store it 
  data.pwmVal_L = map(yValL, 0, 1023, 1400, 1600);  //map incoming potentiometer values to those understood by the thruster // uncomment for less thrust
   // data.pwmVal_L = map(yValR, 0, 1023, 1100, 1900); // Uncomment for maximum thrust
  

  //BALL VALVES
  if (digitalRead(SwitchPin_BVA) == HIGH){
    data.path_A = LOW;
  } else {
    data.path_A = HIGH;
}

  if (digitalRead(SwitchPin_BVB) == HIGH){
    data.path_B = LOW;  
  } else {
    data.path_B = HIGH;
  }

  //PUMP// uncomment if testing with a button 
  // if (millis() - lastTimeButtonStateChangedPUMP > debounceDurationPUMP) {
  //   byte buttonStatePUMP = digitalRead(PumpswitchPin);
  //   if (buttonStatePUMP != lastButtonStatePUMP) {
  //     lastTimeButtonStateChangedPUMP = millis();
  //     lastButtonStatePUMP = buttonStatePUMP;
  //     if (buttonStatePUMP == LOW) {
  //       data.PumpDir = !data.PumpDir;
  //     }}}

  //PUMP// for switch direction control
   if (digitalRead(PumpswitchPin) == HIGH){
    data.PumpDir = HIGH;
  
  } else {
    data.PumpDir = LOW;
}
  int pwmVal = analogRead(PumpPWMpin);
  data.PumpPWM = map(pwmVal, 0, 1023, 0, 255);
 

  //ROTARY ENCODER//
  //Calculates the change in depth based on the rotary encoder notches
  if (counter >= MinCount  && counter <= MaxCount){
    if(lastCounter >= MinCount  && lastCounter <= MaxCount){
      if (counter >= lastCounter){
        change = counter - lastCounter;
      } else if (counter < lastCounter){
        change = lastCounter - counter;
      }
    } else if (lastCounter < MinCount){
      change = counter - MinCount;
    } else {
      change = MaxCount - counter;
    }
  }

  // handle the minimum and maximum cases
  if (counter > MaxCount){
    change = 0;
    if (lastCounter < MaxCount){
      change = MaxCount - lastCounter;
    }
  } else if (counter < MinCount){
    change = 0;
    if (lastCounter > MinCount){
    change = lastCounter - MinCount;
    }
  }

// Uses the button/switch in the rotary encoder to apporve the change in depth 
//Robotics Back End code 
  if (millis() - lastTimeButtonStateChangedRE > debounceDurationRE) {
    byte buttonStateRE = digitalRead(SW);
    if (buttonStateRE != lastButtonStateRE) {
      lastTimeButtonStateChangedRE = millis();
      lastButtonStateRE = buttonStateRE;
      if (buttonStateRE == LOW) {
      data.k = 1;


       //Serial.println("button pressed");  //uncomment to see when the rotary encoder button has been pressed
       lastCounter = counter;

      //Uncomment to see the depth and change displayed in the serial monitor
      // float depth = lastCounter/2.0; // increments are in 0.5ft
      // if (lastCounter > MaxCount){depth = MaxCount/2.0;}
      // if (lastCounter < MinCount){depth = MinCount/2.0;}
      // Serial.print("Depth: ");
      // Serial.print(depth);
      // Serial.print (" ||  Change: ");
      // Serial.println(change);
      // delay(1);
      }
    }
  }  
  if (data.k == 1){
    data.change = change;
    data.StepperDir = dir;
    change = 0;
  } else {
    data.change = 0;
    data.StepperDir = 1;
  }
  radio.write(&data, sizeof(data_pack)); //transmit the data to the receiver 
  data.k = 0;
}