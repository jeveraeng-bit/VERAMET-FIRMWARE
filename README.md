# VERAMET ATUM — Firmware de Estación Agroclimática

Firmware IoT para la estación meteorológica agrícola **VERAMET ATUM**. Desarrollado con el framework Arduino en C++ sobre plataforma **ESP8266 (NodeMCU v2 / ESP-12E)** y con compatibilidad para **ESP32**.

Creado por: **Juan Esteban Vera Rodríguez** (Estudiante de 8vo semestre de Ingeniería Electrónica, 20 años).

---

## 🛠️ Arquitectura de Hardware y Pines

* **Microcontrolador:** NodeMCU v2 (ESP8266EX @ 80/160 MHz, 4MB Flash).
* **Pantalla:** LCD 1602 con módulo de comunicación I2C (PCF8574).
  * `SDA`: GPIO 4 (`D2` en NodeMCU)
  * `SCL`: GPIO 5 (`D1` en NodeMCU)
  * Detección automática de dirección I2C (`0x27` o `0x3F`).
* **Sensores Integrados y Compatibles:**
  * SHT31 / DHT22 (Temperatura y Humedad Ambiente).
  * Pluviómetro de balancín (Lluvia acumulada en mm).
  * Anemómetro digital (Velocidad del viento en km/h y veleta de dirección).
  * Piranómetro / Sensor de Radiación Solar (W/m² y cálculo de horas de sol).
  * **Sondas de Suelo 8 en 1 Modbus RS485:**
    * Humedad volumétrica (%), Temperatura del suelo (°C), Conductividad Eléctrica (µS/cm), pH, Nitrógeno (mg/kg), Fósforo (mg/kg), Potasio (mg/kg).

---

## ⚡ Características del Firmware

1. **Cliente de Telemetría HTTP:**
   * Empaqueta las lecturas de los sensores en formato JSON y realiza peticiones periódicas `HTTP POST` hacia la URL del backend local (`http://192.168.20.41:5000/api/telemetry`) o servidor en la nube.
   * La URL es reconfigurable y se almacena en memoria Flash no volátil (**EEPROM**).
2. **Servidor Web Interno Desacoplado:**
   * Mini portal de estado y configuración en el puerto 80.
   * Soporte completo de encabezados CORS para consumo directo por la plataforma web.
   * Endpoints REST: `/api/data`, `/api/history`, `/api/time/sync`, `/api/lcd`, `/api/server-url`.
3. **Gestor Wi-Fi con Respaldo Automático:**
   * Intenta conectarse a la red Wi-Fi configurada (Modo STA).
   * Si no hay red disponible, activa automáticamente el modo Punto de Acceso (Modo AP: `VERAMET-Agricola-AP`, IP: `192.168.4.1`) con portal cautivo.
4. **Sincronización Horaria Dual:**
   * Sincronización NTP automática con servidores horarios de internet (`pool.ntp.org`) en zona horaria Colombia (**UTC-5**).
   * Sincronización por navegador (`/api/time/sync?epoch=...`) para garantizar hora atómica exacta incluso en zonas rurales sin conexión a internet.
5. **Gestión Inteligente de Energía de la Pantalla LCD:**
   * Auto-apagado de retroiluminación a los 2 minutos de encendido para ahorrar batería.
   * Reactivación bajo demanda por 10 minutos desde el software web.

---

## 📦 Compilación y Flasheo con PlatformIO

### Requisitos
* [PlatformIO Core](https://platformio.org/) o extensión de PlatformIO en VS Code.

### Comandos
```powershell
# Compilar firmware
pio run

# Flashear al microcontrolador conectado por USB
pio run -t upload

# Abrir monitor serie a 115200 baudios
pio device monitor
```

---

## 👨‍💻 Autor
**Juan Esteban Vera Rodríguez**  
Estudiante de Ingeniería Electrónica (8vo Semestre)  
Ibagué, Tolima, Colombia  
*VERAMET — Tecnología para el campo*
