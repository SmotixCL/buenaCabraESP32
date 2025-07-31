/**
 * ============================================================================
 * COLLAR GEOFENCING - IMPLEMENTACIÓN DISPLAY MANAGER
 * ============================================================================
 * 
 * @file DisplayManager.cpp
 * @version 3.0
 */

#include "DisplayManager.h"
#include <Wire.h>

// Variables estáticas
SSD1306Wire* DisplayManager::display = nullptr;
bool DisplayManager::initialized = false;
bool DisplayManager::enabled = true;
uint32_t DisplayManager::last_update = 0;

bool DisplayManager::init() {
    DEBUG_INFO("📺 Inicializando display OLED...");
    
    // Crear instancia del display
    display = new SSD1306Wire(OLED_ADDRESS, OLED_SDA, OLED_SCL);
    
    if (!display) {
        DEBUG_ERROR("❌ Error creando instancia del display");
        return false;
    }
    
    // Reset sequence del OLED
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(50);
    digitalWrite(OLED_RST, HIGH);
    delay(50);
    
    // Inicializar display
    if (!display->init()) {
        DEBUG_ERROR("❌ Error inicializando display OLED");
        delete display;
        display = nullptr;
        return false;
    }
    
    // Configuración inicial
    display->displayOn();
    display->clear();
    display->flipScreenVertically();
    display->setFont(ArialMT_Plain_10);
    display->setBrightness(128);  // 50% brillo inicial
    
    initialized = true;
    enabled = true;
    
    DEBUG_INFO("✅ Display OLED inicializado correctamente");
    
    // Mostrar pantalla de inicio
    showBootScreen();
    
    return true;
}

bool DisplayManager::isInitialized() {
    return initialized && display != nullptr;
}

void DisplayManager::deinit() {
    if (display) {
        display->displayOff();
        delete display;
        display = nullptr;
    }
    
    initialized = false;
    enabled = false;
    
    DEBUG_INFO("📺 Display desinicializado");
}

void DisplayManager::clear() {
    if (!isInitialized() || !enabled) return;
    display->clear();
}

void DisplayManager::display() {
    if (!isInitialized() || !enabled) return;
    display->display();
    last_update = millis();
}

void DisplayManager::turnOn() {
    if (!isInitialized()) return;
    display->displayOn();
    enabled = true;
}

void DisplayManager::turnOff() {
    if (!isInitialized()) return;
    display->displayOff();
    enabled = false;
}

void DisplayManager::setBrightness(uint8_t brightness) {
    if (!isInitialized()) return;
    display->setBrightness(brightness);
}

void DisplayManager::showBootScreen() {
    if (!isInitialized()) return;
    
    clear();
    
    // Título principal
    display->setFont(ArialMT_Plain_16);
    display->drawString(5, 0, "Collar V3.0");
    
    // Información del hardware
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 20, "ESP32-S3 + SX1262");
    display->drawString(0, 32, "OLED V3 - Iniciado");
    display->drawString(0, 44, "Sistema cargando...");
    
    // Barra de progreso simulada
    drawProgressBar(0, 56, 128, 8, 100);
    
    display();
    delay(2000);
}

void DisplayManager::showMainScreen(const Position& pos, const BatteryStatus& battery, const HardwareStatus& hw) {
    if (!isInitialized() || !enabled) return;
    
    clear();
    display->setFont(ArialMT_Plain_10);
    
    // Línea 1: Estado del sistema
    String status = hw.radio_initialized ? "Radio: OK" : "Radio: INIT";
    display->drawString(0, 0, status);
    
    // Icono de batería en la esquina superior derecha
    drawBatteryIcon(100, 0, battery.percentage);
    display->drawString(110, 0, String(battery.percentage) + "%");
    
    // Línea 2: Coordenadas GPS
    if (pos.valid) {
        display->drawString(0, 12, "Lat: " + String(pos.latitude, 4));
        display->drawString(0, 24, "Lng: " + String(pos.longitude, 4));
        drawGPSIcon(100, 12, true);
    } else {
        display->drawString(0, 12, "GPS: Buscando...");
        drawGPSIcon(100, 12, false);
    }
    
    // Línea 4: Información adicional
    display->drawString(0, 36, "Sats: " + String(pos.satellites));
    display->drawString(64, 36, "HDOP: " + String(pos.hdop, 1));
    
    // Línea 5: Estado general
    String system_status = "Sistema: ";
    switch (hw.system_state) {
        case SYSTEM_RUNNING: system_status += "OK"; break;
        case SYSTEM_LOW_POWER: system_status += "ECO"; break;
        case SYSTEM_ERROR: system_status += "ERROR"; break;
        default: system_status += "INIT"; break;
    }
    display->drawString(0, 48, system_status);
    
    // Indicador de actividad (parpadeo)
    if (millis() % 2000 < 1000) {
        display->drawString(118, 48, "●");
    }
    
    display();
}

void DisplayManager::showAlertScreen(AlertLevel level, float distance) {
    if (!isInitialized() || !enabled) return;
    
    clear();
    
    // Título parpadeante para alertas críticas
    bool blink = (level >= ALERT_DANGER && (millis() % 1000 < 500));
    
    if (!blink || level < ALERT_DANGER) {
        display->setFont(ArialMT_Plain_16);
        
        String alert_text;
        switch (level) {
            case ALERT_CAUTION: alert_text = "PRECAUCION"; break;
            case ALERT_WARNING: alert_text = "ADVERTENCIA"; break;
            case ALERT_DANGER: alert_text = "PELIGRO"; break;
            case ALERT_EMERGENCY: alert_text = "EMERGENCIA"; break;
            default: alert_text = "SEGURO"; break;
        }
        
        // Centrar texto
        int text_width = alert_text.length() * 10;  // Aproximación
        int x = (128 - text_width) / 2;
        display->drawString(x, 5, alert_text);
    }
    
    // Información de distancia
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 25, "Distancia al limite:");
    display->drawString(0, 37, String(distance, 1) + " metros");
    
    // Barra de progreso visual de proximidad
    int danger_percent = 100;
    if (distance > 0) {
        danger_percent = constrain(100 - (distance * 10), 0, 100);
    }
    
    drawProgressBar(0, 50, 128, 10, danger_percent);
    
    display();
}

void DisplayManager::showGPSScreen(const Position& pos) {
    if (!isInitialized() || !enabled) return;
    
    clear();
    display->setFont(ArialMT_Plain_10);
    
    // Título
    display->drawString(0, 0, "GPS Status");
    drawGPSIcon(100, 0, pos.valid);
    
    // Coordenadas detalladas
    if (pos.valid) {
        display->drawString(0, 12, "Lat: " + String(pos.latitude, 6));
        display->drawString(0, 24, "Lng: " + String(pos.longitude, 6));
        display->drawString(0, 36, "Alt: " + String(pos.altitude, 1) + "m");
        
        display->drawString(64, 12, "Sats: " + String(pos.satellites));
        display->drawString(64, 24, "HDOP: " + String(pos.hdop, 2));
        
        // Tiempo desde última actualización
        uint32_t age = (millis() - pos.timestamp) / 1000;
        display->drawString(0, 48, "Age: " + String(age) + "s");
    } else {
        display->drawString(0, 20, "GPS no disponible");
        display->drawString(0, 32, "Buscando satelites...");
        
        // Animación de búsqueda
        int dots = (millis() / 500) % 4;
        String animation = "Buscando";
        for (int i = 0; i < dots; i++) {
            animation += ".";
        }
        display->drawString(0, 48, animation);
    }
    
    display();
}

void DisplayManager::showBatteryScreen(const BatteryStatus& battery) {
    if (!isInitialized() || !enabled) return;
    
    clear();
    display->setFont(ArialMT_Plain_10);
    
    // Título
    display->drawString(0, 0, "Bateria");
    
    // Icono grande de batería
    drawBatteryIcon(90, 10, battery.percentage);
    
    // Información detallada
    display->drawString(0, 15, "Voltaje: " + String(battery.voltage, 2) + "V");
    display->drawString(0, 27, "Nivel: " + String(battery.percentage) + "%");
    
    // Estado
    String status = battery.charging ? "Cargando" : 
                   (battery.low_power_mode ? "Bajo consumo" : "Normal");
    display->drawString(0, 39, "Estado: " + status);
    
    // Barra de nivel
    drawProgressBar(0, 52, 128, 12, battery.percentage);
    
    display();
}

void DisplayManager::showRadioScreen(bool connected, uint16_t packets, int rssi) {
    if (!isInitialized() || !enabled) return;
    
    clear();
    display->setFont(ArialMT_Plain_10);
    
    // Título
    display->drawString(0, 0, "Radio LoRa");
    
    // Estado de conexión
    String status = connected ? "Conectado" : "Desconectado";
    display->drawString(0, 12, "Estado: " + status);
    
    // Estadísticas
    display->drawString(0, 24, "Paquetes: " + String(packets));
    
    if (rssi != 0) {
        display->drawString(0, 36, "RSSI: " + String(rssi) + " dBm");
        
        // Barras de señal
        int signal_strength = constrain(map(rssi, -120, -30, 0, 4), 0, 4);
        drawSignalBars(100, 36, signal_strength);
    }
    
    // Frecuencia
    display->drawString(0, 48, "Freq: 915.0 MHz");
    
    display();
}

void DisplayManager::showErrorScreen(const String& error) {
    if (!isInitialized()) return;
    
    clear();
    
    // Título parpadeante
    if (millis() % 1000 < 500) {
        display->setFont(ArialMT_Plain_16);
        display->drawString(30, 5, "ERROR");
    }
    
    // Mensaje de error
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 25, "Error:");
    display->drawString(0, 37, error);
    display->drawString(0, 52, "Reiniciando...");
    
    display();
}

void DisplayManager::showMessage(const String& message, uint16_t duration) {
    if (!isInitialized() || !enabled) return;
    
    clear();
    display->setFont(ArialMT_Plain_10);
    
    // Centrar mensaje verticalmente
    display->drawString(0, 28, message);
    display();
    
    if (duration > 0) {
        delay(duration);
    }
}

void DisplayManager::showSplashScreen(const String& title, const String& subtitle) {
    if (!isInitialized()) return;
    
    clear();
    
    // Título principal
    display->setFont(ArialMT_Plain_16);
    int title_width = title.length() * 10;
    int title_x = (128 - title_width) / 2;
    display->drawString(title_x, 15, title);
    
    // Subtítulo
    if (subtitle.length() > 0) {
        display->setFont(ArialMT_Plain_10);
        int subtitle_width = subtitle.length() * 6;
        int subtitle_x = (128 - subtitle_width) / 2;
        display->drawString(subtitle_x, 40, subtitle);
    }
    
    display();
}

// *** UTILIDADES DE DIBUJO ***

void DisplayManager::drawProgressBar(int x, int y, int width, int height, int progress) {
    if (!isInitialized()) return;
    
    // Marco
    display->drawRect(x, y, width, height);
    
    // Relleno
    int fill_width = (width - 2) * progress / 100;
    if (fill_width > 0) {
        display->fillRect(x + 1, y + 1, fill_width, height - 2);
    }
}

void DisplayManager::drawSignalBars(int x, int y, int strength) {
    if (!isInitialized()) return;
    
    for (int i = 0; i < 4; i++) {
        int bar_height = 2 + (i * 2);
        if (i < strength) {
            display->fillRect(x + (i * 3), y + (8 - bar_height), 2, bar_height);
        } else {
            display->drawRect(x + (i * 3), y + (8 - bar_height), 2, bar_height);
        }
    }
}

void DisplayManager::drawBatteryIcon(int x, int y, int percentage) {
    if (!isInitialized()) return;
    
    // Marco de la batería
    display->drawRect(x, y + 2, 12, 6);
    display->drawRect(x + 12, y + 3, 2, 4);  // Terminal positivo
    
    // Nivel de carga
    int fill_width = (10 * percentage) / 100;
    if (fill_width > 0) {
        display->fillRect(x + 1, y + 3, fill_width, 4);
    }
}

void DisplayManager::drawGPSIcon(int x, int y, bool valid) {
    if (!isInitialized()) return;
    
    if (valid) {
        // GPS válido - círculo relleno
        display->fillCircle(x + 4, y + 4, 3);
        display->drawCircle(x + 4, y + 4, 6);
    } else {
        // GPS inválido - círculo vacío parpadeante
        if (millis() % 1000 < 500) {
            display->drawCircle(x + 4, y + 4, 3);
            display->drawCircle(x + 4, y + 4, 6);
        }
    }
}

void DisplayManager::enable() {
    enabled = true;
    if (isInitialized()) {
        turnOn();
    }
}

void DisplayManager::disable() {
    enabled = false;
    if (isInitialized()) {
        turnOff();
    }
}

bool DisplayManager::isEnabled() {
    return enabled;
}

void DisplayManager::test() {
    if (!isInitialized()) {
        DEBUG_ERROR("❌ Display no inicializado para test");
        return;
    }
    
    DEBUG_INFO("🧪 === TEST DISPLAY OLED ===");
    
    // Test de pantallas
    showSplashScreen("TEST", "Display OLED");
    delay(1500);
    
    showMessage("Probando mensajes...", 1000);
    
    // Test con datos simulados
    Position test_pos = {-33.4489, -70.6693, 500.0, true, millis(), 8, 1.2};
    BatteryStatus test_battery = {3.8, 75, false, false};
    HardwareStatus test_hw = {true, true, true, true, SYSTEM_RUNNING};
    
    showMainScreen(test_pos, test_battery, test_hw);
    delay(2000);
    
    showAlertScreen(ALERT_WARNING, 8.5);
    delay(2000);
    
    showGPSScreen(test_pos);
    delay(2000);
    
    showBatteryScreen(test_battery);
    delay(2000);
    
    showRadioScreen(true, 42, -85);
    delay(2000);
    
    DEBUG_INFO("✅ Test de display completado");
}

void DisplayManager::printStatus() {
    Serial.println(F("\n📺 === ESTADO DEL DISPLAY ==="));
    Serial.println(F("Inicializado: ") + String(initialized ? "SÍ" : "NO"));
    Serial.println(F("Habilitado: ") + String(enabled ? "SÍ" : "NO"));
    Serial.println(F("Última actualización: ") + String((millis() - last_update) / 1000) + "s ago");
    Serial.println(F("Dirección I2C: 0x") + String(OLED_ADDRESS, HEX));
    Serial.println(F("Pins SDA/SCL: ") + String(OLED_SDA) + "/" + String(OLED_SCL));
    Serial.println(F("Pin RST: ") + String(OLED_RST));
    Serial.println(F("=============================\n"));
}

void DisplayManager::benchmark() {
    if (!isInitialized()) return;
    
    DEBUG_INFO("🧪 Benchmark del display...");
    
    uint32_t start = millis();
    
    for (int i = 0; i < 100; i++) {
        clear();
        display->drawString(0, 0, "Benchmark: " + String(i));
        display();
    }
    
    uint32_t duration = millis() - start;
    float fps = 100000.0 / duration;
    
    DEBUG_INFO("✅ Benchmark completado: " + String(fps, 1) + " FPS");
}