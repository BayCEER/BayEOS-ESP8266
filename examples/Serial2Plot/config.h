#include <EEPROM.h>
#include <WString.h>



struct MyCONFIG
{
  long sig;
  char ssid[20];
  char password[20];
  long baud;
  int max_time;
  char channels[9][20];
} cfg;

#define CONFIG_SIG 0xfab1ab25

void eraseConfig() {
  cfg.sig=CONFIG_SIG;
  strcpy(cfg.ssid, "SerialPlotter");
  strcpy(cfg.password, "bayeos24");
  cfg.baud = 38400;
  cfg.max_time = 60;
  strcpy(cfg.channels[0], "CH1");
  for (uint8_t i = 1; i < 9; i++) {
    cfg.channels[i][0] = 0;
  }
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();
}

void saveConfig() {
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
  if(cfg.sig!=CONFIG_SIG) eraseConfig();
}
