// DisplayManager.cpp - Sistema de pantallas mejorado con navegación y mejor UI
#include "DisplayManager.h"
#include <Arduino.h>

// Variables para navegación entre pantallas
enum ScreenType {
    SCREEN_MAIN = 0,
    SCREEN_GPS_DETAIL,
    SCREEN_GEOFENCE_INFO,
    SCREEN_SYSTEM_STATS,
    SCREEN_COUNT
};

// Estado de la pantalla
struct DisplayState {
    ScreenType currentScreen = SCREEN_MAIN;
    uint32_t lastScreenChange = 0;
    uint32_t lastUpdate = 0;
    bool isLoading = false;
    uint8_t loadingProgress = 0;
    
    // Contadores del sistema
    uint32_t uptimeMinutes = 0;
    uint16_t txCounter = 0;
    uint16_t rxCounter = 0;
    
    // Info de geocerca
    String geofenceName = "Sin configurar";
    String geofenceType = "N/A";
    float geofenceRadius = 0;
    float distanceToCenter = 0;
    bool insideGeofence = true;
} displayState;

// Constructor
DisplayManager::DisplayManager(uint8_t address, uint8_t sda, uint8_t scl, uint8_t rst) :
    oledDisplay(address, sda, scl),
    rstPin(rst),
    initialized(false),
    displayOn(true),
    currentBrightness(128) {
}

// Inicialización mejorada con pantalla de carga
Result DisplayManager::init() {
    if (initialized) return Result::SUCCESS;
    
    LOG_I("📺 Inicializando Display Manager...");
    
    // Reset físico del display
    if (rstPin != 255) {
        pinMode(rstPin, OUTPUT);
        digitalWrite(rstPin, LOW);
        delay(50);
        digitalWrite(rstPin, HIGH);
        delay(50);
    }
    
    // Inicializar display
    if (!oledDisplay.init()) {
        LOG_E("❌ Error inicializando OLED");
        return Result::ERROR_HARDWARE;
    }
    
    oledDisplay.flipScreenVertically();
    oledDisplay.setContrast(255);
    
    // Mostrar pantalla de carga mejorada
    showBootScreen();
    
    initialized = true;
    displayOn = true;
    
    LOG_I("✅ Display Manager inicializado");
    return Result::SUCCESS;
}

// Pantalla de inicio mejorada con animación de carga
void DisplayManager::showBootScreen() {
    clear();
    
    // Título
    oledDisplay.setFont(ArialMT_Plain_16);
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    oledDisplay.drawString(64, 0, "COLLAR V3");
    
    // Versión
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.drawString(64, 18, "Geofencing System");
    
    // Barra de progreso inicial (más abajo para no interferir)
    drawProgressBar(14, 35, 100, 8, 0);
    
    // Mensaje de estado (debajo de la barra)
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    oledDisplay.drawString(64, 48, "Iniciando...");
    
    display();
    delay(1000);
    
    // Animación de carga progresiva
    const char* loadingSteps[] = {
        "Verificando HW...",
        "Iniciando GPS...",
        "Config. LoRaWAN...",
        "Cargando geocerca...",
        "Sistema listo!"
    };
    
    for (int i = 0; i < 5; i++) {
        clear();
        
        // Mantener título
        oledDisplay.setFont(ArialMT_Plain_16);
        oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
        oledDisplay.drawString(64, 0, "COLLAR V3");
        
        // Subtítulo
        oledDisplay.setFont(ArialMT_Plain_10);
        oledDisplay.drawString(64, 18, "Geofencing System");
        
        // Barra de progreso
        uint8_t progress = (i + 1) * 20;
        drawProgressBar(14, 35, 100, 8, progress);
        
        // Mostrar paso actual (debajo de la barra)
        oledDisplay.setFont(ArialMT_Plain_10);
        oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
        oledDisplay.drawString(64, 48, loadingSteps[i]);
        
        // Mostrar porcentaje al lado de la barra
        char percStr[8];
        snprintf(percStr, sizeof(percStr), "%d%%", progress);
        oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
        oledDisplay.drawString(128, 35, percStr);
        
        display();
        delay(600);
    }
    
    delay(800);
}

// Pantalla principal mejorada sin superposición
void DisplayManager::showMainScreen(const SystemStatus& status, const Position& position, 
                                   const BatteryStatus& battery, AlertLevel alertLevel) {
    if (!initialized) return;
    
    clear();
    
    // Línea 1: Estado y batería
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    
    // Estado del sistema con icono
    const char* statusIcon = status.radioInitialized ? "📡" : "⚠";
    oledDisplay.drawString(0, 0, statusIcon);
    
    // Batería en la derecha
    oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
    char batteryStr[16];
    snprintf(batteryStr, sizeof(batteryStr), "%d%% %.1fV", battery.percentage, battery.voltage);
    oledDisplay.drawString(128, 0, batteryStr);
    
    // Línea 2-3: Posición GPS (formato decimal simplificado)
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    if (isValidPosition(position)) {
        char latStr[20], lngStr[20];
        // Formato simplificado sin grados
        snprintf(latStr, sizeof(latStr), "LAT: %.5f", position.latitude);
        snprintf(lngStr, sizeof(lngStr), "LNG: %.5f", position.longitude);
        
        oledDisplay.drawString(0, 12, latStr);
        oledDisplay.drawString(0, 24, lngStr);
        
        // Indicador de satelites
        char satStr[10];
        snprintf(satStr, sizeof(satStr), "SAT:%d", position.satellites);
        oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
        oledDisplay.drawString(128, 12, satStr);
    } else {
        oledDisplay.drawString(0, 12, "GPS: Buscando...");
        // Animación de puntos
        int dots = (millis() / 500) % 4;
        String dotsStr = "";
        for (int i = 0; i < dots; i++) dotsStr += ".";
        oledDisplay.drawString(0, 24, dotsStr.c_str());
    }
    
    // Línea 4: Estado de alerta/geocerca
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    if (alertLevel != AlertLevel::SAFE) {
        // Parpadeo para alertas
        if ((millis() / 500) % 2 == 0 || alertLevel < AlertLevel::DANGER) {
            const char* alertText = alertLevelToString(alertLevel);
            oledDisplay.drawString(0, 36, "ALERTA:");
            oledDisplay.drawString(45, 36, alertText);
        }
    } else {
        oledDisplay.drawString(0, 36, "Estado: SEGURO");
    }
    
    // Línea 5: Contadores actualizados correctamente
    char statsStr[32];
    displayState.uptimeMinutes = status.uptime / 60000; // Convertir ms a minutos
    snprintf(statsStr, sizeof(statsStr), "UP:%02lu TX:%d RX:%d", 
             displayState.uptimeMinutes, displayState.txCounter, displayState.rxCounter);
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.drawString(0, 48, statsStr);
    
    // Indicador de pantalla actual (pequeño en esquina)
    oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
    oledDisplay.drawString(128, 48, "1/4");
    
    display();
    currentScreen = SCREEN_MAIN;
}

// Nueva pantalla: Detalles GPS
void DisplayManager::showGPSDetailScreen(const Position& position) {
    if (!initialized) return;
    
    clear();
    
    // Título
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    oledDisplay.drawString(64, 0, "=== DETALLES GPS ===");
    
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    
    if (isValidPosition(position)) {
        char line[32];
        
        // Coordenadas completas
        snprintf(line, sizeof(line), "LAT: %.6f", position.latitude);
        oledDisplay.drawString(0, 12, line);
        
        snprintf(line, sizeof(line), "LNG: %.6f", position.longitude);
        oledDisplay.drawString(0, 22, line);
        
        // Altitud y precisión
        snprintf(line, sizeof(line), "ALT: %.1f m", position.altitude);
        oledDisplay.drawString(0, 32, line);
        
        snprintf(line, sizeof(line), "PREC: %.1f m", position.accuracy);
        oledDisplay.drawString(0, 42, line);
        
        // Satélites y precisión
        snprintf(line, sizeof(line), "SAT: %d  PREC: %.1fm", 
                position.satellites, position.accuracy);
        oledDisplay.drawString(0, 52, line);
    } else {
        oledDisplay.drawString(0, 25, "Sin señal GPS");
        oledDisplay.drawString(0, 35, "Verificar antena");
    }
    
    // Indicador de pantalla
    oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
    oledDisplay.drawString(128, 52, "2/4");
    
    display();
}

// Nueva pantalla: Información de Geocerca
void DisplayManager::showGeofenceInfoScreen(const Geofence& geofence, float distance, bool inside) {
    if (!initialized) return;
    
    clear();
    
    // Título
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    oledDisplay.drawString(64, 0, "=== GEOCERCA ===");
    
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    
    // Nombre de la geocerca
    char line[32];
    snprintf(line, sizeof(line), "Nombre: %s", geofence.name);
    oledDisplay.drawString(0, 12, line);
    
    // Tipo de geocerca (siempre círculo por ahora)
    const char* typeIcon = "○";
    const char* typeText = "Círculo";
    snprintf(line, sizeof(line), "Tipo: %s %s", typeIcon, typeText);
    oledDisplay.drawString(0, 22, line);
    
    // Radio
    snprintf(line, sizeof(line), "Radio: %.0f m", geofence.radius);
    oledDisplay.drawString(0, 32, line);
    
    // Distancia al centro/borde
    snprintf(line, sizeof(line), "Dist: %.1f m", distance);
    oledDisplay.drawString(0, 42, line);
    
    // Estado actual con indicador visual
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    if (inside) {
        oledDisplay.drawString(64, 52, "✓ DENTRO");
    } else {
        // Parpadeo si está fuera
        if ((millis() / 500) % 2 == 0) {
            oledDisplay.drawString(64, 52, "✗ FUERA");
        }
    }
    
    // Indicador de pantalla
    oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
    oledDisplay.drawString(128, 52, "3/4");
    
    display();
}

// Nueva pantalla: Estadísticas del sistema
void DisplayManager::showSystemStatsScreen(const SystemStats& stats) {
    if (!initialized) return;
    
    clear();
    
    // Título
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    oledDisplay.drawString(64, 0, "=== ESTADÍSTICAS ===");
    
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    
    char line[32];
    
    // Paquetes enviados/recibidos
    snprintf(line, sizeof(line), "TX Total: %lu", stats.totalPacketsSent);
    oledDisplay.drawString(0, 12, line);
    
    snprintf(line, sizeof(line), "RX Total: %lu", stats.totalPacketsReceived);
    oledDisplay.drawString(0, 22, line);
    
    // Tasa de éxito
    float successRate = (stats.totalPacketsSent > 0) ? 
        (100.0f * (stats.totalPacketsSent - stats.packetsLost) / stats.totalPacketsSent) : 0;
    snprintf(line, sizeof(line), "Éxito: %.1f%%", successRate);
    oledDisplay.drawString(0, 32, line);
    
    // Violaciones de geocerca
    snprintf(line, sizeof(line), "Violaciones: %lu", stats.geofenceViolations);
    oledDisplay.drawString(0, 42, line);
    
    // Tiempo de operación
    uint32_t hours = stats.totalUptime / 3600000;
    uint32_t minutes = (stats.totalUptime % 3600000) / 60000;
    snprintf(line, sizeof(line), "Tiempo: %luh %lum", hours, minutes);
    oledDisplay.drawString(0, 52, line);
    
    // Indicador de pantalla
    oledDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
    oledDisplay.drawString(128, 52, "4/4");
    
    display();
}

// Navegación entre pantallas con botón PRG
void DisplayManager::nextScreen() {
    displayState.currentScreen = (ScreenType)((displayState.currentScreen + 1) % SCREEN_COUNT);
    displayState.lastScreenChange = millis();
    
    // Feedback visual
    clear();
    oledDisplay.setFont(ArialMT_Plain_10);
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    
    const char* screenNames[] = {
        "Principal",
        "GPS Detalle", 
        "Geocerca",
        "Estadísticas"
    };
    
    oledDisplay.drawString(64, 25, "Cambiando a:");
    oledDisplay.drawString(64, 35, screenNames[displayState.currentScreen]);
    display();
    delay(300);
}

// Actualizar pantalla según el modo actual
void DisplayManager::update() {
    if (!initialized || !displayOn) return;
    
    // Auto-apagado después de timeout
    if (autoSleepEnabled && (millis() - lastActivity > autoSleepTimeout)) {
        turnOff();
        return;
    }
    
    // No actualizar muy frecuentemente
    if (millis() - displayState.lastUpdate < 1000) return;
    displayState.lastUpdate = millis();
    
    // Actualizar según pantalla actual
    // (Este método debe ser llamado desde el loop principal con los datos actuales)
}

// Actualizar contadores
void DisplayManager::updateCounters(uint16_t txCount, uint16_t rxCount) {
    displayState.txCounter = txCount;
    displayState.rxCounter = rxCount;
}

// Actualizar información de geocerca
void DisplayManager::updateGeofenceInfo(const char* name, GeofenceType type, 
                                       float radius, float distance, bool inside) {
    displayState.geofenceName = String(name);
    displayState.geofenceType = (type == GeofenceType::CIRCLE) ? "Círculo" : "Polígono";
    displayState.geofenceRadius = radius;
    displayState.distanceToCenter = distance;
    displayState.insideGeofence = inside;
}

// Barra de progreso mejorada
void DisplayManager::drawProgressBar(int16_t x, int16_t y, int16_t width, 
                                    int16_t height, uint8_t percentage) {
    // Marco de la barra
    oledDisplay.drawRect(x, y, width, height);
    
    // Relleno según porcentaje
    if (percentage > 0) {
        int16_t fillWidth = (width - 2) * percentage / 100;
        oledDisplay.fillRect(x + 1, y + 1, fillWidth, height - 2);
    }
    
    // Texto del porcentaje (opcional, si hay espacio)
    if (width > 30) {
        char percStr[5];
        snprintf(percStr, sizeof(percStr), "%d%%", percentage);
        oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
        oledDisplay.drawString(x + width/2, y + height + 2, percStr);
    }
}

// Funciones auxiliares mejoradas
void DisplayManager::drawBatteryIcon(int16_t x, int16_t y, uint8_t percentage) {
    // Cuerpo de la batería
    oledDisplay.drawRect(x, y, 20, 10);
    // Terminal positivo
    oledDisplay.drawRect(x + 20, y + 3, 2, 4);
    
    // Relleno según porcentaje
    int16_t fillWidth = 18 * percentage / 100;
    if (fillWidth > 0) {
        oledDisplay.fillRect(x + 1, y + 1, fillWidth, 8);
    }
    
    // Indicador de carga baja
    if (percentage < 20) {
        // Parpadeo si está muy baja
        if ((millis() / 500) % 2 == 0) {
            oledDisplay.drawString(x + 25, y, "!");
        }
    }
}

// Manejo del botón PRG para cambiar pantallas
void DisplayManager::handleButtonPress() {
    static uint32_t lastButtonPress = 0;
    const uint32_t debounceTime = 200; // ms
    
    if (millis() - lastButtonPress > debounceTime) {
        nextScreen();
        lastButtonPress = millis();
        updateLastActivity();
    }
}

// Obtener pantalla actual
uint8_t DisplayManager::getCurrentScreen() const {
    return static_cast<uint8_t>(currentScreen);
}

// Obtener nombre de pantalla actual
const char* DisplayManager::getCurrentScreenName() const {
    const char* names[] = {
        "Splash", "Principal", "GPS Detalle", "Geocerca", 
        "Estadísticas", "Alerta", "Batería", "Error", "Config", "Off"
    };
    
    if (currentScreen < SCREEN_OFF) {
        return names[currentScreen];
    }
    return "Desconocido";
}

// Funciones de configuración de fuentes
void DisplayManager::setSmallFont() {
    oledDisplay.setFont(ArialMT_Plain_10);
}

void DisplayManager::setMediumFont() {
    oledDisplay.setFont(ArialMT_Plain_16);
}

void DisplayManager::setLargeFont() {
    oledDisplay.setFont(ArialMT_Plain_24);
}

// Función para dibujar texto centrado
void DisplayManager::drawCenteredText(const char* text, int16_t y) {
    oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    oledDisplay.drawString(64, y, text);
    oledDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

bool DisplayManager::isValidPosition(const Position& pos) const {
    return pos.valid && 
           pos.latitude != 0.0 && 
           pos.longitude != 0.0 &&
           pos.satellites >= 3;
}

const char* DisplayManager::alertLevelToString(AlertLevel level) const {
    switch(level) {
        case AlertLevel::SAFE:
            return "SEGURO";
        case AlertLevel::CAUTION:
            return "PRECAUCIÓN";
        case AlertLevel::WARNING:
            return "ADVERTENCIA";
        case AlertLevel::DANGER:
            return "PELIGRO";
        case AlertLevel::EMERGENCY:
            return "EMERGENCIA";
        default:
            return "DESCONOCIDO";
    }
}

void DisplayManager::clear() {
    if (initialized) {
        oledDisplay.clear();
    }
}

void DisplayManager::display() {
    if (initialized && displayOn) {
        oledDisplay.display();
        updateLastActivity();
    }
}

void DisplayManager::setBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    if (initialized) {
        oledDisplay.setBrightness(brightness);
    }
}

void DisplayManager::turnOn() {
    if (initialized && !displayOn) {
        oledDisplay.displayOn();
        displayOn = true;
        updateLastActivity();
        LOG_D("📺 Display encendido");
    }
}

void DisplayManager::turnOff() {
    if (initialized && displayOn) {
        oledDisplay.displayOff();
        displayOn = false;
        currentScreen = SCREEN_OFF;
        LOG_D("📺 Display apagado");
    }
}

bool DisplayManager::isOn() const {
    return displayOn;
}

bool DisplayManager::isInitialized() const {
    return initialized;
}

void DisplayManager::updateLastActivity() {
    lastActivity = millis();
}

void DisplayManager::setAutoSleep(bool enabled, uint32_t timeoutMs) {
    autoSleepEnabled = enabled;
    autoSleepTimeout = timeoutMs;
}

void DisplayManager::showSplashScreen() {
    showBootScreen(); // Usa la misma pantalla de inicio
}

void DisplayManager::showAlertScreen(AlertLevel level, float distance) {
    if (!initialized) return;
    
    // Guardar tiempo de inicio de la alerta
    static uint32_t alertStartTime = 0;
    if (currentScreen != SCREEN_ALERT) {
        alertStartTime = millis();
    }
    
    clear();
    
    // Título parpadeante para alertas críticas
    bool blink = (level >= AlertLevel::DANGER) && ((millis() / 500) % 2);
    
    if (!blink || level < AlertLevel::DANGER) {
        setLargeFont();
        drawCenteredText("!ALERTA!", 2);
    }
    
    // Nivel de alerta con color más grande
    setMediumFont();
    drawCenteredText(alertLevelToString(level), 18);
    
    // Línea separadora
    oledDisplay.drawLine(10, 32, 118, 32);
    
    // Distancia con formato mejorado
    setSmallFont();
    char distStr[32];
    if (distance > 0) {
        snprintf(distStr, sizeof(distStr), "Distancia: %.1f metros", distance);
    } else {
        snprintf(distStr, sizeof(distStr), "DENTRO DE GEOCERCA");
    }
    drawCenteredText(distStr, 38);
    
    // Icono de alerta grande y animado
    int iconOffset = (millis() / 200) % 3 - 1;
    drawAlertIcon(64 - 8, 48 + iconOffset, level);
    
    display();
    currentScreen = SCREEN_ALERT;
    lastDistance = distance;
    
    // Mantener la pantalla de alerta visible por mínimo 3 segundos
    if (millis() - alertStartTime < 3000) {
        updateLastActivity(); // Mantener display activo
    }
}

void DisplayManager::showBatteryScreen(const BatteryStatus& battery) {
    if (!initialized) return;
    
    clear();
    
    setMediumFont();
    drawCenteredText("BATERÍA", 5);
    
    // Icono de batería grande
    drawBatteryIcon(64 - 12, 20, battery.percentage);
    
    // Información detallada
    setSmallFont();
    char voltageStr[16];
    snprintf(voltageStr, sizeof(voltageStr), "%.2fV", battery.voltage);
    drawCenteredText(voltageStr, 40);
    
    char percentStr[16];
    snprintf(percentStr, sizeof(percentStr), "%d%%", battery.percentage);
    drawCenteredText(percentStr, 50);
    
    display();
    currentScreen = SCREEN_BATTERY;
}

void DisplayManager::showErrorScreen(const char* error) {
    if (!initialized) return;
    
    clear();
    
    setMediumFont();
    drawCenteredText("ERROR", 5);
    
    setSmallFont();
    
    // Texto del error (dividir en líneas si es necesario)
    int16_t y = 20;
    const char* line = error;
    char lineBuffer[20];
    
    while (*line && y < 55) {
        int len = 0;
        while (line[len] && line[len] != '\n' && len < 19) {
            lineBuffer[len] = line[len];
            len++;
        }
        lineBuffer[len] = '\0';
        
        oledDisplay.drawString(0, y, lineBuffer);
        y += 10;
        
        line += len;
        if (*line == '\n') line++;
    }
    
    display();
    currentScreen = SCREEN_ERROR;
}

void DisplayManager::drawAlertIcon(int16_t x, int16_t y, AlertLevel level) {
    // Dibujar un triángulo de advertencia o círculo según el nivel
    switch(level) {
        case AlertLevel::SAFE:
            // Círculo con check
            oledDisplay.drawCircle(x + 8, y + 8, 8);
            oledDisplay.drawLine(x + 5, y + 8, x + 7, y + 11);
            oledDisplay.drawLine(x + 7, y + 11, x + 11, y + 5);
            break;
            
        case AlertLevel::CAUTION:
        case AlertLevel::WARNING:
            // Triángulo
            oledDisplay.drawLine(x + 8, y, x + 2, y + 14);
            oledDisplay.drawLine(x + 2, y + 14, x + 14, y + 14);
            oledDisplay.drawLine(x + 14, y + 14, x + 8, y);
            // Signo de exclamación
            oledDisplay.drawLine(x + 8, y + 4, x + 8, y + 10);
            oledDisplay.setPixel(x + 8, y + 12);
            break;
            
        case AlertLevel::DANGER:
        case AlertLevel::EMERGENCY:
            // Triángulo relleno (parpadeante)
            if ((millis() / 300) % 2) {
                for (int i = 0; i < 14; i++) {
                    int w = i / 2;
                    oledDisplay.drawLine(x + 8 - w, y + i, x + 8 + w, y + i);
                }
            }
            break;
    }
}

void DisplayManager::drawGPSIcon(int16_t x, int16_t y, bool connected) {
    if (connected) {
        // Satélite con señal
        oledDisplay.drawCircle(x + 4, y + 4, 3);
        oledDisplay.drawLine(x + 7, y + 7, x + 10, y + 10);
        oledDisplay.drawLine(x + 1, y + 7, x - 2, y + 10);
        // Ondas de señal
        oledDisplay.drawCircle(x + 4, y + 4, 6);
    } else {
        // Satélite sin señal (parpadeante)
        if ((millis() / 500) % 2) {
            oledDisplay.drawCircle(x + 4, y + 4, 3);
            oledDisplay.drawLine(x, y, x + 8, y + 8);
        }
    }
}

void DisplayManager::drawSignalIcon(int16_t x, int16_t y, uint8_t strength) {
    // Barras de señal (0-5)
    for (int i = 0; i < 5; i++) {
        int barHeight = (i + 1) * 3;
        if (i < strength) {
            oledDisplay.fillRect(x + i * 3, y + 15 - barHeight, 2, barHeight);
        } else {
            oledDisplay.drawRect(x + i * 3, y + 15 - barHeight, 2, barHeight);
        }
    }
}