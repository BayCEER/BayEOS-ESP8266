#include <ArduinoJson.h>
#include <Base64.h>
StaticJsonDocument<1024> doc;
String mes;

//Helper function for formating uint8_t values to HEX
void toHex(uint8_t v, char* hex) {
  hex[2] = 0;
  if (v < 16) {
    hex[0] = '0';
    itoa(v, hex + 1, 16);
  } else
    itoa(v, hex, 16);
}

//Function to send available connections
void sendConnections() {
  mes = F("{\"event\":\"getCon\",\"version\":\"");
  mes += SOFTWARE_VERSION;
  mes += F("\",\"connections\":[");
  uint8_t i = 0;
  char hex[3];
  hex[2] = 0;
  while (i < MAX_RF24) {
    if (!cfg[i].name[0])
      break;
    if (i)
      mes += ",";
    mes += "{\"name\":\"";
    mes += cfg[i].name;
    mes += "\",\"channel\":\"0x";
    toHex(cfg[i].channel, hex);
    mes += hex;
    mes += "\",\"pipe\":\"0x";
    for (uint8_t j = 0; j < 5; j++) {
      uint8_t v;
      memcpy(&v, (uint8_t*) &cfg[i].addr + (4 - j), 1);
      toHex(v, hex);
      mes += hex;
    }
    mes += "\"}";
    i++;
  }
  mes += "],\"serial\":";
#if WITH_SERIAL
  mes +="true";
#else
  mes +="false";
#endif
  mes += "}";
  // send message to client
  webSocket.broadcastTXT(mes);

}

/*
  main method for sending status updates to the clients
*/
unsigned long last_bat;

void sendEvent(void) {
#if WITH_BAT
  if ((millis() - last_bat) > 10000) {
    last_bat = millis();
    mes = F("{\"event\":\"bat\",\"value\":");
    mes += 5.7 * analogRead(A0) / 1023;
    mes += "}";
    webSocket.broadcastTXT(mes);
  }
#endif
  //Wait for Connect
  if (logger.status == 1) {
    if ((millis() - logger.last_message) < 1000)
      return;
    digitalWrite(LED_RED, !digitalRead(LED_RED));
    mes = F("{\"event\":\"wait\",\"seconds\":");
    mes += (millis() - logger.start) / 1000;
    mes += F(",\"tx_error\":");
    mes += logger.tx_error_count;
    mes += F(",\"rx_error\":");
    mes += logger.rx_error_count;
    mes += "}";
    webSocket.broadcastTXT(mes);
    logger.last_message = millis();
    return;
  }

  //Live Mode
  if (logger.status == 64) {
    if (logger.status_update) {
      logger.status_update = false;
      mes = F("{\"event\":\"liveMode\"}");
      webSocket.broadcastTXT(mes);
      return;
    }

    if (!client->available())
      return;
    if (client->readIntoPayload())
      return;
    char tmp[150];
    base64_encode(tmp, (char*) client->getPayload(),
                  client->getPacketLength());
    tmp[base64_enc_len(client->getPacketLength())] = 0;
    mes = F("{\"event\":\"data\",\"data\":\"");
    mes += tmp;
    mes += "\"}";
    webSocket.broadcastTXT(mes);
    return;
  }

  //No Update for Clients
  if (!logger.status_update)
    return;

  logger.status_update = false;

  if (logger.status == 0) {
    mes = F("{\"event\":\"disconnected\"}");
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 2) {
    mes = F("{\"event\":\"connected\",\"tx_error\":");
    mes += logger.tx_error_count;
    mes += F(",\"rx_error\":");
    mes += logger.rx_error_count;
    mes += "}";
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 3) {
    mes = F("{\"event\":\"metadata\",\"bat\":");
    mes += logger.bat;
    mes += F(",\"bat_warning\":");
    mes += logger.bat_warning;
    mes += F(",\"version\":\"");
    mes += logger.version_major;
    mes += '.';
    mes += logger.version_minor;
    mes += F("\",\"channel\":\"");
    mes += logger.channel_map;
    mes += F("\",\"unit\":\"");
    mes += logger.unit_map;
    mes += "\"}";
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 4) {
    mes = F("{\"event\":\"buffer\",\"read\":");
    mes += logger.read_pos;
    mes += ",\"write\":";
    mes += logger.write_pos;
    mes += ",\"end\":";
    mes += logger.end_pos;
    mes += ",\"size\":";
    mes += logger.flash_size;
    mes += ",\"framesize\":";
    mes += logger.framesize;
    mes += ",\"loggingint\":";
    mes += logger.logging_int;
    mes += "}";
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 5) {
    mes = F("{\"event\":\"nameInt\",\"name\":\"");
    mes += logger.name;
    mes += "\",\"int\":";
    mes += logger.logging_int;
    mes += "}";
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 6) {
    mes = F("{\"event\":\"time\",\"value\":");
    mes += logger.timeshift;
    mes += "}";
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 7) {
    mes = F("{\"event\":\"logging_disabled\",\"value\":");
    mes += logger.logging_disabled;
    mes += "}";
    webSocket.broadcastTXT(mes);
    return;
  }

  if (logger.status == 32) {
    mes = F("{\"event\":\"ready\",\"tx_error\":");
    mes += logger.tx_error_count;
    mes += "}";
    webSocket.broadcastTXT(mes);
    return;
  }

}



//WebsocketEvent
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload,
                    size_t length) {
  DeserializationError error;
  const char* command;
  uint8_t i = 0;
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);
        last_bat -= 10000;
        sendConnections();
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }
      command = doc["command"];

      if (strcmp(command, "time") == 0) {
        logger.client_time = doc["value"];
        logger.client_time_set = millis();
        return;
      }
      //make sure to send a status update
      logger.status_update = true;

      if (strcmp(command, "delete") == 0) {
        uint8_t arg = 1;
        if (! sendCommand(BayEOS_BufferCommand, &arg, 1)) return;
        logger.status = 3; //buffer read
        return;
      }

      if (strcmp(command, "save") == 0) {
        const char* name = doc["name"];
        if (! sendCommand(BayEOS_SetName, (const uint8_t*) name, strlen(name))) return;
        uint16_t logging_int = doc["logging_int"];
        if (! sendCommand(BayEOS_SetSamplingInt, (uint8_t*) &logging_int, 2)) return;
        logger.status = 4; //getName
        return;
      }

      if (strcmp(command, "sync_time") == 0) {
        logger.client_time = doc["value"];
        logger.client_time_set = millis();
        if (! sendCommand(BayEOS_SetTime, (uint8_t*) &logger.client_time, 4)) return;
        logger.status = 5; //getTime
        return;
      }

      if (strcmp(command, "logging_disabled") == 0) {
        bool logging_disabled = doc["value"];
        if (! sendCommand(BayEOS_SetLoggingDisabled, (uint8_t*) &logging_disabled, 1)) return;
        logger.status = 6; //getLoggingDisabled
        return;
      }

      if (strcmp(command, "modeStop") == 0) {
        if (logger.serial) client_serial.sendTXBreak();
        else client_rf24.sendTXBreak();
        while (!sendCommand(BayEOS_ModeStop)) {
          delay(200);
          if (logger.serial) client_serial.sendTXBreak();
          else client_rf24.sendTXBreak();
          if (logger.status == 1) return;
        }
        logger.status = 32;
        return;
      }

      if (strcmp(command, "modeLive") == 0) {
        if (sendCommand (BayEOS_StartLiveData)) {
          logger.status = 64;
        }
        return;
      }

      if (strcmp(command, "connect") == 0) {
        logger.tx_error_count = 0;
        digitalWrite(LED_RED, LOW);
        if (!strlen(doc["name"])) {
          radio.powerDown();
          logger.status = 0;
          return;
        }
        i = 0;
        while (i < MAX_RF24) {
          if (strcmp(cfg[i].name, doc["name"]) == 0)
            break;
          i++;
        }
        if (i == MAX_RF24) {
          radio.powerDown();
          logger.status = 0;
          unsigned long baud = atoi(doc["name"]);
          if (baud) initSerial(baud);
          return;
        }
        memcpy_P(&target, cfg + i, sizeof(RadioTarget));
        initRadio();
        return;
      }

      if (strcmp(command, "addCon") == 0) {
        i = 0;
        while (i < MAX_RF24) {
          if (!cfg[i].name[0])
            break;
          if (strcmp(cfg[i].name, doc["name"]) == 0)
            break;
          i++;
        }
        if (i == MAX_RF24) {
          return;
        }
        strncpy(cfg[i].name, doc["name"], 19);
        cfg[i].name[19] = 0;
        char hex[3];
        hex[2] = 0;
        const char* pipe = doc["pipe"];
        uint8_t* addr_p = (uint8_t*) &cfg[i].addr;
        for (uint8_t j = 0; j < 5; j++) {
          memcpy(hex, pipe + (5 - j) * 2, 2);
          uint8_t addr_b = strtol(hex, NULL, 16);
          *(addr_p + j) = addr_b;
        }
        uint8_t channel = doc["channel"];
        cfg[i].channel = channel;
        saveConfig();
        sendConnections();
        return;
      }

      if (strcmp(command, "delCon") == 0) {
        uint8_t id = doc["id"];
        while (id < MAX_RF24) {
          if (!cfg[id].name[0])
            break;
          cfg[id].name[0] = 0;
          if (id < MAX_RF24 && cfg[id + 1].name[0]) {
            //copy config
            memcpy(cfg[id].name, cfg[id + 1].name, 20);
            cfg[id].addr = cfg[id + 1].addr;
            cfg[id].channel = cfg[id + 1].channel;
          }
          id++;
        }
        saveConfig();
        sendConnections();
        return;
      }

      break;
  }

}
