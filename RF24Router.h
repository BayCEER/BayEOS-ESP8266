#include <RF24.h>

#ifndef SAMPLINGINT
#define SAMPLINGINT 60000
#endif

#ifndef SENDINT
#define SENDINT 20000
#endif

uint16_t rx_ok, rx1_count, rx1_error;

#ifndef RF24_CE
#define RF24_CE 16
#endif

#ifndef RF24_CS
#define RF24_CS 15
#endif

RF24 radio(RF24_CE, RF24_CS);
boolean radio_is_up = 0;

unsigned long total_rx, total_tx, total_tx_error;
unsigned long rx_per_pipe[6];
unsigned long rx_per_pipe_failed[6];

#ifdef RX_LED
unsigned long last_rx_switch;
uint8_t rx_blink;
void blink_rx(void) {
	if (! rx_blink) return;
	if ((millis() - last_rx_switch) < 250) return;
	last_rx_switch = millis();
	if (digitalRead(RX_LED)) {
		digitalWrite(RX_LED, LOW);
		rx_blink--;
	} else
	digitalWrite(RX_LED, HIGH);
}
#endif

#ifdef TX_LED
unsigned long last_tx_switch;
uint8_t tx_blink;
void blink_tx(void) {
	if (! tx_blink) return;
	if ((millis() - last_tx_switch) < 250) return;
	last_tx_switch = millis();
	if (digitalRead(TX_LED)) {
		digitalWrite(TX_LED, LOW);
		tx_blink--;
	} else
	digitalWrite(TX_LED, HIGH);
}

#endif

void initRF24(void) {
	radio.begin();
	radio.powerUp();
	radio.setChannel(RF24_CHANNEL);
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
	radio_is_up = 1;
	// radio.printDetails();
}
#ifndef NR_RX_BUFFER
#define NR_RX_BUFFER 10
#endif

uint8_t payload[6][NR_RX_BUFFER][32];
unsigned long rx_time[6][NR_RX_BUFFER];
uint8_t rx_length[6][NR_RX_BUFFER]={{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}};
uint8_t rx_i[6]={0,0,0,0,0,0};


uint8_t handleRF24(void) {
	if (!radio_is_up)
		return 0;
	uint8_t pipe_num, len;
	char origin[] = "P0";
#ifdef RF24_P1_LETTER
	origin[0] = RF24_P1_LETTER;
#endif
	uint8_t count = 0;
	uint8_t rx = 0;
	while (radio.available(&pipe_num)) {
		count++;
		Serial.print("Got RF24...");
		if(pipe_num>5){
			Serial.println(pipe_num);
			continue;
		}
		rx_per_pipe[pipe_num]++;
		if (len = radio.getDynamicPayloadSize()) {
			rx_time[pipe_num][rx_i[pipe_num]]=millis();
			rx_length[pipe_num][rx_i[pipe_num]]=len;
			Serial.print(pipe_num);
			Serial.print("/");
			Serial.println(len);
			rx++;
			origin[1] = '0' + pipe_num;
			client.startOriginFrame(origin, 1); //Routed Origin!
			if (len > 32)
				len = 32;
			radio.read(payload[pipe_num][rx_i[pipe_num]], len);
			for (uint8_t i = 0; i < len; i++) {
				client.addToPayload(payload[pipe_num][rx_i[pipe_num]][i]);
			}
			rx_i[pipe_num]++;
			if(rx_i[pipe_num]>=NR_RX_BUFFER) rx_i[pipe_num]=0;
#ifdef RX_LED
			rx_blink = 1;
#endif

			if (WITH_RF24_CHECKSUM) {
				if (!client.validateChecksum()) {
					client.writeToBuffer();
					rx1_count++;
				} else {
					rx1_error++;
					rx_per_pipe_failed[pipe_num]++;
					Serial.println("CRC failed");
				}
			} else {
				client.writeToBuffer();
				Serial.println();
				rx1_count++;
			}
		} else {
			rx1_error++;
			radio.read(payload, len);
			Serial.println("Zero size");
		}
		if (count > 10)
			break;
	}

	if (count > 10) {
		Serial.println("To many RF24-packages - Restarting RF24!");
		initRF24();
	}

	delay(5); //Calling to often leads to unstable RF24 RX
	total_rx += rx;
	return rx;
}

unsigned long last_tx, tx_time;
uint8_t no_rx_counter;
unsigned long last_sample = -SAMPLINGINT;

String current_IP = "";
uint8_t current_rf24_channel = 128;
uint8_t current_rf24_base[] = { 0, 0, 0, 0 };

void checkTX(void) {
	if ((millis() - last_sample) > SAMPLINGINT) {
		last_sample = millis();
		client.startDataFrame();
		client.addChannelValue(millis());
		client.addChannelValue(rx1_count);
		if (!rx1_count)
			no_rx_counter++;
		else
			no_rx_counter = 0;
		if (no_rx_counter > 10) {
			initRF24();
			no_rx_counter = 0;
		}
		rx1_count = 0;
		client.addChannelValue(rx1_error);
		rx1_error = 0;
		client.addChannelValue(tx_time);
		client.addChannelValue(myBuffer.readPos());
		client.writeToBuffer();

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
		if (current_rf24_channel != RF24_CHANNEL
				|| pipe_0[1] != current_rf24_base[0]
				|| pipe_0[2] != current_rf24_base[1]
				|| pipe_0[3] != current_rf24_base[2]
				|| pipe_0[4] != current_rf24_base[3]) {
			current_rf24_base[0]=pipe_0[1];
			current_rf24_base[1]=pipe_0[2];
			current_rf24_base[2]=pipe_0[3];
			current_rf24_base[3]=pipe_0[4];
			current_rf24_channel=RF24_CHANNEL;
			client.startFrame(BayEOS_Message);
			client.addToPayload("RF24 Config: Channel: 0x");
			client.addToPayload(String(RF24_CHANNEL, HEX));
			client.addToPayload(" ** P0: 0x");
			for (int8_t i = 4; i >= 0; i--) {
				client.addToPayload(String(pipe_0[i], HEX));
			}
			client.addToPayload(" ** P1: 0x");
			for (int8_t i = 4; i >= 0; i--) {
				client.addToPayload(String(pipe_1[i], HEX));
			}
			client.addToPayload(
					"  P2: ...48 ** P3: ...96 ** P4: ...ab ** P5:  ...bf");
			client.writeToBuffer();
		}
	}
	if ((millis() - last_tx) > SENDINT) {
		uint8_t res = 0;
		while (!res && myBuffer.available()) {
			Serial.print("Sending..");
			tx_time = millis();
			res = client.sendMultiFromBuffer(3000);
			if (!res)
				total_tx++;
			else
				total_tx_error++;
			tx_time = millis() - tx_time;
			Serial.print("res=");
			Serial.println(res);
			handleRF24();

		}
#ifdef TX_LED
		tx_blink = res + 1;
#endif;
		last_tx = millis();
	}
}
