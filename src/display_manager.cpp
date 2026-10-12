#include "display_manager.h"
#include "config.h"

DisplayManager displayManager;

// Icono personalizado: Símbolo de Grado centígrado (°)
static byte customDegree[8] = {
    0b01100,
    0b10010,
    0b10010,
    0b01100,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

// Icono personalizado: Gota de agua (humedad)
static byte customDrop[8] = {
    0b00100,
    0b00100,
    0b01010,
    0b01010,
    0b10001,
    0b10001,
    0b01110,
    0b00000
};

// Icono personalizado: Antena / Wi-Fi
static byte customWiFi[8] = {
    0b00000,
    0b01110,
    0b10001,
    0b00100,
    0b01010,
    0b00000,
    0b00100,
    0b00000
};

DisplayManager::DisplayManager()
    : lcd(nullptr),
      i2cAddress(LCD_DEFAULT_ADDR),
      lcdReady(false),
      currentScreen(SCREEN_SYSTEM),
      autoCycle(true),
      backlightOn(true),
      lastCycleMillis(0),
      lastRenderMillis(0),
      backlightOnStartMillis(0),
      backlightTimeoutDuration(LCD_AUTO_OFF_INITIAL_MS) {}

uint8_t DisplayManager::scanI2CAddress() {
    Serial.println(F("[I2C] Escaneando bus I2C (SDA=D2, SCL=D1)..."));
    uint8_t detected = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("[I2C] Dispositivo detectado en direccion 0x%02X\n", addr);
            // Priorizamos las direcciones tipicas de expansores PCF8574
            if (addr == 0x27 || addr == 0x3F) {
                detected = addr;
                break;
            } else if (detected == 0) {
                detected = addr;
            }
        }
    }

    if (detected == 0) {
        Serial.println(F("[I2C] ADVERTENCIA: No se encontro ningun dispositivo I2C."));
        Serial.println(F("[I2C] Revisa conexion fisica: GND, VCC (3.3V o 5V), SDA->D2, SCL->D1."));
        return LCD_DEFAULT_ADDR; // fallback
    }

    Serial.printf("[LCD] Asignando direccion I2C: 0x%02X\n", detected);
    return detected;
}

bool DisplayManager::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    delay(100);

    i2cAddress = scanI2CAddress();
    lcd = new LiquidCrystal_I2C(i2cAddress, LCD_COLS, LCD_ROWS);

    if (lcd != nullptr) {
        lcd->init();
        lcd->backlight();
        backlightOn = true;
        backlightOnStartMillis = millis();
        backlightTimeoutDuration = LCD_AUTO_OFF_INITIAL_MS; // 2 min inicial
        lcdReady = true;

        loadCustomCharacters();

        // Pantalla de inicio
        lcd->clear();
        lcd->setCursor(0, 0);
        lcd->print("VERAMET ATUM v2 ");
        lcd->setCursor(0, 1);
        lcd->print("Tecnologia Campo");
        delay(1200);

        return true;
    }

    return false;
}

void DisplayManager::loadCustomCharacters() {
    if (!lcdReady || lcd == nullptr) return;
    lcd->createChar(0, customDegree);
    lcd->createChar(1, customDrop);
    lcd->createChar(2, customWiFi);
}

void DisplayManager::setBacklight(bool enable) {
    backlightOn = enable;
    if (lcdReady && lcd != nullptr) {
        if (enable) lcd->backlight();
        else lcd->noBacklight();
    }
}

void DisplayManager::wakeUpBacklight(unsigned long durationMs) {
    backlightTimeoutDuration = durationMs;
    backlightOnStartMillis = millis();
    setBacklight(true);
    Serial.printf("[LCD] Luz activada por %lu minutos para consulta.\n", durationMs / 60000UL);
}

unsigned long DisplayManager::getRemainingBacklightSec() const {
    if (!backlightOn || backlightTimeoutDuration == 0) return 0;
    unsigned long elapsed = millis() - backlightOnStartMillis;
    if (elapsed >= backlightTimeoutDuration) return 0;
    return (backlightTimeoutDuration - elapsed) / 1000;
}

void DisplayManager::setAutoCycle(bool enable) {
    autoCycle = enable;
}

void DisplayManager::showScreen(int screenIndex) {
    if (screenIndex >= 0 && screenIndex < SCREEN_COUNT) {
        currentScreen = screenIndex;
        lastCycleMillis = millis(); // reinicia temporizador
        lastRenderMillis = 0;       // fuerza render inmediato
    }
}

void DisplayManager::nextScreen() {
    currentScreen = (currentScreen + 1) % SCREEN_COUNT;
    lastCycleMillis = millis();
    lastRenderMillis = 0;
}

void DisplayManager::prevScreen() {
    currentScreen = (currentScreen - 1 + SCREEN_COUNT) % SCREEN_COUNT;
    lastCycleMillis = millis();
    lastRenderMillis = 0;
}

void DisplayManager::update(const StationData& data, const String& wifiStatus, const String& ipAddress) {
    if (!lcdReady || lcd == nullptr) return;

    unsigned long now = millis();

    // Ahorro de energía de la pantalla: auto-apagado según temporizador
    if (backlightOn && backlightTimeoutDuration > 0) {
        if (now - backlightOnStartMillis >= backlightTimeoutDuration) {
            Serial.println(F("[LCD] Temporizador de luz cumplido. Apagando LCD para ahorro de bateria."));
            setBacklight(false);
        }
    }

    // Rotacion automatica si esta habilitada
    if (autoCycle && (now - lastCycleMillis >= LCD_CAROUSEL_TIME_MS)) {
        lastCycleMillis = now;
        currentScreen = (currentScreen + 1) % SCREEN_COUNT;
        lastRenderMillis = 0;
    }

    // Refrescar contenido cada 500 ms o cuando cambia de pantalla
    if (now - lastRenderMillis >= 500) {
        lastRenderMillis = now;
        renderCurrentScreen(data, wifiStatus, ipAddress);
    }
}

static void printPadded(LiquidCrystal_I2C* lcd, uint8_t row, const String& text) {
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%-16.16s", text.c_str());
    lcd->setCursor(0, row);
    lcd->print(buffer);
}

void DisplayManager::renderCurrentScreen(const StationData& data, const String& wifiStatus, const String& ipAddress) {
    char line1[20];
    char line2[20];

    switch (currentScreen) {
        case SCREEN_SYSTEM: {
            // Pantalla 0: Identificación y Estado General
            snprintf(line1, sizeof(line1), "VERAMET [%d/%d]", currentScreen + 1, SCREEN_COUNT);
            snprintf(line2, sizeof(line2), "IP:%s", ipAddress.c_str());
            printPadded(lcd, 0, line1);
            printPadded(lcd, 1, line2);
            break;
        }

        case SCREEN_AMBIENT: {
            // Pantalla 1: Clima Ambiental (SHT31 Simulado)
            // L1: "T:32.5C  H:54%"
            // L2: "Ambiente [SIM]"
            snprintf(line1, sizeof(line1), "T:%.1f\xDF""C H:%.0f%%", data.ambientTemp, data.ambientHum);
            snprintf(line2, sizeof(line2), "Ambiente [SIM]");
            printPadded(lcd, 0, line1);
            // insertamos caracter personalizado de grado si es posible
            lcd->setCursor(6, 0);
            lcd->write(byte(0)); // grado
            printPadded(lcd, 1, line2);
            break;
        }

        case SCREEN_SOIL_1_2: {
            // Pantalla 2: Sondas de Suelo 1 y 2 (Lote A)
            // L1: "S1:32.0% S2:28.5%"
            // L2: "Suelo Lote A"
            snprintf(line1, sizeof(line1), "S1:%.0f%%  S2:%.0f%%", 
                     data.soilProbes[0].moisture, data.soilProbes[1].moisture);
            snprintf(line2, sizeof(line2), "Suelo Lote A-RS485");
            printPadded(lcd, 0, line1);
            printPadded(lcd, 1, line2);
            break;
        }

        case SCREEN_SOIL_3_4: {
            // Pantalla 3: Sondas de Suelo 3 y 4 (Lote B)
            snprintf(line1, sizeof(line1), "S3:%.0f%%  S4:%.0f%%", 
                     data.soilProbes[2].moisture, data.soilProbes[3].moisture);
            snprintf(line2, sizeof(line2), "Suelo Lote B-RS485");
            printPadded(lcd, 0, line1);
            printPadded(lcd, 1, line2);
            break;
        }

        case SCREEN_IRRIGATION: {
            // Pantalla 4: Recomendación / Estado de Riego
            snprintf(line1, sizeof(line1), "ESTADO DE RIEGO");
            snprintf(line2, sizeof(line2), "%s", data.irrigationStatus.c_str());
            printPadded(lcd, 0, line1);
            printPadded(lcd, 1, line2);
            break;
        }

        case SCREEN_DIAGNOSTIC: {
            // Pantalla 5: Uptime y Conectividad
            unsigned long sec = millis() / 1000;
            unsigned int hrs = sec / 3600;
            unsigned int mins = (sec % 3600) / 60;
            unsigned int s = sec % 60;

            snprintf(line1, sizeof(line1), "Up:%02uh:%02um:%02us", hrs, mins, s);
            snprintf(line2, sizeof(line2), "WiFi: %s", wifiStatus.c_str());
            printPadded(lcd, 0, line1);
            printPadded(lcd, 1, line2);
            break;
        }
    }
}
