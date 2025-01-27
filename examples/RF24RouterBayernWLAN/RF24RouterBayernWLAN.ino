/*
   RF24-WLAN-Router with Web Configuration
   Choose NodeMCU 1.0 (ESP-12E Module) as Board

   You need to install the RF24,WiFi-Manger via library manager
.

   Reconfiguration:
   1. Press RESET
   2. Wait for LEDs to light up
   3. Immediately press and hold PROG
   4. (Optional) Keep pressing PROG and the SPIFFS will get formated

*/

//BUFFER Configuration
//Choose either RAM- _or_ SPIFFSBUFFER
//#define RAMBUFFER_SIZE 20000
#define SPIFFSBUFFER_SIZE 500000
#define MINFREESPACE 5000
const char router_name[] = "BayEOS WIFI RF24 Router 1.1";
//LED Configuration
#define RX_LED D1
#define TX_LED D2

//Default configuration.
const char* bayeos_gateway = "192.168.2.100";
const char* bayeos_user = "import";
const char* bayeos_pw = "ChangeMe";
const char* bayeos_origin = "MyBoard";
const long valid_config = 0x37a52179;
const long rf24_base = 0x45c431ae;
const uint8_t rf24_channel = 0x7e;
const bool rf24_checksum = 1;
const bool disable_leds = 0;




#include <FS.h>           //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "certs.h"

//BayEOS Configuration
const char* port = "80";
const char* ssid = "@BayernWLAN";  // SSID
const char* password = "";         // Password

//RF24 Configruation
bool WITH_RF24_CHECKSUM = 1;
uint8_t RF24_CHANNEL = 0x7e;
uint8_t pipe_0[] = { 0x12, 0xae, 0x31, 0xc4, 0x45 };
uint8_t pipe_1[] = { 0x24, 0xae, 0x31, 0xc4, 0x45 };
const uint8_t pipe_2[] = { 0x48 };
const uint8_t pipe_3[] = { 0x96 };
const uint8_t pipe_4[] = { 0xab };
const uint8_t pipe_5[] = { 0xbf };



#include <EEPROM.h>
#include "myConfig.h"


#include <BayEOS-ESP8266.h>
BayESP8266 client;
#include <BayDebug.h>
#define DEBUGBUFFER_SIZE 100
char debug_buffer[DEBUGBUFFER_SIZE];
BayDebugCharbuffer debug_client(debug_buffer, DEBUGBUFFER_SIZE);

#ifdef RAMBUFFER_SIZE
#include <BayEOSBufferRAM.h>
uint8_t buffer[RAMBUFFER_SIZE];
BayEOSBufferRAM myBuffer(buffer, RAMBUFFER_SIZE);
#endif

#ifdef SPIFFSBUFFER_SIZE
#include <BayEOSBufferSPIFFS.h>
BayEOSBufferSPIFFS myBuffer;
#endif
RTC_Millis myRTC;

#define NR_RX_BUFFER 30
#include <RF24Router.h>

#include "handler.h"

void confirm() {
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
}


void setup(void) {
  pinMode(RX_LED, OUTPUT);
  digitalWrite(RX_LED, HIGH);
  pinMode(TX_LED, OUTPUT);
  digitalWrite(TX_LED, HIGH);
  Serial.begin(115200);
  EEPROM.begin(2048);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("Connected");
  confirm();

  loadConfig();

  char rf24_channel[3];
  char rf24_base[9];
  char rf24_checksum[2];
  itoa(cfg.rf24_base, rf24_base, 16);
  itoa(cfg.rf24_channel, rf24_channel, 16);
  itoa(cfg.rf24_checksum, rf24_checksum, 10);


  *(long*)(pipe_0 + 1) = cfg.rf24_base;
  *(long*)(pipe_1 + 1) = cfg.rf24_base;
  WITH_RF24_CHECKSUM = cfg.rf24_checksum;
  RF24_CHANNEL = cfg.rf24_channel;

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

#ifdef SPIFFSBUFFER_SIZE
  SPIFFS.begin();
  loadBufferData();
  if (buffer_data.time > 0 && buffer_data.time < (20 * 365 * 24 * 3600)) {
    myRTC.set(buffer_data.time + millis() / 1000);
    buffer_data.time = 0;
    saveBufferData();
    myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE, 1);
    myBuffer.set(buffer_data.read_pos, buffer_data.write_pos, buffer_data.end_pos);
  } else
    myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE, 0);
#endif
  myBuffer.setRTC(myRTC, RTC_RELATIVE_SECONDS);  // use the rtc clock but relative
  client.setBuffer(myBuffer);


  //Gateway Configuration
  client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, cfg.bayeos_path, cfg.bayeos_user, cfg.bayeos_pw);
  initRF24();
  digitalWrite(RX_LED, LOW);
  digitalWrite(TX_LED, LOW);
  Serial.println("Setup OK");

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/pipe", handlePipe);
  server.on("/bin", handleBin);
  server.on("/chart", handleChart);
  server.on("/bayeosParser.js", handleBayEOSParser_JS);
  server.on("/base64js.min.js", handleBase64_min_JS);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}


void loop(void) {
#ifdef RX_LED
  if (cfg.disable_leds) digitalWrite(RX_LED, LOW);
  else blink_rx();
#endif;
#ifdef TX_LED
  if (cfg.disable_leds) digitalWrite(TX_LED, LOW);
  else blink_tx();
#endif;

  //Check for buffer is going to wrap around...
  if (radio_is_up) {
    if (myBuffer.freeSpace() < MINFREESPACE) {
      radio.powerDown();
      radio_is_up = 0;
    }
  } else {
    if (myBuffer.freeSpace() > MINFREESPACE)
      initRF24();
  }

  handleRF24();
  server.handleClient();
  checkTX();
  if ((millis()-last_tx_success) > 300000) {
    buffer_data.read_pos = myBuffer.readPos();
    buffer_data.write_pos = myBuffer.writePos();
    buffer_data.end_pos = myBuffer.endPos();
    buffer_data.time = myRTC.get() + 1;
    saveBufferData();
    ESP.restart();
  }
}
