/**
 * ============================================================================
 * COLLAR GEOFENCING - MÓDULO DISPLAY
 * ============================================================================
 * 
 * Gestión del display OLED para mostrar información del sistema
 * 
 * @file DisplayManager.h
 * @version 3.0
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <SSD1306Wire.h>
#include "config.h"

class DisplayManager {
private:
    static SSD1306Wire* display;
    static bool initialized;
    static bool enabled;
    static uint32_t last_update;
    
    // Pantallas
    static void drawBootScreen();
    static void drawMainScreen();
    static void drawAlertScreen();
    static void drawGPSScreen();
    static void drawBatteryScreen();
    static void drawRadioScreen();
    
    // Utilidades de dibujo
    static void drawProgressBar(int x, int y, int width, int height, int progress);
    static void drawSignalBars(int x, int y, int strength);
    static void drawBatteryIcon(int x, int y, int percentage);
    static void drawGPSIcon(int x, int y, bool valid);

public:
    // *** INICIALIZACIÓN ***
    static bool init();
    static bool isInitialized();
    static void deinit();
    
    // *** CONTROL BÁSICO ***
    static void clear();
    static void display();
    static void turnOn();
    static void turnOff();
    static void setBrightness(uint8_t brightness);
    
    // *** GESTIÓN DE PANTALLAS ***
    static void showBootScreen();
    static void showMainScreen(const Position& pos, const BatteryStatus& battery, const HardwareStatus& hw);
    static void showAlertScreen(AlertLevel level, float distance);
    static void showGPSScreen(const Position& pos);
    static void showBatteryScreen(const BatteryStatus& battery);
    static void showRadioScreen(bool connected, uint16_t packets, int rssi = 0);
    static void showErrorScreen(const String& error);
    
    // *** ELEMENTOS DE UI ***
    static void showMessage(const String& message, uint16_t duration = 2000);
    static void showSplashScreen(const String& title, const String& subtitle = "");
    static void showQRCode(const String& text);  // Para futuro
    
    // *** CONFIGURACIÓN ***
    static void enable();
    static void disable();
    static bool isEnabled();
    static void setAutoOff(uint32_t timeout_ms);
    
    // *** UTILIDADES ***
    static void test();
    static void printStatus();
    static void benchmark();
};

#endif // DISPLAY_MANAGER_H