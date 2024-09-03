#include <BayEOSBufferSPIFFS.h>
#include <BayEOS-ESP8266.h>


//WIFI Configuration
const char* ssid     = "@BayernWLAN";      // SSID
const char* password = "";      // Password

//BayEOS Configuration
const char* name="ESP8266-Test";
const char* host="132.180.112.55";
const char* port="80";
const char* path="gateway/frame/saveFlat";
const char* user="import";
const char* pw="import";


BayESP8266 client;
#define BUFFER_SIZE 10000
uint8_t buffer[BUFFER_SIZE];
BayEOSBufferSPIFFS2 myBuffer(BUFFER_SIZE);

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.println("INIT Buffer");
  SPIFFS.begin();
  delay(1000);
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  Serial.printf("Bytes: %d - Used: %d - BlockSize: %d - pageSize: %d - maxOpenFiles: %d - maxPathLength: %d\n",
  fs_info.totalBytes,fs_info.usedBytes,fs_info.blockSize,fs_info.pageSize,fs_info.maxOpenFiles,fs_info.maxPathLength);
  //SPIFFS.remove("bayeos.db2");
  myBuffer.init();
  delay(1000);
  
  client.setBuffer(myBuffer);
  //Gateway Configuration
  client.setConfig(name,host,port,path,user,pw);
  Serial.println("Setup OK");
}

int i;
void loop(void) {

  //Construct DataFrame
  client.startDataFrame(BayEOS_Float32le);
  client.addChannelValue(i);
  client.addChannelValue(millis() / 1000);
  if ((i % 2) == 0) client.addChannelValue(millis() / 1000);
  if ((i % 3) == 0) client.addChannelValue(millis() / 1000);
  if ((i % 4) == 0) client.addChannelValue(millis() / 1000);
  client.writeToBuffer();
  i++;
  Serial.print(i);
  Serial.print(". ");
  Serial.print(myBuffer.readPos());
  Serial.print("\t");
  Serial.print(myBuffer.writePos());
  Serial.print("\t");
  Serial.print(myBuffer.endPos());
  Serial.print("\t");
  Serial.println(myBuffer.available());


  delay(2000);
}
