#include <EEPROM.h>
#include <WString.h>
// Types 'byte' und 'word' doesn't work!
struct MyCONFIG
{
  char name[20];
  uint8_t uint8_val;
  long long_val;
  float float_val;
} cfg;

struct DeviceStatus
{
  bool send_error;
  char error[50];
  bool send_msg;
  char msg[50];
} device;

void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  cfg.name[0]=0;   
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
