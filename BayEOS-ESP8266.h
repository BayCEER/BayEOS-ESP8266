#ifndef BayTCPESP8266_h
#define BayTCPESP8266_h


#include <BayTCP.h>
#include <ESP8266WiFi.h>

class BayESP8266 : protected WiFiClient, public BayTCPInterface {
public:
	/**
	 * Constructor
	 */
	BayESP8266(void);
	uint8_t connect(void);
	void disconnect(void);


private:
	int available(void);
	int read(void);
	int i_available(void);
	size_t write(uint8_t b);
    int peek(void);
    void flush(void);
    void flushMTU(void);
    void finishTransmissionMode(void);
};

#endif
