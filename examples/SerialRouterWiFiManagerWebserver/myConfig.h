#ifndef myConfig_h
#define myConfig_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  long valid;// 0x532178fe = valid configuration --  everything else = invalid
  //define your default values here, if there are different values in bayeos.json, they are overwritten.
  char bayeos_gateway[40];
  char bayeos_name[40];
  char bayeos_path[40];
  char bayeos_user[40];
  char bayeos_pw[40];
  char ip[17];
} configData_t;

configData_t cfg;


void saveConfig() {
  // Save configuration from RAM into EEPROM
  cfg.valid = 0x532178fe;
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
  if (cfg.valid != 0x532178fe) {
    strcpy(cfg.bayeos_name, "MyRouter");
    strcpy(cfg.bayeos_gateway, "");
    strcpy(cfg.bayeos_path, "gateway/frame/saveFlat");
    strcpy(cfg.bayeos_user, "import");
    strcpy(cfg.bayeos_pw, "import");
    cfg.ip[0] = 0;
    saveConfig();;
  }
}

#endif
