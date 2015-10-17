
#include <SHT1x.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <string.h>
#include "font.h"


#define offset 0x00    // SDD1306                      // offset=0 for SSD1306 controller
#define OLED_address  0x3c 

const char* ssid     = "shivermetimbers";
const char* password = "fiddlesticks";
const char* temp_txt = "Temp: ";
const char* humidity_txt = "Hum: ";
const char* dust_txt = "Dust: ";

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
  Wire.begin(4, 5);                               // Initialize I2C and OLED Display
  init_OLED();                                    //
  reset_display();
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
  sendStrXY((char*) ssid, 0, 0);
  char result[16];
  sprintf(result, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  sendStrXY(result, 2, 0);
  delay(2000);
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
  char tempFloatStr[80];
  long inital_time = millis();
  char* test_str;

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
    sprintf(hum_combined, "Hum: %d", tempHumidity);
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
      clear_display();
      totalTemp = totalTemp / 5;
      totalHumidity = totalHumidity / 5;
      dtostrf(totalHumidity,4,4,tempHumidity);
      dtostrf(totalTemp,4,3,temp_as_str);
      Serial.print("Avg Hum: ");
      Serial.println(totalHumidity);
      Serial.println("");
      Serial.print("Avg Temp: ");
      Serial.println(totalTemp);
      Serial.println("");
      totalConcentration = totalConcentration / 5;
      Serial.print("2.5min avg Concentration: ");
      Serial.print(totalConcentration);
      Serial.println(" pcs/0.01cf");
      Serial.println("\n");
      //Functions to print results to OLED
      test_str = strcat(floatToString(tempFloatStr, totalTemp, 2, 0), "F");
      sendStrXY((char *) temp_txt, 0, 0);
      sendStrXY(test_str, 0, strlen(temp_txt)); 
      test_str = strcat(floatToString(tempFloatStr, totalHumidity, 2, 0), "%");
      sendStrXY((char *) humidity_txt, 2, 0);
      sendStrXY(test_str, 2, strlen(humidity_txt)); 
      test_str = strcat(floatToString(tempFloatStr, totalConcentration, 2, 0), "/.01cf");
      sendStrXY((char *) dust_txt, 4, 0);
      sendStrXY(test_str, 4, strlen(dust_txt)); 
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

//==========================================================//
// Resets display depending on the actual mode.
static void reset_display(void)
{
  displayOff();
  clear_display();
  displayOn();
}

//==========================================================//
// Turns display on.
void displayOn(void)
{
  sendcommand(0xaf);        //display on
}

//==========================================================//
// Turns display off.
void displayOff(void)
{
  sendcommand(0xae);    //display off
}

//==========================================================//
// Clears the display by sendind 0 to all the screen map.
static void clear_display(void)
{
  unsigned char i, k;
  for (k = 0; k < 8; k++)
  {
    setXY(k, 0);
    {
      for (i = 0; i < (128 + 2 * offset); i++) //locate all COL
      {
        SendChar(0);         //clear all COL
        //delay(10);
      }
    }
  }
}

//==========================================================//
// Actually this sends a byte, not a char to draw in the display.
// Display's chars uses 8 byte font the small ones and 96 bytes
// for the big number font.
static void SendChar(unsigned char data)
{
  //if (interrupt && !doing_menu) return;   // Stop printing only if interrupt is call but not in button functions

  Wire.beginTransmission(OLED_address); // begin transmitting
  Wire.write(0x40);//data mode
  Wire.write(data);
  Wire.endTransmission();    // stop transmitting
}

//==========================================================//
// Prints a display char (not just a byte) in coordinates X Y,
// being multiples of 8. This means we have 16 COLS (0-15)
// and 8 ROWS (0-7).
static void sendCharXY(unsigned char data, int X, int Y)
{
  setXY(X, Y);
  Wire.beginTransmission(OLED_address); // begin transmitting
  Wire.write(0x40);//data mode

  for (int i = 0; i < 8; i++)
    Wire.write(pgm_read_byte(myFont[data - 0x20] + i));

  Wire.endTransmission();    // stop transmitting
}

//==========================================================//
// Used to send commands to the display.
static void sendcommand(unsigned char com)
{
  Wire.beginTransmission(OLED_address);     //begin transmitting
  Wire.write(0x80);                          //command mode
  Wire.write(com);
  Wire.endTransmission();                    // stop transmitting
}

//==========================================================//
// Set the cursor position in a 16 COL * 8 ROW map.
static void setXY(unsigned char row, unsigned char col)
{
  sendcommand(0xb0 + row);              //set page address
  sendcommand(offset + (8 * col & 0x0f)); //set low col address
  sendcommand(0x10 + ((8 * col >> 4) & 0x0f)); //set high col address
}


//==========================================================//
// Prints a string regardless the cursor position.
static void sendStr(unsigned char *string)
{
  unsigned char i = 0;
  while (*string)
  {
    for (i = 0; i < 8; i++)
    {
      SendChar(pgm_read_byte(myFont[*string - 0x20] + i));
    }
    *string++;
  }
}

//==========================================================//
// Prints a string in coordinates X Y, being multiples of 8.
// This means we have 16 COLS (0-15) and 8 ROWS (0-7).
static void sendStrXY( char *string, int X, int Y)
{
 setXY(X, Y);
  unsigned char i = 0;
  while (*string)
  {
    for (i = 0; i < 8; i++)
    {
      SendChar(pgm_read_byte(myFont[*string - 0x20] + i));
    }
    *string++;
  }
}


//==========================================================//
// Inits oled and draws logo at startup
static void init_OLED(void)
{
  sendcommand(0xae);    //display off
  sendcommand(0xa6);            //Set Normal Display (default)
  // Adafruit Init sequence for 128x64 OLED module
  sendcommand(0xAE);             //DISPLAYOFF
  sendcommand(0xD5);            //SETDISPLAYCLOCKDIV
  sendcommand(0x80);            // the suggested ratio 0x80
  sendcommand(0xA8);            //SSD1306_SETMULTIPLEX
  sendcommand(0x3F);
  sendcommand(0xD3);            //SETDISPLAYOFFSET
  sendcommand(0x0);             //no offset
  sendcommand(0x40 | 0x0);      //SETSTARTLINE
  sendcommand(0x8D);            //CHARGEPUMP
  sendcommand(0x14);
  sendcommand(0x20);             //MEMORYMODE
  sendcommand(0x00);             //0x0 act like ks0108

  //sendcommand(0xA0 | 0x1);      //SEGREMAP   //Rotate screen 180 deg
  sendcommand(0xA0);

  //sendcommand(0xC8);            //COMSCANDEC  Rotate screen 180 Deg
  sendcommand(0xC0);

  sendcommand(0xDA);            //0xDA
  sendcommand(0x12);           //COMSCANDEC
  sendcommand(0x81);           //SETCONTRAS
  sendcommand(0xCF);           //
  sendcommand(0xd9);          //SETPRECHARGE
  sendcommand(0xF1);
  sendcommand(0xDB);        //SETVCOMDETECT
  sendcommand(0x40);
  sendcommand(0xA4);        //DISPLAYALLON_RESUME
  sendcommand(0xA6);        //NORMALDISPLAY

  clear_display();
  sendcommand(0x2e);            // stop scroll
  //----------------------------REVERSE comments----------------------------//
  sendcommand(0xa0);    //seg re-map 0->127(default)
  sendcommand(0xa1);    //seg re-map 127->0
  sendcommand(0xc8);
  delay(1000);
  //----------------------------REVERSE comments----------------------------//
  // sendcommand(0xa7);  //Set Inverse Display
  // sendcommand(0xae);   //display off
  sendcommand(0x20);            //Set Memory Addressing Mode
  sendcommand(0x00);            //Set Memory Addressing Mode ab Horizontal addressing mode
  //  sendcommand(0x02);         // Set Memory Addressing Mode ab Page addressing mode(RESET)
}

char * floatToString(char * outstr, double val, byte precision, byte widthp){
 char temp[16];
 byte i;

 // compute the rounding factor and fractional multiplier
 double roundingFactor = 0.5;
 unsigned long mult = 1;
 for (i = 0; i < precision; i++)
 {
   roundingFactor /= 10.0;
   mult *= 10;
 }
 
 temp[0]='\0';
 outstr[0]='\0';

 if(val < 0.0){
   strcpy(outstr,"-\0");
   val = -val;
 }

 val += roundingFactor;

 strcat(outstr, itoa(int(val),temp,10));  //prints the int part
 if( precision > 0) {
   strcat(outstr, ".\0"); // print the decimal point
   unsigned long frac;
   unsigned long mult = 1;
   byte padding = precision -1;
   while(precision--)
     mult *=10;

   if(val >= 0)
     frac = (val - int(val)) * mult;
   else
     frac = (int(val)- val ) * mult;
   unsigned long frac1 = frac;

   while(frac1 /= 10)
     padding--;

   while(padding--)
     strcat(outstr,"0\0");

   strcat(outstr,itoa(frac,temp,10));
 }

 // generate space padding 
 if ((widthp != 0)&&(widthp >= strlen(outstr))){
   byte J=0;
   J = widthp - strlen(outstr);
   
   for (i=0; i< J; i++) {
     temp[i] = ' ';
   }

   temp[i++] = '\0';
   strcat(temp,outstr);
   strcpy(outstr,temp);
 }
 
 return outstr;
}
