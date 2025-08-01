#pragma once
#include <Arduino.h>
#include <SSD1306Wire.h>
#include "config/pins.h"
#include "config/constants.h"
#include "core/Types.h"
#include "core/Logger.h"

/*
 * ============================================================================
 * DISPLAY MANAGER - GESTIÓN OLED Y PANTALLAS
 * ============================================================================
 */

class DisplayManager {
public:
    DisplayManager(uint8_t address = OLED_ADDR, uint8_t sda = OLED_SDA, 
                   uint8_t scl = OLED_SCL, uint8_t rst = OLED_RST);
    
    // Inicialización
    Result init();
    bool isInitialized() const;
    
    // Control básico del display
    void clear();
    void display();
    void setBrightness(uint8_t brightness);
    void turnOn();
    void turnOff();
    bool isOn() const;
    
    // Pantallas principales del sistema
    void showSplashScreen();
    void showMainScreen(const SystemStatus& status, const Position& position, 
                       const BatteryStatus& battery, AlertLevel alertLevel);
    void showAlertScreen(AlertLevel level, float distance);
    void showBatteryScreen(const BatteryStatus& battery);
    void showGPSScreen(const Position& position);
    void showStatsScreen(const SystemStats& stats);
    void showErrorScreen(const char* error);
    
    // Elementos de UI reutilizables
    void drawBatteryIcon(int16_t x, int16_t y, uint8_t percentage);
    void drawSignalIcon(int16_t x, int16_t y, uint8_t strength);
    void drawGPSIcon(int16_t x, int16_t y, bool connected);
    void drawAlertIcon(int16_t x, int16_t y, AlertLevel level);
    
    // Texto y elementos básicos
    void drawCenteredText(const char* text, int16_t y, const uint8_t* font = nullptr);
    void drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t percentage);
    void drawStatusBar();
    
    // Gestión automática de pantallas
    void setAutoSleep(bool enabled, uint32_t timeoutMs = OLED_TIMEOUT_SLEEP);
    void updateLastActivity();
    void update(); // Llamar desde loop principal
    
    // Configuración de pantallas
    enum ScreenMode {
        SCREEN_SPLASH,
        SCREEN_MAIN,
        SCREEN_ALERT,
        SCREEN_BATTERY,
        SCREEN_GPS,
        SCREEN_STATS,
        SCREEN_ERROR,
        SCREEN_OFF
    };
    
    void setScreenMode(ScreenMode mode);
    ScreenMode getCurrentScreenMode() const;
    
private:
    SSD1306Wire oledDisplay;  // Renombrado para evitar conflicto con función display()
    uint8_t rstPin;
    bool initialized;
    bool displayOn;
    uint8_t currentBrightness;
    
    // Gestión de pantallas
    ScreenMode currentScreen;
    uint32_t lastActivity;
    bool autoSleepEnabled;
    uint32_t autoSleepTimeout;
    
    // Datos actuales para refresh automático
    SystemStatus lastSystemStatus;
    Position lastPosition;
    BatteryStatus lastBattery;
    AlertLevel lastAlertLevel;
    float lastDistance;
    
    // Métodos privados de inicialización
    void setupDisplay();
    void resetDisplay();
    
    // Métodos de dibujado interno
    void drawHeader();
    void drawTime(int16_t x, int16_t y);
    void drawUptime(int16_t x, int16_t y, uint32_t uptime);
    void formatCoordinate(char* buffer, double coordinate, bool isLatitude);
    void drawWifiIcon(int16_t x, int16_t y, bool connected);
    void drawLoRaIcon(int16_t x, int16_t y, bool connected);
    
    // Animaciones simples
    void drawLoadingAnimation(int16_t x, int16_t y);
    void drawPulseAnimation(int16_t x, int16_t y, int16_t size);
    
    // Utilidades de texto
    void drawTextWithBackground(int16_t x, int16_t y, const char* text, 
                               bool inverted = false, const uint8_t* font = nullptr);
    int16_t getTextWidth(const char* text, const uint8_t* font = nullptr);
    
    // Gestión de auto-sleep
    void checkAutoSleep();
    
    // Variables para animaciones
    uint32_t animationCounter;
    
    // Fonts (definir los que se van a usar)
    void setSmallFont();
    void setMediumFont();
    void setLargeFont();
};
