#include <ArduinoJson.h>
#include <Base64.h>
StaticJsonDocument<1024> doc;
String mes;

/*
   Method to send current config
   called at connect and on update
*/
void sendConfig(void) {
  mes = F("{\"event\":\"config\",\"name\":\"");
  mes += cfg.name;
  mes += F("\",\"uint8_val\":");
  mes += cfg.uint8_val;
  mes += F(",\"long_val\":");
  mes += cfg.long_val;
  mes += F(",\"float_val\":");
  mes += cfg.float_val; //note you may have to change the precision!
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
      Serial.println(command);

      if (strcmp(command, "save") == 0) {
        //Got a message with new config values!
        strncpy(cfg.name, doc["name"], 19);
        cfg.name[19] = 0;
        if (strlen(doc["name"]) > 19) {
          device.send_error = true;
          mes = F("Name to long! Truncated");
          mes.toCharArray(device.error, 50);
        }
        cfg.uint8_val = doc["uint8_val"];
        cfg.long_val = doc["long_val"];
        cfg.float_val = doc["float_val"];
        saveConfig(); //save to EEPROM - defined in config.h
        device.send_msg = true;
        mes = F("new config saved to EEPROM");
        mes.toCharArray(device.msg, 50);
        sendConfig(); //send the current config to client
        return;
      }
      //TODO: Add further commands
  }
}
