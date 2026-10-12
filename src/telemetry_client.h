#ifndef TELEMETRY_CLIENT_H
#define TELEMETRY_CLIENT_H

#include <Arduino.h>
#include "sensor_manager.h"

class TelemetryClient {
public:
    TelemetryClient();
    void begin();
    void update(const StationData& data);
    
    void setServerUrl(const String& url);
    String getServerUrl() const { return serverUrl; }
    
    bool isConnectedToServer() const { return lastSuccess; }
    unsigned long getLastPushMs() const { return lastPushMs; }
    int getLastHttpCode() const { return lastHttpCode; }
    unsigned long getSuccessfulPushes() const { return successfulPushes; }
    unsigned long getFailedPushes() const { return failedPushes; }

    String buildJsonPayload(const StationData& d);

private:
    String serverUrl;
    unsigned long lastPushMs;
    unsigned long successfulPushes;
    unsigned long failedPushes;
    bool lastSuccess;
    int lastHttpCode;

    void loadServerUrlFromEEPROM();
    void saveServerUrlToEEPROM(const String& url);
    bool sendPayload(const String& payload);
};

extern TelemetryClient telemetryClient;

#endif // TELEMETRY_CLIENT_H
