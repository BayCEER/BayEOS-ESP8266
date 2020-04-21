#include <EEPROM.h>
#include <WString.h>
// Types 'byte' und 'word' doesn't work!
String mes; //global String!
uint8_t sd_buffer[512];
char char_buffer[50]; //global Buffer for chars

struct MyCONFIG
{
  char ssid[20];
  char password[20];
  long baud;
  long max_runtime;
} cfg;

struct DeviceStatus
{
  bool send_error;
  char error[50];
  bool send_msg;
  char msg[50];
  uint8_t logging; //0 off, 1: waiting for \n; 2: logging: 3: waiting for \n 
  char logging_file[13];
  unsigned long runtime; //in ms - we convert on message
  unsigned long logging_started;
  unsigned long last_logging_update;
  unsigned long last_bat;
} device;

void error(const String& s){
  s.toCharArray(device.error, 50);
  device.send_error=true;
}
void message(const String& s){
  s.toCharArray(device.msg, 50);
  device.send_msg=true;
}

void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  strcpy(cfg.ssid,"serial2SD");  
  strcpy(cfg.password,"bayeos24");  
  cfg.baud=38400;
  cfg.max_runtime=12*3600; 
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
