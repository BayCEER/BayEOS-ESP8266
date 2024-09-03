uint8_t status=0; /*0 undef. - 1 got new line - 2 has data */
char line_buffer[200];
float data[9];
uint8_t pos=0;
uint8_t ch_pos=0;


void sendData(void); //declaration for Websocket Method


void handleSerial(){
   while (Serial.available()) {
    if(! status){
      if(Serial.read()=='\n') status=1;
    } else {
      line_buffer[pos] = Serial.read();
      if (line_buffer[pos] == '\n') {
        char* p = line_buffer;
        line_buffer[pos] = 0;
        while (*p && ch_pos<9) {
          if((*p>='0' && *p<='9') || *p==' ' || *p=='-'){
            data[ch_pos]=strtof(p, &p);
            ch_pos++;
          } else strtof(p, &p);
          p++;
        }
        sendData();
        ch_pos=0;
        pos = 0;
      } else if (pos < 199) pos++;
    }
  }
 
}
