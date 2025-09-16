#include "BuzzerManager.h"
#include "../core/Logger.h"

// ============================================================================
// MELODÍAS PREDEFINIDAS
// ============================================================================
const BuzzerManager::Note BuzzerManager::STARTUP_MELODY[] = {
    {NOTE_C4, 200}, {NOTE_E4, 200}, {NOTE_G4, 200}, {NOTE_C5, 400}};

const BuzzerManager::Note BuzzerManager::SHUTDOWN_MELODY[] = {
    {NOTE_C5, 200}, {NOTE_G4, 200}, {NOTE_E4, 200}, {NOTE_C4, 400}};

const BuzzerManager::Note BuzzerManager::SUCCESS_MELODY[] = {
    {NOTE_G4, 150}, {NOTE_C5, 150}, {NOTE_E5, 300}};

const BuzzerManager::Note BuzzerManager::ERROR_MELODY[] = {
    {NOTE_A4, 100}, {0, 50}, {NOTE_A4, 100}, {0, 50}, {NOTE_A4, 200}};

const BuzzerManager::Note BuzzerManager::WARNING_MELODY[] = {
    {NOTE_F4, 200}, {NOTE_A4, 200}, {NOTE_F4, 200}};

const size_t BuzzerManager::STARTUP_MELODY_SIZE = sizeof(STARTUP_MELODY) / sizeof(Note);
const size_t BuzzerManager::SHUTDOWN_MELODY_SIZE = sizeof(SHUTDOWN_MELODY) / sizeof(Note);
const size_t BuzzerManager::SUCCESS_MELODY_SIZE = sizeof(SUCCESS_MELODY) / sizeof(Note);
const size_t BuzzerManager::ERROR_MELODY_SIZE = sizeof(ERROR_MELODY) / sizeof(Note);
const size_t BuzzerManager::WARNING_MELODY_SIZE = sizeof(WARNING_MELODY) / sizeof(Note);

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

BuzzerManager::BuzzerManager(uint8_t pin) : buzzerPin(pin),
                                            initialized(false),
                                            enabled(true),
                                            currentVolume(VOLUME_MEDIUM),
                                            playing(false),
                                            playStartTime(0),
                                            playDuration(0),
                                            continuousAlertActive(false),
                                            currentAlertLevel(AlertLevel::SAFE),
                                            lastAlertTime(0),
                                            alertRepetitionCount(0)
{
    initializeAlertConfigs();
}

Result BuzzerManager::init()
{
    if (initialized)
    {
        return Result::SUCCESS;
    }

    LOG_I("🎵 Inicializando Buzzer Manager...");

    // Configurar pin
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);

    // Configurar PWM
    setupPWM();

    initialized = true;
    LOG_INIT("Buzzer Manager", true);

    return Result::SUCCESS;
}

bool BuzzerManager::isInitialized() const
{
    return initialized;
}

// ============================================================================
// CONTROL BÁSICO DE TONOS
// ============================================================================

void BuzzerManager::playTone(uint16_t frequency, uint16_t duration, uint8_t volume)
{
    if (!initialized || !enabled)
        return;

    playToneInternal(frequency, volume);
    delay(duration);
    stopToneInternal();
}

void BuzzerManager::playToneAsync(uint16_t frequency, uint16_t duration, uint8_t volume)
{
    if (!initialized || !enabled)
        return;

    playToneInternal(frequency, volume);
    playing = true;
    playStartTime = millis();
    playDuration = duration;
}

void BuzzerManager::stopTone()
{
    stopToneInternal();
    playing = false;
}

bool BuzzerManager::isPlaying() const
{
    return playing;
}

// ============================================================================
// MELODÍAS PREDEFINIDAS
// ============================================================================

void BuzzerManager::playStartupMelody()
{
    if (!initialized || !enabled)
        return;
    LOG_D("🎵 Reproduciendo melodía de inicio");
    playMelody(STARTUP_MELODY, STARTUP_MELODY_SIZE, VOLUME_MEDIUM);
}

void BuzzerManager::playShutdownMelody()
{
    if (!initialized || !enabled)
        return;
    LOG_D("🎵 Reproduciendo melodía de apagado");
    playMelody(SHUTDOWN_MELODY, SHUTDOWN_MELODY_SIZE, VOLUME_MEDIUM);
}

void BuzzerManager::playSuccessTone()
{
    if (!initialized || !enabled)
        return;
    playMelody(SUCCESS_MELODY, SUCCESS_MELODY_SIZE, VOLUME_MEDIUM);
}

void BuzzerManager::playErrorTone()
{
    if (!initialized || !enabled)
        return;
    playMelody(ERROR_MELODY, ERROR_MELODY_SIZE, VOLUME_HIGH);
}

void BuzzerManager::playWarningTone()
{
    if (!initialized || !enabled)
        return;
    playMelody(WARNING_MELODY, WARNING_MELODY_SIZE, VOLUME_HIGH);
}

// ============================================================================
// SISTEMA DE ALERTAS
// ============================================================================

void BuzzerManager::playAlertLevel(AlertLevel level)
{
    if (!initialized || !enabled)
        return;

    const AlertConfig &config = alertConfigs[static_cast<uint8_t>(level)];

    if (config.enabled)
    {
        playTone(config.frequency, config.duration, config.volume);
        LOG_D("🔊 Alerta nivel %s: %dHz, %dms",
              alertLevelToString(level), config.frequency, config.duration);
    }
}

void BuzzerManager::startContinuousAlert(AlertLevel level)
{
    if (!initialized || !enabled)
        return;

    continuousAlertActive = true;
    currentAlertLevel = level;
    lastAlertTime = 0; // Forzar reproducción inmediata
    alertRepetitionCount = 0;

    LOG_D("🚨 Iniciando alerta continua nivel %s", alertLevelToString(level));
}

void BuzzerManager::stopContinuousAlert()
{
    continuousAlertActive = false;
    stopTone();
    LOG_D("🔇 Deteniendo alerta continua");
}

bool BuzzerManager::isContinousAlertActive()
{
    return continuousAlertActive;
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void BuzzerManager::setAlertConfig(AlertLevel level, const AlertConfig &config)
{
    alertConfigs[static_cast<uint8_t>(level)] = config;
}

AlertConfig BuzzerManager::getAlertConfig(AlertLevel level) const
{
    return alertConfigs[static_cast<uint8_t>(level)];
}

void BuzzerManager::setVolume(uint8_t volume)
{
    currentVolume = min(volume, (uint8_t)100);
}

uint8_t BuzzerManager::getVolume() const
{
    return currentVolume;
}

void BuzzerManager::setEnabled(bool enabled)
{
    this->enabled = enabled;
    if (!enabled)
    {
        stopTone();
        stopContinuousAlert();
    }
}

bool BuzzerManager::isEnabled() const
{
    return enabled;
}

// ============================================================================
// UPDATE LOOP
// ============================================================================

void BuzzerManager::update()
{
    if (!initialized)
        return;

    uint32_t currentTime = millis();

    // Manejar reproducción asíncrona
    if (playing && (currentTime - playStartTime >= playDuration))
    {
        stopTone();
    }

    // Manejar alertas continuas
    if (continuousAlertActive)
    {
        updateContinuousAlert();
    }
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void BuzzerManager::setupPWM()
{
    // Configurar canal PWM para ESP32
    ledcSetup(0, 5000, 10); // Canal 0, frecuencia base 5kHz, resolución 10-bit
    ledcAttachPin(buzzerPin, 0);
}

void BuzzerManager::playToneInternal(uint16_t frequency, uint8_t volume)
{
    if (frequency == 0)
    {
        stopToneInternal();
        return;
    }

    // Configurar frecuencia PWM
    ledcWriteTone(0, frequency);

    // Configurar duty cycle basado en volumen
    uint32_t dutyCycle = volumeToDutyCycle(volume);
    ledcWrite(0, dutyCycle);
}

void BuzzerManager::stopToneInternal()
{
    ledcWrite(0, 0);
}

uint32_t BuzzerManager::volumeToDutyCycle(uint8_t volume)
{
    // Convertir volumen (0-100%) a duty cycle (0-1023 para 10-bit)
    return (volume * 1023) / (100 * 2); // Dividir por 2 para 50% duty cycle máximo
}

void BuzzerManager::updateContinuousAlert()
{
    const AlertConfig &config = alertConfigs[static_cast<uint8_t>(currentAlertLevel)];
    uint32_t currentTime = millis();

    if (!config.enabled)
    {
        LOG_D("updateContinuousAlert leyó config.enabled como false y ejecuta stopContinousAlert");
        stopContinuousAlert();
        return;
    }

    // Verificar si es tiempo de reproducir el siguiente tono
    if (currentTime - lastAlertTime >= config.interval)
    {
        LOG_D("updateContinuousAlert leyó que es tiempo de reproducir el sgte tono");
        // Verificar límite de repeticiones
        if (config.repetitions > 0 && alertRepetitionCount >= config.repetitions)
        {
            LOG_D("updateContinuousAlert leyó que llegamos al límite de repeticiones del tono y ejecuta stopContinousAlert");
            stopContinuousAlert();
            return;
        }

        LOG_D("updateContinuousAlert reproduce el tono llamando playToneAsync, con freq:%d dur:%d vol: %d", config.frequency, config.duration, config.volume);
        // Reproducir tono
        playToneAsync(config.frequency, config.duration, config.volume);
        lastAlertTime = currentTime;
        alertRepetitionCount++;
    }
}

void BuzzerManager::initializeAlertConfigs()
{
    alertConfigs[static_cast<uint8_t>(AlertLevel::SAFE)] = AlertConfig(0, 0, 0);
    alertConfigs[static_cast<uint8_t>(AlertLevel::CAUTION)] = AlertConfig(FREQ_CAUTION, TONE_DURATION_SHORT, VOLUME_LOW);
    alertConfigs[static_cast<uint8_t>(AlertLevel::WARNING)] = AlertConfig(FREQ_WARNING, TONE_DURATION_MEDIUM, VOLUME_MEDIUM);

    // Configurar intervalos específicos para cada nivel
    alertConfigs[static_cast<uint8_t>(AlertLevel::CAUTION)].interval = 1000; // Cada 1s
    alertConfigs[static_cast<uint8_t>(AlertLevel::WARNING)].interval = 1000; // Cada 1s
}

void BuzzerManager::playMelody(const Note *melody, size_t noteCount, uint8_t volume)
{
    for (size_t i = 0; i < noteCount; i++)
    {
        if (melody[i].frequency > 0)
        {
            playTone(melody[i].frequency, melody[i].duration, volume);
        }
        else
        {
            delay(melody[i].duration); // Pausa
        }
        delay(50); // Pequeña pausa entre notas
    }
}
