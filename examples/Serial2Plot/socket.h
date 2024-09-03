#include <ArduinoJson.h>
#include <Base64.h>
StaticJsonDocument<1024> doc;
String mes; //global String!
/*
   Method to send current config
   called at connect and on update
*/
void sendConfig(void) {
  mes = F("{\"event\":\"conf\",\"ssid\":\"");
  mes += cfg.ssid;
  mes += F("\",\"password\":\"");
  mes += cfg.password;
  mes += F("\",\"version\":\"");
  mes += SW_VERSION;
  mes += F("\",\"baud\":");
  mes += cfg.baud;
  mes += F(",\"max_time\":");
  mes += cfg.max_time;
  for (uint8_t i = 0; i < 9; i++) {
    mes += F(",\"ch");
    mes += (i + 1);
    mes += F("\":\"");
    mes += cfg.channels[i];
    mes += '"';
  }
  mes += "}";
  webSocket.broadcastTXT(mes);
}


void sendData(void) {
  mes = F("{\"event\":\"data\"");
  for (uint8_t i = 0; i < 9; i++) {
    if (cfg.channels[i][0]) {
      mes += F(",\"ch");
      mes += (i + 1);
      mes += F("\":");
      mes += data[i];
    }
  }
  mes += "}";
  webSocket.broadcastTXT(mes);
}


void sendMessage(const String &s) {
  mes = F("{\"event\":\"msg\",\"text\":\"");
  mes += s;
  mes += F("\"}");
  webSocket.broadcastTXT(mes);
}

void sendError(const String &s) {
  mes = F("{\"event\":\"error\",\"text\":\"");
  mes += s;
  mes += F("\"}");
  webSocket.broadcastTXT(mes);
}



//WebsocketEvent
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload,
                    size_t length) {
  DeserializationError json_error;
  const char* command;
  uint8_t i = 0;
  switch (type) {
    case WStype_DISCONNECTED:
      // Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
        //              ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      // Serial.printf("[%u] get Text: %s\n", num, payload);
      json_error = deserializeJson(doc, payload);
      if (json_error) {
        mes = F("deserializeJson() failed: ");
        mes += json_error.c_str();
        sendError(mes);
        return;
      }
      command = doc["command"];
      // Serial.println(command);

      if (strcmp(command, "setConf") == 0) {
        //Got a message with new config values!
        strncpy(cfg.ssid, doc["ssid"], 19);
        if (strlen(doc["ssid"]) > 19) {
          sendError(String(F("SSID to long! Truncated")));
        }
        strncpy(cfg.password, doc["password"], 19);
        cfg.password[19] = 0;
        cfg.baud = doc["baud"];
        cfg.max_time = doc["max_time"];
        char key[] = "ch1";
        for (uint8_t i = 0; i < 9; i++) {
          key[2] = ('1' + i);
          strncpy(cfg.channels[i], doc[key], 19);
          if (strlen(doc[key]) > 19) {
            sendError(String(F("Channel to long! Truncated")));
          }
          cfg.channels[i][19]=0;
        }
        saveConfig(); //save to EEPROM - defined in config.h
        sendMessage(String(F("new config saved to EEPROM")));
        sendConfig(); //send the current config to client
        return;
      }
      if (strcmp(command, "getConf") == 0) {
        sendConfig(); //send the current config to client
        return;
      }

      if (strcmp(command, "getAll") == 0) {
        sendConfig();
        delay(2);
        return;
      }

  }
}
