/**
 * BayEOSBufferSPIFFS
 *
 */
#ifndef BayEOSBufferSPIFFS_h
#define BayEOSBufferSPIFFS_h
#include <inttypes.h>
#include <FS.h>
#include <BayEOS.h>
#include <BayEOSBuffer.h>



class BayEOSBufferSPIFFS : public BayEOSBuffer {
public:
	/**
	 * Constructor ...
	 */
	BayEOSBufferSPIFFS(void):BayEOSBuffer(){};
	/**
	 * Constructor defining max file size
	 * Setting append=1 will not reset the buffer at arduino startup
	 * this is especially usefull for logger applications
	 */
	BayEOSBufferSPIFFS(unsigned long max_length,uint8_t append=0,const char *f="bayeos.db");

private:
	void resetStorage(void);
	uint8_t write(const uint8_t b);
	uint8_t write(const uint8_t *b,uint8_t length);
	uint8_t seek(unsigned long pos);
	int read(void);
	int read(uint8_t *dest,int length);
	void flush(void);
	char _filename[13];
	File _f;
};

#define METADATA_AREA 16384

class BayEOSBufferSPIFFS2 : public BayEOSBuffer {
public:
	/**
	 * Constructor ...
	 */
	BayEOSBufferSPIFFS2(unsigned long max_length);
	void init(void);

private:
	void resetStorage(void);
	uint8_t write(const uint8_t b);
	uint8_t write(const uint8_t *b,uint8_t length);
	uint8_t seek(unsigned long pos);
	int read(void);
	int read(uint8_t *dest,int length);
	void flush(void);
	char _filename[13];
	File _f;
	File _o;
	unsigned long _old_size;
};
#endif
