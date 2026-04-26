//3/27/26: RevH adds new EC probe code 

//  pins
#define ONE_WIRE_BUS   2 //defines temperature pin on Arduino Uno
#define DO_PIN         A1 //defines dissolved oxygen pin
#define TURBIDITY_PIN  A0 //defines turbidity pin
#define EC_PIN         A3 //defines electrical conductivity pin
#define PH_PIN         A2 //defines pH pin
#define SD_CS          10 //defines SD card data pin
#define UART_RX        8 //serial receiver pin for software communication between the Uno and MKR 
#define UART_TX        9 //serial transmitter pin for software communication between the Uno and MKR 

// libs
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DFRobot_EC10.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include "DFRobot_ECPRO.h"

//  globals
SoftwareSerial linkSerial(UART_RX, UART_TX); //creates a software serial port named linkSerial

OneWire oneWire(ONE_WIRE_BUS); //initializes OneWire communication on pin 2 for temp sensor
DallasTemperature sensors(&oneWire);
DeviceAddress tempSensor; //stores address of the temp sensor
DFRobot_ECPRO ec; //EC sensor object
RTC_DS3231 rtc; //real-time clock object
File logfile; //SD card logging file object

uint16_t InputVoltage;
float Conductivity;

#define V_REF 5000 //reference voltage 5000mV = 5V
#define ADC_RES 1024 //10-bit ADC resolution (0-1023)

char currentFilename[13];   // stores filename in YYYYMMDD.csv format
int lastLoggedDay = -1; //tracks last day logged to detect date change

bool sdAvailable = false;   // track SD availability

// DO reference table used for calibration (from DFRobot website)
const uint16_t DO_RefTABLE[41] = {
 14460,14220,13820,13440,13090,12740,12420,12110,11810,11530,
 11260,11010,10770,10530,10300,10080,9860,9660,9460,9270,
 9080,8900,8730,8570,8410,8250,8110,7960,7690,7560,
 7430,7300,7180,7070,6950,6840,6630,6530,6410
};

#define CAL1_V 644 //calibration voltage for DO sensor
#define CAL1_T 19 //temperature reference for DO sensor

// fxns 

// DO calculation
float readDO(uint32_t voltage_mv, uint8_t temperatureC) {
  uint16_t Vsat = CAL1_V + 35 * temperatureC - 35 * CAL1_T; //adjusts saturation based on T
  return (voltage_mv * DO_RefTABLE[temperatureC] / Vsat); //uses lookup table to compute DO
}

// Average array with min/max removal
double averageArray(int* arr, int count) {
  long sum = 0; //initializes tracking variables
  int minVal = arr[0], maxVal = arr[0];
  for (int i = 0; i < count; i++) { //loops through array
    if (arr[i] < minVal) minVal = arr[i]; //finds minimum value
    if (arr[i] > maxVal) maxVal = arr[i]; //finds maximun value
    sum += arr[i]; //adds values
  }
  return (double)(sum - minVal - maxVal) / (count - 2); //removes min and max to reduce noise
}

// Zero-padding helper
String pad2(int val) { //ensures number like 3 look like 03
  if (val < 10) return "0" + String(val);
  return String(val);
}

// Build daily CSV filename
void makeFilename(DateTime now) { //builds filename in YYYYMMDD.csv format
  snprintf(currentFilename, sizeof(currentFilename), //formats string
           "%04d%02d%02d.csv",
           now.year(), now.month(), now.day());
}

// SD Logging 
bool initLogging() { //formats string
  Serial.println(F("[LOGGER] Initializing SD..."));

  pinMode(SD_CS, OUTPUT); //prepares SD chip select
  digitalWrite(SD_CS, HIGH);  // ensure CS is idle

  SPI.begin(); //starts SPI
  delay(500); // allow SD module to power up

  if (!SD.begin(SD_CS)) { //attempts SD initialization
    Serial.println(F("[LOGGER] SD init FAILED!")); //print if failure
    return false;
  }

  Serial.println(F("[LOGGER] SD init SUCCESS")); //print if success

  // RTC initialization
  if (!rtc.begin()) { //attempts RTC initialization
    Serial.println(F("[LOGGER] RTC not found!")); //print if failure
    return false;
  }

  //Resetting RTC:
  // OPTION 1: Force reset RTC to compile time
  // Uncomment the next 2 lines, upload once, then comment again.
  //Serial.println(F("[LOGGER] FORCING RTC RESET TO COMPILE TIME"));
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // (BEST) OPTION 2: Set a specific custom date/time manually
  // Example: January 1, 2026 at 12:00:00 -> (2026, 1, 1, 12, 0, 0)
  //rtc.adjust(DateTime(2026, 3, 16, 14, 23, 50));

  if (rtc.lostPower()) { //if RTC reset, set time to compile time
    Serial.println(F("[LOGGER] RTC lost power, resetting time..."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc.now(); //gets current time
  makeFilename(now); //creates today's filename
  lastLoggedDay = now.day();

  logfile = SD.open(currentFilename, FILE_WRITE); //opens or creates CSV file
  if (!logfile) {
    Serial.println(F("[LOGGER] Failed to create CSV file!"));
    return false;
  }

  if (logfile.size() == 0) //if new file, write new header
    logfile.println("Timestamp,TemperatureC,TurbidityV,DOmgL,ECmS,pH");

  logfile.close(); //closes file
  return true;
}

void logToCSV(float t, float tv, float d, float ecv, float ph) { //logs sensor data to SD card
  if (!sdAvailable) return; // skip if SD is unavailable

  DateTime now = rtc.now();

  if (now.day() != lastLoggedDay) { //creates new file each new day
    makeFilename(now);
    lastLoggedDay = now.day();
  }

  logfile = SD.open(currentFilename, FILE_WRITE); //opens CSV file
  if (!logfile) {
    Serial.println(F("[LOGGER] ERROR: Cannot open CSV"));
    sdAvailable = false; // mark SD as unavailable
    return;
  }

  //writes timestamp
  logfile.print(now.year()); logfile.print("-");
  logfile.print(pad2(now.month())); logfile.print("-");
  logfile.print(pad2(now.day())); logfile.print(" ");
  logfile.print(pad2(now.hour())); logfile.print(":");
  logfile.print(pad2(now.minute())); logfile.print(":");
  logfile.print(pad2(now.second())); logfile.print(",");

  //writes sensor values
  logfile.print(t); logfile.print(",");
  logfile.print(tv); logfile.print(",");
  logfile.print(d); logfile.print(",");
  logfile.print(ecv); logfile.print(",");
  logfile.println(ph);

  logfile.close(); //saves file
}

// Send to MKR 
void sendPacket(float t, float tv, float d, float ecv, float ph) {
  DateTime now = rtc.now();

  //sends timestamp over serial
  linkSerial.print("<");
  linkSerial.print(now.year()); linkSerial.print("-");
  linkSerial.print(pad2(now.month())); linkSerial.print("-");
  linkSerial.print(pad2(now.day())); linkSerial.print(" ");
  linkSerial.print(pad2(now.hour())); linkSerial.print(":");
  linkSerial.print(pad2(now.minute())); linkSerial.print(":");
  linkSerial.print(pad2(now.second())); linkSerial.print(",");

  //sends sensor values
  linkSerial.print(t); linkSerial.print(",");
  linkSerial.print(tv); linkSerial.print(",");
  linkSerial.print(d); linkSerial.print(",");
  linkSerial.print(ecv); linkSerial.print(",");
  linkSerial.print(ph);
  linkSerial.println(">");
}

// Setup
void setup() {
  Serial.begin(9600); //starts serial communication
  linkSerial.begin(9600);

  // SD first
  sdAvailable = initLogging(); //initializes SD and RTC
  if (!sdAvailable) {
    Serial.println(F("[SETUP] WARNING: SD card unavailable. Logging will be skipped."));
  }

  // Sensors
  sensors.begin(); //starts sensors
  //ec.begin();
  ec.setCalibration(1.41);  //found/replace K value with calibration
  if (!sensors.getAddress(tempSensor, 0)) //gets temp sensor address
    Serial.println("[SETUP] Temp sensor not found!");

  Serial.println(F("[SETUP] System ready."));
}

// LOOP 
void loop() {
  // Temperature
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(tempSensor); //reads temperature
  if (tempC == DEVICE_DISCONNECTED_C) tempC = 25; //fallback if sensor fails

  // Turbidity
  float turbV = analogRead(TURBIDITY_PIN) * (5.0 / 1024.0); //converts analog reading to voltage

  // DO
  uint32_t DOmV = analogRead(DO_PIN) * V_REF / ADC_RES; //converts ADC to millivolts
  float DOmg = readDO(DOmV, tempC) / 1000.0; //converts to mg/L

  // EC
  InputVoltage = (uint32_t)analogRead(EC_PIN) * 5000 / 1024; //reads EC with temperature compensation
  Conductivity = ec.getEC_us_cm(InputVoltage);

  // pH
  static int pHbuf[40]; //buffer for smoothing
  static int idx = 0;
  pHbuf[idx++] = analogRead(PH_PIN); //store pH reading
  if (idx >= 40) idx = 0; //circular buffer

  float phV = averageArray(pHbuf, 40) * 5.0 / 1024.0; //smooth and convert to voltage
  float phRaw = (10.34 * phV - 13.69); //linear calibration
  float pH = 0.0309 * phRaw * phRaw + 0.924 * phRaw - 0.2; //polynomial correction

  // Debug: reads printable output to serial monitor
  Serial.println("========== SENSOR READINGS ==========");
  Serial.print("Temp: "); Serial.println(tempC);
  Serial.print("Turb: "); Serial.println(turbV);
  Serial.print("DO: "); Serial.println(DOmg);
  Serial.println("Conductivity: " + String(Conductivity) + " us/cm");
  Serial.print("pH: "); Serial.println(pH);
  Serial.println("-------------------------------------");

  //  Log to SD if available 
  logToCSV(tempC, turbV, DOmg, Conductivity, pH);

  //  Always send to MKR 
  sendPacket(tempC, turbV, DOmg, Conductivity, pH);

  delay(5000); // 5-second loop
}
