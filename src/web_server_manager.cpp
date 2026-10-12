#include "web_server_manager.h"
#include "config.h"

WebServerManager webServerManager;

WebServerManager::WebServerManager() : server(WEB_SERVER_PORT) {}

void WebServerManager::begin() {
    setupRoutes();
    server.begin();
    Serial.printf("[Web] Servidor HTTP VERAMET activo en puerto %d (Modo ligero desacoplado)\n", WEB_SERVER_PORT);
}

void WebServerManager::update() {
    server.handleClient();
}

void WebServerManager::sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void WebServerManager::setupRoutes() {
    server.on("/", HTTP_GET, std::bind(&WebServerManager::handleRoot, this));
    server.on("/generate_204", HTTP_GET, std::bind(&WebServerManager::handleRoot, this));
    server.on("/gen_204", HTTP_GET, std::bind(&WebServerManager::handleRoot, this));
    server.on("/hotspot-detect.html", HTTP_GET, std::bind(&WebServerManager::handleRoot, this));
    
    server.on("/api/data", HTTP_GET, std::bind(&WebServerManager::handleApiData, this));
    server.on("/api/data", HTTP_OPTIONS, [this]() { sendCorsHeaders(); server.send(204); });
    
    server.on("/api/history", HTTP_GET, std::bind(&WebServerManager::handleApiHistory, this));
    server.on("/api/history", HTTP_OPTIONS, [this]() { sendCorsHeaders(); server.send(204); });
    
    server.on("/api/time/sync", HTTP_GET, std::bind(&WebServerManager::handleTimeSync, this));
    server.on("/api/time/sync", HTTP_OPTIONS, [this]() { sendCorsHeaders(); server.send(204); });
    
    server.on("/api/lcd", HTTP_GET, std::bind(&WebServerManager::handleLcdControl, this));
    server.on("/api/lcd", HTTP_OPTIONS, [this]() { sendCorsHeaders(); server.send(204); });
    
    server.on("/api/server-url", HTTP_GET, std::bind(&WebServerManager::handleServerUrl, this));
    server.on("/api/server-url", HTTP_POST, std::bind(&WebServerManager::handleServerUrl, this));
    server.on("/api/server-url", HTTP_OPTIONS, [this]() { sendCorsHeaders(); server.send(204); });

    server.on("/api/sample", HTTP_GET, [this]() {
        sensorManager.takeInstantSample();
        sendCorsHeaders();
        server.send(200, "application/json", "{\"success\":true,\"msg\":\"Muestra instantanea registrada con exito\"}");
    });
    server.on("/api/sample", HTTP_OPTIONS, [this]() { sendCorsHeaders(); server.send(204); });

    server.on("/wifi/save", HTTP_POST, std::bind(&WebServerManager::handleWifiSave, this));
    server.on("/wifi/reset", HTTP_POST, std::bind(&WebServerManager::handleWifiReset, this));
    server.onNotFound(std::bind(&WebServerManager::handleNotFound, this));
}

void WebServerManager::handleNotFound() {
    sendCorsHeaders();
    server.send(404, "text/plain", "Recurso no encontrado (404) - VERAMET Node");
}

void WebServerManager::handleRoot() {
    String html = F("<!DOCTYPE html><html lang='es'><head><meta charset='utf-8'>"
                    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<title>VERAMET ATUM — Nodo Central</title>"
                    "<style>"
                    ":root{--bg:#0b130e;--card:#132219;--border:#1d3828;--text:#ecfdf5;--muted:#6ee7b7;--p:#10b981;--hover:#059669;}"
                    "body{margin:0;font-family:system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--text);padding:20px;display:flex;justify-content:center;}"
                    ".box{max-width:540px;width:100%;background:var(--card);border:1px solid var(--border);border-radius:14px;padding:24px;box-shadow:0 12px 30px rgba(0,0,0,0.5);}"
                    "h1{margin:0 0 4px;font-size:22px;color:#a7f3d0;display:flex;align-items:center;gap:8px;}"
                    ".sub{font-size:13px;color:#6ee7b7;margin-bottom:20px;}"
                    ".badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:12px;font-weight:600;background:rgba(16,185,129,0.15);color:#34d399;border:1px solid #059669;margin-bottom:15px;}"
                    ".card{background:#0a1710;border:1px solid var(--border);border-radius:10px;padding:14px;margin-bottom:14px;font-size:13px;line-height:1.6;}"
                    ".card b{color:#6ee7b7;}"
                    ".btn{display:block;width:100%;box-sizing:border-box;padding:11px;border-radius:8px;background:var(--p);color:#022c22;font-weight:700;font-size:14px;text-align:center;text-decoration:none;border:none;cursor:pointer;margin-top:10px;transition:0.2s;}"
                    ".btn:hover{background:var(--hover);color:#fff;}"
                    ".btn-outline{background:transparent;border:1px solid var(--p);color:var(--p);}"
                    ".btn-outline:hover{background:rgba(16,185,129,0.1);color:#a7f3d0;}"
                    "input{width:100%;box-sizing:border-box;padding:10px;border-radius:6px;background:#05120a;border:1px solid var(--border);color:#fff;margin-bottom:10px;font-size:13px;}"
                    "hr{border:0;border-top:1px solid var(--border);margin:18px 0;}"
                    "</style></head><body>"
                    "<div class='box'>"
                    "<h1>VERAMET ATUM</h1>"
                    "<div class='sub'>Estación Agroclimática Inteligente • Modo Nodo IoT</div>"
                    "<div class='badge'>Dispositivo: ");
    html += getDeviceUniqueId();
    html += F("</div>"
              "<div class='card'>"
              "<div><b>Estado Wi-Fi:</b> ");
    html += wifiManager.getStatusString();
    html += F("</div><div><b>IP de la Estación:</b> ");
    html += wifiManager.getIPAddress();
    html += F("</div><div><b>Red conectada:</b> ");
    html += wifiManager.getSSID();
    html += F("</div><div><b>Señal RSSI:</b> ");
    html += String(wifiManager.getRSSI()) + " dBm";
    html += F("</div></div>"
              "<div class='card'>"
              "<div><b>Servidor de Telemetría (PC / Nube):</b></div>"
              "<div style='word-break:break-all;color:#34d399;margin:6px 0;font-family:monospace;font-size:12px;'>");
    html += telemetryClient.getServerUrl();
    html += F("</div><div><b>Estado envío:</b> ");
    html += (telemetryClient.isConnectedToServer() ? "<span style='color:#34d399;'>Conectado (HTTP 200)</span>" : "<span style='color:#f87171;'>Pendiente de servidor</span>");
    html += F("</div><div><b>Pushes exitosos:</b> ");
    html += String(telemetryClient.getSuccessfulPushes());
    html += F("</div>"
              "<form method='POST' action='/api/server-url' style='margin-top:10px;'>"
              "<input type='text' name='url' placeholder='http://192.168.20.41:5000/api/telemetry' value='");
    html += telemetryClient.getServerUrl();
    html += F("'>"
              "<button type='submit' class='btn btn-outline'>Actualizar URL del Servidor</button>"
              "</form></div>"
              "<a href='http://192.168.20.41:5000' target='_blank' class='btn'>Abrir Plataforma Web en tu PC</a>"
              "<hr>"
              "<div class='card'>"
              "<div><b>Configuración de Red Wi-Fi:</b></div>"
              "<form method='POST' action='/wifi/save' style='margin-top:8px;'>"
              "<input type='text' name='ssid' placeholder='Nombre de red Wi-Fi (SSID)' required>"
              "<input type='password' name='pass' placeholder='Contraseña de Wi-Fi'>"
              "<button type='submit' class='btn btn-outline'>Guardar y Conectar Wi-Fi</button>"
              "</form></div>"
              "<div style='display:flex;gap:8px;margin-top:10px;'>"
              "<a href='/api/data' target='_blank' class='btn btn-outline' style='font-size:12px;'>Ver API JSON</a>"
              "<a href='/api/history' target='_blank' class='btn btn-outline' style='font-size:12px;'>Historial JSON</a>"
              "</div></div></body></html>");

    server.send(200, "text/html; charset=utf-8", html);
}

void WebServerManager::handleServerUrl() {
    sendCorsHeaders();
    if (server.method() == HTTP_POST) {
        String newUrl = server.arg("url");
        if (newUrl.length() > 0 && (newUrl.startsWith("http://") || newUrl.startsWith("https://"))) {
            telemetryClient.setServerUrl(newUrl);
            String html = "<html><body style='font-family:sans-serif;background:#0b130e;color:#ecfdf5;text-align:center;padding:40px;'>"
                          "<h2 style='color:#34d399;'>URL de Telemetria Actualizada</h2>"
                          "<p>Los datos ahora se enviaran a: <b>" + newUrl + "</b></p>"
                          "<a href='/' style='color:#6ee7b7;'>Volver</a>"
                          "</body></html>";
            server.send(200, "text/html", html);
            return;
        }
        server.send(400, "application/json", "{\"success\":false,\"error\":\"URL invalida. Debe iniciar con http:// o https://\"}");
        return;
    }

    String json = "{\"url\":\"" + telemetryClient.getServerUrl() + "\",\"connected\":" + 
                  (telemetryClient.isConnectedToServer() ? "true" : "false") + 
                  ",\"successfulPushes\":" + String(telemetryClient.getSuccessfulPushes()) + 
                  ",\"failedPushes\":" + String(telemetryClient.getFailedPushes()) + "}";
    server.send(200, "application/json", json);
}

void WebServerManager::handleLcdControl() {
    sendCorsHeaders();
    String action = server.arg("action");
    
    if (action == "next") {
        displayManager.nextScreen();
    } else if (action == "prev") {
        displayManager.prevScreen();
    } else if (action == "screen") {
        int id = server.arg("id").toInt();
        displayManager.showScreen(id);
    } else if (action == "autocycle") {
        bool current = displayManager.isAutoCycleEnabled();
        displayManager.setAutoCycle(!current);
    } else if (action == "backlight") {
        if (!displayManager.isBacklightEnabled()) {
            displayManager.wakeUpBacklight(LCD_AUTO_OFF_WEB_MS);
        } else {
            displayManager.setBacklight(false);
        }
    }

    String json = "{\"success\":true,\"currentScreen\":" + String(displayManager.getCurrentScreen()) + 
                  ",\"autoCycle\":" + (displayManager.isAutoCycleEnabled() ? "true" : "false") + 
                  ",\"backlight\":" + (displayManager.isBacklightEnabled() ? "true" : "false") + 
                  ",\"remainingSec\":" + String(displayManager.getRemainingBacklightSec()) + "}";
    server.send(200, "application/json", json);
}

void WebServerManager::handleWifiSave() {
    String newSsid = server.arg("ssid");
    String newPass = server.arg("pass");

    if (newSsid.length() > 0) {
        String resp = "<html><body style='font-family:sans-serif;text-align:center;padding:50px;background:#0b130e;color:#ecfdf5;'>"
                      "<h2 style='color:#34d399;'>Guardando red en memoria flash...</h2>"
                      "<p>Conectando a: <b>" + newSsid + "</b></p>"
                      "<p>Verifica la pantalla LCD o el monitor serial para la nueva IP asignada.</p>"
                      "</body></html>";
        server.send(200, "text/html", resp);
        delay(500);
        wifiManager.saveCredentials(newSsid, newPass);
    } else {
        server.send(400, "text/plain", "El nombre de la red (SSID) no puede estar vacio");
    }
}

void WebServerManager::handleWifiReset() {
    String resp = "<html><body style='font-family:sans-serif;text-align:center;padding:50px;background:#0b130e;color:#ecfdf5;'>"
                  "<h2 style='color:#fbbf24;'>Restaurando a Modo Punto de Acceso...</h2>"
                  "<p>Se han borrado las credenciales guardadas en la memoria flash.</p>"
                  "<p>La estacion creara la red <b>" + String(AP_SSID) + "</b> (IP: 192.168.4.1).</p>"
                  "</body></html>";
    server.send(200, "text/html", resp);
    delay(500);
    wifiManager.resetWiFiSettings();
}

void WebServerManager::handleApiData() {
    sendCorsHeaders();
    String json = telemetryClient.buildJsonPayload(sensorManager.getData());
    server.send(200, "application/json", json);
}

void WebServerManager::handleTimeSync() {
    sendCorsHeaders();
    if (server.hasArg("epoch")) {
        time_t epoch = (time_t)server.arg("epoch").toInt();
        if (epoch > 100000) {
            timeval tv = { epoch, 0 };
            settimeofday(&tv, nullptr);
            Serial.printf("[NTP/RTC] Reloj sincronizado con el cliente web: epoch %lld\n", (long long)epoch);
            server.send(200, "application/json", "{\"success\":true,\"synced\":true,\"epoch\":" + String((unsigned long)epoch) + "}");
            return;
        }
    }
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Parametro epoch invalido\"}");
}

void WebServerManager::handleApiHistory() {
    sendCorsHeaders();
    int count = sensorManager.getHistoryCount();
    String json = "{\"count\":" + String(count) + ",\"samples\":[";
    for (int i = 0; i < count; i++) {
        const HistoricalSample& s = sensorManager.getHistorySample(i);
        char timeBuf[20];
        time_t t = s.timestamp;
        struct tm* tm_info = localtime(&t);
        if (t > 100000 && tm_info != nullptr) {
            strftime(timeBuf, sizeof(timeBuf), "%H:%M", tm_info);
        } else {
            snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", (unsigned int)((s.uptimeSec / 60) % 24), (unsigned int)(s.uptimeSec % 60));
        }

        json += "{\"time\":\"" + String(timeBuf) + "\",";
        json += "\"timestamp\":" + String(s.timestamp) + ",";
        json += "\"temp\":" + String(s.ambientTemp, 1) + ",";
        json += "\"humidity\":" + String(s.ambientHum, 1) + ",";
        json += "\"rain\":" + String(s.rainAccumulated, 1) + ",";
        json += "\"wind\":" + String(s.windSpeed, 1) + ",";
        json += "\"radiation\":" + String(s.solarRadiation, 0) + ",";
        json += "\"soilMoist\":" + String(s.moisture, 1) + ",";
        json += "\"soilTemp\":" + String(s.temp, 1) + ",";
        json += "\"ec\":" + String(s.ec, 0) + ",";
        json += "\"ph\":" + String(s.ph, 1) + ",";
        json += "\"n\":" + String(s.n, 0) + ",";
        json += "\"p\":" + String(s.p, 0) + ",";
        json += "\"k\":" + String(s.k, 0) + "}";
        if (i < count - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}
