#include <Arduino.h>
#include "config.h"
#include "display_manager.h"
#include "wifi_manager.h"
#include "web_server_manager.h"
#include "sensor_manager.h"
#include "telemetry_client.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F(" ESTACION METEOROLOGICA AGRICOLA INTELIGENTE"));
    Serial.println(F(" VERAMET ATUM — Nodo Central de Sensores"));
    Serial.println(F(" Creado por: Juan Esteban Vera Rodriguez"));
    Serial.println(F("=================================================="));

    // 1. Iniciar gestor de sensores (datos de prueba/sensores reales)
    sensorManager.begin();

    // 2. Iniciar pantalla LCD I2C (con escaneo automático de dirección)
    displayManager.begin();

    // 3. Iniciar subsistema Wi-Fi (Modo STA o Modo AP si no hay red)
    wifiManager.begin();

    // 4. Sincronización Horaria NTP automática (UTC-5 Colombia: -18000s)
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.google.com", "time.windows.com");
    Serial.println(F("[NTP] Sincronizacion horaria en segundo plano configurada (UTC -5 Colombia)."));

    // 5. Iniciar cliente de telemetría hacia servidor local en PC o nube
    telemetryClient.begin();

    // 6. Iniciar servidor web ligero desacoplado (CORS habilitado)
    webServerManager.begin();

    Serial.println(F("[SISTEMA] Inicializacion completa. Sistema transmitiendo a servidor."));
}

void loop() {
    // Tarea 1: Actualizar lecturas y cálculos agronómicos
    sensorManager.update();

    // Tarea 2: Mantener conexión Wi-Fi y mDNS
    wifiManager.update();

    // Tarea 3: Atender peticiones del servidor Web interno (REST API)
    webServerManager.update();

    // Tarea 4: Enviar telemetría periódica hacia servidor en PC local o nube
    telemetryClient.update(sensorManager.getData());

    // Tarea 5: Actualizar pantalla LCD (maneja rotación automática y refresco)
    displayManager.update(
        sensorManager.getData(),
        wifiManager.getStatusString(),
        wifiManager.getIPAddress()
    );

    // Pequeño rendimiento cooperativo para el watchdog del ESP8266
    yield();
}
