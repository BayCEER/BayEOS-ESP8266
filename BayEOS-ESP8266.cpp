#include "BayEOS-ESP8266.h"
#define ESP8266_DEBUG 0

uint8_t BayESP8266::connect(void) {
	int con=0;
	if(*_server>='0' && *_server<='9') con=WiFiClient::connect(parseIP(_server), atoi(_port));
	else if(*_server) con=WiFiClient::connect(_server, atoi(_port));
	if (! con) {
#if ESP8266_DEBUG
		Serial.println(_server);
		for(uint8_t i=0;i<4;i++) {
			Serial.print(*(parseIP(_server)+i));
			Serial.print(":");
		}
		Serial.println();
#endif
		WiFiClient::stop();
		return (1);
	}
	return (0);
}

void BayESP8266::disconnect(void) {
	WiFiClient::flush();
	WiFiClient::stop();
}


BayESP8266::BayESP8266(void) {
	_urlencode = 1;
}
;

int BayESP8266::available(void) {
	return WiFiClient::available();
}

int BayESP8266::read(void) {
#if ESP8266_DEBUG
	int c=WiFiClient::read();
	if(c!=-1) Serial.write(c);
	return c;
#else
	return WiFiClient::read();
#endif
}


int BayESP8266::i_available(void) {
	return WiFiClient::available();
}

size_t BayESP8266::write(uint8_t b) {
#if ESP8266_DEBUG
	Serial.write(b);
#endif
	return WiFiClient::write(b);
}
int BayESP8266::peek(void) {
	return WiFiClient::peek();
}
void BayESP8266::flush(void) {
	WiFiClient::flush();
}
void BayESP8266::flushMTU(void) {

}
void BayESP8266::finishTransmissionMode(void) {

}

