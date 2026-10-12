#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==============================================================================
// METADATOS DE LA ESTACIÓN AGRÍCOLA
// ==============================================================================
#define DEVICE_NAME         "VERAMET"
#define FIRMWARE_VERSION    "v2.0-ATUM"
#define CROP_TYPE           "Mango, Limon, Arroz, Trigo"

// Genera un Identificador Único de Hardware irrepetible basado en el Chip ID del ESP8266
inline String getDeviceUniqueId() {
    char buf[32];
    snprintf(buf, sizeof(buf), "VERAMET-ESP-%06X", ESP.getChipId());
    return String(buf);
}

// ==============================================================================
// CONFIGURACIÓN PANTALLA LCD 16x2 I2C
// ==============================================================================
// NodeMCU V3 Pinout para I2C
#define I2C_SDA_PIN         D2  // GPIO 4
#define I2C_SCL_PIN         D1  // GPIO 5

#define LCD_COLS            16
#define LCD_ROWS            2
#define LCD_DEFAULT_ADDR    0x27 // Direcciones comunes: 0x27 o 0x3F (el código auto-detecta)

// Intervalo de cambio automático de pantalla (milisegundos) si no se usa la web
#define LCD_CAROUSEL_TIME_MS 4500

// Gestión de ahorro de energía de la pantalla LCD:
// Al arrancar: se apaga a los 2 minutos (120,000 ms) para ahorrar batería
#define LCD_AUTO_OFF_INITIAL_MS  120000UL
// Si el usuario la enciende desde la web: dura 10 minutos (600,000 ms) encendida
#define LCD_AUTO_OFF_WEB_MS      600000UL

// ==============================================================================
// CONFIGURACIÓN WI-FI (MODO ESTACIÓN Y PUNTO DE ACCESO)
// ==============================================================================
// Red Wi-Fi a la que intentará conectarse inicialmente (Modo STA)
#define DEFAULT_WIFI_SSID   "TU_RED_WIFI"
#define DEFAULT_WIFI_PASS   "TU_PASSWORD"
#define WIFI_CONNECT_TIMEOUT_SEC 15

// Red Wi-Fi de rescate/configuración que crea la ESP si no se conecta (Modo AP)
#define AP_SSID             "VERAMET-Agricola-AP"
#define AP_PASS             "12345678" // Mínimo 8 caracteres
#define AP_IP_DEFAULT       "192.168.4.1"

// Hostname mDNS (permite entrar como http://veramet.local o http://estacion.local)
#define MDNS_HOSTNAME       "veramet"

// Puerto del servidor Web
#define WEB_SERVER_PORT     80

// ==============================================================================
// PARÁMETROS DE MUESTREO Y REGISTRO (CIENTÍFICO FAO / CORPOICA)
// ==============================================================================
// Intervalo de registro histórico: cada 15 minutos (900,000 ms) para evitar saturación y cuidar batería
#define DATA_LOG_INTERVAL_MS      900000UL
#define SENSOR_UPDATE_INTERVAL_MS 3000

// ==============================================================================
// CONFIGURACIÓN DE TELEMETRÍA HACIA SERVIDOR (PC LOCAL O NUBE)
// ==============================================================================
// URL por defecto del servidor en el PC local (puerto 5000) o servidor en la nube
#define DEFAULT_TELEMETRY_URL     "http://192.168.20.41:5000/api/telemetry"
#define TELEMETRY_INTERVAL_MS     5000UL // Envío cada 5 segundos para desarrollo en tiempo real
#define TELEMETRY_TIMEOUT_MS      3500   // Timeout de petición HTTP (ms)

#endif // CONFIG_H
