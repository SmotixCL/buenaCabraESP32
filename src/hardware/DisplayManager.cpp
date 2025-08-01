#include "DisplayManager.h"
#include <OLEDDisplayFonts.h>
#include <Wire.h>

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

DisplayManager::DisplayManager(uint8_t address, uint8_t sda, uint8_t scl, uint8_t rst) :
    oledDisplay(address, sda, scl),
    rstPin(rst),
    initialized(false),
    displayOn(false),
    currentBrightness(255),
    currentScreen(SCREEN_OFF),
    lastActivity(0),
    autoSleepEnabled(true),
    autoSleepTimeout(OLED_TIMEOUT_SLEEP),
    lastAlertLevel(AlertLevel::SAFE),
    lastDistance(0.0f),
    animationCounter(0)
{
    updateLastActivity();
}

Result DisplayManager::init() {
    if (initialized) {
        return Result::SUCCESS;
    }
    
    LOG_I("📺 Inicializando Display Manager...");
    
    // CRÍTICO: Activar VEXT para alimentar OLED y periféricos
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(100); // Tiempo para estabilizar alimentación
    
    // Configurar I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(I2C_FREQUENCY);
    
    // Reset del display
    resetDisplay();
    
    // Intentar inicializar display con timeout
    LOG_D("📺 Intentando conectar con OLED en dirección 0x%02X...", OLED_ADDR);
    
    bool displayFound = false;
    for (int attempt = 0; attempt < 3 && !displayFound; attempt++) {
        Wire.beginTransmission(OLED_ADDR);
        if (Wire.endTransmission() == 0) {
            displayFound = true;
            LOG_D("📺 OLED detectado en intento %d", attempt + 1);
        } else {
            LOG_W("📺 OLED no detectado en intento %d, reintentando...", attempt + 1);
            delay(100);
        }
    }
    
    if (!displayFound) {
        LOG_E("❌ OLED no encontrado en dirección 0x%02X", OLED_ADDR);
        return Result::ERROR_HARDWARE;
    }
    
    // Inicializar display
    if (!oledDisplay.init()) {
        LOG_E("❌ Error inicializando OLED");
        return Result::ERROR_HARDWARE;
    }
    
    setupDisplay();
    
    initialized = true;
    displayOn = true;
    
    LOG_INIT("Display Manager", true);
    
    return Result::SUCCESS;
}

bool DisplayManager::isInitialized() const {
    return initialized;
}

// ============================================================================
// CONTROL BÁSICO DEL DISPLAY
// ============================================================================

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
    // SSD1306 no tiene control directo de brillo, pero podemos simular
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

// ============================================================================
// PANTALLAS PRINCIPALES
// ============================================================================

void DisplayManager::showSplashScreen() {
    if (!initialized) return;
    
    clear();
    
    // Logo/Texto principal
    setLargeFont();
    drawCenteredText("COLLAR", 5);
    
    setMediumFont();
    drawCenteredText("Geofencing V3", 20);
    
    setSmallFont();
    drawCenteredText("Buena Cabra Tech", 35);
    drawCenteredText(FIRMWARE_VERSION, 45);
    
    // Barra de progreso de inicialización
    drawProgressBar(20, 55, 88, 6, 100);
    
    display();
    currentScreen = SCREEN_SPLASH;
}

void DisplayManager::showMainScreen(const SystemStatus& status, const Position& position, 
                                   const BatteryStatus& battery, AlertLevel alertLevel) {
    if (!initialized) return;
    
    clear();
    
    // Header con iconos de estado
    drawStatusBar();
    
    // Información principal
    setMediumFont();
    
    // Coordenadas GPS
    if (isValidPosition(position)) {
        char latStr[16], lngStr[16];
        formatCoordinate(latStr, position.latitude, true);
        formatCoordinate(lngStr, position.longitude, false);
        
        oledDisplay.drawString(0, 15, latStr);
        oledDisplay.drawString(0, 25, lngStr);
    } else {
        oledDisplay.drawString(0, 15, "GPS: No Fix");
        oledDisplay.drawString(0, 25, "-- , --");
    }
    
    // Estado de alerta
    if (alertLevel != AlertLevel::SAFE) {
        setSmallFont();
        oledDisplay.drawString(0, 40, "ALERTA:");
        oledDisplay.drawString(45, 40, alertLevelToString(alertLevel));
        
        // Icono de alerta parpadeante
        if ((millis() / 500) % 2) {
            drawAlertIcon(100, 38, alertLevel);
        }
    }
    
    // Información de sistema en la parte inferior
    setSmallFont();
    char statusLine[32];
    snprintf(statusLine, sizeof(statusLine), "UP:%02lum TX:%d", 
             status.uptime / 60, lastSystemStatus.resetCount);
    oledDisplay.drawString(0, 54, statusLine);
    
    display();
    currentScreen = SCREEN_MAIN;
    
    // Guardar estado para comparación
    lastSystemStatus = status;
    lastPosition = position;
    lastBattery = battery;
    lastAlertLevel = alertLevel;
}

void DisplayManager::showAlertScreen(AlertLevel level, float distance) {
    if (!initialized) return;
    
    clear();
    
    // Título parpadeante para alertas críticas
    bool blink = (level >= AlertLevel::DANGER) && ((millis() / 300) % 2);
    
    if (!blink || level < AlertLevel::DANGER) {
        setLargeFont();
        drawCenteredText("ALERTA", 5);
    }
    
    // Nivel de alerta
    setMediumFont();
    drawCenteredText(alertLevelToString(level), 20);
    
    // Distancia
    setSmallFont();
    char distStr[32];
    snprintf(distStr, sizeof(distStr), "Distancia: %.1fm", distance);
    drawCenteredText(distStr, 35);
    
    // Icono de alerta grande
    drawAlertIcon(64 - 8, 45, level);
    
    display();
    currentScreen = SCREEN_ALERT;
    lastDistance = distance;
}

void DisplayManager::showBatteryScreen(const BatteryStatus& battery) {
    if (!initialized) return;
    
    clear();
    
    setMediumFont();
    drawCenteredText("BATERIA", 5);
    
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

void DisplayManager::showGPSScreen(const Position& position) {
    if (!initialized) return;
    
    clear();
    
    setMediumFont();
    drawCenteredText("GPS INFO", 5);
    
    setSmallFont();
    
    if (isValidPosition(position)) {
        char latStr[20], lngStr[20];
        formatCoordinate(latStr, position.latitude, true);
        formatCoordinate(lngStr, position.longitude, false);
        
        oledDisplay.drawString(0, 18, "Lat:");
        oledDisplay.drawString(25, 18, latStr);
        
        oledDisplay.drawString(0, 28, "Lng:");
        oledDisplay.drawString(25, 28, lngStr);
        
        char altStr[16];
        snprintf(altStr, sizeof(altStr), "Alt: %.1fm", position.altitude);
        oledDisplay.drawString(0, 38, altStr);
        
        char satStr[16];
        snprintf(satStr, sizeof(satStr), "Sat: %d", position.satellites);
        oledDisplay.drawString(0, 48, satStr);
        
        char accStr[16];
        snprintf(accStr, sizeof(accStr), "Acc: %.1fm", position.accuracy);
        oledDisplay.drawString(0, 58, accStr);
    } else {
        drawCenteredText("Sin señal GPS", 30);
        drawCenteredText("Buscando satelites...", 40);
        drawLoadingAnimation(64 - 8, 50);
    }
    
    display();
    currentScreen = SCREEN_GPS;
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
        
        drawCenteredText(lineBuffer, y);
        y += 10;
        
        line += len;
        if (*line == '\n') line++;
    }
    
    display();
    currentScreen = SCREEN_ERROR;
}

// ============================================================================
// ELEMENTOS DE UI REUTILIZABLES
// ============================================================================

void DisplayManager::drawBatteryIcon(int16_t x, int16_t y, uint8_t percentage) {
    // Contorno de batería
    oledDisplay.drawRect(x, y, 20, 10);
    oledDisplay.drawRect(x + 20, y + 2, 2, 6); // Terminal positivo
    
    // Relleno según porcentaje
    int16_t fillWidth = (percentage * 18) / 100;
    if (fillWidth > 0) {
        oledDisplay.fillRect(x + 1, y + 1, fillWidth, 8);
    }
    
    // Indicador de batería baja
    if (percentage <= 20) {
        if ((millis() / 500) % 2) {
            oledDisplay.fillRect(x, y, 20, 10);
        }
    }
}

void DisplayManager::drawSignalIcon(int16_t x, int16_t y, uint8_t strength) {
    // Barras de señal (0-4)
    uint8_t bars = (strength * 4) / 100;
    
    for (uint8_t i = 0; i < 4; i++) {
        int16_t barHeight = 2 + (i * 2);
        if (i <= bars) {
            oledDisplay.fillRect(x + (i * 3), y + 8 - barHeight, 2, barHeight);
        } else {
            oledDisplay.drawRect(x + (i * 3), y + 8 - barHeight, 2, barHeight);
        }
    }
}

void DisplayManager::drawGPSIcon(int16_t x, int16_t y, bool connected) {
    if (connected) {
        // Icono GPS conectado (círculo con punto)
        oledDisplay.drawCircle(x + 4, y + 4, 4);
        oledDisplay.fillCircle(x + 4, y + 4, 2);
    } else {
        // Icono GPS desconectado (círculo vacío con X)
        oledDisplay.drawCircle(x + 4, y + 4, 4);
        oledDisplay.drawLine(x + 2, y + 2, x + 6, y + 6);
        oledDisplay.drawLine(x + 2, y + 6, x + 6, y + 2);
    }
}

void DisplayManager::drawAlertIcon(int16_t x, int16_t y, AlertLevel level) {
    switch (level) {
        case AlertLevel::CAUTION:
            // Triángulo de precaución
            oledDisplay.drawTriangle(x + 8, y, x, y + 12, x + 16, y + 12);
            oledDisplay.drawString(x + 6, y + 4, "!");
            break;
            
        case AlertLevel::WARNING:
            // Triángulo relleno
            oledDisplay.fillTriangle(x + 8, y, x, y + 12, x + 16, y + 12);
            oledDisplay.setColor(BLACK);
            oledDisplay.drawString(x + 6, y + 4, "!");
            oledDisplay.setColor(WHITE);
            break;
            
        case AlertLevel::DANGER:
        case AlertLevel::EMERGENCY:
            // Círculo con exclamación parpadeante
            if ((millis() / 200) % 2) {
                oledDisplay.fillCircle(x + 8, y + 6, 8);
                oledDisplay.setColor(BLACK);
                oledDisplay.drawString(x + 6, y + 2, "!");
                oledDisplay.setColor(WHITE);
            }
            break;
            
        default:
            break;
    }
}

void DisplayManager::drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t percentage) {
    // Contorno
    oledDisplay.drawRect(x, y, width, height);
    
    // Relleno
    int16_t fillWidth = (percentage * (width - 2)) / 100;
    if (fillWidth > 0) {
        oledDisplay.fillRect(x + 1, y + 1, fillWidth, height - 2);
    }
}

void DisplayManager::drawStatusBar() {
    // Línea superior
    oledDisplay.drawLine(0, 12, 128, 12);
    
    // Iconos de estado
    drawGPSIcon(0, 2, isValidPosition(lastPosition));
    drawSignalIcon(15, 2, lastSystemStatus.radioInitialized ? 75 : 0);
    drawBatteryIcon(100, 2, lastBattery.percentage);
    
    // Reloj/uptime en el centro
    char timeStr[16];
    uint32_t hours = (lastSystemStatus.uptime / 3600) % 24;
    uint32_t minutes = (lastSystemStatus.uptime / 60) % 60;
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", hours, minutes);
    
    setSmallFont();
    int16_t textWidth = getTextWidth(timeStr);
    oledDisplay.drawString(64 - textWidth/2, 2, timeStr);
}

// ============================================================================
// GESTIÓN DE PANTALLAS Y AUTO-SLEEP
// ============================================================================

void DisplayManager::setScreenMode(ScreenMode mode) {
    currentScreen = mode;
    updateLastActivity();
}

DisplayManager::ScreenMode DisplayManager::getCurrentScreenMode() const {
    return currentScreen;
}

void DisplayManager::setAutoSleep(bool enabled, uint32_t timeoutMs) {
    autoSleepEnabled = enabled;
    autoSleepTimeout = timeoutMs;
}

void DisplayManager::updateLastActivity() {
    lastActivity = millis();
}

void DisplayManager::update() {
    if (!initialized) return;
    
    animationCounter++;
    
    // Verificar auto-sleep
    if (autoSleepEnabled) {
        checkAutoSleep();
    }
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void DisplayManager::setupDisplay() {
    oledDisplay.displayOn();
    oledDisplay.clear();
    oledDisplay.flipScreenVertically();
    oledDisplay.setFont(ArialMT_Plain_10);
}

void DisplayManager::resetDisplay() {
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, LOW);
    delay(50);
    digitalWrite(rstPin, HIGH);
    delay(50);
}

void DisplayManager::formatCoordinate(char* buffer, double coordinate, bool isLatitude) {
    char direction = ' ';
    if (isLatitude) {
        direction = (coordinate >= 0) ? 'N' : 'S';
    } else {
        direction = (coordinate >= 0) ? 'E' : 'W';
    }
    
    snprintf(buffer, 16, "%.4f%c", abs(coordinate), direction);
}

void DisplayManager::drawLoadingAnimation(int16_t x, int16_t y) {
    uint8_t frame = (animationCounter / 4) % 8;
    
    for (uint8_t i = 0; i < 8; i++) {
        if (i == frame) {
            oledDisplay.fillCircle(x + (i % 4) * 4, y + (i / 4) * 4, 1);
        } else {
            oledDisplay.drawCircle(x + (i % 4) * 4, y + (i / 4) * 4, 1);
        }
    }
}

void DisplayManager::checkAutoSleep() {
    if (displayOn && (millis() - lastActivity > autoSleepTimeout)) {
        turnOff();
    }
}

int16_t DisplayManager::getTextWidth(const char* text, const uint8_t* font) {
    if (font) oledDisplay.setFont(font);
    return oledDisplay.getStringWidth(text);
}

void DisplayManager::setSmallFont() {
    oledDisplay.setFont(ArialMT_Plain_10);
}

void DisplayManager::setMediumFont() {
    oledDisplay.setFont(ArialMT_Plain_16);
}

void DisplayManager::setLargeFont() {
    oledDisplay.setFont(ArialMT_Plain_24);
}

void DisplayManager::drawCenteredText(const char* text, int16_t y, const uint8_t* font) {
    if (font) oledDisplay.setFont(font);
    int16_t textWidth = oledDisplay.getStringWidth(text);
    oledDisplay.drawString(64 - textWidth/2, y, text);
}
