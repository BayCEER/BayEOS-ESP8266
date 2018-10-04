/*
   RF24-WLAN-Router with Web Configuration
   Choose NodeMCU 1.0 (ESP-12E Module) as Board

   You need to install the RF24,WiFi-Manger via library manager

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

//BUFFER Configuration
//Choose either RAM- _or_ SPIFFSBUFFER
//#define RAMBUFFER_SIZE 20000
#define SPIFFSBUFFER_SIZE 500000
#define MINFREESPACE 5000

//LED Configuration
#define RX_LED D1
#define TX_LED D2

//Force Checksum Frames
bool WITH_RF24_CHECKSUM = 1;

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


//BayEOS Configuration
const char* port = "80";
const char* path = "gateway/frame/saveFlat";

//RF24 Configruation
uint8_t RF24_CHANNEL = 0x7e;
uint8_t pipe_0[] = {0x12, 0xae, 0x31, 0xc4, 0x45};
uint8_t pipe_1[] = {0x24, 0xae, 0x31, 0xc4, 0x45};
const uint8_t pipe_2[] = {0x48};
const uint8_t pipe_3[] = {0x96};
const uint8_t pipe_4[] = {0xab};
const uint8_t pipe_5[] = {0xbf};



#include <EEPROM.h>
#include "myConfig.h"


#include <BayEOS-ESP8266.h>
BayESP8266 client;
#include <BayDebug.h>
#define DEBUGBUFFER_SIZE 100
char debug_buffer[DEBUGBUFFER_SIZE];
BayDebugCharbuffer debug_client(debug_buffer,DEBUGBUFFER_SIZE);

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

#include "handler.h"

void setup(void) {
  pinMode(RX_LED, OUTPUT);
  digitalWrite(RX_LED, HIGH);
  pinMode(TX_LED, OUTPUT);
  digitalWrite(TX_LED, HIGH);
  Serial.begin(115200);
  EEPROM.begin(2048);

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
      Serial.print("deleting Config in EEPROM ...");
      eraseConfig();
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
  loadConfig();

  char rf24_channel[3];
  char rf24_base[9];
  char rf24_checksum[2];
  itoa(cfg.rf24_base,rf24_base,16);
  itoa(cfg.rf24_channel,rf24_channel,16);
  itoa(cfg.rf24_checksum,rf24_checksum,10);
  
 
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 254), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  WiFiManagerParameter custom_text_gateway("<h4>BayEOS-Gateway Configuration</h4>");
  WiFiManagerParameter custom_bayeos_name("bayeos_name", "Origin", cfg.bayeos_name, 40);
  WiFiManagerParameter custom_bayeos_gateway("server", "BayEOS Gateway", cfg.bayeos_gateway, 40);
  WiFiManagerParameter custom_bayeos_user("bayeos_user", "BayEOS User", cfg.bayeos_user, 40);
  WiFiManagerParameter custom_bayeos_pw("bayeos_pw", "BayEOS Password", cfg.bayeos_pw, 40);

  wifiManager.addParameter(&custom_text_gateway);
  wifiManager.addParameter(&custom_bayeos_name);
  wifiManager.addParameter(&custom_bayeos_gateway);
  wifiManager.addParameter(&custom_bayeos_user);
  wifiManager.addParameter(&custom_bayeos_pw);

  WiFiManagerParameter custom_text_rf24("<h4>RF24-Configuration</h4>");
  WiFiManagerParameter custom_rf24_channel("rf24_channel", "RF24 Channel (HEX)", rf24_channel, 2);
  WiFiManagerParameter custom_rf24_base("rf24base", "RF24 pipe base (HEX)", rf24_base, 8);
  WiFiManagerParameter custom_rf24_checksum("rf24_checksum", "Only frames with Checksum (0/1)", rf24_checksum, 1);
  WiFiManagerParameter custom_text_rf24_expl("<p>Channel and rf24 pipe base must be given in hex numbers. The resulting listen pipes will be<br>0x_BASE_12, 0x_BASE_24, 0x_BASE_48, 0x_BASE_96, 0x_BASE_ab, 0x_BASE_bf<br></p>");

  wifiManager.addParameter(&custom_text_rf24);
  wifiManager.addParameter(&custom_rf24_channel);
  wifiManager.addParameter(&custom_rf24_base);
  wifiManager.addParameter(&custom_rf24_checksum);
  wifiManager.addParameter(&custom_text_rf24_expl);


  if (!wifiManager.autoConnect("BayEOS-RF24-Router 1.0", "bayeos24")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(cfg.bayeos_gateway, custom_bayeos_gateway.getValue());
  strcpy(cfg.bayeos_name, custom_bayeos_name.getValue());
  strcpy(cfg.bayeos_user, custom_bayeos_user.getValue());
  strcpy(cfg.bayeos_pw, custom_bayeos_pw.getValue());
  strcpy(rf24_channel, custom_rf24_channel.getValue());
  strcpy(rf24_base, custom_rf24_base.getValue());
  strcpy(rf24_checksum, custom_rf24_checksum.getValue());
  cfg.rf24_channel=strtol(rf24_channel, 0, 16);
  RF24_CHANNEL = cfg.rf24_channel;

  cfg.rf24_base = strtol(rf24_base, 0, 16);
  *(long*)(pipe_0 + 1) = cfg.rf24_base;
  *(long*)(pipe_1 + 1) = cfg.rf24_base;

  cfg.rf24_checksum=atoi(rf24_checksum);
  WITH_RF24_CHECKSUM=cfg.rf24_checksum;


  if (shouldSaveConfig) {
    Serial.println("saving config");
    saveConfig();

    //end save
  }
  //EEPROM.end();

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

#ifdef SPIFFSBUFFER_SIZE
  SPIFFS.begin();
  myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE);
#endif
  client.setBuffer(myBuffer);


  //Gateway Configuration
  client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, path, cfg.bayeos_user, cfg.bayeos_pw);
  initRF24();
  digitalWrite(RX_LED, LOW);
  digitalWrite(TX_LED, LOW);
  Serial.println("Setup OK");

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/pipe", handlePipe);
  server.on("/bin", handleBin);
  server.on("/chart",handleChart);
  server.on("/bayeosParser.js",handleBayEOSParser_JS);
  server.on("/base64js.min.js",handleBase64_min_JS);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

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
  server.handleClient();
  checkTX();
 }


 


