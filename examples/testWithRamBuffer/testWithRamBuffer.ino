#include <BayEOSBufferRAM.h>
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
BayEOSBufferRAM myBuffer(buffer,BUFFER_SIZE);
unsigned long last_data;
unsigned long last_buffered_data;


void setup(void) {
  Serial.begin(9600);
  client.setBuffer(myBuffer);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

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
  Serial.print(myBuffer.readPos());
  Serial.print("\t");
  Serial.print(myBuffer.writePos());
  Serial.print("\t");
  Serial.print(myBuffer.endPos());
  Serial.print("\t");
  Serial.println(myBuffer.available());
  if((i% 6)==0){
      client.sendMultiFromBuffer();
  }

  delay(2000);
}
