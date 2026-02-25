#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

void initWifi();
void initWebSocket();
void handleWebSocket();
void sendStateEvent (const char* eventType, const char* uid = nullptr);

#endif