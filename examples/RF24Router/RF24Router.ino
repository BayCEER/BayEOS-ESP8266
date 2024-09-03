/*
   RF24-WLAN-Router

   You need to install the RF24-library via library manager

   RF24-Module is connected via SPI (SCK D5/GPIO14, MISO D6/GPIO12, MOSI D7/GPIO13)
   and a CE + CS pin

*/

//BUFFER Configuration
//Choose either RAM- _or_ SPIFFSBUFFER
#define RAMBUFFER_SIZE 10000
//#define SPIFFSBUFFER_SIZE 500000


//WIFI Configuration
const char* ssid     = "@BayernWLAN";      // SSID
const char* password = "";      // Password

//BayEOS Configuration
const char* name = "ESP8266-Test";
const char* host = "192.168.2.108";
const char* port = "80";
const char* path = "gateway/frame/saveFlat";
const char* user = "import";
const char* pw = "import";

//RF24 Configruation
const uint8_t RF24_CHANNEL=0x7e;
const bool WITH_RF24_CHECKSUM=1;
const uint8_t pipe_0[] = {0x12, 0xae, 0x31, 0xc4, 0x45};
const uint8_t pipe_1[] = {0x24, 0xae, 0x31, 0xc4, 0x45};
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
#ifdef RX_LED
  pinMode(RX_LED, OUTPUT);
  digitalWrite(RX_LED, HIGH);
#endif;
#ifdef TX_LED
  pinMode(TX_LED, OUTPUT);
  digitalWrite(TX_LED, HIGH);
#endif
  Serial.begin(9600);
#ifdef SPIFFSBUFFER_SIZE
  SPIFFS.begin();
  myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE);
#endif

  client.setBuffer(myBuffer);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

  //Gateway Configuration
  client.setConfig(name, host, port, path, user, pw);
  initRF24();
  Serial.println("Setup OK");
#ifdef RX_LED
  digitalWrite(RX_LED, LOW);
#endif;
#ifdef TX_LED
  digitalWrite(TX_LED, LOW);
#endif

}


void loop(void) {
#ifdef RX_LED
  blink_rx();
#endif;
#ifdef TX_LED
  blink_tx();
#endif

  handleRF24();
  checkTX();
}
