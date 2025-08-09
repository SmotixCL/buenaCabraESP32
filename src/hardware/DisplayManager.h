// DisplayManager.h - Sistema de display mejorado con múltiples pantallas
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#pragma once

#include <Arduino.h>
#include <SSD1306Wire.h>
#include "config/pins.h"
#include "config/constants.h"
#include "core/Types.h"
#include "core/Logger.h"

// ============================================================================
// ENUMERACIONES Y TIPOS ADICIONALES
// ============================================================================

enum class GeofenceType {
    CIRCLE = 0,
    POLYGON = 1,
    RECTANGLE = 2
};

// ============================================================================
// CLASE DISPLAY MANAGER
// ============================================================================

class DisplayManager {
public:
    // Constructor
    DisplayManager(uint8_t address = OLED_ADDR, uint8_t sda = OLED_SDA, 
                   uint8_t scl = OLED_SCL, uint8_t rst = OLED_RST);
    
    // === INICIALIZACIÓN ===
    Result init();
    bool isInitialized() const;
    
    // === CONTROL BÁSICO DEL DISPLAY ===
    void clear();
    void display();
    void setBrightness(uint8_t brightness);
    void turnOn();
    void turnOff();
    bool isOn() const;
    
    // === PANTALLAS PRINCIPALES ===
    
    // Pantalla de inicio con animación de carga
    void showBootScreen();
    void showSplashScreen();
    
    // Pantalla principal con información resumida
    void showMainScreen(const SystemStatus& status, const Position& position, 
                       const BatteryStatus& battery, AlertLevel alertLevel);
    
    // Pantalla de detalles GPS
    void showGPSDetailScreen(const Position& position);
    
    // Pantalla de información de geocerca
    void showGeofenceInfoScreen(const Geofence& geofence, float distance, bool inside);
    
    // Pantalla de estadísticas del sistema
    void showSystemStatsScreen(const SystemStats& stats);
    
    // Pantallas de alerta y error
    void showAlertScreen(AlertLevel level, float distance);
    void showBatteryScreen(const BatteryStatus& battery);
    void showErrorScreen(const char* error);
    
    // === NAVEGACIÓN ENTRE PANTALLAS ===
    void nextScreen();
    void previousScreen();
    void setScreen(uint8_t screenIndex);
    uint8_t getCurrentScreen() const;
    
    // === ACTUALIZACIÓN DE INFORMACIÓN ===
    void updateCounters(uint16_t txCount, uint16_t rxCount);
    void updateGeofenceInfo(const char* name, GeofenceType type, 
                          float radius, float distance, bool inside);
    void updateSystemStats(const SystemStats& stats);
    
    // === ELEMENTOS DE UI ===
    
    // Iconos y elementos gráficos
    void drawBatteryIcon(int16_t x, int16_t y, uint8_t percentage);
    void drawSignalIcon(int16_t x, int16_t y, uint8_t strength);
    void drawGPSIcon(int16_t x, int16_t y, bool connected);
    void drawAlertIcon(int16_t x, int16_t y, AlertLevel level);
    void drawGeofenceIcon(int16_t x, int16_t y, GeofenceType type);
    
    // Barras de progreso y gráficos
    void drawProgressBar(int16_t x, int16_t y, int16_t width, 
                        int16_t height, uint8_t percentage);
    void drawGraph(int16_t x, int16_t y, int16_t width, int16_t height, 
                  float* values, uint8_t count);
    
    // Texto y fuentes
    void drawCenteredText(const char* text, int16_t y);
    void drawScrollingText(const char* text, int16_t y, int16_t& offset);
    void setSmallFont();
    void setMediumFont();
    void setLargeFont();
    
    // === ANIMACIONES ===
    void showLoadingAnimation(uint8_t progress);
    void showConnectingAnimation();
    void showSearchingAnimation();
    void flashScreen(uint8_t times = 3);
    
    // === GESTIÓN DE PANTALLA ===
    
    // Auto-apagado y gestión de energía
    void setAutoSleep(bool enabled, uint32_t timeoutMs = OLED_TIMEOUT_SLEEP);
    void updateLastActivity();
    bool isAutoSleepEnabled() const;
    
    // Rotación automática de pantallas
    void setAutoRotate(bool enabled, uint32_t intervalMs = 10000);
    bool isAutoRotateEnabled() const;
    
    // Manejo de botones
    void handleButtonPress();
    void handleLongPress();
    
    // === UTILIDADES ===
    
    // Actualización periódica (llamar desde loop)
    void update();
    
    // Información de estado
    void printStatus();
    const char* getCurrentScreenName() const;
    
    // Test y debugging
    void testAllScreens();
    void showTestPattern();
    
    // === MODOS ESPECIALES ===
    
    // Modo de configuración
    void showConfigScreen(const char* ssid, const char* ip);
    
    // Modo de actualización OTA
    void showOTAProgress(uint8_t percentage);
    
    // Modo nocturno (brillo reducido)
    void setNightMode(bool enabled);
    
private:
    // === VARIABLES PRIVADAS ===
    SSD1306Wire oledDisplay;
    uint8_t rstPin;
    bool initialized;
    bool displayOn;
    uint8_t currentBrightness;
    bool nightMode;
    
    // Gestión de pantallas
    enum ScreenMode {
        SCREEN_SPLASH,
        SCREEN_MAIN,
        SCREEN_GPS_DETAIL,
        SCREEN_GEOFENCE_INFO,
        SCREEN_SYSTEM_STATS,
        SCREEN_ALERT,
        SCREEN_BATTERY,
        SCREEN_ERROR,
        SCREEN_CONFIG,
        SCREEN_OFF
    };
    
    ScreenMode currentScreen;
    uint32_t lastActivity;
    bool autoSleepEnabled;
    uint32_t autoSleepTimeout;
    bool autoRotateEnabled;
    uint32_t autoRotateInterval;
    uint32_t lastRotation;
    
    // Cache de datos para actualización
    SystemStatus lastSystemStatus;
    Position lastPosition;
    BatteryStatus lastBattery;
    AlertLevel lastAlertLevel;
    float lastDistance;
    
    // === MÉTODOS PRIVADOS ===
    
    // Helpers para dibujo
    void drawStatusBar();
    void drawNavigationHints();
    void drawFrame(int16_t x, int16_t y, int16_t width, int16_t height);
    
    // Formateo de texto
    void formatCoordinate(char* buffer, double coord, bool isLatitude);
    void formatTime(char* buffer, uint32_t seconds);
    void formatDistance(char* buffer, float meters);
    
    // Validación
    bool isValidPosition(const Position& pos) const;
    const char* alertLevelToString(AlertLevel level) const;
    
    // Animaciones internas
    void drawLoadingDots(int16_t x, int16_t y);
    void drawWaveAnimation(int16_t x, int16_t y, int16_t width);
};

// ============================================================================
// FUNCIONES AUXILIARES GLOBALES
// ============================================================================

// Conversión de tipos a strings
const char* geofenceTypeToString(GeofenceType type);
const char* screenModeToString(uint8_t mode);

// Formateo de unidades
String formatBatteryVoltage(float voltage);
String formatGPSCoordinate(double coord, bool isLatitude);
String formatUptime(uint32_t milliseconds);

// ============================================================================
// MACROS DE CONFIGURACIÓN
// ============================================================================

// Configuración de fuentes
#define FONT_SMALL      ArialMT_Plain_10
#define FONT_MEDIUM     ArialMT_Plain_16  
#define FONT_LARGE      ArialMT_Plain_24

// Configuración de tiempos
#define SCREEN_UPDATE_INTERVAL      1000    // 1 segundo
#define ANIMATION_FRAME_DELAY       100     // 100ms por frame
#define BUTTON_LONG_PRESS_TIME      1000    // 1 segundo para long press
#define AUTO_ROTATE_DEFAULT_TIME    10000   // 10 segundos por pantalla

// Configuración de elementos UI
#define PROGRESS_BAR_HEIGHT         8
#define BATTERY_ICON_WIDTH          22
#define SIGNAL_BARS_COUNT           5
#define GRAPH_MAX_POINTS            20

#endif // DISPLAY_MANAGER_H