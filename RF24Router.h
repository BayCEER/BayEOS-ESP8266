#include <RF24.h>

uint16_t rx_ok, rx1_count, rx1_error;
RF24 radio(D0, D8);
boolean radio_is_up = 0;

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
	radio.setChannel(NRF24_CHANNEL);
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

uint8_t handleRF24(void) {
	if (!radio_is_up)
		return 0;
	uint8_t pipe_num, len;
	uint8_t payload[32];
	char origin[] = "P0";
#ifdef RF24_P1_LETTER
	origin[0] = RF24_P1_LETTER;
#endif
	uint8_t count = 0;
	uint8_t rx = 0;
	while (radio.available(&pipe_num)) {
		Serial.print("Got RF24...");
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
#ifdef RX_LED
			rx_blink = 1;
#endif

#if WITH_RF24_CHECKSUM
			if (! client.validateChecksum()) {
				client.writeToBuffer();
				Serial.print(pipe_num);
				Serial.print("/");
				Serial.println(len);
				rx1_count++;
			} else {
				rx1_error++;
				Serial.println("CRC failed");
			}
#else
			client.writeToBuffer();
			Serial.println();
			rx1_count++;
#endif
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
	return rx;
}

unsigned long last_tx, tx_time;
uint8_t no_rx_counter;

void checkTX(void) {
	if ((last_tx - millis()) < SENDINT)
		return;
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

	uint8_t res = 0;
	tx_time = millis();
	while (!res && myBuffer.available()) {
		Serial.print("Sending..");
		res = client.sendMultiFromBuffer(3000);
		Serial.print("res=");
		Serial.println(res);
		handleRF24();
	}
#ifdef TX_LED
	tx_blink = res + 1;
#endif;
	tx_time = millis() - tx_time;
	last_tx = millis();
}
