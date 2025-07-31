/*
 * ============================================================================
 * IMPLEMENTACIÓN MÓDULO OLED DISPLAY - Interfaz Visual V3
 * ============================================================================
 */

#include "hardware/oled_display.h"
#include <Wire.h>

// Variables externas que necesitamos acceder
extern SystemStatus_t system_status;
extern Position_t position;
extern Geofence_t geofence;
extern AlertState_t alert_state;
extern PowerStatus_t power_status;

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
OLEDController Display;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
OLEDController::OLEDController() 
    : display(nullptr), initialized(false), current_screen(SCREEN_SPLASH),
      last_update(0), screen_timer(0), auto_rotate(false), brightness(255) {
}

OLEDController::~OLEDController() {
    end();
}

// ============================================================================
// INICIALIZACIÓN Y CONTROL
// ============================================================================
bool OLEDController::begin() {
    DEBUG_PRINTLN(F("📺 Inicializando OLED V3..."));
    
    // Configurar I2C con pins específicos de V3
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // Reset sequence obligatorio para V3
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(50);
    digitalWrite(OLED_RST, HIGH);
    delay(50);
    
    // Crear instancia del display
    display = new SSD1306Wire(OLED_ADDRESS, OLED_SDA, OLED_SCL);
    
    if (!display) {
        DEBUG_PRINTLN(F("❌ Error: No se pudo crear instancia OLED"));
        return false;
    }
    
    // Inicializar display
    if (display->init()) {
        display->displayOn();
        display->clear();
        display->flipScreenVertically();
        display->setContrast(brightness);
        
        initialized = true;
        current_screen = SCREEN_SPLASH;
        last_update = millis();
        
        DEBUG_PRINTLN(F("✅ OLED V3 inicializado correctamente"));
        
        // Mostrar splash inicial
        drawSplashScreen();
        display->display();
        delay(OLED_SPLASH_TIME);
        
        return true;
    } else {
        DEBUG_PRINTLN(F("❌ Error: OLED V3 falló al inicializar"));
        delete display;
        display = nullptr;
        return false;
    }
}

void OLEDController::end() {
    if (!initialized) return;
    
    if (display) {
        display->displayOff();
        display->end();
        delete display;
        display = nullptr;
    }
    
    initialized = false;
    DEBUG_PRINTLN(F("📺 OLED desactivado"));
}

// ============================================================================
// CONTROL DE PANTALLAS
// ============================================================================
void OLEDController::update() {
    if (!initialized) return;
    
    uint32_t current_time = millis();
    
    // Auto-rotación de pantallas
    if (auto_rotate && (current_time - screen_timer > 5000)) {
        nextScreen();
        screen_timer = current_time;
    }
    
    // Actualización periódica
    if (current_time - last_update < OLED_UPDATE_INTERVAL) {
        return;
    }
    
    display->clear();
    
    // Renderizar pantalla actual
    switch (current_screen) {
        case SCREEN_SPLASH:
            drawSplashScreen();
            break;
        case SCREEN_STATUS:
            drawStatusScreen();
            break;
        case SCREEN_POSITION:
            drawPositionScreen();
            break;
        case SCREEN_ALERT:
            drawAlertScreen();
            break;
        case SCREEN_BATTERY:
            drawBatteryScreen();
            break;
        case SCREEN_RADIO:
            drawRadioScreen();
            break;
        case SCREEN_DEBUG:
            drawDebugScreen();
            break;
        default:
            drawStatusScreen();
            break;
    }
    
    display->display();
    last_update = current_time;
}

void OLEDController::setScreen(DisplayScreen screen) {
    if (screen >= SCREEN_COUNT) return;
    
    current_screen = screen;
    screen_timer = millis();
    last_update = 0; // Forzar actualización inmediata
}

void OLEDController::nextScreen() {
    current_screen = (DisplayScreen)((current_screen + 1) % SCREEN_COUNT);
    setScreen(current_screen);
}

void OLEDController::previousScreen() {
    current_screen = (DisplayScreen)((current_screen - 1 + SCREEN_COUNT) % SCREEN_COUNT);
    setScreen(current_screen);
}

// ============================================================================
// PANTALLAS ESPECÍFICAS
// ============================================================================
void OLEDController::drawSplashScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    
    display->drawString(64, 0, "🐐 COLLAR GEOFENCING");
    display->drawString(64, 12, "Versión " COLLAR_VERSION);
    display->drawString(64, 24, HARDWARE_VERSION);
    display->drawString(64, 36, MCU_TYPE " + " RADIO_TYPE);
    display->drawString(64, 48, "Sistema iniciando...");
    
    // Barra de progreso animada
    int progress = (millis() / 100) % 100;
    drawProgressBar(14, 58, 100, 4, progress);
}

void OLEDController::drawStatusScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    
    // Título
    display->drawString(0, 0, "ESTADO SISTEMA");
    
    // Estados de hardware
    String radio_status = system_status.radio_initialized ? "✓" : "✗";
    String oled_status = system_status.oled_initialized ? "✓" : "✗";
    String buzzer_status = system_status.buzzer_initialized ? "✓" : "✗";
    
    display->drawString(0, 12, "Radio: " + radio_status + " OLED: " + oled_status + " Buzz: " + buzzer_status);
    
    // Contador de paquetes
    display->drawString(0, 24, "Paquetes TX: " + String(system_status.packet_counter));
    
    // Estado de alerta
    if (alert_state.level > 0) {
        display->drawString(0, 36, "⚠️ ALERTA NIVEL " + String(alert_state.level));
    } else {
        display->drawString(0, 36, "✅ Sistema normal");
    }
    
    // Uptime
    uint32_t uptime_min = system_status.uptime / 60000;
    display->drawString(0, 48, "Uptime: " + String(uptime_min) + " min");
    
    // Indicador de actividad
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->drawString(128, 0, millis() % 2000 < 1000 ? "●" : "○");
}

void OLEDController::drawPositionScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    
    display->drawString(0, 0, "POSICIÓN GPS");
    
    if (position.valid) {
        display->drawString(0, 12, "Lat: " + String(position.latitude, 6));
        display->drawString(0, 24, "Lng: " + String(position.longitude, 6));
        display->drawString(0, 36, "Alt: " + String(position.altitude, 1) + "m");
        display->drawString(0, 48, "Sats: " + String(position.satellites));
    } else {
        display->drawString(0, 24, "❌ Sin señal GPS");
        display->drawString(0, 36, "Posición no válida");
    }
    
    // Geocerca
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    if (geofence.active) {
        // Calcular distancia aproximada al centro
        float distance = sqrt(pow((position.latitude - geofence.center_lat) * 111000, 2) + 
                             pow((position.longitude - geofence.center_lng) * 111000, 2));
        display->drawString(128, 48, String(distance, 0) + "m");
    }
}

void OLEDController::drawAlertScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    
    if (alert_state.level == 0) {
        display->drawString(64, 20, "✅ ZONA SEGURA");
        display->drawString(64, 32, "Sin alertas activas");
    } else {
        // Título parpadeante para alertas activas
        if (millis() % 1000 < 500) {
            display->drawString(64, 0, "🚨 ALERTA ACTIVA 🚨");
        }
        
        String level_text;
        switch (alert_state.level) {
            case 1: level_text = "PRECAUCIÓN"; break;
            case 2: level_text = "ADVERTENCIA"; break;
            case 3: level_text = "PELIGRO"; break;
            case 4: level_text = "EMERGENCIA"; break;
            default: level_text = "DESCONOCIDO"; break;
        }
        
        display->drawString(64, 16, "NIVEL " + String(alert_state.level));
        display->drawString(64, 28, level_text);
        display->drawString(64, 40, "Dist: " + String(alert_state.last_distance, 1) + "m");
        
        // Tiempo en alerta
        uint32_t alert_time = (millis() - alert_state.start_time) / 1000;
        display->drawString(64, 52, "Tiempo: " + String(alert_time) + "s");
    }
}

void OLEDController::drawBatteryScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    
    display->drawString(0, 0, "BATERÍA");
    
    // Voltaje
    display->drawString(0, 16, "Voltaje: " + String(power_status.battery_voltage, 2) + "V");
    
    // Porcentaje estimado
    display->drawString(0, 28, "Carga: " + String(power_status.percentage) + "%");
    
    // Estado
    String status = power_status.charging ? "Cargando" : 
                   power_status.low_power_mode ? "Modo ahorro" : "Normal";
    display->drawString(0, 40, "Estado: " + status);
    
    // Icono de batería grande
    drawBatteryIcon(80, 16, power_status.percentage, power_status.charging);
    
    // Barra de progreso de batería
    drawProgressBar(0, 54, 128, 8, power_status.percentage);
}

void OLEDController::drawRadioScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    
    display->drawString(0, 0, "RADIO LoRa");
    
    if (system_status.radio_initialized) {
        display->drawString(0, 12, "Freq: " + String(LORA_FREQUENCY, 1) + " MHz");
        display->drawString(0, 24, "SF: " + String(LORA_SPREADING_FACTOR) + 
                           " BW: " + String(LORA_BANDWIDTH, 0));
        display->drawString(0, 36, "Power: " + String(LORA_OUTPUT_POWER) + " dBm");
        display->drawString(0, 48, "Packets: " + String(system_status.packet_counter));
        
        // Indicador de estado de transmisión
        drawConnectionStatus(100, 12, true);
    } else {
        display->drawString(0, 24, "❌ Radio no inicializado");
        drawConnectionStatus(100, 24, false);
    }
}

void OLEDController::drawDebugScreen() {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    
    display->drawString(0, 0, "DEBUG INFO");
    
    // Memoria libre
    display->drawString(0, 12, "Free heap: " + String(ESP.getFreeHeap()));
    
    // Temperatura del chip (si está disponible)
    display->drawString(0, 24, "Temp: N/A°C");
    
    // Frecuencia de CPU
    display->drawString(0, 36, "CPU: " + String(ESP.getCpuFreqMHz()) + " MHz");
    
    // Versión del firmware
    display->drawString(0, 48, "FW: " COLLAR_VERSION);
}

// ============================================================================
// UTILIDADES DE DIBUJO
// ============================================================================
void OLEDController::drawProgressBar(int x, int y, int width, int height, int progress) {
    progress = CLAMP(progress, 0, 100);
    
    // Marco
    display->drawRect(x, y, width, height);
    
    // Relleno
    int fill_width = (width - 2) * progress / 100;
    if (fill_width > 0) {
        display->fillRect(x + 1, y + 1, fill_width, height - 2);
    }
}

void OLEDController::drawBatteryIcon(int x, int y, int percentage, bool charging) {
    // Cuerpo de la batería
    display->drawRect(x, y, 20, 12);
    display->drawRect(x + 20, y + 3, 2, 6); // Terminal positivo
    
    // Nivel de carga
    int fill_width = (18 * percentage) / 100;
    if (fill_width > 0) {
        display->fillRect(x + 1, y + 2, fill_width, 8);
    }
    
    // Indicador de carga
    if (charging && (millis() % 1000 < 500)) {
        display->drawString(x + 24, y + 2, "⚡");
    }
}

void OLEDController::drawAlertIcon(int x, int y, uint8_t level) {
    switch (level) {
        case 1: case 2:
            display->drawString(x, y, "⚠️");
            break;
        case 3: case 4:
            display->drawString(x, y, "🚨");
            break;
        default:
            display->drawString(x, y, "✅");
            break;
    }
}

void OLEDController::drawConnectionStatus(int x, int y, bool connected) {
    if (connected) {
        // Indicador de conexión con barras
        for (int i = 0; i < 4; i++) {
            int bar_height = 2 + (i * 2);
            display->fillRect(x + (i * 3), y + (8 - bar_height), 2, bar_height);
        }
    } else {
        display->drawString(x, y, "✗");
    }
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================
void OLEDController::setAutoRotate(bool enable, uint32_t interval_ms) {
    auto_rotate = enable;
    screen_timer = millis();
    DEBUG_PRINTF("📺 Auto-rotación: %s\n", enable ? "ON" : "OFF");
}

void OLEDController::setBrightness(uint8_t level) {
    brightness = level;
    if (initialized && display) {
        display->setContrast(brightness);
    }
}

void OLEDController::setOrientation(bool flip) {
    if (initialized && display) {
        if (flip) {
            display->flipScreenVertically();
        } else {
            display->resetOrientation();
        }
    }
}

// ============================================================================
// MENSAJES RÁPIDOS
// ============================================================================
void OLEDController::showMessage(const char* message, uint32_t duration_ms) {
    if (!initialized) return;
    
    display->clear();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64, 28, message);
    display->display();
    
    delay(duration_ms);
    last_update = 0; // Forzar actualización de pantalla normal
}

void OLEDController::showAlert(const char* title, const char* message, uint32_t duration_ms) {
    if (!initialized) return;
    
    display->clear();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    
    display->drawString(64, 16, title);
    display->drawString(64, 32, message);
    display->display();
    
    delay(duration_ms);
    last_update = 0;
}

void OLEDController::showProgress(const char* title, int progress, const char* status) {
    if (!initialized) return;
    
    display->clear();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    
    display->drawString(64, 16, title);
    drawProgressBar(14, 32, 100, 8, progress);
    display->drawString(64, 44, String(progress) + "%");
    
    if (status) {
        display->drawString(64, 56, status);
    }
    
    display->display();
}

// ============================================================================
// TEST Y DIAGNÓSTICO
// ============================================================================
void OLEDController::runSelfTest() {
    if (!initialized) {
        DEBUG_PRINTLN(F("❌ OLED no inicializado para self-test"));
        return;
    }
    
    DEBUG_PRINTLN(F("🧪 === SELF-TEST OLED ==="));
    
    // Test de texto
    showMessage("Test de texto", 1000);
    
    // Test de progreso
    for (int i = 0; i <= 100; i += 10) {
        showProgress("Testing...", i, "Progress test");
        delay(100);
    }
    
    // Test de pantallas
    for (int screen = 0; screen < SCREEN_COUNT; screen++) {
        setScreen((DisplayScreen)screen);
        update();
        delay(1000);
    }
    
    DEBUG_PRINTLN(F("✅ Self-test OLED completado"));
}

void OLEDController::showTestPattern() {
    if (!initialized) return;
    
    display->clear();
    
    // Patrón de líneas
    for (int i = 0; i < 128; i += 8) {
        display->drawVerticalLine(i, 0, 64);
    }
    for (int i = 0; i < 64; i += 8) {
        display->drawHorizontalLine(0, i, 128);
    }
    
    display->display();
}