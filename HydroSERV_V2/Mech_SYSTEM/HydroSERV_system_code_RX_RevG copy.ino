//Arduino Wireless Communication
//Hydro_SERV system: ball valves, thrusters, pump, and stepper motor
//Receiver Code by Megan Smith 4/29/2025

//include necessary libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <AccelStepper.h> 
#include <Rotary.h> https://github.com/brianlow/Rotary.git

//declare RF pins, address, and variables
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "HSERV";

//declare global variables
byte ThrusterPinR = A0; //THRUSTER -R //
byte ThrusterPinL = A1; //THRUSER-L //
int BVAPin = 4; //BALL VALVE A//
int BVBPin = 5; //BALL VALVE B//
#define PumpENApin  A2 //PUMP//
#define PumpDirPin  2 //PUMP//
#define driverPUL 9  // HOSE REEL// PUL- pin
#define driverDIR 10  // HOSE REEL// DIR- pin
//const float steps = 1671.54;  // Steps per encoder step. Adjust for amount of rotation. This equates to 0.5 ft
const float steps = 5484.04359;

struct data_pack
{
  int pwmVal_R = 1500; // THRUSTER-R //initialize pwm val
  int pwmVal_L = 1500; //THRUSTER -L //initialize pwm val
  boolean path_A = HIGH; //BALL VALVE 1 // set default path
  boolean path_B = HIGH; //BALL VALVE 2 // set default path
  byte PumpDir = LOW; //PUMP // set default direction
  int PumpPWM = 1500; //PUMP // set default speed
  int StepperDir = 1; //HOSE REEL// stepper motor direction
  int change = 0; //HOSE REEL // rotary encoder change 
  int k = 0; //HOSE REEL// SW check for release
};

data_pack data;
Servo ThrusterR; // set up right thruster 
Servo ThrusterL; //set up left thruster 



//set up to run once 
void setup() {

  //THRUSTER SET UP//
  ThrusterR.attach(ThrusterPinR); // attatch pin A0 to right thruster 
  ThrusterL.attach(ThrusterPinL); //attach pin A1 to left thruster 
  ThrusterR.writeMicroseconds(data.pwmVal_R); // write an output to the servo pin of 1500 microseconds. Initializes the speed controller
  ThrusterL.writeMicroseconds(data.pwmVal_L); // write an output to the servo pin of 1500 microseconds. Initializes the speed controller

  //BALL VALVE SET UP//
  pinMode(BVAPin,OUTPUT); //configures BVAPin to behave as an output
  pinMode(BVBPin, OUTPUT); //configures BVBPin to behave as an output

  //PUMP SET UP//
  pinMode(PumpENApin, OUTPUT);
  pinMode(PumpDirPin, OUTPUT);
 
  //HOSE REEL-STEPPER MOTOR SET UP//
  pinMode (driverPUL, OUTPUT);
  pinMode (driverDIR, OUTPUT);

  //SERIAL MONITOR & NRF TRANSCIEVER SET UP
  Serial.begin(9600); //sets baud of serial monitor
  radio.begin(); //initialize radio object
  radio.openReadingPipe(0, address); //listen to pipe 0 and set same address as transmitter
  radio.setPALevel(RF24_PA_MIN); // set to min since the modules are close to each other
   //radio.setPALevel(RF24_PA_MAX); //uncomment to set to max range
  radio.startListening(); //fxn as receiver
}

int hoseReel (int depthChange){
      while (depthChange > 0){ // move the stepper motor based on the changes in depth 
        for (long i = 0; i < steps; i++) {
          digitalWrite(driverPUL, HIGH);
          delayMicroseconds(710);  // Adjust speed if needed
          digitalWrite(driverPUL, LOW);
          delayMicroseconds(710);
        }
      depthChange --;
    }
}

void loop() {
  if (radio.available()){  //checks if there is data to be received 
    radio.read(&data, sizeof(data_pack)); //reads the incoming data

    //THRUSTER CONTROL//
    ThrusterR.writeMicroseconds(data.pwmVal_R); //write pwm signal to thruster R
    ThrusterL.writeMicroseconds(data.pwmVal_L); //write pwm signal to thruster L 

    //BALL VALVE CONTROL//
    digitalWrite(BVAPin, data.path_A); //writes state/command of switch to the ball valve 
    digitalWrite(BVBPin, data.path_B);
   
    //PUMP CONTROL//
    digitalWrite(PumpDirPin, data.PumpDir);
    analogWrite(PumpENApin, data.PumpPWM);
    Serial.println(data.PumpPWM);

    //HOSE REEL-STEPPER MOTOR CONTROL//
    if(data.k == 1){
      digitalWrite(driverDIR, data.StepperDir);  // Set motor direction from rotary encoder
      data.change = hoseReel(data.change);
    }
  }
}