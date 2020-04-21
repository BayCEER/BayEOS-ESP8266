
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
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
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
  mes += logger.dump_end;
  mes += F(",\"sent\":");
  mes += logger.dump_pos;
  mes += "}";
  webSocket.broadcastTXT(mes);
}

void Download(bool full = false) {
  unsigned long arg;
  unsigned long last_loop;
  char buffer[2];
  server.arg(0).toCharArray(buffer, 2);
  bool old_format = (buffer[0] == '1');
  if (! full) {
    arg = 5; //get read Pos
    if (! sendCommand(BayEOS_BufferCommand, (uint8_t*)&arg, 1)) {
      sendError();
      return;
    }
    memcpy((uint8_t*)&arg, client.getPayload() + 2, 4);
    if (! sendCommand(BayEOS_StartBinaryDump, (uint8_t*)&arg, 4)) {
      sendError();
      return;
    }
  } else {
    if (! sendCommand(BayEOS_StartBinaryDump)) {
      sendError();
      return;
    }
  }
  memcpy((uint8_t*)&logger.dump_end, client.getPayload() + 2, 4);
  Serial.print("Dump:");
  Serial.println(logger.dump_end);
  server.client().write("HTTP/1.1 200\r\n", 14);
  server.client().write("Content-Type: application/octet-stream\r\n", 40);
  server.client().write("Content-Description: ", 21);
  server.client().write(logger.name, strlen(logger.name));
  if (old_format) server.client().write(".db\r\n", 5);
  else server.client().write(".bdb\r\n", 6);
  server.client().write("Content-Transfer-Encoding: binary\r\n", 35);
  server.client().write("Expires: 0\r\n", 12);
  server.client().write("Cache-Control: must-revalidate, post-check=0, pre-check=0\r\n", 59);
  server.client().write("Content-disposition: inline; filename=", 38);
  server.client().write(logger.name, strlen(logger.name));
  if (old_format) server.client().write(".db\r\n", 5);
  else server.client().write(".bdb\r\n", 6);
  server.client().write("Content-Length: ", 16);
  char len[12];
  uint8_t l = strlen(logger.name);
  if (old_format) itoa(logger.dump_end, len, 10);
  else itoa(logger.dump_end + l + 5, len, 10);
  server.client().write(len, strlen(len));
  server.client().write("\r\n", 2);
  server.client().write("Connection: close\r\n", 19);
  server.client().write("\r\n", 2);
  logger.status = 128; //binary dump modus
  logger.dump_pos = 0;
  logger.status_update = true;
  downloadStatus();
  if (! old_format) {
    const long sig = 0x50e0a10b; //File Signature 0BA1E050
    server.client().write((uint8_t*)&sig, 4);
    server.client().write(l);
    server.client().write((uint8_t*)logger.name, l);
  }
  unsigned long pos = 0;
  while (logger.status == 128 && logger.dump_pos < logger.dump_end && server.client().connected()) {
    if (! client.readIntoPayload()) {
      yield();
      //got Data
      Serial.print(".");
      memcpy((uint8_t*)&pos, client.getPayload() + 1, 4);
      if (pos == logger.dump_pos) {
        server.client().write(client.getPayload() + 5, client.getPacketLength() - 5);
        logger.dump_pos += client.getPacketLength() - 5;
        logger.status_update = true;
        Serial.print(logger.dump_pos);
        downloadStatus();
      }
    }
    if ((millis() - last_loop) > 1000) {
      Serial.print("loop");
      last_loop = millis();
      webSocket.loop();
    }
  }
  
  Serial.println("end");
  server.client().stop();
  if(logger.dump_end==logger.dump_pos){
    downloadStatus(true); 
    arg = 4;
    sendCommand(BayEOS_BufferCommand, (uint8_t*)&arg, 1);
  }
  sendCommand(BayEOS_ModeStop);

  logger.status = 3; //buffer read
  logger.status_update = true;
  return;

}

void handleDownload(void) {
  Download(false);

}

void handleDownloadFull(void) {
  Download(true);
}
