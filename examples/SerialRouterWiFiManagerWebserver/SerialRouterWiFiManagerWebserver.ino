/*
   Serial-WLAN-Router with Web Configuration
   for ESP01-Module connected to ATMega

   You need to install the RF24,WiFi-Manger via library manager

   Configuration:
   Connect to AP "BayEOS-RF24-Router" (PW: bayeos24)
   Open http://192.168.4.1
   After configuration the module will start automacally if AP is available.

*/

//BUFFER Configuration
#define RAMBUFFER_SIZE 4000
#define BAUD_RATE 38400
const char router_name[] = "BayEOS WIFI Serial Router 1.4";
unsigned long rx_count;

#define SEND_IP_MESSAGE 0

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}


//BayEOS Configuration
const char* port = "80";


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
  Serial.begin(BAUD_RATE);
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

  loadConfig();



  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 254), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  WiFiManagerParameter custom_text_gateway("<h4>BayEOS-Gateway Configuration</h4>");
  WiFiManagerParameter custom_bayeos_name("bayeos_name", "Origin", cfg.bayeos_name, 40);
  WiFiManagerParameter custom_bayeos_gateway("server", "BayEOS Gateway IP", cfg.bayeos_gateway, 40);
  WiFiManagerParameter custom_bayeos_path("path", "BayEOS Gateway Path", cfg.bayeos_path, 40);
  WiFiManagerParameter custom_bayeos_user("bayeos_user", "BayEOS User", cfg.bayeos_user, 40);
  WiFiManagerParameter custom_bayeos_pw("bayeos_pw", "BayEOS Password", cfg.bayeos_pw, 40);

  wifiManager.addParameter(&custom_text_gateway);
  wifiManager.addParameter(&custom_bayeos_name);
  wifiManager.addParameter(&custom_bayeos_gateway);
  wifiManager.addParameter(&custom_bayeos_path);
  wifiManager.addParameter(&custom_bayeos_user);
  wifiManager.addParameter(&custom_bayeos_pw);

  wifiManager.setConfigPortalTimeout(180);

  if (!wifiManager.autoConnect("BayEOS-Serial-Router", "bayeos24")) {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //read updated parameters
  strcpy(cfg.bayeos_gateway, custom_bayeos_gateway.getValue());
  strcpy(cfg.bayeos_name, custom_bayeos_name.getValue());
  strcpy(cfg.bayeos_path, custom_bayeos_path.getValue());
  strcpy(cfg.bayeos_user, custom_bayeos_user.getValue());
  strcpy(cfg.bayeos_pw, custom_bayeos_pw.getValue());


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
  rx_client.setBuffer(myBuffer);


  //Gateway Configuration
  client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, cfg.bayeos_path, cfg.bayeos_user, cfg.bayeos_pw);


  // BayernWLAN
  if (WiFi.SSID() == "@BayernWLAN") {
    Serial.println("@BayernWLAN:");
    std::unique_ptr<BearSSL::WiFiClientSecure> ssl_client(new BearSSL::WiFiClientSecure);
    // ignore the SSL certificate
    ssl_client->setInsecure();

    HTTPClient https;

    https.begin(*ssl_client, "hotspot.vodafone.de", 443, "/api/v4/login?loginProfile=6", true);
    int httpCode = https.GET();
    Serial.println(https.getString());
    Serial.flush();
    https.end();
  }


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
  rx_client.begin(BAUD_RATE);


}

void loop(void) {
  server.handleClient();
  if (Serial.available()) {
    if (! rx_client.readIntoPayload()) {
      //got Packet
      rx_count++;
      if (rx_client.getPayload(0) == BayEOS_Command && rx_client.getPayload(1) == BayEOS_RouterCommand ) {
        uint8_t command = rx_client.getPayload(2);
        rx_client.startCommandResponse(BayEOS_RouterCommand);
        rx_client.addToPayload(command);
        switch (command) {
          case ROUTER_IS_READY:
            if ( WiFi.status() == WL_CONNECTED) rx_client.addToPayload((uint8_t) 0);
            else rx_client.addToPayload((uint8_t) 1);
            break;
          case ROUTER_GET_AVAILABLE:
            rx_client.addToPayload(myBuffer.available());
            break;
          case ROUTER_SET_NAME:
            strcpy(cfg.bayeos_name, (char*) rx_client.getPayload() + 3);
            saveConfig();
            client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, cfg.bayeos_path, cfg.bayeos_user, cfg.bayeos_pw);
            rx_client.addToPayload((uint8_t) (10 + strlen(*client.getConfigPointer(BayTCP_CONFIG_SENDER))));
            break;
          case ROUTER_SET_CONFIG:
            switch (rx_client.getPayload(3)) {
              case BaySerialESP_NAME:
                strcpy(cfg.bayeos_name, (char*) rx_client.getPayload() + 4);
                break;
              case BaySerialESP_GATEWAY:
                strcpy(cfg.bayeos_gateway, (char*) rx_client.getPayload() + 4);
                break;
              case BaySerialESP_USER:
                strcpy(cfg.bayeos_user, (char*) rx_client.getPayload() + 4);
                break;
              case BaySerialESP_PW:
                strcpy(cfg.bayeos_pw, (char*) rx_client.getPayload() + 4);
                break;
            }
            saveConfig();
            client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, cfg.bayeos_path, cfg.bayeos_user, cfg.bayeos_pw);
            rx_client.addToPayload((uint8_t) (10 + strlen((char*) rx_client.getPayload() + 4)));
            break;
          case ROUTER_SEND:
            uint8_t res = 0;
            while (myBuffer.available() && ! res) {
              res = client.sendMultiFromBuffer(3000);
            }
            rx_client.addToPayload(res);
            break;
        }
        rx_client.sendPayload();
      } else {
        rx_client.writeToBuffer();
      }
    }

  }
  if (WiFi.status() == WL_CONNECTED) {
    last_connect = millis();
  } else if ((millis() - last_connect) > 120000) {
    ESP.restart();
    delay(5000);
  }

#if SEND_IP_MESSAGE
  if (WiFi.status() == WL_CONNECTED && strcmp(WiFi.localIP().toString().c_str(), cfg.ip)) {
    strcpy(cfg.ip, WiFi.localIP().toString().c_str());
    saveConfig();
    client.startFrame(BayEOS_Message);
    client.addToPayload("IP: <a href=\"http://");
    client.addToPayload(cfg.ip);
    client.addToPayload("\">");
    client.addToPayload(cfg.ip);
    client.addToPayload("</a>");
    client.writeToBuffer();
  }
#endif
}
