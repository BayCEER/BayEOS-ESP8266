#include "BayEOSBufferSPIFFS.h"
BayEOSBufferSPIFFS::BayEOSBufferSPIFFS(unsigned long max_length, uint8_t append,
		const char *f) :
		BayEOSBuffer() {
	strncpy(_filename, f, 12);
	_filename[12] = 0;
#if SERIAL_DEBUG
	Serial.println(_filename);
	Serial.println("open file:");
	delay(100);
#endif
	_f = SPIFFS.open(_filename, "a+");
	_max_length = max_length;
#if SERIAL_DEBUG
	Serial.println(_f.size());
#endif
	if (append)
		set(_f.size());
	else
		reset();
}

void BayEOSBufferSPIFFS::resetStorage(void) {
	_f.close();
	SPIFFS.remove(_filename);
	_f = SPIFFS.open(_filename, "w+");
//  Serial.println("SPIFFS reset");
}

uint8_t BayEOSBufferSPIFFS::write(const uint8_t b) {
	return _f.write(b);
}

uint8_t BayEOSBufferSPIFFS::write(const uint8_t *b, uint8_t length) {
//	Serial.print("SPIFFS.write:");
//	Serial.println(_f.size());
	return _f.write(b, length);
}

uint8_t BayEOSBufferSPIFFS::seek(unsigned long pos) {
	return _f.seek(pos, SeekSet);
}

int BayEOSBufferSPIFFS::read(void) {
	return _f.read();
}

int BayEOSBufferSPIFFS::read(uint8_t *dest, int length) {
	return _f.read(dest, length);
}

void BayEOSBufferSPIFFS::flush(void) {
	_f.flush();
	//	Serial.println(_f.size());
}

BayEOSBufferSPIFFS2::BayEOSBufferSPIFFS2(unsigned long max_length) :
		BayEOSBuffer() {
	_max_length = max_length;
}

void BayEOSBufferSPIFFS2::init(void) {
	_old_size=0;
	if (SPIFFS.exists("bayeos.db")) {
	//	Serial.println("existing file");
		SPIFFS.remove("bayeos.odb");
		SPIFFS.rename("bayeos.db","bayeos.odb");
		_o = SPIFFS.open("bayeos.odb","r");
		_old_size=_o.size();

	}
	_f = SPIFFS.open("bayeos.db", "w+");
	set(_old_size);
//	Serial.printf("init %d",_old_size);
	_end=0;
}

void BayEOSBufferSPIFFS2::resetStorage(void) {
	_f.close();
	_o.close();
	SPIFFS.remove("bayeos.db");
	SPIFFS.remove("bayeos.odb");
	init();
	//  Serial.println("SPIFFS reset");
}

uint8_t BayEOSBufferSPIFFS2::write(const uint8_t b) {
	return _f.write(b);
}

uint8_t BayEOSBufferSPIFFS2::write(const uint8_t *b, uint8_t length) {
//	Serial.print("SPIFFS.write:");
//	Serial.println(_f.size());
	return _f.write(b, length);
}

uint8_t BayEOSBufferSPIFFS2::seek(unsigned long pos) {
	if(pos>=_old_size)
		return _f.seek(pos - _old_size, SeekSet);
	else
		return _o.seek(pos);
}

int BayEOSBufferSPIFFS2::read(void) {
	if(_pos>=_old_size)
		return _f.read();
	else
		return _o.read();
}

int BayEOSBufferSPIFFS2::read(uint8_t *dest, int length) {
	if(_pos>=_old_size)
		return _f.read(dest, length);
	else
		return _o.read(dest, length);
}

void BayEOSBufferSPIFFS2::flush(void) {
	_f.flush();
	if(_pos>=_max_length/2 ){
		_o.close();
		if(_read_pos<_old_size){
			_read_pos=0;
		} else {
			_read_pos-=_old_size;
		}
		_f.close();
		SPIFFS.remove("bayeos.odb");
		SPIFFS.rename("bayeos.db","bayeos.odb");
		//Serial.println("existing file");
		_o = SPIFFS.open("bayeos.odb","r");
		_old_size=_o.size();
		_f = SPIFFS.open("bayeos.db", "w+");
		_write_pos=_old_size;
		_end=0;
		_pos=_old_size;

	}
//    Serial.printf("pos: %d - size %d\n",_f.position(),_f.size());
}
