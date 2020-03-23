/*
 * This is an alternativ for sending BayEOS-Frames over RF24 using the BaySerial protocol
 *
 * BaySerial layer is on top of RF24
 * We have additional checksum and no limitation to 32 byte as for simple RF24
 * The receiver sends a acknowlege to the sender via RF24. This is done by the
 *
 *
 */


#ifndef BaySerialRF24_h
#define BaySerialRF24_h

#include <BaySerial.h>
#include <RF24.h>
#define RXBUFFER_SIZE 140
class BaySerialRF24 : public BaySerialInterface {
private:
	void stopListenMode();
	void startListenMode();
protected:
	RF24* _radio; //Pointer to existing radio!!
	uint8_t tx_buffer[32];
	uint8_t rx_buffer[RXBUFFER_SIZE];
	uint16_t rx_write;
	uint16_t rx_read;
	uint16_t write_pos;
	uint16_t length;
	uint8_t _flush_size;
	uint8_t _r_counter;
	uint8_t _w_counter;
	bool _listen_mode;
	bool _send_timeout;
public:
	/**
	 * Constructor
	 */
	BaySerialRF24(RF24& radio,int timeout=1000,uint8_t retries=1);

	int available(void);
	int i_available(void);
	void begin(long baud);
	void init(uint8_t ch, uint8_t *adr, uint8_t flush_size=12);
	void flush(void);
	void end(void);
	int read(void);
	void poll(uint8_t max_c=10); //call in isr to fill rx_buffer
	size_t write(uint8_t c);
	unsigned long last_activity;

};




#endif
