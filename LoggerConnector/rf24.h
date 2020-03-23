#include <BaySerialRF24.h>

#define RF24_CE 16
#define RF24_CS 15
RF24 radio(RF24_CE, RF24_CS);
BaySerialRF24 client(radio, 300);

struct LoggerStatus {
	uint8_t status;
	bool status_update;
	uint16_t logging_int;
	char name[20];
	long timeshift;
	unsigned long start;
	unsigned long last_message;
	unsigned long client_time;
	unsigned long client_time_set;
	unsigned long dump_pos;
	unsigned long dump_end;
	unsigned long logger_time;
	unsigned long read_pos;
  unsigned long write_pos;
  unsigned long end_pos;
  unsigned long flash_size;
	uint16_t bat;
	uint16_t bat_warning;
} logger;

void poll(void) {
	if (logger.status <= 32)
		return;
	client.poll(1);
}

bool sendCommand(uint8_t cmd, const uint8_t* arg = NULL,
		uint8_t arg_length = 0) {
	client.startCommand(cmd);
	if (arg_length)
		client.addToPayload(arg, arg_length);
	client.sendPayload();
	if (client.readIntoPayload())
		return false;
	if (client.getPayload(0) != BayEOS_CommandResponse)
		return false;
	if (client.getPayload(1) != cmd)
		return false;
	return true;
}

void handleLogger(void) {
	if (!logger.status){
		return;
	}
	if (logger.status >= 32)
		return;


	if (logger.status == 1 && !radio.available()) {
		delay(1);
		return;
	}

	if (logger.status == 1) {
		logger.status = 2;
		logger.status_update = true;
    digitalWrite(RX_LED, HIGH);

		return;
	}

	if (logger.status == 2) {
		if (!sendCommand(BayEOS_GetBatStatus))
			return;
		memcpy((uint8_t*) &logger.bat, client.getPayload() + 2, 2);
		memcpy((uint8_t*) &logger.bat_warning, client.getPayload() + 4, 2);
		logger.status = 3;
		logger.status_update = true;
		return;
	}

	if (logger.status == 3) {
		if (!sendCommand(BayEOS_BufferInfo))
			return;
    memcpy((uint8_t*) &logger.read_pos, client.getPayload() + 2, 4);
    memcpy((uint8_t*) &logger.write_pos, client.getPayload() + 6, 4);
    memcpy((uint8_t*) &logger.end_pos, client.getPayload() + 10, 4);
    memcpy((uint8_t*) &logger.flash_size, client.getPayload() + 14, 4);
		logger.status = 4;
		logger.status_update = true;
		return;
	}

	if (logger.status == 4) {
		//fetch Name
		if (!sendCommand(BayEOS_GetName))
			return;
		uint8_t name_length = client.getPacketLength() - 2;
		if (name_length > 19)
			name_length = 19;
		memcpy(logger.name, client.getPayload() + 2, name_length);
		logger.name[name_length] = 0;
		//Sampling_int
		if (!sendCommand(BayEOS_GetSamplingInt))
			return;
		memcpy((uint8_t*) &logger.logging_int, client.getPayload() + 2, 2);
		logger.status = 5;
		logger.status_update = true;
		return;
	}


	if (logger.status == 5) {
		//get Time
		if (!sendCommand(BayEOS_GetTime))
			return;
		memcpy((uint8_t*) &logger.logger_time, client.getPayload() + 2, 4);
		logger.timeshift = (logger.client_time
				+ (millis() - logger.client_time_set) / 1000
				- logger.logger_time);
		logger.status = 6;
		logger.status_update = true;
		return;
	}


	if (logger.status == 6) {
		logger.status = 32; //ready
		logger.status_update = true;
		return;

	}
}

void initRadio() {
	Serial.print("Channel:0x");
	Serial.println(target.channel, HEX);
	Serial.print("Pipe:0x");
	uint8_t* p = (uint8_t*) &target.addr;
	for (uint8_t i = 0; i < 8; i++)
		Serial.print(*(p + i), HEX);
	Serial.println();

	radio.begin();
	radio.powerUp();
	radio.setChannel(target.channel);
	radio.setPayloadSize(32);
	radio.enableDynamicPayloads();
	radio.setCRCLength(RF24_CRC_16);
	radio.setDataRate(RF24_250KBPS);
	radio.setPALevel(RF24_PA_HIGH);
	radio.setRetries(15, 15);
	radio.setAutoAck(true);
	radio.openWritingPipe(target.addr);
	radio.openReadingPipe(0, target.addr);
	radio.startListening();
	logger.status = 1;
	logger.name[0] = 0;
	logger.start = millis();
}
