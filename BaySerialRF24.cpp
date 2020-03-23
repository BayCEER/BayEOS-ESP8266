#include "BaySerialRF24.h"

BaySerialRF24::BaySerialRF24(RF24 &radio, int timeout, uint8_t retries) {
	BaySerialInterface::_timeout = timeout;
	_radio = &radio;
	_retries=retries;
}

void BaySerialRF24::poll(uint8_t max_c){
	uint16_t free_space;
	uint8_t payload[32];
	uint8_t count=0;
	while(_radio->available()){
		if(rx_write<rx_read) free_space=rx_read-rx_write;
		else free_space=rx_read+RXBUFFER_SIZE-rx_write;
		if(free_space<32) return;
		uint8_t l=_radio->getDynamicPayloadSize();
		if(! l) continue;
		_radio->read(payload, l);
		if(payload[0] == _r_counter) continue;
		_r_counter++;
		l--;
		if((rx_write+l)>=RXBUFFER_SIZE){
			uint8_t first=RXBUFFER_SIZE-rx_write;
			memcpy(rx_buffer+rx_write,payload+1,first);
			rx_write=l-first;
			if(rx_write) memcpy(rx_buffer,payload+1+first,rx_write);
		} else {
			memcpy(rx_buffer+rx_write,payload+1,l);
			rx_write+=l;
		}
		count++;
		if(count>=max_c) return;
		delay(1);
		//yield();
	}
}

int BaySerialRF24::available(void) {
	if (write_pos > 1)
		flush();
	if(rx_read==rx_write){
		poll();
	}
	if(rx_write>=rx_read) return rx_write-rx_read;
	else return rx_write+RXBUFFER_SIZE-rx_read;
}

int BaySerialRF24::i_available(void) {
	return BaySerialRF24::available();
}

void BaySerialRF24::begin(long baud) {
}

void BaySerialRF24::init(uint8_t ch, uint8_t *adr, uint8_t flush_size) {
	_radio->begin();
	_radio->setChannel(ch);
	_radio->enableDynamicPayloads();
	_radio->setCRCLength(RF24_CRC_16);
	_radio->setDataRate(RF24_250KBPS);
	_radio->setPALevel(RF24_PA_HIGH);
	_radio->setRetries(15, 8);
	_radio->setAutoAck(true);
	_radio->openWritingPipe(adr);
	_radio->openReadingPipe(0, adr);
	_radio->startListening();
	_flush_size = flush_size;
	last_activity = millis();
	write_pos = 1;
	length = 1;
	rx_read=0;
	rx_write=0;
	_r_counter = 0;
	_w_counter = 0;

}

void BaySerialRF24::flush(void) {
	if (write_pos < 2)
		return;
	_w_counter++;
	if (!_w_counter)
		_w_counter++; //no zero for counters
	tx_buffer[0] = _w_counter;
	//delayMicroseconds(1500);
	stopListenMode();

	uint8_t tx_try = 0;
	uint8_t res;
	//Serial.print("W");
	//Serial.print(_w_counter);
	//Serial.print(millis());
	while (tx_try < 2) {
		////Serial.print("X");
		res = _radio->write(tx_buffer, write_pos);
		uint8_t curr_pa = 0;
		while (!res && curr_pa < 4) {
			_radio->setPALevel((rf24_pa_dbm_e) curr_pa);
			delayMicroseconds(random(1000));
			res = _radio->write(tx_buffer, write_pos);
			curr_pa++;
		}
		if (res)
			break;
		tx_try++;
		delay(10);
	}
	if (!res) {
		_radio->setPALevel(RF24_PA_HIGH);
		//Serial.print("-");
	}
	write_pos = 1;
	_send_timeout = !res;
	startListenMode();
	if(res) last_activity = millis();

}
void BaySerialRF24::end(void) {
//	_radio->powerDown();
}
void BaySerialRF24::startListenMode(void) {
	_radio->flush_tx();
	_radio->startListening();
	_listen_mode=true;
}

void BaySerialRF24::stopListenMode(void) {
	_radio->stopListening();
	while (_radio->available()) { //This is old stuff - discard!
		_radio->flush_rx();
	}
	rx_read=rx_write;
	_listen_mode=false;
}

int BaySerialRF24::read(void) {
	if (!available())
		return -1;
	uint8_t b = rx_buffer[rx_read];
	rx_read++;
	if(rx_read>=RXBUFFER_SIZE) rx_read=0;
	return b;
}

size_t BaySerialRF24::write(uint8_t c) {
	if (_listen_mode) {
		stopListenMode();
	}
	tx_buffer[write_pos] = c;
	write_pos++;
	_send_timeout = false;
	if (write_pos >= _flush_size) {
		flush();
	}
	if (_send_timeout)
		return 0;
	return 1;
}

