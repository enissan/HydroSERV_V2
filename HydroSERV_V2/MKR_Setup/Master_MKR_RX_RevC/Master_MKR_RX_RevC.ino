//MKR 1310 Reciever code - used for onshore visualization
//Created by Katelyn Byrne on 1/6/2026
//Rev C: Instead of using PLX-DAQ, this revision projects the data to Arduino IDE's built in serial plotter


#include <LoRa.h>

// LoRa Settings 
#define LORA_FREQ 915E6  // Must match TX (US915)

// Globals
String incomingPacket = "";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize LoRa
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Match TX settings
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  // Optional legend in monitor (ignored by plotter)
  Serial.println("Legend:");
  Serial.println("TempC = Temperature (C)");
  Serial.println("TurbV = Turbidity (V)");
  Serial.println("DOmg  = Dissolved Oxygen (mg/L)");
  Serial.println("ECmS  = Conductivity (mS)");
  Serial.println("pH    = pH");
  Serial.println("---- Plot Below ----");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    incomingPacket = "";
    while (LoRa.available()) {
      char c = (char)LoRa.read();
      incomingPacket += c;
    }

    // Remove leading '<' and trailing '>' if present
    if (incomingPacket.startsWith("<") && incomingPacket.endsWith(">")) {
      incomingPacket.remove(0, 1); // remove '<'
      incomingPacket.remove(incomingPacket.length() - 1); // remove '>'
    }

    // incomingPacket format:
    // YYYY:MM:DD HH:MM:SS,TempC,TurbV,DOmg,ECmS,pH

    int firstComma = incomingPacket.indexOf(',');
    if (firstComma < 0) return; // malformed

    String sensorValues = incomingPacket.substring(firstComma + 1);

    // Parse CSV values 
    float vals[5];
    int idx = 0;
    int last = 0;

    for (int i = 0; i < sensorValues.length(); i++) {
      if (sensorValues.charAt(i) == ',' || i == sensorValues.length() - 1) {
        int endIdx = (i == sensorValues.length() - 1) ? i + 1 : i;
        vals[idx++] = sensorValues.substring(last, endIdx).toFloat();
        last = i + 1;
        if (idx >= 5) break;
      }
    }

    if (idx < 5) return; // not enough data

    // Send labeled data to Serial Plotter 
    Serial.print("TempC:"); Serial.print(vals[0]); Serial.print(" ");
    Serial.print("TurbV:"); Serial.print(vals[1]); Serial.print(" ");
    Serial.print("DOmg:");  Serial.print(vals[2]); Serial.print(" ");
    Serial.print("ECmS:");  Serial.print(vals[3]); Serial.print(" ");
    Serial.print("pH:");    Serial.println(vals[4]);
  }
}

