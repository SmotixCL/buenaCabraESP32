/*
 * ============================================================================
 * MÓDULO OLED DISPLAY - Interfaz Visual V3
 * ============================================================================
 * 
 * Sistema de display OLED optimizado para Heltec V3 con pins corregidos.
 * Incluye múltiples pantallas, animaciones y gestión de estados.
 * ============================================================================
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "config.h"
#include <SSD1306Wire.h>

// ============================================================================
// ENUMERACIONES
// ============================================================================
enum DisplayScreen {
    SCREEN_SPLASH = 0,
    SCREEN_STATUS,
    SCREEN_POSITION,
    SCREEN_ALERT,
    SCREEN_BATTERY,
    SCREEN_RADIO,
    SCREEN_DEBUG,
    SCREEN_COUNT  // Número total de pantallas
};

enum DisplayAnimation {
    ANIM_NONE = 0,
    ANIM_FADE,
    ANIM_SLIDE,
    ANIM_PULSE
};

// ============================================================================
// CLASE OLED CONTROLLER
// ============================================================================
class OLEDController {
private:
    SSD1306Wire* display;
    bool initialized;
    DisplayScreen current_screen;
    uint32_t last_update;
    uint32_t screen_timer;
    bool auto_rotate;
    uint8_t brightness;
    
    // Métodos privados de renderizado
    void drawSplashScreen();
    void drawStatusScreen();
    void drawPositionScreen();
    void drawAlertScreen();
    void drawBatteryScreen();
    void drawRadioScreen();
    void drawDebugScreen();
    
    // Utilidades de dibujo
    void drawProgressBar(int x, int y, int width, int height, int progress);
    void drawSignalStrength(int x, int y, int strength);
    void drawBatteryIcon(int x, int y, int percentage, bool charging = false);
    void drawAlertIcon(int x, int y, uint8_t level);
    void drawConnectionStatus(int x, int y, bool connected);
    
    // Animaciones
    void fadeTransition();
    void slideTransition(bool left_to_right = true);
    
public:
    OLEDController();
    ~OLEDController();
    
    // Inicialización y control
    bool begin();
    void end();
    bool isInitialized() const { return initialized; }
    
    // Control de pantallas
    void update();
    void setScreen(DisplayScreen screen);
    void nextScreen();
    void previousScreen();
    DisplayScreen getCurrentScreen() const { return current_screen; }
    
    // Configuración
    void setAutoRotate(bool enable, uint32_t interval_ms = 5000);
    void setBrightness(uint8_t level); // 0-255
    void setOrientation(bool flip = false);
    
    // Mensajes rápidos
    void showMessage(const char* message, uint32_t duration_ms = 2000);
    void showAlert(const char* title, const char* message, uint32_t duration_ms = 3000);
    void showProgress(const char* title, int progress, const char* status = nullptr);
    
    // Control directo
    void clear();
    void display();
    SSD1306Wire* getDisplay() { return display; }
    
    // Test y diagnóstico
    void runSelfTest();
    void showTestPattern();
};

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
extern OLEDController Display;

#endif // OLED_DISPLAY_H