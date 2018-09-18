#include "BayEOSBufferSPIFFS.h"
BayEOSBufferSPIFFS::BayEOSBufferSPIFFS(unsigned long max_length,uint8_t append,const char *f):BayEOSBuffer(){
	strncpy(_filename,f,12);
	_filename[12]=0;
#if SERIAL_DEBUG
	Serial.println(_filename);
	 Serial.println("open file:");
	 delay(100);
#endif
    _f=SPIFFS.open(_filename, "a+");
	 _max_length=max_length;
#if SERIAL_DEBUG
	 Serial.println(_f.size());
#endif
	 if(append) set(_f.size());
     else reset();
}



void BayEOSBufferSPIFFS::resetStorage(void){
  _f.close();
  SPIFFS.remove(_filename);
  _f=SPIFFS.open(_filename,"w+");
//  Serial.println("SPIFFS reset");
}

uint8_t BayEOSBufferSPIFFS::write(const uint8_t b){
	return _f.write(b);
}

uint8_t BayEOSBufferSPIFFS::write(const uint8_t *b,uint8_t length){
//	Serial.print("SPIFFS.write:");
//	Serial.println(_f.size());
	return _f.write(b,length);
}

uint8_t BayEOSBufferSPIFFS::seek(unsigned long pos){
	return _f.seek(pos,SeekSet);
}

int BayEOSBufferSPIFFS::read(void){
	return _f.read();
}

int BayEOSBufferSPIFFS::read(uint8_t *dest,int length){
	return _f.read(dest,length);
}

void BayEOSBufferSPIFFS::flush(void){
	_f.flush();
	//	Serial.println(_f.size());
}
