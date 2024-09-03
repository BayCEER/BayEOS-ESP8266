/*
   WifiPowerPlug with Web Configuration
   for ESP01-Module connected to ATMega

   You need to install the RF24,WiFi-Manger via library manager

   Configuration:
   Connect to AP "WifiPowerPlug" (PW: bayeos24)
   Open http://192.168.4.1
   After configuration the module will start automacally if AP is available.

*/

#define SWITCH_OFF_FACTOR 1

uint8_t relais;
float relais_on;
uint16_t relais_minutes;
unsigned long relais_millis;


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

#include "handler.h"



void setup(void) {
  Serial.begin(9600);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  //WiFiManager
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 254), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  wifiManager.setConfigPortalTimeout(180);

  if (!wifiManager.autoConnect("ESP01Relais", "fablab24")) {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }


  server.on("/", handleRoot);
  server.on("/command", handleCommand);
  server.onNotFound(handleNotFound);

  server.begin();

}


void loop(void) {
  server.handleClient();

  if (relais) {
    relais_on = relais_minutes - (float)((millis() - relais_millis)) / 60000;
    if (relais_on <= 0) {
      digitalWrite(0, LOW);
      relais = 0;
      relais_on = 0;

    }
  }
}
