#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "sensor_manager.h"
#include "display_manager.h"
#include "wifi_manager.h"
#include "telemetry_client.h"

class WebServerManager {
public:
    WebServerManager();
    void begin();
    void update();

private:
    ESP8266WebServer server;

    void setupRoutes();
    void handleRoot();
    void handleApiData();
    void handleApiHistory();
    void handleTimeSync();
    void handleLcdControl();
    void handleServerUrl();
    void handleWifiSave();
    void handleWifiReset();
    void handleNotFound();
    void sendCorsHeaders();
};

extern WebServerManager webServerManager;

#endif // WEB_SERVER_MANAGER_H
