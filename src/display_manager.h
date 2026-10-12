#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "sensor_manager.h"

enum ScreenView {
    SCREEN_SYSTEM = 0,
    SCREEN_AMBIENT = 1,
    SCREEN_SOIL_1_2 = 2,
    SCREEN_SOIL_3_4 = 3,
    SCREEN_IRRIGATION = 4,
    SCREEN_DIAGNOSTIC = 5,
    SCREEN_COUNT = 6
};

class DisplayManager {
public:
    DisplayManager();
    bool begin();
    void update(const StationData& data, const String& wifiStatus, const String& ipAddress);
    
    void showScreen(int screenIndex);
    void nextScreen();
    void prevScreen();
    
    void setAutoCycle(bool enable);
    bool isAutoCycleEnabled() const { return autoCycle; }
    
    void setBacklight(bool enable);
    void wakeUpBacklight(unsigned long durationMs = 600000UL);
    bool isBacklightEnabled() const { return backlightOn; }
    unsigned long getRemainingBacklightSec() const;
    
    int getCurrentScreen() const { return currentScreen; }
    uint8_t getDetectedI2CAddress() const { return i2cAddress; }
    bool isLcdReady() const { return lcdReady; }

private:
    LiquidCrystal_I2C* lcd;
    uint8_t i2cAddress;
    bool lcdReady;
    int currentScreen;
    bool autoCycle;
    bool backlightOn;
    unsigned long lastCycleMillis;
    unsigned long lastRenderMillis;
    unsigned long backlightOnStartMillis;
    unsigned long backlightTimeoutDuration;
    
    uint8_t scanI2CAddress();
    void renderCurrentScreen(const StationData& data, const String& wifiStatus, const String& ipAddress);
    void loadCustomCharacters();
};

extern DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
