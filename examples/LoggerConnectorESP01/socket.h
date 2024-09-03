#include <ArduinoJson.h>
#include <Base64.h>
StaticJsonDocument<1024> doc;
String mes;

//Function to send available connections
void sendConfig() {
  mes = F("{\"event\":\"getConf\",\"version\":\"");
  mes += SOFTWARE_VERSION;
  mes += F("\",\"baud\":");
  mes += cfg.baud;
  mes += F(",\"ssid\":\"");
  mes += cfg.ssid;
  mes += F("\",\"password\":\"");
  mes += cfg.password;
  mes += "\"}";
  // send message to client
  webSocket.broadcastTXT(mes);
}


void sendEvent(void) {

  
  
  //Wait for Connect
  if (logger.status == 1) {
    if ((millis() - logger.last_message) < 1000)
      return;
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

    if (!client.available())
      return;
    if (client.readIntoPayload())
      return;
    char tmp[150];
    base64_encode(tmp, (char*) client.getPayload(),
                  client.getPacketLength());
    tmp[base64_enc_len(client.getPacketLength())] = 0;
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
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        logger.status=1;
        sendConfig();
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
        client.sendTXBreak();
        while (!sendCommand(BayEOS_ModeStop)) {
          delay(200);
          client.sendTXBreak();
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

 

      if (strcmp(command, "saveConf") == 0) {
        strncpy(cfg.ssid, doc["ssid"], 19);
        cfg.ssid[19] = 0;
        strncpy(cfg.password, doc["password"], 19);
        cfg.password[19] = 0;
        cfg.baud=doc["baud"];
        saveConfig();
        sendConfig();
        return;
      }

      break;
  }

}
