#include <ArduinoJson.h>
#include <Base64.h>
StaticJsonDocument<1024> doc;

/*
   Method to send current config
   called at connect and on update
*/
void sendConfig(void) {
  mes = F("{\"event\":\"conf\",\"ssid\":\"");
  mes += cfg.ssid;
  mes += F("\",\"password\":\"");
  mes += cfg.password;
  mes += F("\",\"baud\":");
  mes += cfg.baud;
  mes += F(",\"max_runtime\":");
  mes += cfg.max_runtime;
  mes += "}";
  webSocket.broadcastTXT(mes);
}

void sendTime(void) {
  mes = F("{\"event\":\"time\",\"time\":");
  mes += myRTC.now().get();
  mes += "}";
  webSocket.broadcastTXT(mes);
}

void sendDownload(bool active) {
  mes = F("{\"event\":\"download\",\"active\":");
  mes += (active?"true":"false");
  mes += "}";
  webSocket.broadcastTXT(mes);
}


void sendLogging(void) {
  mes = F("{\"event\":\"logging\",\"logging\":");
  mes += (device.logging ? "true" : "false");
  if (device.logging) {
    mes += F(",\"file\":\"");
    mes += device.logging_file;
    mes += F("\",\"time\":");
    mes += (millis() - device.logging_started) / 1000;
    mes += F(",\"size\":");
    mes += file.position();
    mes += F(",\"runtime\":");
    mes += device.runtime / 1000;
  }
  mes += "}";
  webSocket.broadcastTXT(mes);
}



/*
  main method for sending status updates to the clients
*/
void sendEvent(void) {
  if (device.send_error) {
    mes = F("{\"event\":\"error\",\"text\":\"");
    mes += device.error;
    mes += "\"}";
    webSocket.broadcastTXT(mes);
    device.send_error = false;
    return;
  }
  if (device.send_msg) {
    mes = F("{\"event\":\"msg\",\"text\":\"");
    mes += device.msg;
    mes += "\"}";
    webSocket.broadcastTXT(mes);
    device.send_msg = false;
    return;
  }
  if ((millis() - device.last_bat) > 10000) {
    device.last_bat = millis();
    mes = F("{\"event\":\"bat\",\"value\":");
    mes += 5.7 * analogRead(A0) / 1023;
    mes += "}";
    webSocket.broadcastTXT(mes);
  }

}

/*
   Send content or root folder
*/
void sendSDContent(void) {
  int rootFileCount = 0;
  root = SD.open("/");
  if (!root.isDirectory()) {
    error("SD-Error: Open root failed");
    return;
  }
  root.rewindDirectory();
  mes = F("{\"event\":\"SDContent\",\"files\":[");
  while (file = root.openNextFile()) {
    if (! file.isDirectory()) {
      if (rootFileCount) mes += ",";
      mes += "{\"n\":\"";
      mes += file.name();
      mes += "\",\"s\":";
      mes += file.size();
      mes += "}";
      rootFileCount++;
    }

    file.close();
    if (rootFileCount == 50) {
      error(String(F("There are many files in the folder. Please delete some of them.")));
    }
  }
  mes += F("]}");
  root.close();
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
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);
      }
      device.last_bat -= 10000;
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      json_error = deserializeJson(doc, payload);
      if (json_error) {
        mes = F("deserializeJson() failed: ");
        mes += json_error.c_str();
        error(mes);
        return;
      }
      command = doc["command"];
      Serial.println(command);

      if (strcmp(command, "setConf") == 0) {
        //Got a message with new config values!
        strncpy(cfg.ssid, doc["ssid"], 19);
        cfg.ssid[19] = 0;
        if (strlen(doc["ssid"]) > 19) {
          error(String(F("SSID to long! Truncated")));
        }
        strncpy(cfg.password, doc["password"], 19);
        cfg.password[19] = 0;
        cfg.baud = doc["baud"];
        cfg.max_runtime = doc["max_runtime"];
        saveConfig(); //save to EEPROM - defined in config.h
        message(String(F("new config saved to EEPROM")));
        sendConfig(); //send the current config to client
        return;
      }
      if (strcmp(command, "getConf") == 0) {
        sendConfig(); //send the current config to client
        return;
      }

      if (strcmp(command, "setTime") == 0) {
        unsigned long time = doc["value"];
        myRTC.adjust(time);
        sendTime();
        return;
      }
      if (strcmp(command, "getTime") == 0) {
        sendTime();
        return;
      }
      if (strcmp(command, "delFile") == 0) {
        if (startSD()) return;
        strncpy(char_buffer, doc["file"], 49);
        SD.remove(char_buffer);
        sendSDContent();
        return;
      }
      if (strcmp(command, "getDir") == 0) {
        if (startSD()) return;
        sendSDContent();
        return;
      }
      if (strcmp(command, "getAll") == 0) {
        sendTime();
        delay(2);
        sendLogging();
        delay(2);
        sendConfig();
        delay(2);
        if (startSD()) return;
        sendSDContent();
        return;
      }
      if (strcmp(command, "startLogging") == 0) {
        char f[13];
        strncpy(f, doc["file"], 12);
        device.runtime = doc["runtime"];
        device.runtime *= 1000;
        startLogging(f);
        return;
      }

      if (strcmp(command, "stopLogging") == 0) {
        stopLogging();
        return;
      }
  }
}
