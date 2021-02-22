#ifndef AJAOne_h
#define AJAOne_h

#include <i2cscanner.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>

#include "OTA.h"
#include <nickstuff.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <TinyPICO.h>
#include <BME280I2C.h>
#include <BH1750.h>

// OLED
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#define I2C_OLEDADDRESS 0x3C
SSD1306AsciiWire oled;
#define RST_PIN -1

#define AJAONE_VERSION "AJAOne V1.0"
#define BOARDNAME "AJAOne_"
#define WIFIPORTAL "AJAOneAP"

#define dot_mqtttraffic 0x4C0099   // blue-purple
#define dot_booting 0xFFFF00       // yellow
#define dot_wifi 0x990099          // purple
#define dot_goodboot 0x00FF00      // green
#define dot_reset 0xFF0000         // red
#define dot_mqttconnect 0x0000FF   // blue
#define dot_gettemp 0x99004C       // burgundy


//mqtt credentials
char mqtt_server[40] = "";
char mqtt_port[6] = "1883";
char mqtt_user[15] = "";
char mqtt_password[15] = "";
bool oledavailable = false;

int pressTime = 0;
char boardname[20];

char ison[3] = "ON";
char isoff[4] = "OFF";

//Firmware revision
const char myversion[15] = AJAONE_VERSION; 

bool dBME280;
bool dBH1750;
byte mac[6];
unsigned long status_delay = 300000;  // 5 mins
unsigned long last_status  = 290000;  
long loop_delay = 15000; // 15 seconds
long lastMsg = 10000;
long lastkeyMsg = 0;

// save last status of the digital input items
int di_last36 = 0;
int di_last39 = 0;
int di_last34 = 0;
int di_last35 = 0;
int di_last32 = 0;
int di_last33 = 0;
int di_last25 = 0;
int di_last26 = 0;

//flag for saving data
bool shouldSaveConfig = false;

//===== GPS stuff
//uint32_t timer = millis();
//String ts;
//String stGPRMC = "";
//String stGPGGA = "";
//const byte numChars = 200;
//char receivedChars[numChars]; // an array to store the received data
//boolean newData = false;

//===== MFRC522 card reader
#include <MFRC522.h>
#define SS_PIN 5
#define RST_PIN 13
byte readCard[4];


#endif
