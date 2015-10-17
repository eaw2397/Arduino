
#include <SHT1x.h>
#include <ESP8266WiFi.h>

const char* ssid     = "shivermetimbers";
const char* password = "fiddlesticks";

// SHT11 pins
#define dataPin 12
#define clockPin 14

long prevTime = 0;
int pin = 16;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
float totalConcentration = 0;
float totalTemp = 0;
float totalHumidity = 0;
int attempts = 0;

// instantiate SHT1x object
SHT1x sht1x(dataPin, clockPin);

void setup() {
  Serial.begin(115200); // Open serial connection to report values to host
  Serial.println("Starting up");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(2, OUTPUT);
  pinMode(13,INPUT);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  float temp_f;
  float humidity;
  char tempHumidity[20];
  char temp_as_str[20];
  char hum_combined[40];
  char temp_combined[40];
  char new_hum_combined[80];
  char new_temp_combined[80];
  long inital_time = millis();

  duration = pulseIn(pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;

  if (inital_time - prevTime > sampletime_ms) {
    attempts++;
    prevTime = millis();
    // Print the values to the serial port
    temp_f = sht1x.readTemperatureF();
    humidity = sht1x.readHumidity();
    Serial.print("Temperature: ");
    Serial.print(temp_f, DEC);
    Serial.print("F. Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    totalHumidity += humidity;
    totalTemp += temp_f;
    dtostrf(humidity,4,4,tempHumidity);
    dtostrf(temp_f,4,3,temp_as_str);
    sprintf(hum_combined, "Hum: %s", tempHumidity);
    sprintf(temp_combined, "Temp: %d", temp_as_str);
    ratio = lowpulseoccupancy / (sampletime_ms * 10.0); // Integer percentage 0=>100
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
    Serial.print("concentration = ");
    Serial.print(concentration);
    Serial.println(" pcs/0.01cf");
    Serial.println("\n");
    totalConcentration += concentration;
    Serial.println(attempts);
    if (attempts == 5){
      totalTemp = totalTemp / 5;
      totalHumidity = totalHumidity / 5;
      dtostrf(totalHumidity,4,4,tempHumidity);
      dtostrf(totalTemp,4,3,temp_as_str);
      sprintf(new_hum_combined, "Avg Hum: %d", int(totalHumidity));
      sprintf(new_temp_combined, "Avg Temp: %s", temp_as_str);
      totalConcentration = totalConcentration / 5;
      Serial.print("2.5min avg Concentration = ");
      Serial.print(totalConcentration);
      Serial.println(" pcs/0.01cf");
      Serial.println("\n");
      totalConcentration = 0;
      totalHumidity = 0;
      totalTemp = 0;
      attempts = 0;;
    }
    lowpulseoccupancy = 0;
  }
  delay(1000);
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
}
