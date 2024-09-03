#include <EEPROM.h>
#include <WString.h>
// Types 'byte' und 'word' doesn't work!
struct Config
{
  long sig = 0xba1e0e24;
  long baud;
  char ssid[20];
  char password[20];
}
cfg;



void saveConfig() {
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
  if (cfg.sig != 0xba1e0e24) {
    cfg.sig = 0xba1e0e24;
    cfg.baud = baud;
    strcpy(cfg.ssid, ssid);
    strcpy(cfg.password, password);
    saveConfig();
  }
}
