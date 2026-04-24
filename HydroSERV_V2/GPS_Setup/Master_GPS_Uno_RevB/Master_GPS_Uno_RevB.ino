//Code created by Katelyn Byrne on 1/30/26
//RevB: manual rtc reset

// pins
#define SD_CS   10
#define GPS_RX  7
#define GPS_TX  6

// libs
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// objects
RTC_DS3231 rtc;
File logfile;

SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// globals
char currentFilename[13];   // YYYYMMDD.csv
int lastLoggedDay = -1;

// Helpers
String pad2(int v) {
  if (v < 10) return "0" + String(v);
  return String(v);
}

void makeFilename(DateTime now) {
  snprintf(currentFilename, sizeof(currentFilename),
           "%04d%02d%02d.csv",
           now.year(), now.month(), now.day());
}

// Initialize logging
bool initLogging() {
  Serial.println(F("[LOGGER] Initializing SD..."));
  if (!SD.begin(SD_CS)) return false;

  if (!rtc.begin()) return false;

  // Resetting RTC

  // OPTION 1: Force reset RTC to compile time
  // Uncomment the next 2 lines, upload once, then comment again.
  //Serial.println(F("[LOGGER] FORCING RTC RESET TO COMPILE TIME"));
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // OPTION 2: Set a specific custom date/time manually
  // Example: February 14, 2026 at 13:14:00
  //rtc.adjust(DateTime(2026, 4, 6, 14, 38, 30));

  // ================================

  DateTime now = rtc.now();
  makeFilename(now);
  lastLoggedDay = now.day();

  logfile = SD.open(currentFilename, FILE_WRITE);
  if (!logfile) return false;

  if (logfile.size() == 0)
    logfile.println("Timestamp,Latitude,Longitude,Satellites");

  logfile.close();
  return true;
}

// Log to CSV

void logToCSV(double lat, double lng, int sats) {

  DateTime now = rtc.now();

  // New file if day changes
  if (now.day() != lastLoggedDay) {
    makeFilename(now);
    lastLoggedDay = now.day();
  }

  logfile = SD.open(currentFilename, FILE_WRITE);
  if (!logfile) return;

  logfile.print(now.year()); logfile.print("-");
  logfile.print(pad2(now.month())); logfile.print("-");
  logfile.print(pad2(now.day())); logfile.print(" ");
  logfile.print(pad2(now.hour())); logfile.print(":");
  logfile.print(pad2(now.minute())); logfile.print(":");
  logfile.print(pad2(now.second())); logfile.print(",");

  logfile.print(lat, 6); logfile.print(",");
  logfile.print(lng, 6); logfile.print(",");
  logfile.println(sats);

  logfile.close();
}

// Setup
void setup() {

  Serial.begin(9600);
  gpsSerial.begin(9600);
  delay(1000);

  if (!initLogging()) {
    Serial.println(F("[SETUP] Logger init failed"));
    while (1);
  }

  Serial.println(F("[SETUP] RTC is authoritative for timestamps"));
}

// Loop
void loop() {

  // Read GPS data
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  double lat = gps.location.isValid() ? gps.location.lat() : 0.0;
  double lng = gps.location.isValid() ? gps.location.lng() : 0.0;
  int sats   = gps.satellites.isValid() ? gps.satellites.value() : 0;

  // Log using RTC timestamp ONLY
  logToCSV(lat, lng, sats);

  // Serial debug output
  DateTime now = rtc.now();
  Serial.print("[LOGGED] ");
  Serial.print(now.year()); Serial.print("-");
  Serial.print(pad2(now.month())); Serial.print("-");
  Serial.print(pad2(now.day())); Serial.print(" ");
  Serial.print(pad2(now.hour())); Serial.print(":");
  Serial.print(pad2(now.minute())); Serial.print(":");
  Serial.print(pad2(now.second()));
  Serial.print(" | Lat: "); Serial.print(lat, 6);
  Serial.print(" | Lng: "); Serial.print(lng, 6);
  Serial.print(" | Sats: "); Serial.println(sats);

  delay(5000);
}
