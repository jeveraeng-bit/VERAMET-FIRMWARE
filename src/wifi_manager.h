#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>

class WiFiManager {
public:
    WiFiManager();
    void begin();
    void update();
    
    bool isConnected() const;
    bool isAPMode() const { return apActive; }
    
    String getIPAddress() const;
    String getSSID() const;
    String getStatusString() const;
    int getRSSI() const;
    
    void saveCredentials(const String& newSsid, const String& newPass);
    void resetWiFiSettings();
    void switchToAP();

private:
    String ssid;
    String password;
    bool apActive;
    unsigned long lastReconnectAttempt;
    DNSServer dnsServer;
    
    bool connectSTA();
    void startAP();
    void loadCredentialsFromEEPROM();
    void writeCredentialsToEEPROM(const String& s, const String& p);
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
