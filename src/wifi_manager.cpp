#include "wifi_manager.h"
#include "config.h"
#include <EEPROM.h>

#define EEPROM_SIZE         512
#define EEPROM_MAGIC_ADDR   0
#define EEPROM_MAGIC_VAL    0x5A
#define EEPROM_SSID_ADDR    1
#define EEPROM_SSID_MAX     32
#define EEPROM_PASS_ADDR    34
#define EEPROM_PASS_MAX     64

WiFiManager wifiManager;

WiFiManager::WiFiManager()
    : ssid(DEFAULT_WIFI_SSID),
      password(DEFAULT_WIFI_PASS),
      apActive(false),
      lastReconnectAttempt(0) {}

void WiFiManager::loadCredentialsFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);

    if (magic == EEPROM_MAGIC_VAL) {
        String loadedSsid = "";
        String loadedPass = "";

        for (int i = 0; i < EEPROM_SSID_MAX; i++) {
            char c = (char)EEPROM.read(EEPROM_SSID_ADDR + i);
            if (c == '\0') break;
            loadedSsid += c;
        }

        for (int i = 0; i < EEPROM_PASS_MAX; i++) {
            char c = (char)EEPROM.read(EEPROM_PASS_ADDR + i);
            if (c == '\0') break;
            loadedPass += c;
        }

        if (loadedSsid.length() > 0) {
            ssid = loadedSsid;
            password = loadedPass;
            Serial.printf("[WiFi] Credenciales recuperadas de memoria flash (EEPROM): '%s'\n", ssid.c_str());
            return;
        }
    }

    Serial.println(F("[WiFi] No hay credenciales guardadas en flash. Usando valores por defecto."));
    ssid = DEFAULT_WIFI_SSID;
    password = DEFAULT_WIFI_PASS;
}

void WiFiManager::writeCredentialsToEEPROM(const String& s, const String& p) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);

    int sLen = s.length();
    for (int i = 0; i < EEPROM_SSID_MAX; i++) {
        if (i < sLen) {
            EEPROM.write(EEPROM_SSID_ADDR + i, s[i]);
        } else {
            EEPROM.write(EEPROM_SSID_ADDR + i, 0);
        }
    }

    int pLen = p.length();
    for (int i = 0; i < EEPROM_PASS_MAX; i++) {
        if (i < pLen) {
            EEPROM.write(EEPROM_PASS_ADDR + i, p[i]);
        } else {
            EEPROM.write(EEPROM_PASS_ADDR + i, 0);
        }
    }

    EEPROM.commit();
    Serial.println(F("[WiFi] Credenciales guardadas exitosamente en memoria persistente (EEPROM)."));
}

void WiFiManager::resetWiFiSettings() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(EEPROM_MAGIC_ADDR, 0x00);
    EEPROM.commit();
    Serial.println(F("[WiFi] Memoria EEPROM borrada. Restaurando red a Modo AP por defecto."));
    ssid = DEFAULT_WIFI_SSID;
    password = DEFAULT_WIFI_PASS;
    WiFi.disconnect();
    delay(500);
    startAP();
}

void WiFiManager::begin() {
    Serial.println(F("[WiFi] Inicializando subsistema Wi-Fi..."));

    // 1. Cargar credenciales guardadas en memoria permanente (flash/EEPROM)
    loadCredentialsFromEEPROM();

    // 2. Si no hay red guardada o es el placeholder inicial, activar Modo AP
    if (ssid == "TU_RED_WIFI" || ssid.length() == 0) {
        Serial.println(F("[WiFi] Sin red configurada. Iniciando Punto de Acceso (AP)..."));
        startAP();
        return;
    }

    // 3. Intentar conectar a la red guardada
    if (!connectSTA()) {
        Serial.println(F("[WiFi] No fue posible conectar a la red guardada. Activando Modo AP de contingencia..."));
        startAP();
    }
}

bool WiFiManager::connectSTA() {
    Serial.printf("[WiFi] Intentando conexion a red: '%s'\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < (WIFI_CONNECT_TIMEOUT_SEC * 1000))) {
        delay(350);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        apActive = false;
        Serial.printf("[WiFi] ¡Conectado exitosamente! IP asignada: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] Potencia de senal RSSI: %d dBm\n", WiFi.RSSI());

        if (MDNS.begin(MDNS_HOSTNAME)) {
            Serial.printf("[mDNS] http://%s.local listo\n", MDNS_HOSTNAME);
        }
        return true;
    }

    return false;
}

void WiFiManager::startAP() {
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(AP_SSID, (strlen(AP_PASS) >= 8) ? AP_PASS : nullptr);

    if (result) {
        apActive = true;
        IPAddress apIP = WiFi.softAPIP();
        Serial.printf("[WiFi-AP] Red de contingencia/configuracion activa: %s\n", AP_SSID);
        Serial.printf("[WiFi-AP] IP del Punto de Acceso: %s\n", apIP.toString().c_str());

        if (MDNS.begin(MDNS_HOSTNAME)) {
            Serial.printf("[mDNS] http://%s.local listo en red AP\n", MDNS_HOSTNAME);
        }
    } else {
        Serial.println(F("[WiFi-AP] Error iniciando SoftAP."));
    }
}

void WiFiManager::update() {
    MDNS.update();

    // Si estamos en modo STA y se cae la red, intentar reconectar sin bloquear
    if (!apActive) {
        if (WiFi.status() != WL_CONNECTED) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt >= 30000) {
                lastReconnectAttempt = now;
                Serial.println(F("[WiFi] Conexion perdida. Reintentando reconexion STA..."));
                WiFi.reconnect();
            }
        }
    }
}

bool WiFiManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED) || apActive;
}

String WiFiManager::getIPAddress() const {
    if (apActive) {
        return WiFi.softAPIP().toString();
    }
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getSSID() const {
    if (apActive) return String(AP_SSID);
    return ssid;
}

String WiFiManager::getStatusString() const {
    if (apActive) return "MODO AP";
    if (WiFi.status() == WL_CONNECTED) return "CONECTADO";
    return "DESCONECTADO";
}

int WiFiManager::getRSSI() const {
    if (apActive) return 0;
    if (WiFi.status() == WL_CONNECTED) return WiFi.RSSI();
    return -100;
}

void WiFiManager::switchToAP() {
    startAP();
}

void WiFiManager::saveCredentials(const String& newSsid, const String& newPass) {
    ssid = newSsid;
    password = newPass;
    
    // Guardar permanentemente en la memoria flash (EEPROM)
    writeCredentialsToEEPROM(ssid, password);

    Serial.printf("[WiFi] Reiniciando conexion para conectarse a: %s\n", ssid.c_str());
    WiFi.disconnect();
    delay(500);
    if (!connectSTA()) {
        startAP();
    }
}
