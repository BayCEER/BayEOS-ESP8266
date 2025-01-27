#include <EEPROM.h>
#define SPIFFSBUFFER_SIZE 500

#define BUFFER_DATA_OFFSET 1024
typedef struct {
  unsigned long read_pos;
  unsigned long write_pos;
  unsigned long end_pos;
  unsigned long time;
} bufferData_t;
bufferData_t buffer_data;

#include <BayEOSBufferSPIFFS.h>
BayEOSBufferSPIFFS myBuffer;
RTC_Millis myRTC;

#include <BayDebug.h>
BayDebug client(Serial);
void saveBufferData() {
  EEPROM.put(BUFFER_DATA_OFFSET, buffer_data);
  delay(200);
  EEPROM.commit();
}

void loadBufferData() {
  EEPROM.get(BUFFER_DATA_OFFSET, buffer_data);
}

void printBufferPos() {
  Serial.print("Read: ");
  Serial.print(myBuffer.readPos());
  Serial.print(" - Write: ");
  Serial.print(myBuffer.writePos());
  Serial.print(" - End: ");
  Serial.print(myBuffer.endPos());
  Serial.print(" - Millis: ");
  Serial.print(millis());
  Serial.print(" - RTC: ");
  Serial.println(myRTC.get());
}

void setup() {
  // put your setup code here, to run once:
  EEPROM.begin(2048);
  SPIFFS.begin();
  client.begin(9600, 1);
  Serial.println("Loding Buffer Data");
  loadBufferData();
  if (buffer_data.time > 0 && buffer_data.time < (20 * 365 * 24 * 3600)) {
    myRTC.set(buffer_data.time + millis() / 1000);
    Serial.print("Restored: ");
    Serial.print(buffer_data.read_pos);
    Serial.print(" - ");
    Serial.print(buffer_data.write_pos);
    Serial.print(" - ");
    Serial.print(buffer_data.write_pos);
    Serial.print(" - ");
    Serial.println(buffer_data.time + millis() / 1000);
    buffer_data.time = 0;

    saveBufferData();
    myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE, 1);
    myBuffer.set(buffer_data.read_pos, buffer_data.write_pos, buffer_data.end_pos);
  } else
    myBuffer = BayEOSBufferSPIFFS(SPIFFSBUFFER_SIZE, 0);
  myBuffer.setRTC(myRTC, RTC_RELATIVE_SECONDS);  // use the rtc clock but relative
  client.setBuffer(myBuffer);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'a':
        client.startDataFrame();
        client.addChannelValue(17);
        client.addChannelValue(millis());
        client.sendPayload();
        client.writeToBuffer();
        break;
      case 'b':
        client.sendFromBuffer();
        break;
      case 'c':
        printBufferPos();
        break;
      case 'r':
        buffer_data.read_pos = myBuffer.readPos();
        buffer_data.write_pos = myBuffer.writePos();
        buffer_data.end_pos = myBuffer.endPos();
        buffer_data.time = myRTC.get() + 1;
        saveBufferData();
        ESP.restart();
        break;
    }
  }
}
