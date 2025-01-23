/*
   Serial-WLAN-Router with Web Configuration
   for ESP01-Module connected to ATMega

   You need to install the RF24,WiFi-Manger via library manager
*/

//BUFFER Configuration
#define RAMBUFFER_SIZE 4000
#define BAUD_RATE 38400
unsigned long rx_count;

#define SEND_IP_MESSAGE 0

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "certs.h"

const char* ssid     = "@BayernWLAN";      // SSID
const char* password = "";      // Password

//Default configuration.
//can be overwritten by ATMega
const char* bayeos_gateway = "192.168.2.100";
const char* bayeos_user = "import";
const char* bayeos_pw = "ChangeMe";
const char* bayeos_origin = "MyBoard";
const long valid_config=0x37a52179;

//BayEOS Configuration
const char* port = "80";


#include <EEPROM.h>
#include "myConfig.h"
#include "BayEOSCommands.h"


#include <BayEOS-ESP8266.h>
BayESP8266 client;
#include <BaySerial.h>
BaySerial rx_client(Serial);

#include <BayEOSBufferRAM.h>
uint8_t buffer[RAMBUFFER_SIZE];
BayEOSBufferRAM myBuffer(buffer, RAMBUFFER_SIZE);


unsigned long last_connect;


void setup(void) {
  Serial.begin(BAUD_RATE);
  EEPROM.begin(2048);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("Connected");
  std::unique_ptr<BearSSL::WiFiClientSecure> ssl_client(new BearSSL::WiFiClientSecure);
  //ssl_client->setFingerprint(fingerprint_hotspot_vodafone_de);
  // Or, if you happy to ignore the SSL certificate, then use the following line instead:
  ssl_client->setInsecure();

  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");
  https.begin(*ssl_client, vodafone_host, vodafone_port, "/api/v4/login?loginProfile=6", true);
  int httpCode = https.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
    Serial.println(https.getString());
  } else {
    Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
  }
  https.end();


  loadConfig();
  Serial.println("Config:");
  Serial.println(cfg.bayeos_name);
  Serial.println(cfg.bayeos_gateway);
  Serial.println(cfg.bayeos_path);
  Serial.flush();

  client.setBuffer(myBuffer);
  rx_client.setBuffer(myBuffer);

  //Gateway Configuration
  client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, cfg.bayeos_path, cfg.bayeos_user, cfg.bayeos_pw);


  last_connect = millis();
  rx_client.begin(BAUD_RATE);

}

void loop(void) {
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
            if ( WiFi.status() == WL_CONNECTED)
            {
              rx_client.addToPayload((uint8_t) 0);
            }
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
