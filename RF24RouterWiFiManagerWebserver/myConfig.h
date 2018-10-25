#ifndef myConfig_h
#define myConfig_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  uint8_t valid;// 1=valid configuration --  everything else = invalid
  //define your default values here, if there are different values in bayeos.json, they are overwritten.
  char bayeos_gateway[40];
  char bayeos_name[40];
  char bayeos_user[40];
  char bayeos_pw[40];
  long rf24_base=0x45c431ae;
  uint8_t rf24_channel=0x7e;
  bool rf24_checksum;
  uint8_t disable_leds=0;
} configData_t;

configData_t cfg;

void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  cfg.valid=0;
  strcpy(cfg.bayeos_name,"MyRouter");
  strcpy(cfg.bayeos_gateway,"");
  strcpy(cfg.bayeos_user,"import");
  strcpy(cfg.bayeos_pw,"import");
  cfg.rf24_base=0x45c431ae;
  cfg.rf24_channel=0x7e;
  cfg.rf24_checksum=1;
  cfg.disable_leds=0;
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void saveConfig() {
  // Save configuration from RAM into EEPROM
  cfg.valid = 1;
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
  if (cfg.valid != 1) {
    eraseConfig();
    EEPROM.get( 0, cfg );
  }
  if(cfg.disable_leds>1) cfg.disable_leds=0;
}

#endif

