#include "sensor_manager.h"
#include "config.h"

SensorManager sensorManager;

SensorManager::SensorManager()
    : lastUpdateMillis(0),
      historyCount(0),
      historyHead(0) {
    data.isSimulated = true;
    data.ambientTemp = 33.2f;
    data.ambientHum = 52.0f;
    data.solarRadiation = 780.0f; // W/m² en El Espinal
    data.solarHours = 5.9f;       // Horas de sol promedio Tolima (Ideam)
    data.rainAccumulated = 0.0f;
    data.irrigationStatus = "Humedad Optima";
    data.lastLogMillis = 0;
    data.nextLogSec = 900; // 15 minutos

    // Sonda 1: Lote A - 20 cm (Estrato radicular 0-30 cm)
    data.soilProbes[0] = {32.4f, 26.8f, 320.0f, 6.2f, 22.0f, 13.0f, 125.0f, 24.0f, true};

    // Sonda 2: Lote A - 40 cm (Estrato profundo / infiltración)
    data.soilProbes[1] = {28.1f, 27.2f, 340.0f, 6.1f, 19.0f, 12.0f, 118.0f, 30.0f, true};

    // Sonda 3: Lote B - 20 cm (Zona más expuesta / suelo franco-arenoso)
    data.soilProbes[2] = {21.0f, 28.5f, 280.0f, 5.9f, 16.0f, 10.0f, 105.0f, 48.0f, true};

    // Sonda 4: Lote B - 40 cm (Reserva profunda)
    data.soilProbes[3] = {19.2f, 29.1f, 295.0f, 5.8f, 15.0f, 9.0f, 102.0f, 56.0f, true};
}

void SensorManager::begin() {
    randomSeed(micros());
    simulateReadings();
    evaluateIrrigationStatus();
    takeInstantSample(); // Primera muestra al encender
}

void SensorManager::update() {
    unsigned long currentMillis = millis();

    // Actualización de oscilaciones dinámicas (cada 3 segundos)
    if (currentMillis - lastUpdateMillis >= SENSOR_UPDATE_INTERVAL_MS) {
        lastUpdateMillis = currentMillis;
        simulateReadings();
        evaluateIrrigationStatus();
    }

    // Cuenta regresiva y muestreo histórico cada 15 minutos (900,000 ms)
    unsigned long elapsedSinceLog = currentMillis - data.lastLogMillis;
    if (elapsedSinceLog >= DATA_LOG_INTERVAL_MS) {
        data.lastLogMillis = currentMillis;
        logSample();
        data.nextLogSec = DATA_LOG_INTERVAL_MS / 1000;
        Serial.println(F("[METEO-LOG] Muestra automatica de 15 minutos registrada en memoria."));
    } else {
        data.nextLogSec = (DATA_LOG_INTERVAL_MS - elapsedSinceLog) / 1000;
    }
}

void SensorManager::takeInstantSample() {
    logSample();
    data.lastLogMillis = millis();
    data.nextLogSec = DATA_LOG_INTERVAL_MS / 1000;
    Serial.println(F("[METEO-LOG] Muestra INSTANTANEA bajo demanda registrada con exito."));
}

void SensorManager::logSample() {
    HistoricalSample s;
    s.timestamp = time(nullptr);
    s.uptimeSec = millis() / 1000;
    s.moisture = data.soilProbes[0].moisture;
    s.temp = data.soilProbes[0].temperature;
    s.ec = data.soilProbes[0].ec;
    s.ph = data.soilProbes[0].ph;
    s.n = data.soilProbes[0].nitrogen;
    s.p = data.soilProbes[0].phosphorus;
    s.k = data.soilProbes[0].potassium;
    s.tensionCb = data.soilProbes[0].tensionCb;
    s.ambientTemp = data.ambientTemp;
    s.ambientHum = data.ambientHum;
    s.rainAccumulated = data.rainAccumulated;
    s.solarRadiation = data.solarRadiation;
    s.windSpeed = 8.6f;

    history[historyHead] = s;
    historyHead = (historyHead + 1) % MAX_HISTORY_SAMPLES;
    if (historyCount < MAX_HISTORY_SAMPLES) {
        historyCount++;
    }
}

void SensorManager::simulateReadings() {
    // Variaciones climáticas Tolima/Huila
    float tempDelta = (random(-8, 9) / 100.0f);
    data.ambientTemp += tempDelta;
    if (data.ambientTemp < 28.0f) data.ambientTemp = 28.5f;
    if (data.ambientTemp > 38.0f) data.ambientTemp = 37.5f;

    float humDelta = (random(-12, 13) / 100.0f);
    data.ambientHum += humDelta;
    if (data.ambientHum < 40.0f) data.ambientHum = 41.0f;
    if (data.ambientHum > 75.0f) data.ambientHum = 74.0f;

    float solDelta = (random(-15, 16) / 2.0f);
    data.solarRadiation += solDelta;
    if (data.solarRadiation < 400.0f) data.solarRadiation = 450.0f;
    if (data.solarRadiation > 1050.0f) data.solarRadiation = 1000.0f;

    // Sondas 8 en 1
    for (int i = 0; i < 4; i++) {
        float soilDelta = (random(-4, 5) / 100.0f);
        data.soilProbes[i].moisture += soilDelta;
        if (data.soilProbes[i].moisture < 12.0f) data.soilProbes[i].moisture = 14.0f;
        if (data.soilProbes[i].moisture > 45.0f) data.soilProbes[i].moisture = 42.0f;

        // Tensión en centibares inversamente proporcional a la humedad (Tabla 14 Corpoica)
        data.soilProbes[i].tensionCb = (45.0f - data.soilProbes[i].moisture) * 2.2f;
        if (data.soilProbes[i].tensionCb < 5.0f) data.soilProbes[i].tensionCb = 6.0f;
    }

    data.lastUpdate = millis();
}

void SensorManager::evaluateIrrigationStatus() {
    // Diagnóstico según tensión en centibares y humedad promedio (Corpoica Tabla 14)
    float avgTension = 0;
    int validCount = 0;
    for (int i = 0; i < 4; i++) {
        if (data.soilProbes[i].valid) {
            avgTension += data.soilProbes[i].tensionCb;
            validCount++;
        }
    }
    if (validCount > 0) avgTension /= validCount;

    if (avgTension <= 15.0f) {
        data.irrigationStatus = "Suelo Saturado";
    } else if (avgTension <= 30.0f) {
        data.irrigationStatus = "Humedad Optima";
    } else if (avgTension <= 60.0f) {
        data.irrigationStatus = "Iniciar Riego";
    } else {
        data.irrigationStatus = "Deficit Hidrico Critico";
    }
}

void SensorManager::setSimulatedProbe(int index, float moisture, float temp) {
    if (index >= 0 && index < 4) {
        data.soilProbes[index].moisture = moisture;
        data.soilProbes[index].temperature = temp;
        data.soilProbes[index].valid = true;
        evaluateIrrigationStatus();
    }
}
