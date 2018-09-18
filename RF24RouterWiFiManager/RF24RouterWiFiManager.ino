/*
   RF24-WLAN-Router with Web Configuration

   You need to install the RF24,WiFi-Manger,ArduinoJson-library via library manager
   ATTENTION: ArduinoJson must be version 5.x.x!!

   Configuration:
   Connect to AP "BayEOS-RF24-Router" (PW: bayeos24)
   Open http://192.168.4.1
   After configuration the module will start automacally if AP is available.

   Reconfiguration:
   1. Press RESET
   2. Wait for LEDs to light up
   3. Immediately press and hold PROG
   4. When LEDs start to flash WLAN-Config was deleted
   5. (Optional) Keep pressing PROG and the SPIFFS will get formated

*/
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//define your default values here, if there are different values in bayeos.json, they are overwritten.
char bayeos_server[40] = "192.168.2.108";
char bayeos_name[40] = "MyRF24-Router";
char bayeos_user[40] = "import";
char bayeos_pw[40] = "import";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//BUFFER Configuration
//Choose either RAM- _or_ SPIFFSBUFFER
//#define RAMBUFFER_SIZE 20000
#define SPIFFSBUFFER_SIZE 500000
#define MINFREESPACE 5000


//BayEOS Configuration
const char* port = "80";
const char* path = "gateway/frame/saveFlat";

//RF24 Configruation
char rf24_channel[3] = "7e";
char rf24_base[9] = "45c431ae";
uint8_t NRF24_CHANNEL = 0x7e;
#define WITH_RF24_CHECKSUM 1
uint8_t pipe_0[] = {0x12, 0xae, 0x31, 0xc4, 0x45};
uint8_t pipe_1[] = {0x24, 0xae, 0x31, 0xc4, 0x45};
const uint8_t pipe_2[] = {0x48};
const uint8_t pipe_3[] = {0x96};
const uint8_t pipe_4[] = {0xab};
const uint8_t pipe_5[] = {0xbf};


//LED Configuration
#define RX_LED D1
#define TX_LED D2



#include <BayEOS-ESP8266.h>
BayESP8266 client;

#define SENDINT 60000

#ifdef RAMBUFFER_SIZE
#include <BayEOSBufferRAM.h>
uint8_t buffer[RAMBUFFER_SIZE];
BayEOSBufferRAM myBuffer(buffer, RAMBUFFER_SIZE);
#endif

#ifdef SPIFFSBUFFER_SIZE
#include <BayEOSBufferSPIFFS.h>
BayEOSBufferSPIFFS myBuffer;
#endif

#include <RF24Router.h>


void setup(void) {
  pinMode(RX_LED, OUTPUT);
  digitalWrite(RX_LED, HIGH);
  pinMode(TX_LED, OUTPUT);
  digitalWrite(TX_LED, HIGH);
  Serial.begin(115200);

  //WiFiManager
  WiFiManager wifiManager;

  uint8_t reset_storage = 0;
  delay(1000);
  while (! digitalRead(0)) {
    reset_storage++;
    delay(50);
    if (reset_storage > 20) {
      Serial.print("reseting WiFiManager... ");
      wifiManager.resetSettings();
      Serial.println("done");
      for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(TX_LED, LOW);
        delay(300);
        digitalWrite(RX_LED, LOW);
        digitalWrite(TX_LED, HIGH);
        delay(300);
        digitalWrite(RX_LED, HIGH);
      }
      delay(1000);
      break;
    }
  }

  reset_storage = 0;
  while (! digitalRead(0)) {
    reset_storage++;
    delay(50);
    if (reset_storage > 20) {
      Serial.print("deleting files on SPIFFS ...");
      SPIFFS.format();
      Serial.println("done");
      for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(TX_LED, LOW);
        delay(300);
        digitalWrite(RX_LED, LOW);
        digitalWrite(TX_LED, HIGH);
        delay(300);
        digitalWrite(RX_LED, HIGH);
      }
      break;

    }
  }

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/bayeos.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/bayeos.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(bayeos_name, json["bayeos_name"]);
          strcpy(bayeos_server, json["bayeos_server"]);
          strcpy(bayeos_user, json["bayeos_user"]);
          strcpy(bayeos_pw, json["bayeos_pw"]);
          strcpy(rf24_channel, json["rf24_channel"]);
          strcpy(rf24_base, json["rf24_base"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read



  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 254), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  WiFiManagerParameter custom_text_gateway("<h4>BayEOS-Gateway Configuration</h4>");
  WiFiManagerParameter custom_bayeos_name("bayeos_name", "bayeos name", bayeos_name, 40);
  WiFiManagerParameter custom_bayeos_server("server", "bayeos server", bayeos_server, 40);
  WiFiManagerParameter custom_bayeos_user("bayeos_user", "bayeos user", bayeos_user, 40);
  WiFiManagerParameter custom_bayeos_pw("bayeos_pw", "bayeos password", bayeos_pw, 40);

  wifiManager.addParameter(&custom_text_gateway);
  wifiManager.addParameter(&custom_bayeos_name);
  wifiManager.addParameter(&custom_bayeos_server);
  wifiManager.addParameter(&custom_bayeos_user);
  wifiManager.addParameter(&custom_bayeos_pw);

  WiFiManagerParameter custom_text_rf24("<h4>RF24-Configuration</h4>");
  WiFiManagerParameter custom_rf24_channel("rf24_channel", "bayeos name (HEX)", rf24_channel, 2);
  WiFiManagerParameter custom_rf24_base("rf24base", "rf24 pipe base (HEX)", rf24_base, 8);
  WiFiManagerParameter custom_text_rf24_expl("<p>Channel and rf24 pipe base must be given in hex numbers. The resulting listen pipes will be<br>0x_BASE_12, 0x_BASE_24, 0x_BASE_48, 0x_BASE_96, 0x_BASE_ab, 0x_BASE_bf<br></p>");

  wifiManager.addParameter(&custom_text_rf24);
  wifiManager.addParameter(&custom_rf24_channel);
  wifiManager.addParameter(&custom_rf24_base);
  wifiManager.addParameter(&custom_text_rf24_expl);


  if (!wifiManager.autoConnect("BayEOS-RF24-Router", "bayeos24")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(bayeos_server, custom_bayeos_server.getValue());
  strcpy(bayeos_name, custom_bayeos_name.getValue());
  strcpy(bayeos_user, custom_bayeos_user.getValue());
  strcpy(bayeos_pw, custom_bayeos_pw.getValue());
  strcpy(rf24_channel, custom_rf24_channel.getValue());
  strcpy(rf24_base, custom_rf24_base.getValue());
  NRF24_CHANNEL = strtol(rf24_channel, 0, 16);
  long base = strtol(rf24_base, 0, 16);
  *(long*)(pipe_0 + 1) = base;
  *(long*)(pipe_1 + 1) = base;


  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["bayeos_name"] = bayeos_name;
    json["bayeos_server"] = bayeos_server;
    json["bayeos_user"] = bayeos_user;
    json["bayeos_pw"] = bayeos_pw;
    json["rf24_channel"] = rf24_channel;
    json["rf24_base"] = rf24_base;

    File configFile = SPIFFS.open("/bayeos.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());


#ifdef SPIFFSBUFFER_SIZE
  myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE);
#endif
  client.setBuffer(myBuffer);


  //Gateway Configuration
  client.setConfig(bayeos_name, bayeos_server, port, path, bayeos_user, bayeos_pw);
  initRF24();
  digitalWrite(RX_LED, LOW);
  digitalWrite(TX_LED, LOW);
  Serial.println("Setup OK");

}


void loop(void) {
#ifdef RX_LED
  blink_rx();
#endif;
#ifdef TX_LED
  blink_tx();
#endif;

  //Check for buffer is going to wrap around...  
  if(radio_is_up){
    if(myBuffer.freeSpace()<MINFREESPACE){
      radio.powerDown();
      radio_is_up=0;
    }
  } else {
    if(myBuffer.freeSpace()>MINFREESPACE)
      initRF24();
  }

  handleRF24();
  checkTX();

 }
