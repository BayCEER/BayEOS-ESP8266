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


void setup(void){
  Serial.begin(9600);
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
  //client._urlencode=1;
  Serial.println("Setup OK");
}

void loop(void){
  //Construct DataFrame
   client.startDataFrame();
   client.addChannelValue(millis()); 
   uint8_t res=client.sendPayload();
   Serial.print("res=");
   Serial.println(res);
   //client.sendMessage("Just a message!");
 ESP.deepSleep(20e6); // 20e6 is 20 seconds
   
}

