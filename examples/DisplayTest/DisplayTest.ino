#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN    2
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#include <BaySerialRF24.h>
#define RF24_CE 16
#define RF24_CS 15
RF24 radio(RF24_CE, RF24_CS);
BaySerialRF24 client(radio);

const uint8_t rf24_channel = 0x3e;
const uint64_t pipes[6] = { 0x45c431ae12LL, 0x45c431ae24LL, 0x45c431ae48LL,
                            0x45c431ae96LL, 0x45c431aeabLL, 0x45c431aebfLL
                          };

#include <SPI.h>


void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting RF24");
  radio.begin();
  radio.setChannel(rf24_channel);
  radio.setPayloadSize(32);
  radio.enableDynamicPayloads();
  radio.setCRCLength( RF24_CRC_16 ) ;
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setRetries(15, 15);
  radio.setAutoAck(true);
  //radio.openWritingPipe(pipes[0]);
  for (uint8_t i = 0; i < 6; i++) {
    radio.openReadingPipe(i, pipes[i]);
  }
  radio.startListening();
  Serial.println("Starting Display");
  P.begin();
  //SPI.setFrequency(500000);
  P.setIntensity(8);
  P.displayText("TEST", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();
  Serial.println("Setup END");
}

uint8_t frameDelay = 25;  // default frame delay value
textEffect_t  scrollEffect = PA_SCROLL_LEFT;

String text;
char text_array[500];
char origin[]="RF0";

void loop(void) {
  if (P.displayAnimate()) {
    uint8_t pipe_num, len;
    uint8_t payload[32];
    if ( radio.available(&pipe_num) && (len = radio.getDynamicPayloadSize())) {
      Serial.println(len);
      text="Pipe";
      text+=pipe_num;
      radio.read(payload, len);
      //origin[3] = '0' + pipe_num;
      //client.startOriginFrame(origin, 1);
      client.startFrame();
      for (uint8_t i = 0; i < len; i++) {
        client.addToPayload(payload[i]);
      }
      BayEOSframe_t frame;
      client.parse(&frame);
      for(int i=0;i<frame.channel_count;i++) {
        text+=" CH";
        text+=i;
        text+=": ";
        text+=*(float*) (frame.data+i*4);      
      }
      Serial.println(text);
      text.toCharArray(text_array,500);
      P.displayScroll(text_array, PA_LEFT, scrollEffect, frameDelay);
      P.displayReset();
    }
  }
  delay(1);

}
