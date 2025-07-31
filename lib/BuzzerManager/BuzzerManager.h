/**
 * ============================================================================
 * COLLAR GEOFENCING - MÓDULO BUZZER
 * ============================================================================
 * 
 * Sistema avanzado de buzzer con PWM optimizado para alertas progresivas
 * 
 * @file BuzzerManager.h
 * @version 3.0
 */

#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "driver/ledc.h"

class BuzzerManager {
private:
    static bool initialized;
    static bool playing;
    static uint32_t last_alert_time;
    static AlertLevel current_alert_level;
    
    // PWM interno
    static void setupPWM();
    static void playTone(uint32_t frequency, uint32_t duration_ms, uint8_t volume = 75);
    static void stopTone();

public:
    // *** INICIALIZACIÓN ***
    static bool init();
    static bool isInitialized();
    static void deinit();
    
    // *** CONTROL BÁSICO ***
    static void playTonePublic(uint32_t frequency, uint32_t duration_ms, uint8_t volume = 75);
    static void stop();
    static bool isPlaying();
    
    // *** MELODÍAS PREDEFINIDAS ***
    static void playStartupMelody();
    static void playShutdownMelody();
    static void playSuccessTone();
    static void playErrorTone();
    static void playWarningTone();
    
    // *** SISTEMA DE ALERTAS ***
    static void startAlert(AlertLevel level, float distance = 0.0);
    static void stopAlert();
    static void executeAlert();
    static AlertLevel getCurrentAlertLevel();
    
    // *** PATRONES DE ALERTA ***
    static void playCautionPattern();
    static void playWarningPattern();
    static void playDangerPattern();
    static void playEmergencyPattern();
    
    // *** CONFIGURACIÓN ***
    static void setVolume(uint8_t volume);  // 0-100
    static uint8_t getVolume();
    static void mute();
    static void unmute();
    static bool isMuted();
    
    // *** TESTING ***
    static void testAllPatterns();
    static void testFrequencyRange();
    static void testVolumeRange();
    
    // *** INFORMACIÓN ***
    static void printStatus();
};

#endif // BUZZER_MANAGER_H