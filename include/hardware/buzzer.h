/*
 * ============================================================================
 * MÓDULO BUZZER - Sistema PWM Optimizado
 * ============================================================================
 * 
 * Sistema de buzzer PWM de alta calidad para alertas de geofencing.
 * Incluye melodías, tonos individuales y patrones de alerta progresivos.
 * 
 * SOLUCIÓN: Configuración LEDC corregida para ESP32-S3
 * ============================================================================
 */

#ifndef BUZZER_H
#define BUZZER_H

#include "config.h"
#include "driver/ledc.h"

// ============================================================================
// CLASE BUZZER
// ============================================================================
class BuzzerController {
private:
    bool initialized;
    uint8_t current_volume;
    
    // Configuración LEDC correcta para ESP32-S3
    bool setupLEDC();
    void setFrequency(uint32_t frequency);
    void setVolume(uint8_t volume);
    
public:
    BuzzerController();
    ~BuzzerController();
    
    // Inicialización y control
    bool begin();
    void end();
    bool isInitialized() const { return initialized; }
    
    // Control básico de tonos
    void playTone(uint32_t frequency, uint32_t duration_ms, uint8_t volume = VOLUME_MID);
    void playToneAsync(uint32_t frequency, uint8_t volume = VOLUME_MID);
    void stopTone();
    
    // Melodías predefinidas
    void playStartupMelody();
    void playShutdownMelody();
    void playSuccessTone();
    void playErrorTone();
    void playWarningTone();
    
    // Alertas de geofencing
    void playCautionAlert();
    void playWarningAlert();
    void playDangerAlert();
    void playEmergencyAlert();
    
    // Patrones especiales
    void playHeartbeat();
    void playSOS();
    void playConnectionTone();
    void playBatteryLowTone();
    
    // Control de volumen
    void setMasterVolume(uint8_t volume);
    uint8_t getMasterVolume() const { return current_volume; }
    
    // Test y diagnóstico
    void runSelfTest();
    void playFrequencySweep(uint32_t start_freq, uint32_t end_freq, uint32_t duration_ms);
};

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
extern BuzzerController Buzzer;

#endif // BUZZER_H