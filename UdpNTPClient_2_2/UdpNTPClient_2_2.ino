/****************************************************************************************************************************
  UdpNTPClient.ino - Simple Arduino web server sample for ESP8266 AT-command shield
  For ESP8266/ESP32 AT-command running shields

  ESP8266_AT_WebServer is a library for the ESP8266/ESP32 AT-command shields to run WebServer
  Based on and modified from ESP8266 https://github.com/esp8266/Arduino/releases
  Built by Khoi Hoang https://github.com/khoih-prog/ESP8266_AT_WebServer
  Licensed under MIT license
  
  Original author:
  @file       Esp8266WebServer.h
  @author     Ivan Grokhotkov

  - NTP clock by Nicu FLORICA (niq_ro)
  - v.1.0 - serial info
  - v.2.0 - added DST (winter/summer time) selector + i2c LCD1602 display
  - v.2.1 - small changes
  - v.2.2 - give NTP clock more rarely not every seconds
 *****************************************************************************************************************************/
//https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library  
 #include <Wire.h> //This library allows you to communicate with I2C devices  
 //SDA -> GP4 and SCL -> GP5    
 #include <LiquidCrystal_I2C.h> //  This library is for character LCDs based on the HD44780 controller connected via I2C bus using the cheap I2C backpack modules based on the PCF8574 
 //LiquidCrystal_I2C lcd(0x3F,16,2);  
 LiquidCrystal_I2C lcd(0x27,16,2); 
 // if lcd is not print then use this 0x27..  
 //initialize the liquid crystal library
//the first parameter is the I2C address
//the second parameter is how many rows are on your display
//the third parameter is how many columns are o
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

#include "ESP8266_AT_Udp.h"

int status = WL_IDLE_STATUS;      // the Wifi radio's status

char timeServer[]         = "time.nist.gov";  // NTP server
unsigned int localPort    = 2390;             // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48;       // NTP timestamp is in the first 48 bytes of the message
const int UDP_TIMEOUT     = 1000;     // timeout in miliseconds to wait for an UDP packet to arrive

byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
ESP8266_AT_UDP Udp;

#define led 25

#include <TimeLib.h>  // https://www.appsloveworld.com//arduinocode/1/convert-to-and-from-unix-timestamp
int timeZone = 2;
int an, luna, zi;
int ora, minut, secunda;
int zi2;
byte DST = 1;
byte DST0 = 7;
#define DSTpin 22 // 
String weekDays1[7]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//String weekDays2[7]={"Dum", "Lun", "Mar", "Mie", "Joi", "Vin", "Sam"};

unsigned long tpNTP;
unsigned long tpNTP0;
unsigned long tpNTP01 = 60000;
unsigned long tpNTP02 = 5000;
unsigned long tpsec;
unsigned long tpsec1;
unsigned long epoch1, epoch2;
unsigned long tpserial;
 
void setup()
{
  pinMode (DSTpin, INPUT);
  pinMode(led, OUTPUT);
  lcd.begin();    //initialize lcd screen   
  lcd.backlight();  // turn on the backlight
  lcd.setCursor(0,0); 
  lcd.print("NTPclock: RP2040");  
  lcd.setCursor(0,1);  
  lcd.print("+ESP8266-01 v2.2");  
  delay(3000); 
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.print(F("\nStarting UdpNTPClient on ")); Serial.print(BOARD_NAME);
  Serial.print(F(" with ")); Serial.println(SHIELD_TYPE); 
  Serial.println(ESP8266_AT_WEBSERVER_VERSION);

  // initialize serial for ESP module
  EspSerial.begin(115200);

  // initialize ESP module
  WiFi.init(&EspSerial);

  Serial.println(F("WiFi shield init done"));

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println(F("WiFi shield not present"));
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED)
  {
    Serial.print(F("Connecting to WPA SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.print(F("You're connected to the network, IP = "));
  Serial.println(WiFi.localIP());

  Udp.begin(localPort);
  lcd.clear();
}

void loop()
{
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (millis() - tpNTP > tpNTP0)
  {
  lcd.setCursor(11,0);  
  lcd.print(">");
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait for a reply for UDP_TIMEOUT miliseconds
  unsigned long startMs = millis();
  
  while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT) {}

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
    Serial.print(F("Unix time = "));
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch;
    epoch = secsSince1900 - seventyYears;  // local time
    epoch1 = epoch + timeZone*3600;  // local time
    tpNTP = millis();
      lcd.setCursor(11,0);  
      lcd.print("=");
   }
 tpsec = millis()/1000;

  }  // end NTP requestt
/*
if (DST0 != DST)
 {
  epoch2 = epoch1 + DST*3600;  // local time + DST (summer/winter time)
  DST0 = DST;
 }
 */ 7                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
 tpsec1 = millis()/1000 - tpsec;
 epoch2 = epoch1 + DST*3600 + tpsec1;
  
    Serial.println(epoch2 % 60); // print the second
    digitalWrite(led, epoch2%60%2);

an = year(epoch2); 
luna = month(epoch2);
zi = day(epoch2);
ora = hour(epoch2);
minut = minute(epoch2);
secunda = second(epoch2);
zi2 = weekday(epoch2);

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
lcd.setCursor(0,1);  
lcd.print(zi/10);
lcd.print(zi%10); 
lcd.print("-");
lcd.print(luna/10);
lcd.print(luna%10); 
lcd.print("-");
lcd.print(an);
}
lcd.setCursor(11,1); 
if (tpsec1 < 10) lcd.print(".");
if (tpsec1 < 100) lcd.print(".");
if (tpsec1 < 1000) lcd.print(".");
if (tpsec1 < 10000) lcd.print(".");
lcd.print(tpsec1);

  // wait a second before asking for the time again
  if (an < 1971)
  {
    tpNTP0 = tpNTP02;
  }
  else
  {
    tpNTP0 = tpNTP01;
  }
  delay(50);
  
delay(500);
}  // end main loop

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
