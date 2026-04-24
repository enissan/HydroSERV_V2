//Turbidity pin init
#define turbidityPin A2

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  int turbidity = analogRead(turbidityPin); //gets turb values from analog sensor 
  float voltage = turbidity * (5.0f /1024.0f) ; //cnv ADC reading to Voltage
  Serial.print("Voltage: ");
  Serial.println(voltage);

  delay(5000);

}
