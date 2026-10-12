#include "telemetry_client.h"
#include "config.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include <EEPROM.h>
#include <time.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
#endif

#define EEPROM_SIZE            512
#define EEPROM_URL_MAGIC_ADDR  120
#define EEPROM_URL_MAGIC_VAL   0x5C
#define EEPROM_URL_ADDR        121
#define EEPROM_URL_MAX         128

TelemetryClient telemetryClient;

TelemetryClient::TelemetryClient()
    : serverUrl(DEFAULT_TELEMETRY_URL),
      lastPushMs(0),
      successfulPushes(0),
      failedPushes(0),
      lastSuccess(false),
      lastHttpCode(0) {}

void TelemetryClient::begin() {
    loadServerUrlFromEEPROM();
}

void TelemetryClient::setServerUrl(const String& url) {
    saveServerUrlToEEPROM(url);
}

void TelemetryClient::loadServerUrlFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    uint8_t magic = EEPROM.read(EEPROM_URL_MAGIC_ADDR);
    if (magic == EEPROM_URL_MAGIC_VAL) {
        String loaded = "";
        for (int i = 0; i < EEPROM_URL_MAX; i++) {
            char c = (char)EEPROM.read(EEPROM_URL_ADDR + i);
            if (c == '\0') break;
            loaded += c;
        }
        if (loaded.startsWith("http://") || loaded.startsWith("https://")) {
            serverUrl = loaded;
            Serial.printf("[Telemetria] URL de servidor recuperada de EEPROM: %s\n", serverUrl.c_str());
            return;
        }
    }
    serverUrl = DEFAULT_TELEMETRY_URL;
    Serial.printf("[Telemetria] Usando URL de servidor por defecto: %s\n", serverUrl.c_str());
}

void TelemetryClient::saveServerUrlToEEPROM(const String& url) {
    if (url.length() == 0 || (int)url.length() >= EEPROM_URL_MAX) return;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(EEPROM_URL_MAGIC_ADDR, EEPROM_URL_MAGIC_VAL);
    int len = url.length();
    for (int i = 0; i < EEPROM_URL_MAX; i++) {
        if (i < len) {
            EEPROM.write(EEPROM_URL_ADDR + i, (uint8_t)url[i]);
        } else {
            EEPROM.write(EEPROM_URL_ADDR + i, 0);
        }
    }
    EEPROM.commit();
    serverUrl = url;
    Serial.printf("[Telemetria] Nueva URL guardada permanentemente en flash: %s\n", serverUrl.c_str());
}

String TelemetryClient::buildJsonPayload(const StationData& d) {
    String json = "{";
    json += "\"deviceId\":\"" + getDeviceUniqueId() + "\",";
    json += "\"deviceName\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"macAddress\":\"" + WiFi.macAddress() + "\",";
    json += "\"chipId\":\"0x" + String(ESP.getChipId(), HEX) + "\",";
    json += "\"crop\":\"" + String(CROP_TYPE) + "\",";
    
    // Conectividad
    json += "\"ip\":\"" + wifiManager.getIPAddress() + "\",";
    json += "\"wifiStatus\":\"" + wifiManager.getStatusString() + "\",";
    json += "\"ssid\":\"" + wifiManager.getSSID() + "\",";
    json += "\"rssi\":" + String(wifiManager.getRSSI()) + ",";
    json += "\"isSimulated\":" + String(d.isSimulated ? "true" : "false") + ",";
    
    // Tiempo Real Sincronizado
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timeStr[32] = "--:--:--";
    char dateStr[32] = "----/--/--";
    if (now > 100000 && tm_info != nullptr) {
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_info);
    }
    json += "\"currentTime\":\"" + String(timeStr) + "\",";
    json += "\"currentDate\":\"" + String(dateStr) + "\",";
    json += "\"epoch\":" + String((unsigned long)now) + ",";

    // Variables Atmosféricas
    json += "\"temp\":" + String(d.ambientTemp, 1) + ",";
    json += "\"ambientTemp\":" + String(d.ambientTemp, 1) + ",";
    json += "\"humidity\":" + String(d.ambientHum, 1) + ",";
    json += "\"ambientHum\":" + String(d.ambientHum, 1) + ",";
    json += "\"solarRadiation\":" + String(d.solarRadiation, 0) + ",";
    json += "\"solarHours\":" + String(d.solarHours, 1) + ",";
    json += "\"rainMm\":" + String(d.rainAccumulated, 1) + ",";
    json += "\"rainAccumulated\":" + String(d.rainAccumulated, 1) + ",";
    json += "\"windSpeed\":8.6,";
    json += "\"windDir\":\"NE\",";
    json += "\"irrigationStatus\":\"" + d.irrigationStatus + "\",";
    json += "\"nextLogSec\":" + String(d.nextLogSec) + ",";
    
    // 4 Sondas de Suelo (8 en 1 RS485 Modbus)
    json += "\"soil\":[";
    for (int i = 0; i < 4; i++) {
        json += "{\"id\":" + String(i + 1) + ",";
        json += "\"moisture\":" + String(d.soilProbes[i].moisture, 1) + ",";
        json += "\"temp\":" + String(d.soilProbes[i].temperature, 1) + ",";
        json += "\"ec\":" + String(d.soilProbes[i].ec, 0) + ",";
        json += "\"ph\":" + String(d.soilProbes[i].ph, 1) + ",";
        json += "\"n\":" + String(d.soilProbes[i].nitrogen, 0) + ",";
        json += "\"p\":" + String(d.soilProbes[i].phosphorus, 0) + ",";
        json += "\"k\":" + String(d.soilProbes[i].potassium, 0) + ",";
        json += "\"tension\":" + String(d.soilProbes[i].tensionCb, 1) + ",";
        json += "\"valid\":" + String(d.soilProbes[i].valid ? "true" : "false") + "}";
        if (i < 3) json += ",";
    }
    json += "],";

    // Estado físico LCD
    json += "\"lcd\":{";
    json += "\"currentScreen\":" + String(displayManager.getCurrentScreen()) + ",";
    json += "\"autoCycle\":" + String(displayManager.isAutoCycleEnabled() ? "true" : "false") + ",";
    json += "\"backlight\":" + String(displayManager.isBacklightEnabled() ? "true" : "false") + ",";
    json += "\"remainingBacklightSec\":" + String(displayManager.getRemainingBacklightSec()) + ",";
    json += "\"i2cAddr\":\"0x" + String(displayManager.getDetectedI2CAddress(), HEX) + "\",";
    json += "\"ready\":" + String(displayManager.isLcdReady() ? "true" : "false");
    json += "},";

    json += "\"uptimeSec\":" + String(millis() / 1000);
    json += "}";
    return json;
}

bool TelemetryClient::sendPayload(const String& payload) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

#if defined(ESP8266)
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(TELEMETRY_TIMEOUT_MS);
    if (http.begin(client, serverUrl)) {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("X-Device-Id", getDeviceUniqueId());
        int httpCode = http.POST(payload);
        lastHttpCode = httpCode;
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
                lastSuccess = true;
                successfulPushes++;
                Serial.printf("[Telemetria] Push exitoso hacia %s (HTTP %d)\n", serverUrl.c_str(), httpCode);
            } else {
                lastSuccess = false;
                failedPushes++;
                Serial.printf("[Telemetria] Servidor respondio con HTTP %d\n", httpCode);
            }
        } else {
            lastSuccess = false;
            failedPushes++;
            Serial.printf("[Telemetria] Error conexion HTTP: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
        return lastSuccess;
    }
#elif defined(ESP32)
    HTTPClient http;
    http.setTimeout(TELEMETRY_TIMEOUT_MS);
    if (http.begin(serverUrl)) {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("X-Device-Id", getDeviceUniqueId());
        int httpCode = http.POST(payload);
        lastHttpCode = httpCode;
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
            lastSuccess = true;
            successfulPushes++;
        } else {
            lastSuccess = false;
            failedPushes++;
        }
        http.end();
        return lastSuccess;
    }
#endif
    return false;
}

void TelemetryClient::update(const StationData& data) {
    unsigned long now = millis();
    if (now - lastPushMs >= TELEMETRY_INTERVAL_MS) {
        lastPushMs = now;
        if (WiFi.status() == WL_CONNECTED) {
            String payload = buildJsonPayload(data);
            sendPayload(payload);
        }
    }
}
