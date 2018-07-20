/*
   RF24-WLAN-Router

   You need to install the RF24-library via library manager

   RF24-Module is connected via SPI (SCK D5/GPIO14, MISO D6/GPIO12, MOSI D7/GPIO13)
   and a CE + CS pin

*/

#include <BayEOSBufferRAM.h>
#include <BayEOS-ESP8266.h>

#include <RF24.h>

//WIFI Configuration
const char* ssid     = "@BayernWLAN";      // SSID
const char* password = "";      // Password

//BayEOS Configuration
const char* name="ESP8266-Test";
const char* host="132.180.112.55";
const char* port="80";
const char* path="gateway/frame/saveFlat";
const char* user="import";
const char* pw="import";

//RF24 Configruation
#define NRF24_CHANNEL 0x7e
#define WITH_RF24_CHECKSUM 1
const uint8_t pipe_0[] = {0x12, 0xae, 0x31, 0xc4, 0x45};
const uint8_t pipe_1[] = {0x24, 0xae, 0x31, 0xc4, 0x45};
const uint8_t pipe_2[] = {0x48};
const uint8_t pipe_3[] = {0x96};
const uint8_t pipe_4[] = {0xab};
const uint8_t pipe_5[] = {0xbf};

uint16_t rx_ok, rx1_count, rx1_error;
RF24 radio(D0, D8);
//RF24 radio(D0, D1);



BayESP8266 client;
#define BUFFER_SIZE 10000
#define SENDINT 60000
uint8_t buffer[BUFFER_SIZE];
BayEOSBufferRAM myBuffer(buffer, BUFFER_SIZE);

void initRF24(void) {
  radio.begin();
  radio.powerUp();
  radio.setChannel(NRF24_CHANNEL);
  radio.setPayloadSize(32);
  radio.enableDynamicPayloads();
  radio.setCRCLength(RF24_CRC_16);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setRetries(15, 15);
  radio.setAutoAck(true);
  radio.openReadingPipe(0, pipe_0);
  radio.openReadingPipe(1, pipe_1);
  radio.openReadingPipe(2, pipe_2);
  radio.openReadingPipe(3, pipe_3);
  radio.openReadingPipe(4, pipe_4);
  radio.openReadingPipe(5, pipe_5);
  radio.startListening();
  // radio.printDetails();

}

uint8_t handleRF24(void) {
  uint8_t pipe_num, len;
  uint8_t payload[32];
  char origin[] = "P0";
#ifdef RF24_P1_LETTER
  origin[0] = RF24_P1_LETTER;
#endif
  uint8_t count;
  uint8_t rx = 0;
  while (radio.available(&pipe_num)) {
    Serial.println("Got RF24...");
    count++;
    if (len = radio.getDynamicPayloadSize()) {
      rx++;
      origin[1] = '0' + pipe_num;
      client.startOriginFrame(origin, 1); //Routed Origin!
      if (len > 32)
        len = 32;
      radio.read(payload, len);
      for (uint8_t i = 0; i < len; i++) {
        client.addToPayload(payload[i]);
      }
#if WITH_RF24_CHECKSUM
      if (! client.validateChecksum()) {
        client.writeToBuffer();
        rx1_count++;
      } else
        rx1_error++;
#else
      client.writeToBuffer();
      rx1_count++;
#endif

    } else {
      rx1_error++;
      radio.read(payload, len);
    }
    if (count > 10)
      break;
  }

  if (count > 10)
    initRF24();

  delay(10); //Calling to often leads to unstable RF24 RX
  return rx;
}



void setup(void) {
  Serial.begin(9600);
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
  client.setConfig(name,host,port,path,user,pw);
  initRF24();
  Serial.println("Setup OK");
}

unsigned long next, tx_time;

void loop(void) {
  handleRF24();
  if (next < millis()) {
    client.startDataFrame();
    client.addChannelValue(millis());
    client.addChannelValue(rx1_count);
    rx1_count = 0;
    client.addChannelValue(rx1_error);
    rx1_error = 0;
    client.addChannelValue(tx_time);
    client.writeToBuffer();

    Serial.print("Sending..");
    uint8_t res = 0;
    tx_time = millis();
    while (! res && myBuffer.available()) {
      res = client.sendMultiFromBuffer(3000);
    }
    tx_time = millis() - tx_time;
    Serial.print("res=");
    Serial.println(res);
    next = millis() + SENDINT;
  }
}
