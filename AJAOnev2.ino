#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include "ajaone.h"


//  Create the various "clients"
OneWire ds(14);
DallasTemperature sensors(&ds);
WiFiClient espClient;
PubSubClient client(espClient);
TinyPICO tp = TinyPICO();
BME280I2C bme;
BH1750 lightMeter(0x23);
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
//==================================================================================
//==================================================================================
//==================================================================================


//===== Setup ======================================================================
//===== Setup ======================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

 //Uncomment if you are using the SPI (VSPI) bus
  SPI.begin(18,19,23,5);      // Init SPI bus

  tp.DotStar_SetBrightness(16);
  tp.DotStar_SetPixelColor( dot_booting ); 

  // Set the top 4 pins to input and the bottom 4 to output
  pinMode(36, INPUT); //These are input only anyway . . .
  pinMode(39, INPUT);
  pinMode(34, INPUT);
  pinMode(35, INPUT);

  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);

 // make sure they are all low at boot
  digitalWrite(32,LOW);
  digitalWrite(33,LOW);
  digitalWrite(25,LOW);
  digitalWrite(26,LOW);

  // Get the last 2 bytes of the MAC address and set our MQTT name to AJAOne-<last 4 of MAC>
  WiFi.macAddress(mac);
  sprintf(boardname,"%s%02X%02X",BOARDNAME,mac[4],mac[5]);

  Serial.print("Hello, my name is ");
  Serial.println(boardname); 
  Serial.print("Version ");
  Serial.println(AJAONE_VERSION);
  
//===== WiFI Manager items =====
  loadconfig();  //read the config.json from SPIFFS
  
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 15);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 15);  
  WiFiManagerParameter custom_boardname("boardname", "boardname", boardname, 20);    

  tp.DotStar_SetPixelColor( dot_wifi );
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);  
  wifiManager.addParameter(&custom_boardname); 
  wifiManager.setTimeout(120);
  
  if (!wifiManager.autoConnect(WIFIPORTAL, "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }  
  
  Serial.println("connected...");
  Serial.println("");
  
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());  
  strcpy(boardname, custom_boardname.getValue());  
  
  if (shouldSaveConfig) {
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(boardname, custom_boardname.getValue());    
    saveconfig();  //save back to config.json 
  }  
//===== WiFI Manager items =====  

//==============================================================================================  
  
  //Set our OTA name to the same as above.
  Serial.println("Starting OTA");
  ArduinoOTA.setHostname(boardname);
  setupOTA();
  
  // Setup the MQTT client
  Serial.println("Starting MQTT");
  client.setServer(mqtt_server, strtol (mqtt_port,NULL,10 ));
  client.setCallback(callback);
  Serial.println("");
  
 //===== For I2C and OLED
   Serial.println("===== Starting I2C =====");
   Wire.begin(21,22);    
   Wire.setClock(400000L);
   nt_i2cscanner();

//===== search for standard I2C devices and enable them =====  
//===== search for standard I2C devices and enable them ===== 
//===== search for standard I2C devices and enable them ===== 

   //===== Setup the OLED =====
   oledavailable = nt_checki2caddress(I2C_OLEDADDRESS);
   if (oledavailable) {
     Serial.println("Starting OLED");
     #if RST_PIN >= 0
       oled.begin(&Adafruit128x64, I2C_OLEDADDRESS, RST_PIN);
     #else // RST_PIN >= 0
       oled.begin(&Adafruit128x64, I2C_OLEDADDRESS);
     #endif // RST_PIN >= 0  

     oled.setFont(Adafruit5x7);
     //oled.setFont(fixed_bold10x15);
     oled.clear();
   }

  //===== Setup the BME280
  dBME280 = false;
  bme.begin();
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       dBME280 = true;
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       dBME280 = true;
       break;
  }

  //===== Setup the BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
    dBH1750 = true;
  }
  else {
    dBH1750 = false;
  }  

   Serial.println("===== I2C Complete =====");
   Serial.println("");
//===== search for standard I2C devices and enable them ====^
//===== search for standard I2C devices and enable them ====^ 
//===== search for standard I2C devices and enable them ====^ 

   // Get the initial temps from the 1-wire bus and discard.  First pass is uaually used for calibration 
   getalltemp();
   delay(500);
   getalltemp();

   // Write the Wi-Fi details to the OLED
   fastwrite(0,0, "IP addr = " + String(WiFi.localIP().toString().c_str() ) );   
   fastwrite(0,1, "My name = " + String(boardname) ); 

   // Set the initial digital input value;
   di_last36 = digitalRead(36); 
   di_last39 = digitalRead(39); 
   di_last34 = digitalRead(34); 
   di_last35 = digitalRead(35); 

   tp.DotStar_SetPixelColor( dot_goodboot  );
   delay(1000);
   tp.DotStar_Clear();
   
   mfrc522.PCD_Init(); // Init MFRC522 card
   
   Serial.println("");
   Serial.print("Hello, my name is ");
   Serial.println(boardname);   
   Serial.println("");  
}
//===== Setup =====================================================================^
//===== Setup =====================================================================^


//===== Loop =======================================================================
//===== Loop =======================================================================
void loop() {

    ArduinoOTA.handle();

    // Reconnect MQTT if needed wil reboot after 2.5 minutes
    if (!client.connected()) {
       if (mqtt_server != "") {
        reconnect();
       }
    }  
  
    //  MQTT task
    client.loop();  

   //===== 15 second loop =====
    long now = millis();
    if (now - lastMsg > loop_delay) {
    lastMsg = now;
    Serial.println("");
    Serial.print("Loop ... ");
    Serial.println(boardname);
 
    tp.DotStar_SetPixelColor( dot_gettemp ); 
    getalltemp();  //onewire bus
    readdevices(); //other I2C devices
    ArduinoOTA.handle();
    tp.DotStar_Clear();    
    
   }
   //===== 15 second loop ====^

   //===== 10 second loop ====
    byte key = digitalRead(0);
    if (key == LOW) { // that means the key is pressed, reset after 10 seconds.  Don't really like this but it works....
       pressTime++;
       delay(1000); // works like a timer with one second steps
       if (pressTime <= 10){
       }else {
          Serial.print("Long click - Resetting ");
          Serial.println(pressTime);

         //Reset everything and restart 
         hardreset();
         }    
        }
        else {  
        //reset PressTime if 2 seconds elapsed with no key press  
        if (now - lastkeyMsg > 2000) {
           pressTime = 0;
           lastkeyMsg = now;
         }    
        }  
   //===== 10 second loop ====


   //===== 5 min loop =====
    now = millis();
    if (now - last_status > status_delay) {
    last_status = now;
    utilityloop();
    digitalportstatus();
  } 
  //===== 5 min loop ====^   

  //===== Read all the onboard digital inputs
  delay(10);  //helps stop bounce
  readalldigitalinputs();

  //===== Look for a card to read
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
     processcard(); 
    } //readcardserial
  } //cardpresent
  
}
//===== Loop ======================================================================^
//===== Loop ======================================================================^

//===== Process an rfid card ======================================================
void processcard(){
  char ccard[15];
  
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  } 

  array_to_string(readCard, 4, ccard);
  mfrc522.PICC_HaltA(); // Stop reading  

  sendMQTTMessage("status/rfidcard",ccard);  
}
//===== Process an rfid card =====================================================^


//===== Set an output pin ==========================================================
void setoutputpin(byte PIN, String MODE  ) {
 Serial.print("Setting pin ");

 MODE.toUpperCase();
   
 if (MODE == "ON") {
  digitalWrite(PIN, HIGH); 
  Serial.print(PIN);
  Serial.println(MODE);  
 }

 if (MODE == "OFF") {
  digitalWrite(PIN, LOW); 
  Serial.print(PIN);
  Serial.println(MODE);  
 }
}
//===== Set an output pin =========================================================^


//===== Toggle the output for 250 ms ===============================================
void togglepin2(String PIN) {
 String temp = ""; 

 Serial.println(PIN);

 temp = PIN.substring(PIN.length()-2);
 int APIN = temp.toInt();

 Serial.print("Toggling2 ");
 Serial.println(APIN);
 
 digitalWrite(APIN,HIGH);  
 tp.DotStar_SetPixelColor( dot_gettemp  );   
 delay(250);
 digitalWrite(APIN,LOW);   
 tp.DotStar_Clear();    
}
//===== Toggle the output for 250 ms ==============================================^


//===== Get the status of a digital pin ============================================
void sendportstatus(byte PIN,char* topic) {
 
 int val = digitalRead(PIN);

 if (val == 0) {
  sendMQTTMessage(topic,isoff);
 }

if (val == 1) {
  sendMQTTMessage(topic,ison);
 } 

}
//===== Get the status of a digital pin ===========================================^


//===== Get the status of a digital pin ============================================
void sendportstatus2(byte PIN,char* topic) {
 
 if (PIN == 0) {
  sendMQTTMessage(topic,isoff);
 }

if (PIN == 1) {
  sendMQTTMessage(topic,ison);
 } 

}
//===== Get the status of a digital pin ===========================================^



//===== MQTT Callback function =====================================================
void callback(char* topic, byte* message, unsigned int length) {
  tp.DotStar_SetPixelColor( dot_mqtttraffic );  
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp = "";
  
  for (int i = 0; i < length; i++) {    
    messageTemp += (char)message[i];
  }

  Serial.println(messageTemp);
  Serial.println("");

  if (String(topic).indexOf("/cmd/toggle") > 0) {
   togglepin2(String(topic)); 
  }
  else
  if (String(topic) == String(boardname)+"/cmd/status") {
    utilityloop();
    digitalportstatus();
  }
  else
  if (String(topic) == String(boardname)+"/cmd/status/port36") { 
   sendportstatus(36,"status/port36"); 
  }  
  else
  if (String(topic) == String(boardname)+"/cmd/status/port39") { 
   sendportstatus(39,"status/port39"); 
  }   
  else
  if (String(topic) == String(boardname)+"/cmd/status/port34") { 
   sendportstatus(34,"status/port34"); 
  }   
  else
  if (String(topic) == String(boardname)+"/cmd/status/port35") { 
   sendportstatus(35,"status/port35"); 
  }   
  else
  if (String(topic) == String(boardname)+"/cmd/status/port32") { 
   sendportstatus(32,"status/port32"); 
  }    
  else
  if (String(topic) == String(boardname)+"/cmd/status/port33") { 
   sendportstatus(33,"status/port33"); 
  }   
  else
  if (String(topic) == String(boardname)+"/cmd/status/port25") { 
   sendportstatus(25,"status/port25"); 
  }    
  else
  if (String(topic) == String(boardname)+"/cmd/status/port26") { 
   sendportstatus(26,"status/port26"); 
  }   
  else
 // OLED Print commands
  if (String(topic) == String(boardname)+"/cmd/printline2") {
    fastwrite(0,2,messageTemp);
  }
  else
  if (String(topic) == String(boardname)+"/cmd/printline3") {
    fastwrite(0,3,messageTemp);
  }  
  else
  if (String(topic) == String(boardname)+"/cmd/printline4") {
    fastwrite(0,4,messageTemp);
  }  
  else
  if (String(topic) == String(boardname)+"/cmd/printline5") {
    fastwrite(0,5,messageTemp);
  }  
  else
  if (String(topic) == String(boardname)+"/cmd/printline6") {
    fastwrite(0,6,messageTemp);
  }  
  else
  // Output commands
  if (String(topic) == String(boardname)+"/cmd/output32") {
    setoutputpin(32,messageTemp);  
  }   
  else
  if (String(topic) == String(boardname)+"/cmd/output33") {
    setoutputpin(33,messageTemp);  
  }   
  else
  if (String(topic) == String(boardname)+"/cmd/output25") {
    setoutputpin(25,messageTemp);  
  } 
  else
  if (String(topic) == String(boardname)+"/cmd/output26") {
    setoutputpin(26,messageTemp);  
  }     
  else
  if (String(topic).indexOf("/cmd/reboot") > 0) {
    ESP.restart(); 
  }
  tp.DotStar_Clear();
}
//===== Callback function ==========================================================^


//===== Subscribe to a topic =======================================================
void doSubscribe(const char* topic, const char* item) {
   char tempc[60] = "";

   sprintf(tempc, "%s/%s",topic,item);  
   client.subscribe(tempc);   
}
//===== Subscribe to a topic =======================================================


//===== Send an MQTT message =======================================================
void sendMQTTMessage(const char* minortopic, const char* payload ) {
 char fulltopic[60] = "";  
 tp.DotStar_SetPixelColor( dot_mqtttraffic ); 
 //===== Send the message
 sprintf(fulltopic, "%s/%s",boardname,minortopic);  
 client.publish(fulltopic, payload  );

 //debug
 Serial.print(fulltopic);
 Serial.print(" : ");
 Serial.println(payload);
 tp.DotStar_Clear();
}
//===== Send an MQTT message ======================================================^


//===== Peform hard reset ===========================================================
void hardreset() {
 tp.DotStar_SetPixelColor( dot_reset  );
 Serial.println("RESETTING!!");
 esp_wifi_restore();
 delay(2000); //wait a bit          
 SPIFFS.format();
 ESP.restart();  
}
//===== Peform hard reset ===========================================================


//===== Reconnect to the MQTT server ================================================
void reconnect() {
  // Loop until we're reconnected
  tp.DotStar_SetPixelColor( dot_mqttconnect );
  int retries = 0;
  while (!client.connected()) {
   if (retries < 15) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect(boardname,mqtt_user,mqtt_password)) {
      Serial.println("connected!");
      
//      // Print Subscribes 
      doSubscribe(boardname,"cmd/printline2");
      doSubscribe(boardname,"cmd/printline3");
      doSubscribe(boardname,"cmd/printline4");
      doSubscribe(boardname,"cmd/printline5");

      //Output subscribes
      doSubscribe(boardname,"cmd/output32");
      doSubscribe(boardname,"cmd/output33");
      doSubscribe(boardname,"cmd/output25");
      doSubscribe(boardname,"cmd/output26");

//      //Toggle subscribe
      doSubscribe(boardname,"cmd/toggle32");
      doSubscribe(boardname,"cmd/toggle33");
      doSubscribe(boardname,"cmd/toggle25");
      doSubscribe(boardname,"cmd/toggle26");

//      //Status subscribes
      doSubscribe(boardname,"cmd/status");
      doSubscribe(boardname,"cmd/status/port36");
      doSubscribe(boardname,"cmd/status/port39");
      doSubscribe(boardname,"cmd/status/port34");
      doSubscribe(boardname,"cmd/status/port35");     
      doSubscribe(boardname,"cmd/status/port32");
      doSubscribe(boardname,"cmd/status/port33");
      doSubscribe(boardname,"cmd/status/port25");
      doSubscribe(boardname,"cmd/status/port26");    

      doSubscribe(boardname,"cmd/reboot"); 
      sendMQTTMessage("status/booted","hello");   

      tp.DotStar_Clear();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" Rebooting soon if a connection is not made. . . ");
      Serial.print(retries);
      Serial.println(" (10)");
      // Wait 10 seconds before retrying
      delay(10000);
      retries++;

     byte key = digitalRead(0);
     if (key == LOW) {
       hardreset();
         } 
    }
   }
   if(retries > 10)
   {
   tp.DotStar_Clear();
   ESP.restart();
   }   
  } 
}
//===== Reconnect to the MQTT server ===============================================^


//===== Just some house keeping items =============================
void utilityloop() {
 
 char buf[16];

 //===== Send the IP address
 sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );   
 sendMQTTMessage("status/ipadress",buf);

 fastwrite(0,0, "IP addr = " + String(WiFi.localIP().toString().c_str() ) );   
 fastwrite(0,1, "My name = " + String(boardname) );  

 //===== Sent our version
 sendMQTTMessage("status/firmware",myversion);
   
}
//===== Just some house keeping items ============================^


//===== Get the temp for all devices on the one wire bus===========
void getalltemp () {
  
  byte addr[8];
  byte i;
  String address = "";
  char Sval[8];
  char caddress[20];

 //===== get all temps at once
 sensors.requestTemperatures();

 //===== read each device and send the data
 while (ds.search(addr)) {
  
   //===== found a device
   address = "";

   for (i=0; i<8; i++) {
    address = address+ String(addr[i],HEX);
   }
   address = "bus/"+address;
   address.toCharArray(caddress,address.length()+1);
     
   //make up the full topic
   dtostrf(sensors.getTempF(addr), 1, 2, Sval);   
   sendMQTTMessage(caddress,Sval);
 }

 ds.reset_search();
}
//========================================================


//===== Read all devices ===========================================================
void readdevices() {
 char Sval[15];

 
  //===== Read the BH1750 light meter ======
  if (dBH1750) {
    if (lightMeter.measurementReady()) {
     float lux = lightMeter.readLightLevel();
     dtostrf(lux, 1, 2, Sval);
     sendMQTTMessage("status/light",Sval);     
    }
  } 
  //===== Read the BH1750 light meter ======

  //===== read the BME280 if it exists =====
  if (dBME280) {
    float temp = 0;
    float hum = 0;
    float pres = 0;

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa); 

    bme.read(pres, temp, hum, tempUnit, presUnit);   

    dtostrf(pres, 1, 2, Sval); 
    sendMQTTMessage("status/pressure",Sval);
 
    dtostrf(temp, 1, 2, Sval); 
    sendMQTTMessage("status/temperature",Sval);  

    dtostrf(hum, 1, 2, Sval);
    sendMQTTMessage("status/humidity",Sval);     
    
  }
  //===== read the BME280 if it exists ====^  
  
}
//===== Read all devices ==========================================================^


//===== fastwrite function ======================================
void fastwrite(uint8_t x, uint8_t y, String m) { 
  if(oledavailable) {
    oled.setRow(y);
    oled.setCol(x);
    oled.print(m);
    oled.clearToEOL();
  } 
}
//===== fastwrite function =====================================^


//===== Scan the 4 digital input ================================

// Performed as soon as a state change is detected. Also do analogs on AJAPort
void readalldigitalinputs(){
 int val = 0;
 char buf[20];

 val = digitalRead(36);
 if (di_last36 != val) {
   delay(10);  //debounce delay
   val = digitalRead(36);
   if (di_last36 != val) {
   di_last36 = val;
   sendportstatus2(val,"status/port36");
  }
 }

 val = digitalRead(39);
 if (di_last39 != val) {
   delay(10);
   val = digitalRead(39);
   if (di_last39 != val) {
   di_last39 = val;
   sendportstatus2(val,"status/port39");
  }
 }

 val = digitalRead(34);
 if (di_last34 != val) {
   delay(10);
   val = digitalRead(34);
   if (di_last34 != val) {
   di_last34 = val;
   sendportstatus2(val,"status/port34");
  }
 }
 
 val = digitalRead(35);
 if (di_last35 != val) {
   delay(10);
   val = digitalRead(35);
   if (di_last35 != val) {
   di_last35 = val;
   sendportstatus2(val,"status/port35");
  }
 }

 val = digitalRead(32);
 if (di_last32 != val) {
   delay(10);
   val = digitalRead(32);
   if (di_last32 != val) {
   di_last32 = val;
   sendportstatus2(val,"status/port32");
  }
 } 

 val = digitalRead(33);
 if (di_last33 != val) {
   delay(10);
   val = digitalRead(33);
   if (di_last33 != val) {
   di_last33 = val;
   sendportstatus2(val,"status/port33");
  }
 }  

 val = digitalRead(25);
 if (di_last25 != val) {
   delay(10);
   val = digitalRead(25);
   if (di_last25 != val) {
   di_last25 = val;
   sendportstatus2(val,"status/port25");
  }
 }  

 val = digitalRead(26);
 if (di_last26 != val) {
   delay(10);
   val = digitalRead(26);
   if (di_last26 != val) {
   di_last26 = val;
   sendportstatus2(val,"status/port26");
  }
 }   
 
}
//===== Scan the 4 digital input ===============================^


//===== Report all digital port status ==========================
// Perfomed at boot and then every 5 mins.
void digitalportstatus() {
  int val = 0;  

  val = digitalRead(36);
  sendportstatus2(val,"status/port36"); 

  val = digitalRead(39);
  sendportstatus2(val,"status/port39"); 

  val = digitalRead(34);
  sendportstatus2(val,"status/port34");   

  val = digitalRead(35);
  sendportstatus2(val,"status/port35");     
  
  val = digitalRead(32);
  sendportstatus2(val,"status/port32"); 

  val = digitalRead(33);
  sendportstatus2(val,"status/port33"); 

  val = digitalRead(25);
  sendportstatus2(val,"status/port25");   

  val = digitalRead(26);
  sendportstatus2(val,"status/port26");    
}
//===== Report all digital port status =========================^


//===== Config file routines ====================================
//===== Config file routines ====================================
//===== Config file routines ====================================
void loadconfig() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  //===== if we cannot mount the SPIFFS, prob means it's not formatted
  if (!SPIFFS.begin(true)) {
   Serial.println("Formatting SPIFFS"); 
   SPIFFS.format(); 
  }
  

  if (SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
       DynamicJsonDocument json(1024);
       auto error = deserializeJson(json, buf.get());
       
       // serializeJson(json, Serial);
        if (!error) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);   
          strcpy(boardname, json["boardname"]);                    
          
        } else {
          Serial.println("failed to load json config");
          Serial.println(error.c_str());
        }
      }
    }
    else {
     Serial.println("/config.json not found"); 
    }
    
  } else {
    Serial.println("failed to mount FS");
  }
}

void saveconfig () {
    Serial.println("saving config");
    DynamicJsonDocument json(512);    
    Serial.println("Saving . . . . ");
    
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["boardname"] = boardname;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    //serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
}
//===== Config file routines ===================================^
