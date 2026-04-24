#include "DFRobot_ECPRO.h"

#define EC_PIN A1

DFRobot_ECPRO ec;

uint16_t InputVoltage;
float Conductivity;

void setup()
{
  Serial.begin(9600);

  ec.setCalibration(1.41); //Replace the 1 with the calibrated K value if it's calibrated
  Serial.println("Default Calibration K=" + String(ec.getCalibration()));
}

void loop()
{
  InputVoltage = (uint32_t)analogRead(EC_PIN) * 5000 / 1024;
  Conductivity = ec.getEC_us_cm(InputVoltage);

  Serial.print("InputVoltage: " + String(InputVoltage) + " mV\t");
  Serial.println("Conductivity: " + String(Conductivity) + " us/cm");

  delay(1000);
}