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
 v.3.0 - added NTP clock v.2.2 
 v.3.1..v.3.3 - small bugfixex, show on display if wifi not connected
 v.3.4 - added updated time for weater on display, added percent of clouds if exist
 v.3.5 - moved to RP2040-Zero (GP8 for DST switch). but works as well on Rpi Pico (flashing led works just with RPi Pico)
 v.3.6 - ingored wrong data for clock, using previous good data + RGB led flash on RP2040-Zero
 *****************************************************************************************************************************/

 #include <Wire.h> //This library allows you to communicate with I2C devices  
 //SDA -> GP4 and SCL -> GP5    
 #include <LiquidCrystal_I2C.h> //  This library is for character LCDs based on the HD44780 controller connected via I2C bus using the cheap I2C backpack modules based on the PCF8574 
 //LiquidCrystal_I2C lcd(0x3F,16,2);  // //https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library  
 LiquidCrystal_I2C lcd(0x27,16,2);  // if lcd is not print then use this 0x27..  

#include <Adafruit_NeoPixel.h>  
#define NUMPIXELS 1  // https://github.com/tehniq3/RP2040/blob/main/RP2040Zero_RGB.ino

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

Adafruit_NeoPixel pixels(NUMPIXELS, rgbled, NEO_GRB + NEO_KHZ800);

int status = WL_IDLE_STATUS;      // the Wifi radio's status
unsigned long lastConnectionTime    = 285000; // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 300000; // delay between updates, in milliseconds

// Initialize the Web client object
ESP8266_AT_Client client;

#include "ESP8266_AT_Udp.h"
// A UDP instance to let us send and receive packets over UDP
ESP8266_AT_UDP Udp;

char timeServer[]         = "time.nist.gov";  // NTP server
unsigned int localPort    = 2390;             // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48;       // NTP timestamp is in the first 48 bytes of the message
const int UDP_TIMEOUT     = 1000;     // timeout in miliseconds to wait for an UDP packet to arrive
byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets

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

#include <TimeLib.h>  // https://www.appsloveworld.com//arduinocode/1/convert-to-and-from-unix-timestamp
int timeZone = 2;
int an, luna, zi;
int ora, minut, secunda;
int ora1, minut1;
int zi2;
byte DST = 1;
byte DST0 = 7;
#define DSTpin 8 // 
String weekDays1[7]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//String weekDays2[7]={"Dum", "Lun", "Mar", "Mie", "Joi", "Vin", "Sam"};

unsigned long tpNTP;
unsigned long tpNTP0;
unsigned long tpNTP01 = 600000;  // time refresh for NTP
unsigned long tpNTP02 = 100000;
unsigned long tpsec;
unsigned long tpsec1;
unsigned long epoch1, epoch2;
unsigned long tpserial;
byte liber = 1;
byte liber0 = 7;
byte bataie, rled, gled, bled, cled;
byte nook2 = 1;
unsigned long tpclipire = 1000;
unsigned long tpclipit = 0;


void setup()
{
  pixels.begin();
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  lcd.begin();    //initialize lcd screen   
  lcd.backlight();  // turn on the backlight
  lcd.setCursor(0,0); 
  lcd.print(" NTP clock with ");  
  lcd.setCursor(0,1);  
  lcd.print(" Weatherstation ");  
 
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
  lcd.print("RP2040 + ESP8266");  
  lcd.setCursor(0,1);  
  lcd.print(" v3.6 by niq_ro ");  

 //check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD)
  {
    lcd.setCursor(0,0);  
    lcd.print("  WiFi module   "); 
    lcd.setCursor(0,1);  
    lcd.print("  not present!  ");   
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
  Udp.begin(localPort);
  lcd.clear();
}

void loop()
{
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  while (liber == 1)
  {
  if (millis() - tpNTP > tpNTP0)
  {
  lcd.setCursor(11,0);  
  lcd.print(">");
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait for a reply for UDP_TIMEOUT miliseconds
  unsigned long startMs = millis();
  
  while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT) {}

  if (!Udp.available())
  {
//     lcd.setCursor(11,0);  
//     lcd.print("!");
     nook2 = 1;
    // delay(200);
     Serial.println(F("No UDP Packet received !"));
     tpNTP = millis();
     liber = 4;
  }
    else
    {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();

  if (packetSize)
  {
    Serial.print(F("UDP Packet received, size "));
    Serial.println(packetSize);

    #if 0
    Serial.print(F("From "));
    IPAddress remoteIp = Udp.remoteIP();    
    Serial.print(remoteIp);
    Serial.print(F(", port "));
    Serial.println(Udp.remotePort());
    #endif
    
    // We've received a packet, read the data from it into the buffer
    Udp.read(packetBuffer, NTP_PACKET_SIZE);

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print(F("Seconds since Jan 1 1900 = "));
    Serial.println(secsSince1900);

    // now convert NTP time into )everyday time:
    //Serial.print(F("Unix time = "));
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch;
    epoch = secsSince1900 - seventyYears;  // local time
    epoch1 = epoch + timeZone*3600;  // local time
    tpNTP = millis();
    //  lcd.setCursor(11,0);  
    //  lcd.print("="); 
    nook2 = 0; 
   }
  tpsec = millis()/1000;
  liber = 3;
   }
//  tpNTP = millis();
  }  // end NTP requestt
  }

   while (liber == 2)
   {
  // if there's incoming data from the net connection send it out the serial port
  // this is for debugging purposes only 
  while (client.available())
  {
    char c = client.read();
   // Serial.write(c);
    raspuns = raspuns + c;
    receptie = 1;
  }
  
  if (receptie == 1)
  {
  client.stop();
 // Serial.flush();
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
ora1 = ora;
minut1 = minut;
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
Serial.println(icon);
nook = 0;
}
else 
{
  nook = 1;
  Serial.println(" no good data ! "); 
}
 aratad = 0;
 liber = 3;
  }

  // if time for check the weather info have passed since your last connection,
  // then connect again and send data
  if (millis() - lastConnectionTime > postingInterval)
  {
  //  Serial.flush();
    lcd.setCursor(10,0); 
    lcd.print(">");
    httpRequest();
   // lastConnectionTime = millis();
    arata = 1;
  }
/*
// lcd part
  lcd.setCursor(0,0); 
  lcd.print("Craiova, Romania");  
  lcd.setCursor(0,1);  
*/
//liber = 3;
   }
   
if (schimbare0 != schimbare)
if (umiditate !=0)
{
   if (nook2 == 1)
    {
    lcd.setCursor(11,0); 
    lcd.print("!");
    }
    else
    {
    lcd.setCursor(11,0); 
    lcd.print("=");
    }
    if (nook == 1)
    {
    lcd.setCursor(10,0); 
    lcd.print("!");
    }
    else
    {
    lcd.setCursor(10,0); 
    lcd.print("=");
    }
  //lcd.clear();
  lcd.setCursor(0,1); 
  lcd.print("                ");
  lcd.setCursor(0,1);
   if (schimbare == 0)
    {
      lcd.print("last info->");
      lcd.print(ora1/10);
      lcd.print(ora1%10); 
      lcd.print(":");
      lcd.print(minut1/10);
      lcd.print(minut1%10); 
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
    else
   if (schimbare == 5) 
    {  
      lcd.print(zi/10);
      lcd.print(zi%10); 
      lcd.print("-");
      lcd.print(luna/10);
      lcd.print(luna%10); 
      lcd.print("-");
      lcd.print(an); 
    }
    else
    if (schimbare == 6) 
    { 
    lcd.print(descriere);
    }
    else
    if (schimbare == 7) 
    {  
      lcd.print("clouds: ");
      if (nori < 100)
          lcd.print(" ");
      if (nori < 10)
          lcd.print(" ");          
      lcd.print(nori); 
      lcd.print("%");
    }
}
else
{
  lcd.setCursor(0,1); 
  if (an > 1970)
  {
      lcd.print(zi/10);
      lcd.print(zi%10); 
      lcd.print("-");
      lcd.print(luna/10);
      lcd.print(luna%10); 
      lcd.print("-");
      lcd.print(an);  
  }
  //lcd.print("no weather info!");
}
  
schimbare0 = schimbare;
//delay(1);
if (millis() - tpschimbare > tpsch)
{
  schimbare++;
   if (nori > 0)
  {
   if (schimbare > 7)
      schimbare = 0;   
  }
  else
  if (schimbare > 6)
      schimbare = 0; 
  tpschimbare = millis();    
}

//digitalWrite(led, (millis()/1000)%2);

 tpsec1 = millis()/1000 - tpsec;
 epoch2 = epoch1 + DST*3600 + tpsec1;
  
   // Serial.println(epoch2 % 60); // print the second
    digitalWrite(led, epoch2%60%2);

     if (millis() - tpclipit > tpclipire)
     {
     bataie++;
     if (bataie > 16) bataie = 2;
      rled = 10*(bataie/8);
      gled = 10*((bataie%8)/4);
      bled = 10*(((bataie%8)%4)/2);
      cled = ((bataie%8)%4)%2;
    if (cled%2 == 1)
    pixels.setPixelColor(0, pixels.Color(rled, gled, bled));
    else
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    tpclipit = millis();
     }
         
an = year(epoch2); 
luna = month(epoch2);
zi = day(epoch2);
ora = hour(epoch2);
minut = minute(epoch2);
secunda = second(epoch2);
zi2 = weekday(epoch2);
/*
if (millis() - tpserial > 1000)
{
Serial.print("year = ");
Serial.println(an);
Serial.print("month = ");
Serial.println(luna);
Serial.print("day = ");
Serial.println(zi);
Serial.print("hour = ");
Serial.println(ora);
Serial.print("minute = ");
Serial.println(minut);
Serial.print("seconds = ");
Serial.println(secunda);
Serial.print("day of week= ");
Serial.print(zi2-1);
Serial.print(" -> day of week= ");
Serial.println(weekDays1[zi2-1]);
tpserial = millis();
}
*/
if (an > 1970)
{
lcd.setCursor(0,0);  
lcd.print(ora/10);
lcd.print(ora%10); 
lcd.print(":");
lcd.print(minut/10);
lcd.print(minut%10); 
lcd.print(":");
lcd.print(secunda/10);
lcd.print(secunda%10);
lcd.setCursor(13,0); 
//lcd.print(zi2-1);
lcd.print(weekDays1[zi2-1]); 
/*
lcd.setCursor(0,1);  
lcd.print(zi/10);
lcd.print(zi%10); 
lcd.print("-");
lcd.print(luna/10);
lcd.print(luna%10); 
lcd.print("-");
lcd.print(an);
*/
}
/*
lcd.setCursor(11,1); 
if (tpsec1 < 10) lcd.print(".");
if (tpsec1 < 100) lcd.print(".");
if (tpsec1 < 1000) lcd.print(".");
if (tpsec1 < 10000) lcd.print(".");
lcd.print(tpsec1);
*/
  // wait a second before asking for the time again
  if (an < 1971)
  {
    tpNTP0 = tpNTP02;
    delay(50);
  }
  else
  {
    tpNTP0 = tpNTP01;
    delay(50);
  }
  
if (liber0 != liber)
{
  Serial.println("================ ");
  Serial.print("liber = ");
  Serial.println(liber);
  liber0 = liber;
}
  if ((liber == 3) and (millis() - tpNTP > tpNTP0)) 
  {
    liber = 1;
  }
  if ((liber >= 3) and (millis() - lastConnectionTime > postingInterval))
  {
    liber = 2;
  }
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
   // lastConnectionTime = millis();
  }
  else
  {
    // if you couldn't make a connection
    Serial.println(F("Connection failed"));
  //  lastConnectionTime = millis();
  nook = 1;
  liber = 3;
  }
lastConnectionTime = millis();
raspuns = "";
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char *ntpSrv)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123

  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  Udp.endPacket();
}
