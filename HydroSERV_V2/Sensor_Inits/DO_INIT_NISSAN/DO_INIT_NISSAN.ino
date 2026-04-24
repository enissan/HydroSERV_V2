#include <Arduino.h>
#define V_REF    5000//VREF(mv)
#define ADC_RES 1024//ADC Resolution

void setup() {
  Serial.begin(9600);
}

uint32_t raw;

void loop()
{
    raw=analogRead(A1);
    Serial.println("raw:\t"+String(raw)+"\tVoltage(mv)"+String(raw*V_REF/ADC_RES));
    delay(1000);
}
