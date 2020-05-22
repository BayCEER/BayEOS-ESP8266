#include <BaySerialRF24.h>

#define RF24_CE 16
#define RF24_CS 15
RF24 radio(RF24_CE, RF24_CS);
BaySerialRF24 client_rf24(radio, 300);

#include <BaySerial.h>
BaySerial client_serial(Serial);

BayEOS* client;

struct LoggerStatus {
  uint8_t status;
  bool status_update;
  uint16_t logging_int;
  char name[40];
  char file_name[40];
  uint8_t version_major;
  uint8_t version_minor;
  long timeshift;
  unsigned long start;
  unsigned long last_message;
  unsigned long client_time;
  unsigned long client_time_set;
  unsigned long dump_pos;
  unsigned long dump_end;
  unsigned long dump_offset;
  unsigned long logger_time;
  unsigned long read_pos;
  unsigned long write_pos;
  unsigned long end_pos;
  unsigned long flash_size;
  int8_t framesize;
  uint16_t bat;
  uint16_t bat_warning;
  uint16_t tx_error_count;
  char channel_map[101];
  char unit_map[101];
  bool logging_disabled;
  bool serial;
} logger;



void poll(void) {
  if (logger.status <= 32 || logger.serial)
    return;
  client_rf24.poll(1);
}

bool sendCommand(uint8_t cmd, const uint8_t* arg = NULL,
                 uint8_t arg_length = 0) {
  //Lost connection!
  if (logger.tx_error_count > 3) {
    logger.status = 1;
    logger.start = millis();
    logger.status_update = true;
    return false;
  }

  client->startCommand(cmd);
  if (arg_length)
    client->addToPayload(arg, arg_length);
  if (client->sendPayload()) {
    logger.tx_error_count++;
    logger.status_update = true;
    return false;
  } else
    logger.tx_error_count = 0;
  if (client->readIntoPayload()) {
    logger.tx_error_count++;
    logger.status_update = true;
    return false;
  }

  if (client->getPayload(0) != BayEOS_CommandResponse)
    return false;
  if (client->getPayload(1) != cmd)
    return false;
  return true;
}

void handleLogger(void) {
  if (!logger.status) {
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
    digitalWrite(LED_RED, HIGH);
    logger.tx_error_count = 0;
    return;
  }


  if (logger.status == 2) { //Version, Bat + Map
    if (! sendCommand(BayEOS_ModeStop))
      return;
    if (!sendCommand(BayEOS_GetVersion))
      return;
    char* p = (char*) (client->getPayload() + 2);
    logger.version_major = strtol(p, &p, 10);
    p++;
    logger.version_minor = atoi(p);

    if (!sendCommand(BayEOS_GetBatStatus))
      return;
    memcpy((uint8_t*) &logger.bat, client->getPayload() + 2, 2);
    memcpy((uint8_t*) &logger.bat_warning, client->getPayload() + 4, 2);

    if ((1000 * logger.version_major + logger.version_minor) > 1005) {
      if (!sendCommand(BayEOS_GetChannelMap))
        return;
      uint8_t map_length = client->getPacketLength() - 2;
      memcpy(logger.channel_map, client->getPayload() + 2, map_length);
      logger.channel_map[map_length] = 0;
      if (!sendCommand(BayEOS_GetUnitMap))
        return;
      map_length = client->getPacketLength() - 2;
      memcpy(logger.unit_map, client->getPayload() + 2, map_length);
      logger.unit_map[map_length] = 0;

    } else {
      logger.channel_map[0] = 0;
      logger.unit_map[0] = 0;
    }
    logger.status = 3;
    logger.status_update = true;
    return;
  }

  if (logger.status == 3) {
    if (!sendCommand(BayEOS_BufferInfo))
      return;
    memcpy((uint8_t*) &logger.read_pos, client->getPayload() + 2, 4);
    memcpy((uint8_t*) &logger.write_pos, client->getPayload() + 6, 4);
    memcpy((uint8_t*) &logger.end_pos, client->getPayload() + 10, 4);
    memcpy((uint8_t*) &logger.flash_size, client->getPayload() + 14, 4);
    if (client->getPacketLength() > 18) {
      memcpy((uint8_t*) &logger.framesize, client->getPayload() + 18, 1);
      memcpy((uint8_t*) &logger.logging_int, client->getPayload() + 19, 2);
    } else {
      logger.framesize=0;
    }
    logger.status = 4;
    logger.status_update = true;
    return;
  }

  if (logger.status == 4) {
    //fetch Name
    if (!sendCommand(BayEOS_GetName))
      return;
    uint8_t name_length = client->getPacketLength() - 2;
    if (name_length > 39)
      name_length = 39;
    memcpy(logger.name, client->getPayload() + 2, name_length);
    logger.name[name_length] = 0;
    memcpy(logger.file_name, logger.name, 40);
    for (int i = 0; i < 40; i++) {
      if (logger.file_name[i] == ' ') logger.file_name[i] = '_';
    }
    //Sampling_int
    if (!sendCommand(BayEOS_GetSamplingInt))
      return;
    memcpy((uint8_t*) &logger.logging_int, client->getPayload() + 2, 2);
    logger.status = 5;
    logger.status_update = true;
    return;
  }


  if (logger.status == 5) {
    //get Time
    if (!sendCommand(BayEOS_GetTime))
      return;
    memcpy((uint8_t*) &logger.logger_time, client->getPayload() + 2, 4);
    logger.timeshift = (logger.client_time
                        + (millis() - logger.client_time_set) / 1000
                        - logger.logger_time);
    logger.status = 6;
    logger.status_update = true;
    return;
  }

  if (logger.status == 6) {
    if (logger.version_major == 1 && + logger.version_minor <= 5) {
      logger.status = 16;
      return;
    }
    if (!sendCommand(BayEOS_GetLoggingDisabled))
      return;
    logger.logging_disabled = client->getPayload(2);
    logger.status = 7;
    logger.status_update = true;
    return;
  }

  if (logger.status == 7) {
    logger.status = 16;
    return;
  }

  if (logger.status == 16) {
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
  logger.status = 1;
  logger.name[0] = 0;
  logger.start = millis();
  //Send Test-Byte to see if logger is already listening!
  uint8_t test_byte[] = {XOFF};
  radio.stopListening();
  if (radio.write(test_byte, 1)) {
    logger.status = 2;
    logger.status_update = true;
  }
  radio.startListening();
  client=& client_rf24;
  logger.serial=false;
}

void initSerial(unsigned long baud){
  client_serial.begin(baud);
  client=& client_serial;
  logger.status=2;
  logger.serial=true;
 
}
