//#include <DS18B20.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//8 Bit sensor array
DeviceAddress sensorAddress;

void setup() {
  Serial.begin(9600);
  sensors.begin();

//setup if statement taken from ChatGPT when I had to change the database format
if (!sensors.getAddress(sensorAddress, 0)) {
  Serial.println("Device not responding");
} else {
  Serial.print("Sensor Address:");
  for (uint8_t i = 0; i < 8; i++) {
      Serial.print(sensorAddress[i], HEX);
      if (i < 7) Serial.print(":");
  }
  Serial.println();
}
  //Setup complete, now indicating the start of temp sensing
  Serial.println("DS18B20 temp reading");
  delay(1000);
}


void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(sensorAddress); //gets temp values from digital sensor 

  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println("°C");
  } else {
    Serial.println("Error: Trouble reading data, check connection");
  }

  delay(5000);
}
