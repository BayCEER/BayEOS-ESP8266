/*
   WifiSwitch with Web Configuration
   for ESP01-Module connected to ATMega

   You need to install the RF24,WiFi-Manger via library manager

   Configuration:
   Connect to AP "WifiSwitch" (PW: bayeos24)
   Open http://192.168.4.1
   After configuration the module will start automacally if AP is available.

*/

//BUFFER Configuration
#define RAMBUFFER_SIZE 2000
#define BAUD_RATE 9600
#define SWITCH_OFF_FACTOR 1

unsigned long rx_count;
uint8_t relais;
float relais_on;
float relais_rx;
float relais_rx_error;


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
#include <BaySerial.h>
BaySerial rx_client(Serial);

#include <BayEOSBufferRAM.h>
uint8_t buffer[RAMBUFFER_SIZE];
BayEOSBufferRAM myBuffer(buffer, RAMBUFFER_SIZE);


#include "handler.h"

unsigned long last_connect;


void setup(void) {
  rx_client.begin(BAUD_RATE);
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

  wifiManager.addParameter(&custom_text_gateway);
  wifiManager.addParameter(&custom_bayeos_name);
  wifiManager.addParameter(&custom_bayeos_gateway);
  wifiManager.addParameter(&custom_bayeos_user);
  wifiManager.addParameter(&custom_bayeos_pw);

  wifiManager.setConfigPortalTimeout(180);

  if (!wifiManager.autoConnect("WifiSwitch", "bayeos24")) {
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


  if (shouldSaveConfig) {
    saveConfig();
    //end save
  }
  //EEPROM.end();




  //Gateway Configuration
  client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, path, cfg.bayeos_user, cfg.bayeos_pw);

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/command", handleCommand);
  server.on("/bin", handleBin);
  server.on("/chart", handleChart);
  server.on("/bayeosParser.js", handleBayEOSParser_JS);
  server.on("/base64js.min.js", handleBase64_min_JS);

  server.onNotFound(handleNotFound);

  server.begin();

  last_connect = millis();
  client.setBuffer(myBuffer);
  rx_client.setBuffer(myBuffer);

}

String current_IP = "";

void handleRX(void) {
  if (!Serial.available()) return;

  if (! rx_client.readIntoPayload()) {
    //got Packet
    rx_count++;
    rx_client.writeToBuffer();
    if(rx_client.getPayload(0)==BayEOS_DataFrame){
      BayEOSframe_t frame;
      rx_client.parse(&frame);
      if (frame.channel_count) {
        relais = *(float*) frame.data;
        relais_on = *(float*) (frame.data + 4);
        relais_rx = *(float*) (frame.data + 8);
        relais_rx_error = *(float*) (frame.data + 12);
      }
    }
  }



}

void loop(void) {
  server.handleClient();
  handleRX();
  if (WiFi.status() == WL_CONNECTED) {
    last_connect = millis();
    if (myBuffer.available())
      client.sendMultiFromBuffer(3000);
  } else if ((millis() - last_connect) > 120000) {
    ESP.restart();
    delay(5000);
  }

  if (current_IP != WiFi.localIP().toString()) {
    current_IP = WiFi.localIP().toString();
    client.startFrame(BayEOS_Message);
    client.addToPayload("IP: <a href=\"http://");
    client.addToPayload(current_IP);
    client.addToPayload("\">");
    client.addToPayload(current_IP);
    client.addToPayload("</a>");
    client.writeToBuffer();
  }

}
