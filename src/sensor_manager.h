#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

#define MAX_HISTORY_SAMPLES 24

struct SoilProbe8in1 {
    float moisture;      // % Humedad volumétrica (VWC)
    float temperature;   // °C Temperatura del suelo
    float ec;            // µS/cm Conductividad eléctrica (salinidad)
    float ph;            // pH del suelo (ideal 5.8 - 6.8 según Corpoica)
    float nitrogen;      // N (mg/kg)
    float phosphorus;    // P (mg/kg)
    float potassium;     // K (mg/kg)
    float tensionCb;     // Centibares de succión (Tabla 14 Corpoica: 0-10 saturado, 10-30 húmedo, 30-60 inicio riego)
    bool valid;          // Lectura válida Modbus RS485
};

struct HistoricalSample {
    time_t timestamp;
    uint32_t uptimeSec;
    float moisture;
    float temp;
    float ec;
    float ph;
    float n;
    float p;
    float k;
    float tensionCb;
    float ambientTemp;
    float ambientHum;
    float rainAccumulated;
    float solarRadiation;
    float windSpeed;
};

struct StationData {
    // Ambiente (SHT31 + Piranómetro + Pluviómetro)
    float ambientTemp;
    float ambientHum;
    float solarRadiation; // W/m² radiación solar
    float solarHours;     // Horas de brillo solar (promedio 5.8 a 6.0 h/día en El Espinal)
    float rainAccumulated;// mm precipitación

    // 4 Sondas de Suelo (Arquitectura 8-en-1 Modbus RS485)
    SoilProbe8in1 soilProbes[4];

    // Diagnóstico y muestreo de 15 minutos
    bool isSimulated;
    unsigned long lastUpdate;
    unsigned long lastLogMillis;
    unsigned long nextLogSec;
    String irrigationStatus;
};

class SensorManager {
public:
    SensorManager();
    void begin();
    void update();
    
    const StationData& getData() const { return data; }
    void setSimulatedProbe(int index, float moisture, float temp);
    
    // Muestreo bajo demanda (instante) o cada 15 min
    void takeInstantSample();
    int getHistoryCount() const { return historyCount; }
    const HistoricalSample& getHistorySample(int index) const { return history[index]; }

private:
    StationData data;
    unsigned long lastUpdateMillis;
    HistoricalSample history[MAX_HISTORY_SAMPLES];
    int historyCount;
    int historyHead;

    void simulateReadings();
    void evaluateIrrigationStatus();
    void logSample();
};

extern SensorManager sensorManager;

#endif // SENSOR_MANAGER_H
