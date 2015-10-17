
#include <SHT1x.h>

// SHT11 pins
#define dataPin 12
#define clockPin 14

long prevTime = 0;
// instantiate SHT1x object
SHT1x sht1x(dataPin, clockPin);

void setup() {
  Serial.begin(115200); // Open serial connection to report values to host
  Serial.println("Starting up");
  pinMode(2, OUTPUT);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  float temp_c;
  float temp_f;
  float humidity;
  char stringone[20];
  char temp_as_str[20];
  char hum_combined[40];
  char temp_combined[40];
  long inital_time = millis();
      
  if (inital_time - prevTime > 60000) {
    prevTime = millis();
    // Print the values to the serial port
    temp_c = sht1x.readTemperatureC();
    temp_f = sht1x.readTemperatureF();
    humidity = sht1x.readHumidity();
    Serial.print("Temperature: ");
    Serial.print(temp_c, DEC);
    Serial.print("C / ");
    Serial.print(temp_f, DEC);
    Serial.print("F. Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    dtostrf(humidity,4,4,stringone);
    dtostrf(temp_f,4,3,temp_as_str);
    sprintf(hum_combined, "Hum: %s", stringone);
    sprintf(temp_combined, "Temp: %s", temp_as_str);
  }
  delay(1000);
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
}
