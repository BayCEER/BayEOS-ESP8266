
String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    return true;
  }
  return false;
}

void sendNotFound(void) {
  server.send(404, "text/plain", "404: File Not Found");
}

void sendError(void) {
  server.send(501, "text/plain", "501: Logger Connection Error");
  logger.status = 32; //ready
  logger.status_update = true;
}


void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    sendNotFound();
  }
}

unsigned long last_downloadstatus;

void downloadStatus(bool force = false) {
  if ((millis() - last_downloadstatus) < 500 && ! force) return;
  last_downloadstatus = millis();
  mes = F("{\"type\":\"event\",\"event\":\"download\",\"total\":");
  mes += logger.dump_end + logger.dump_offset;
  mes += F(",\"sent\":");
  mes += logger.dump_pos + logger.dump_offset;
  mes += "}";
  webSocket.broadcastTXT(mes);
}

void sendFileDownloadHeader(char* filename, char* ending, unsigned long length) {
  server.client().print(F("HTTP/1.1 200\r\n"));
  server.client().print(F("Content-Type: application/octet-stream\r\n"));
  server.client().print(F("Content-Description: "));
  server.client().print(filename);
  server.client().write('.');
  server.client().print(ending);
  server.client().print(F("\r\n"));
  server.client().print(F("Content-Transfer-Encoding: binary\r\n"));
  server.client().print(F("Expires: 0\r\n"));
  server.client().print(F("Cache-Control: must-revalidate, post-check=0, pre-check=0\r\n"));
  server.client().print(F("Content-disposition: inline; filename="));
  server.client().print(filename);
  server.client().write('.');
  server.client().print(ending);
  server.client().print(F("\r\n"));
  server.client().print(F("Content-Length: "));
  char tmp[12];
  itoa(length, tmp, 10);
  server.client().print(tmp);
  server.client().print(F("\r\n"));
  server.client().print(F("Connection: close\r\n\r\n"));
}

void DownloadMetadata(void) {
  logger.dump_end = strlen(logger.name) + strlen(logger.channel_map) + strlen(logger.unit_map) + 44;
  logger.dump_pos = 0;
  downloadStatus();
  sendFileDownloadHeader(logger.file_name, "json", logger.dump_end);
  server.client().print("{\"origin\":\"");
  server.client().print(logger.name);
  server.client().print("\",\"channel_map\":\"");
  server.client().print(logger.channel_map);
  server.client().print("\",\"unit_map\":\"");
  server.client().print(logger.unit_map);
  server.client().print("\"}");
  server.client().stop();

  logger.dump_pos = logger.dump_end;
  downloadStatus(true);
  logger.status = 32; //ready
  logger.status_update = true;
}

void handleDownload(void) {
  unsigned long arg[2];
  unsigned long last_loop;
  char buffer[10];
  server.arg(0).toCharArray(buffer, 10);
  if(buffer[0] == '2'){
    DownloadMetadata();
    return;
  }
  bool old_format = (buffer[0] == '1');
  server.arg(1).toCharArray(buffer, 10);
  long dl_size=atoi(buffer);
  
  if (!sendCommand(BayEOS_BufferInfo)) {
    sendError();
    return;
  }
  memcpy((uint8_t*) &logger.read_pos, client->getPayload() + 2, 4);
  memcpy((uint8_t*) &logger.write_pos, client->getPayload() + 6, 4);
  memcpy((uint8_t*) &logger.end_pos, client->getPayload() + 10, 4);
  memcpy((uint8_t*) &logger.flash_size, client->getPayload() + 14, 4);
  if(client->getPacketLength()>18){
    memcpy((uint8_t*) &logger.framesize, client->getPayload() + 18, 1);
    memcpy((uint8_t*) &logger.logging_int, client->getPayload() + 19, 2);
  } else {
    logger.framesize=0;
  }
  arg[1] = logger.write_pos;

  if (dl_size>=0) {
    if(dl_size>0){
      arg[0]=logger.write_pos-dl_size;
      if(arg[0]>logger.flash_size) arg[0]+=logger.flash_size;
    } else arg[0]=logger.read_pos;
    if (! sendCommand(BayEOS_StartBinaryDump, (uint8_t*)arg, 4)) {
      sendError();
      return;
    }
    arg[0] = logger.read_pos;
  } else {
    if (! sendCommand(BayEOS_StartBinaryDump)) {
      sendError();
      return;
    }
    arg[0] = logger.end_pos;
  }
  memcpy((uint8_t*)&logger.dump_end, client->getPayload() + 2, 4);
  if (old_format) sendFileDownloadHeader(logger.file_name, "db", logger.dump_end);
  else sendFileDownloadHeader(logger.file_name, "bdb", logger.dump_end + strlen(logger.name) + 5);

  logger.status = 128; //binary dump modus
  logger.dump_pos = 0;
  logger.dump_offset = 0;
  logger.status_update = true;
  downloadStatus();
  if (! old_format) {
    const long sig = 0x50e0a10b; //File Signature 0BA1E050
    server.client().write((uint8_t*)&sig, 4);
    server.client().write(strlen(logger.name));
    server.client().write((uint8_t*)logger.name, strlen(logger.name));
  }
  unsigned long pos = 0;
  unsigned long last_data = millis();
  while (logger.status == 128 && logger.dump_pos < logger.dump_end &&  server.client().connected()) {
    yield();
    if (! client->readIntoPayload()) {
      //got Data
      memcpy((uint8_t*)&pos, client->getPayload() + 1, 4);
      if (pos == logger.dump_pos) {
        last_data = millis();
        server.client().write(client->getPayload() + 5, client->getPacketLength() - 5);
        logger.dump_pos += client->getPacketLength() - 5;
        logger.status_update = true;
      }
    }
    if ((millis() - last_loop) > 1000) {
      last_loop = millis();
      downloadStatus();
      webSocket.loop();
    }
    if ((millis() - last_data) > 3000) {
      //missed a packet or no data for long time
      if(logger.serial) client_serial.sendTXBreak();
      else client_rf24.sendTXBreak();
      sendCommand(BayEOS_ModeStop);
      arg[0] += logger.dump_pos;
      if (arg[0] >= logger.flash_size) arg[0] -= logger.flash_size;
      if (! sendCommand(BayEOS_StartBinaryDump, (uint8_t*)arg, 8)) {
        logger.status = 2; //lost connection
        break;
      }

      memcpy((uint8_t*)&logger.dump_end, client->getPayload() + 2, 4);
      logger.dump_offset += logger.dump_pos;
      pos = 0;
      logger.dump_pos = 0;
    }


  }

  server.client().stop();
  if (logger.dump_end == logger.dump_pos) {
    downloadStatus(true);
    arg[0] = 4;
    sendCommand(BayEOS_BufferCommand, (uint8_t*)arg, 1);
  }
  sendCommand(BayEOS_ModeStop);

  logger.status = 3; //buffer read
  logger.status_update = true;
  return;

}
