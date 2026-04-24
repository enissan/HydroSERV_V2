//MKR WAN 1310 TX code
//Receives UART packets from Arduino Uno and forwards via LoRa

#include <LoRa.h>

//LoRa Settings 
#define LORA_FREQ 915E6   // US915, must match RX
#define LORA_SPREAD 7     // default SF7
#define LORA_BW 125E3     // bandwidth
#define LORA_CR 5         // coding rate

// UART from UNO 
#define UNO_RX_PIN 14   // MKR RX connected to Uno TX
#define UNO_TX_PIN 13   // optional

String buffer = "";     // stores incoming packet

// Setup
void setup() {
  Serial.begin(9600);       //Debug monitor
  //while (!Serial); //keep this commented!!

  Serial1.begin(9600);      //UART from Uno

  Serial.println("MKR TX (UART → LoRa) starting...");

  // Initialize LoRa
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  LoRa.setSpreadingFactor(LORA_SPREAD);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);

  Serial.println("LoRa initialized. Ready to forward packets.");
}

//  LOOP
void loop() {
  // Read all characters from Uno over Serial1
  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '<') buffer = ""; //start of packet
    else if (c == '>') {  //end of packet
      sendLoRa(buffer); //forward via LoRa
      buffer = "";
    }
    else buffer += c;
  }
}

// LoRa Send to MKR RX
void sendLoRa(String payload) {
  if (payload.length() == 0) return;

  Serial.print("Sending LoRa: ");
  Serial.println(payload);

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
}
