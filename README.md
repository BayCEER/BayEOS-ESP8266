# BayEOS-ESP8266
BayEOS-Library for ESP8266 Arduino modules like WEMOS D1.

##Installation
Download the zip file of the library and extract below your Arduino/library folder

##Usage

    //Include a BayEOS Transport Class
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

	//Instance of BayEOS-Client
	BayESP8266 client;    

    void setup(void){
	  Serial.begin(9600);
	  
	  //Connect to Access Point
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
	
	  //BayEOS Gateway Configuration
	  client.setConfig(name,host,port,path,user,pw);
	  Serial.println("Setup OK");
    }

    void loop(void){
      //Construct DataFrame
      //e.g. by reading out a sensor
      client.startDataFrame();
      client.addChannelValue(73.43);
      client.addChannelValue(3.18);
      client.addChannelValue(millis()/1000);
      client.sendPayload();
      //Send a message
      client.sendMessage("Just a message!");
      delay(5000);
    }

