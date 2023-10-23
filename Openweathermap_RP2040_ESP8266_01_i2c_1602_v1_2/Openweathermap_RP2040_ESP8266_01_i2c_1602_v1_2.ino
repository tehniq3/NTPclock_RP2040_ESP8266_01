/****************************************************************************************************************************
  WebClientRepeating.ino - Simple Arduino web server sample for ESP8266 AT-command shield
  For ESP8266/ESP32 AT-command running shields

  ESP8266_AT_WebServer is a library for the ESP8266/ESP32 AT-command shields to run WebServer
  Based on and modified from ESP8266 https://github.com/esp8266/Arduino/releases
  Built by Khoi Hoang https://github.com/khoih-prog/ESP8266_AT_WebServer
  Licensed under MIT license
  
  Original author:
  @file       Esp8266WebServer.h
  @author     Ivan Grokhotkov

 Weather station by Nicu Florica
 v.0.0 - initial version (stable)
 v.1.0 - added i2c LCD1602 display + find solution to extract good info
 v.1.1 - if no new good info, remain previous data, see https://www.delftstack.com/howto/arduino/arduino-split-string/
 v.1.2 - moved crediantial info in defines.h
 *****************************************************************************************************************************/

 #include <Wire.h> //This library allows you to communicate with I2C devices  
 //SDA -> GP4 and SCL -> GP5    
 #include <LiquidCrystal_I2C.h> //  This library is for character LCDs based on the HD44780 controller connected via I2C bus using the cheap I2C backpack modules based on the PCF8574 
 //LiquidCrystal_I2C lcd(0x3F,16,2);  // //https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library  
 LiquidCrystal_I2C lcd(0x27,16,2);  // if lcd is not print then use this 0x27..  

// Credits of [Miguel Alexandre Wisintainer](https://github.com/tcpipchip) for this simple yet effective method
// For some STM32, there is only definition of Serial in variant.h, and is used for Serial/USB Debugging
// For example, in Nucleo-144 F767ZI original variant.h
//
// #define SERIAL_PORT_MONITOR     Serial
// #define SERIAL_PORT_HARDWARE    Serial
//
// To use ESP8266/ESP32-AT, we need another Serial, such as Serial1
// To do this, first, in corresponding variant.h, modify as follows:

// #define SERIAL_PORT_HARDWARE    Serial1
//
// then assign pins D0 = RX/D1 = TX to be Hardware Serial1 by putting in sketch as follows:
//
// #define EspSerial      SERIAL_PORT_HARDWARE    //Serial1
// HardwareSerial         Serial1(D0, D1);
//
// This must be included in defines.h for each board you'd like to use ESPSerial as Serial1
//
// The pin usage must be modified according to your boards.

#include "defines.h"

int status = WL_IDLE_STATUS;      // the Wifi radio's status

unsigned long lastConnectionTime = 800000L;         // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 900000L; // delay between updates, in milliseconds

// Initialize the Web client object
ESP8266_AT_Client client;


String raspuns;
String raspunsok;
byte arata = 0;
byte aratad = 0;
unsigned long tpmestecare;
byte receptie = 0;
String weatherDescription ="";
String weatherLocation = "";
String Country;
float temperatura=0.0;
int tempint, temprest;
float tempmin, tempmax;
int umiditate;
int presiune;
int vant, directie, nori;
String descriere;
byte schimbare, schimbare0;
unsigned long tpschimbare;
unsigned long tpsch = 3000;
byte nook = 1;

void setup()
{
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  lcd.begin();    //initialize lcd screen   
  lcd.backlight();  // turn on the backlight
  lcd.setCursor(0,0); 
  lcd.print(" Weatherstation ");  
  lcd.setCursor(0,1);  
  lcd.print(" RP2040+ESP8266 ");  
 
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.print(F("\nStarting WebClientRepeating on ")); Serial.print(BOARD_NAME);
  Serial.print(F(" with ")); Serial.println(SHIELD_TYPE); 
  Serial.println(ESP8266_AT_WEBSERVER_VERSION);

  // initialize serial for ESP module
  EspSerial.begin(115200);
  // initialize ESP module
  WiFi.init(&EspSerial);

  Serial.println(F("WiFi shield init done"));

  lcd.setCursor(0,0); 
  lcd.print(" Weatherstation ");  
  lcd.setCursor(0,1);  
  lcd.print(" v1.2 by niq_ro ");  

 //check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println(F("WiFi shield not present"));
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED)
  {
    Serial.print(F("Connecting to SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  printWifiStatus();
  
  lcd.clear();
}

void loop()
{

  // if there's incoming data from the net connection send it out the serial port
  // this is for debugging purposes only 
  while (client.available())
  {
    char c = client.read();
  //  Serial.write(c);
    raspuns = raspuns + c;
    receptie = 1;
  }

  if (receptie == 1)
  {
  client.stop();
  Serial.flush();
  Serial.println(F("---------\\\\\\\\\\\-----"));
  Serial.println(F("---------\\\\begin\\\\-----"));
  Serial.println(raspuns);
  Serial.println(F("---------////end////-----"));
  receptie = 0;
  aratad = 1;
  }
  
if (aratad == 1)
{
 raspuns.replace('[', ' ');
 raspuns.replace(']', ' ');
 int index1 = raspuns.lastIndexOf("coord");
 int index2 = raspuns.length();
 int rest = index2 - index1;
// String sub_S = raspuns.substring(index1-2, index2); // https://www.delftstack.com/howto/arduino/arduino-split-string/
// Serial.println(sub_S);
 
 Serial.print(F("rest = "));
 Serial.print(index2);
 Serial.print(F(" - "));
 Serial.print(index1);
 Serial.print(F(" = "));
 Serial.println(rest);
// raspunsok = raspuns.substring(365);
raspunsok = raspuns.substring(index1-2, index2); // https://www.delftstack.com/howto/arduino/arduino-split-string/
  
  Serial.print(F("clean info: "));
  Serial.println(raspunsok.length());
  Serial.println(raspunsok);
   
char jsonArray [raspunsok.length()+1];
raspunsok.toCharArray(jsonArray,sizeof(jsonArray));
jsonArray[raspunsok.length() + 1] = '\0';
StaticJsonBuffer<1024> json_buf;  // v.5
JsonObject &root = json_buf.parseObject(jsonArray); // v.5

if (!root.success())
  {
  //  Serial.println("parseObject() failed ?!!!!");
  }

String location = root["name"];
String icon = root["weather"]["icon"];    // "weather": {"id":800,"main":"Clear","description":"clear sky","icon":"01n"} 
String id = root["weather"]["description"]; 
String description = root["weather"]["description"];
int clouds = root["clouds"]["all"]; // "clouds":{"all":0},
String country = root["sys"]["country"];
float temperature1 = root["main"]["temp"];
float temperaturemin = root["main"]["temp_min"];
float temperaturemax = root["main"]["temp_max"];
float humidity = root["main"]["humidity"];
String weather = root["weather"]["main"];
//String description = root["weather"]["description"];
float pressure = root["main"]["pressure"];
float wind = root["wind"]["speed"];//"wind":{"speed":2.68,"deg":120},
int deg = root["wind"]["deg"];

// "sys":{"type":2,"id":50395,"country":"RO","sunrise":1683428885,"sunset":1683480878},
//weatherDescription = description;
if (humidity !=0)
{
weatherDescription = weather;
weatherLocation = location;
Country = country;
temperatura = temperature1;
tempint = (int)(10*temperatura)/10;
temprest = (int)(10*temperatura)%10;
tempmin = temperaturemin;
tempmax = temperaturemax;
umiditate = humidity;
presiune = pressure*0.75006;  // mmH20
vant = (float)(3.6*wind+0.5); // km/h
directie = deg;
nori = clouds;
descriere = description;

Serial.print(weather);
Serial.print(" / ");
Serial.println(description);
Serial.print(temperature1);
Serial.print("°C / ");  // as at https://github.com/tehniq3/datalloger_SD_IoT/
Serial.print(umiditate);
Serial.print("%RH, ");
// check https://github.com/tehniq3/matrix_clock_weather_net/blob/master/LEDMatrixV2ro2a/weather.ino
Serial.print(presiune);
Serial.print("mmHg / wind = ");
Serial.print(vant);
Serial.print("km/h ,  direction = ");
Serial.print(deg);
if (deg >=360) deg = deg%360;  // 0..359 degree
Serial.println("° ");
Serial.print("ID weather = ");
Serial.print(id);
Serial.print(" + clouds = ");
Serial.print(nori);
Serial.print("% + icons = ");
Serial.print(icon);
Serial.println(" (d = day, n = night)");

//Serial.print(weather);
//Serial.print(" / ");
Serial.println(description);
Serial.print(temperatura);
Serial.print("°C / ");  // as at https://github.com/tehniq3/datalloger_SD_IoT/
Serial.print(humidity);
Serial.println("%RH, ");
// check https://github.com/tehniq3/matrix_clock_weather_net/blob/master/LEDMatrixV2ro2a/weather.ino
Serial.print(presiune);
Serial.print("mmHg / wind = ");
Serial.print(vant);
Serial.print("km/h ,  direction = ");
Serial.print(deg);
if (deg >=360) deg = deg%360;  // 0..359 degree
Serial.println("° ");
Serial.print("ID weather = ");
Serial.print(id);
Serial.print(" + clouds = ");
Serial.print(nori);
Serial.print("% + icons = ");
Serial.print(icon);
nook = 0;
}
else 
{
  nook = 1;
  Serial.println(" no good data ! "); 
}
 aratad = 0;
  }

  // if time for check the weather info have passed since your last connection,
  // then connect again and send data
  if (millis() - lastConnectionTime > postingInterval)
  {
    httpRequest();
    arata = 1;
  }

// lcd part
  lcd.setCursor(0,0); 
  lcd.print("Craiova, Romania");  
  lcd.setCursor(0,1);  


if (schimbare0 != schimbare)
if (umiditate !=0)
{
  //lcd.clear();
  lcd.setCursor(0,1); 
  lcd.print("                ");
  lcd.setCursor(0,1);
  if (schimbare == 0) 
    { 
    lcd.print(descriere);
    if (nook == 1)
    {
    lcd.setCursor(15,1); 
    lcd.print("!");
    }
    }
  else
  if (schimbare == 1) 
    {
     lcd.print("temp = ");
     if (temperatura > 0) 
        lcd.print("+"); 
     lcd.print(temperatura);
     lcd.write(0b11011111);
     lcd.print("C "); 
     /*
     lcd.print("(");
     lcd.print(tempmin);
     lcd.print("..");
     lcd.print(tempmax);
     lcd.print(")");
     */
    }
  else
  if (schimbare == 2) 
    {
     lcd.print("humidity:");
     if (umiditate < 100)
        lcd.print(" ");
     if (umiditate < 10)  
        lcd.print(" ");
     lcd.print(umiditate);
     lcd.print("%RH");
    } 
   else
   if (schimbare == 3) 
    {
     lcd.print("pressure:");
     lcd.print(presiune);
     lcd.print("mmHg");
    } 
     else
   if (schimbare == 4) 
    {
     lcd.print("wind:");
     if (vant < 10)  
        lcd.print(" ");
     lcd.print(vant);
     lcd.print("km/h ");
     if (directie < 100)
        lcd.print(" ");
     if (directie < 10)  
        lcd.print(" ");
     lcd.print(directie);
     lcd.write(0b11011111);
    }  
}
else
{
  lcd.setCursor(0,1); 
  lcd.print("no weather info!");
}
  
schimbare0 = schimbare;
//delay(1);
if (millis() - tpschimbare > tpsch)
{
  schimbare++;
  if (schimbare > 4)
      schimbare = 0; 
  tpschimbare = millis();    
}

digitalWrite(led, (millis()/1000)%2);
}  // end main loop

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  // you're connected now, so print out the data
  Serial.print(F("You're connected to the network, IP = "));
  Serial.println(WiFi.localIP());

  Serial.print(F("SSID: "));
  Serial.print(WiFi.SSID());

  // print the received signal strength:
  int32_t rssi = WiFi.RSSI();
  Serial.print(F(", Signal strength (RSSI): "));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

// this method makes a HTTP connection to the server
void httpRequest()
{
  Serial.println();
 // close any connection before send a new request
  // this will free the socket on the WiFi shield
  client.stop();
  
  // if there's a successful connection
  if (client.connect(servername, 80))
  {
    Serial.println(F("Connecting..."));

    // send the HTTP PUT request
   client.print(F("GET /data/2.5/weather?id="));
   client.print(CityID);
   client.print(F("&units=metric&APPID="));
   client.print(APIKEY);
   client.println(F(" HTTP/1.1"));
   client.println(F("Host: api.openweathermap.org"));
   client.println(F("Connection: close"));
   client.println();

    // note the time that the connection was made
    lastConnectionTime = millis();
  }
  else
  {
    // if you couldn't make a connection
    Serial.println(F("Connection failed"));
  }
raspuns = "";
}
