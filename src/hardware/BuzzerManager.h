#pragma once
#include <Arduino.h>
#include "../config/pins.h"
#include "../config/constants.h"
#include "../core/Types.h"
#include "musical_notes.h" // Notas musicales

/*
 * ============================================================================
 * BUZZER MANAGER - GESTIÓN DE AUDIO Y ALERTAS
 * ============================================================================
 */

// Definiciones de volumen
#define VOLUME_LOW 64
#define VOLUME_MEDIUM 128
#define VOLUME_HIGH 255

class BuzzerManager
{
public:
    BuzzerManager(uint8_t pin = BUZZER_PIN);

    // Inicialización
    Result init();
    bool isInitialized() const;

    // Control básico de tonos
    void playTone(uint16_t frequency, uint16_t duration, uint8_t volume = VOLUME_MEDIUM);
    void playToneAsync(uint16_t frequency, uint16_t duration, uint8_t volume = VOLUME_MEDIUM);
    void stopTone();
    bool isPlaying() const;

    // Melodías predefinidas
    void playStartupMelody();
    void playShutdownMelody();
    void playSuccessTone();
    void playErrorTone();
    void playWarningTone();

    // Sistema de alertas progresivas
    void playAlertLevel(AlertLevel level);
    void playAlertTone(AlertLevel level) { playAlertLevel(level); } // Alias para compatibilidad
    // Sistema de alertas continuas
    void startContinuousAlert(AlertLevel level);
    void stopContinuousAlert();
    void updateContinuousAlert();
    bool isContinousAlertActive();

    // Configuración de alertas personalizadas
    void setAlertConfig(AlertLevel level, const AlertConfig &config);
    AlertConfig getAlertConfig(AlertLevel level) const;

    // Control de volumen y configuración
    void setVolume(uint8_t volume); // 0-100%
    uint8_t getVolume() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Update loop (llamar desde loop principal)
    void update();

private:
    uint8_t buzzerPin;
    bool initialized;
    bool enabled;
    uint8_t currentVolume;

    // Estado de reproducción asíncrona
    bool playing;
    uint32_t playStartTime;
    uint16_t playDuration;

    // Sistema de alertas continuas
    bool continuousAlertActive;
    AlertLevel currentAlertLevel;
    AlertConfig alertConfigs[3]; // Para cada AlertLevel: Safe, Caution y Warning
    uint32_t lastAlertTime;
    uint8_t alertRepetitionCount;

    // Métodos privados
    void setupPWM();
    void playToneInternal(uint16_t frequency, uint8_t volume);
    void stopToneInternal();
    uint32_t volumeToDutyCycle(uint8_t volume);

    // Manejo de alertas continuas
    void initializeAlertConfigs();

    // Melodías (definidas como arrays de frecuencias y duraciones)
    struct Note
    {
        uint16_t frequency;
        uint16_t duration;
    };

    void playMelody(const Note *melody, size_t noteCount, uint8_t volume = VOLUME_MEDIUM);

    // Melodías predefinidas
    static const Note STARTUP_MELODY[];
    static const Note SHUTDOWN_MELODY[];
    static const Note SUCCESS_MELODY[];
    static const Note ERROR_MELODY[];
    static const Note WARNING_MELODY[];
    static const size_t STARTUP_MELODY_SIZE;
    static const size_t SHUTDOWN_MELODY_SIZE;
    static const size_t SUCCESS_MELODY_SIZE;
    static const size_t ERROR_MELODY_SIZE;
    static const size_t WARNING_MELODY_SIZE;
};
