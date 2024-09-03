#include <EEPROM.h>
#include <WString.h>
// Types 'byte' und 'word' doesn't work!
#define MAX_RF24 50
struct RadioTarget
{
  uint8_t channel;
  uint64_t addr;
  char name[20];
};
RadioTarget cfg[MAX_RF24];
RadioTarget target;


void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  for(uint8_t i=0;i<MAX_RF24;i++){
    cfg[i].name[0]=0;   
  }
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void saveConfig() {
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
}
