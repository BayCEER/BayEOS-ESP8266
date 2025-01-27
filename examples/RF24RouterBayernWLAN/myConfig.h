#ifndef myConfig_h
#define myConfig_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  unsigned long sig;  
  //define your default values here, if there are different values in bayeos.json, they are overwritten.
  char bayeos_gateway[40];
  char bayeos_name[40];
  char bayeos_path[40];
  char bayeos_user[40];
  char bayeos_pw[40];
  long rf24_base = 0x45c431ae;
  uint8_t rf24_channel = 0x7e;
  bool rf24_checksum;
  uint8_t disable_leds = 0;
} configData_t;

configData_t cfg;

void saveConfig() {
  // Save configuration from RAM into EEPROM
  cfg.sig = valid_config;
  EEPROM.put(0, cfg);
  delay(200);
  EEPROM.commit();  // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get(0, cfg);
  if (cfg.sig != valid_config) {
    strcpy(cfg.bayeos_name, bayeos_origin);
    strcpy(cfg.bayeos_gateway, bayeos_gateway);
    strcpy(cfg.bayeos_path, "gateway/frame/saveFlat");
    strcpy(cfg.bayeos_user, bayeos_user);
    strcpy(cfg.bayeos_pw, bayeos_pw);
    cfg.rf24_base = rf24_base;
    cfg.rf24_channel = rf24_channel;
    cfg.rf24_checksum = rf24_checksum;
    cfg.disable_leds = disable_leds;
    saveConfig();
  }
  if (cfg.disable_leds > 1) cfg.disable_leds = 0;
}


#define BUFFER_DATA_OFFSET 1024
typedef struct {
  unsigned long read_pos;
  unsigned long write_pos;
  unsigned long end_pos;
  unsigned long time;
} bufferData_t;
bufferData_t buffer_data;

void saveBufferData() {
  EEPROM.put(BUFFER_DATA_OFFSET, buffer_data);
  delay(200);
  EEPROM.commit();
}

void loadBufferData() {
  EEPROM.get(BUFFER_DATA_OFFSET, buffer_data);
}


#endif
