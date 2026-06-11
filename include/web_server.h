#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>

extern WebServer server;

void setupWebServer();
void handleRoot();
void handleAPI();
void handleConfig();
void handleCmd();
void handleClearDevices();

#endif