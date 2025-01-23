#ifndef myConfig_h
#define myConfig_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  long valid;// valid==valid_config --  everything else = invalid
  //define your default values here, if there are different values in bayeos.json, they are overwritten.
  char bayeos_gateway[40];
  char bayeos_name[40];
  char bayeos_path[40];
  char bayeos_user[40];
  char bayeos_pw[40];
} configData_t;

configData_t cfg;

void saveConfig() {
  // Save configuration from RAM into EEPROM
  cfg.valid = valid_config;
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
  if (cfg.valid != valid_config) {
    strcpy(cfg.bayeos_name, bayeos_origin);
    strcpy(cfg.bayeos_gateway, bayeos_gateway);
    strcpy(cfg.bayeos_path, "gateway/frame/saveFlat");
    strcpy(cfg.bayeos_user, bayeos_user);
    strcpy(cfg.bayeos_pw, bayeos_pw);
    saveConfig();
  }
}

#endif
