#include <ESP8266WebServer.h>

ESP8266WebServer server(80);
void handleRoot() {
  String message = FPSTR(HTTP_HEAD);
  message.replace("{v}", "BayEOS WIFI RF24 Router");
  message += FPSTR(HTTP_STYLE);
  message += String(F("<style>"));
  message += String(F("table{border-collapse:collapse;width:100%;}"));
  message += String(F("td,th{border:1px solid #ddd;padding: 8px;}tr:nth-child(even){background-color: #f2f2f2;}tr:hover {background-color: #ddd;}"));
  message += String(F("th{padding-top:12px;padding-bottom:12px;text-align:left;background-color:#4CAF50;color:white;}</style>"));
  message += FPSTR(HTTP_HEAD_END);
  message += String(F("<h1>BayEOS WIFI RF24 Router</h1>"));
  message += String(F("<p><table><tr><th colspan=2>Info</th></tr><tr><td>Name</td><td>"));
  message += bayeos_name;
  message += String(F("</td></tr><tr><td>Gateway</td><td><a href=\"http://"));
  message += bayeos_server;
  message += String(F("/gateway/\">"));
  message += bayeos_server;
  message += String(F("</a></td></tr><tr><td>Total TX</td><td>"));
  message += total_tx;
  message += String(F("</td></tr><tr><td>Total TX Errors</td><td>"));
  message += total_tx_error;
  message += String(F("</td></tr><tr><td>Unsent Data</td><td>"));
  message += myBuffer.available();
  message += String(F(" Bytes</td></tr></table>"));
  message += String(F("<br/><table><tr><th colspan=2>RF24-Configuration</th></tr><tr><td>Channel</td><td>0x"));
  message += rf24_channel;
  message += String(F("</td></tr><tr><td>Total RX</td><td>"));
  message += total_rx;
 message += String(F("</td></tr><tr><td>Accept</td><td>"));
#if WITH_RF24_CHECKSUM
  message += String(F("Only frames with checksum"));
#else
  message += String(F("All"));
#endif
  message += String(F("</td></tr></table><br/><table><tr><th>Pipe</th><th>Address</th><th>RX</th>"));
#if WITH_RF24_CHECKSUM
  message += String(F("<th>CRC failed</th>"));
#endif
  message += String(F("</tr>"));

  char pipe_ends[6][3] = {"12", "24", "48", "96", "ab", "bf"};
  for (uint8_t i = 0; i < 6; i++) {
    message += "<tr><td>";
    message += i;
    message += "</td><td>0x";
    message += rf24_base;
    message += pipe_ends[i];
    message += "</td><td>";
    message += rx_per_pipe[i];
#if WITH_RF24_CHECKSUM
    message += "</td><td>";
    message += rx_per_pipe_failed[i];
#endif
    message += "</td></tr>";
  }
  message += "</table>";

  message += "</p>";
  message += FPSTR(HTTP_END);

  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

