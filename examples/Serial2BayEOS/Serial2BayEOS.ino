/*
   Serial-WLAN-Router with Web Configuration
   for ESP01-Module connected to ATMega

   You need to install the RF24,WiFi-Manger via library manager

   Configuration:
   Connect to AP "Serial2BayEOS" (PW: bayeos24)
   Open http://192.168.4.1
   After configuration the module will start automacally if AP is available.

*/

//BUFFER Configuration
#define RAMBUFFER_SIZE 4000
const char router_name[] = "Serial2BayEOS Router 1.0";
unsigned long rx_count;



#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}


//BayEOS Configuration
const char* port = "80";
const char* path = "gateway/frame/saveFlat";


#include <EEPROM.h>
#include "myConfig.h"


#include <BayEOS-ESP8266.h>
BayESP8266 client;

#include <BayEOSBufferRAM.h>
uint8_t buffer[RAMBUFFER_SIZE];
BayEOSBufferRAM myBuffer(buffer, RAMBUFFER_SIZE);


#include "handler.h"

unsigned long last_connect;


bool got_newline = 0;

char line_buffer[512];

void setup(void) {
  Serial.setRxBufferSize(512);

  Serial.begin(9600);
  EEPROM.begin(2048);

  //WiFiManager
  WiFiManager wifiManager;

  uint8_t reset_storage = 0;
  delay(1000);
  while (! digitalRead(0)) {
    reset_storage++;
    delay(50);
    if (reset_storage > 20) {
      wifiManager.resetSettings();
      delay(1000);
      break;
    }
  }

  reset_storage = 0;
  while (! digitalRead(0)) {
    reset_storage++;
    delay(50);
    if (reset_storage > 20) {
      eraseConfig();
      break;
    }
  }
  loadConfig();



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
  char buffer[10];
  WiFiManagerParameter custom_baud("baud", "Serial Baud Rate", ltoa(cfg.baud, buffer, 10), 10);

  wifiManager.addParameter(&custom_text_gateway);
  wifiManager.addParameter(&custom_bayeos_name);
  wifiManager.addParameter(&custom_bayeos_gateway);
  wifiManager.addParameter(&custom_bayeos_user);
  wifiManager.addParameter(&custom_bayeos_pw);
  wifiManager.addParameter(&custom_baud);

  wifiManager.setConfigPortalTimeout(180);

  if (!wifiManager.autoConnect("Serial2BayEOS", "bayeos24")) {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }


  //read updated parameters
  strcpy(cfg.bayeos_gateway, custom_bayeos_gateway.getValue());
  strcpy(cfg.bayeos_name, custom_bayeos_name.getValue());
  strcpy(cfg.bayeos_user, custom_bayeos_user.getValue());
  strcpy(cfg.bayeos_pw, custom_bayeos_pw.getValue());
  cfg.baud = atol(custom_baud.getValue());

  if (shouldSaveConfig) {
    saveConfig();

    //end save
  }
  //EEPROM.end();


#ifdef SPIFFSBUFFER_SIZE
  SPIFFS.begin();
  myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE);
#endif
  client.setBuffer(myBuffer);


  //Gateway Configuration
  client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, path, cfg.bayeos_user, cfg.bayeos_pw);

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/bin", handleBin);
  server.on("/chart", handleChart);
  server.on("/bayeosParser.js", handleBayEOSParser_JS);
  server.on("/base64js.min.js", handleBase64_min_JS);

  server.onNotFound(handleNotFound);

  server.begin();

  last_connect = millis();
  Serial.begin(cfg.baud);

}

String current_IP = "";

uint16_t pos = 0;
void loop(void) {
  server.handleClient();
  while (Serial.available()) {
    if(! got_newline){
      if(Serial.read()=='\n') got_newline=1;
    } else {
      line_buffer[pos] = Serial.read();
      if (line_buffer[pos] == '\n') {
        char* p = line_buffer;
        line_buffer[pos] = 0;
        client.startDataFrame();
        while (*p) {
          if((*p>='0' && *p<='9') || *p==' ' || *p=='-'){
            client.addChannelValue(strtof(p, &p));
          } else strtof(p, &p);
          p++;
        }
        client.writeToBuffer();
        pos = 0;
      } else if (pos < 510) pos++;
    }
  }
  if (current_IP != WiFi.localIP().toString()) {
    current_IP = WiFi.localIP().toString();
    client.startFrame(BayEOS_Message);
    client.addToPayload("Router IP: <a href=\"http://");
    client.addToPayload(current_IP);
    client.addToPayload("\">");
    client.addToPayload(current_IP);
    client.addToPayload("</a>");
    client.writeToBuffer();
  }

  if (WiFi.status() == WL_CONNECTED) {
    last_connect = millis();
  } else if ((millis() - last_connect) > 120000) {
    ESP.restart();
    delay(5000);
  }
  client.sendFromBuffer();
}
