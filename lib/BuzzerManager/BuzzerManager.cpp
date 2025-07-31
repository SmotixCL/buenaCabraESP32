/**
 * ============================================================================
 * COLLAR GEOFENCING - IMPLEMENTACIÓN BUZZER MANAGER
 * ============================================================================
 * 
 * @file BuzzerManager.cpp
 * @version 3.0
 */

#include "BuzzerManager.h"

// Variables estáticas
bool BuzzerManager::initialized = false;
bool BuzzerManager::playing = false;
uint32_t BuzzerManager::last_alert_time = 0;
AlertLevel BuzzerManager::current_alert_level = ALERT_SAFE;

// Variables de configuración
static uint8_t global_volume = 75;
static bool muted = false;

bool BuzzerManager::init() {
    DEBUG_INFO("🎵 Inicializando sistema de buzzer...");
    
    if (!setupPWM()) {
        DEBUG_ERROR("❌ Error configurando PWM para buzzer");
        return false;
    }
    
    initialized = true;
    playing = false;
    current_alert_level = ALERT_SAFE;
    
    DEBUG_INFO("✅ Buzzer inicializado correctamente");
    
    // Test rápido de funcionamiento
    playTone(BUZZER_FREQ_MID, 100, 50);
    delay(200);
    
    return true;
}

void BuzzerManager::setupPWM() {
    // Configurar timer PWM
    ledc_timer_config_t timer_conf = {
        .speed_mode = BUZZER_PWM_SPEED_MODE,
        .timer_num = BUZZER_PWM_TIMER,
        .duty_resolution = BUZZER_PWM_RESOLUTION,
        .freq_hz = BUZZER_FREQ_MID,
        .clk_cfg = LEDC_AUTO_CLK
    };
    
    if (ledc_timer_config(&timer_conf) != ESP_OK) {
        DEBUG_ERROR("❌ Error configurando timer PWM");
        return;
    }
    
    // Configurar canal PWM
    ledc_channel_config_t channel_conf = {
        .channel = BUZZER_PWM_CHANNEL,
        .duty = 0,
        .gpio_num = BUZZER_PIN,
        .speed_mode = BUZZER_PWM_SPEED_MODE,
        .timer_sel = BUZZER_PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE
    };
    
    if (ledc_channel_config(&channel_conf) != ESP_OK) {
        DEBUG_ERROR("❌ Error configurando canal PWM");
        return;
    }
    
    DEBUG_INFO("✅ PWM configurado (10-bit, alta resolución)");
}

void BuzzerManager::playTone(uint32_t frequency, uint32_t duration_ms, uint8_t volume) {
    if (!initialized || muted) return;
    
    // Limitar volumen
    if (volume > 100) volume = 100;
    
    // Aplicar volumen global
    volume = (volume * global_volume) / 100;
    
    // Calcular duty cycle (0-1023 para 10-bit)
    uint32_t duty = (1023 * volume) / 100;
    
    playing = true;
    
    // Configurar frecuencia y activar
    ledc_set_freq(BUZZER_PWM_SPEED_MODE, BUZZER_PWM_TIMER, frequency);
    ledc_set_duty(BUZZER_PWM_SPEED_MODE, BUZZER_PWM_CHANNEL, duty);
    ledc_update_duty(BUZZER_PWM_SPEED_MODE, BUZZER_PWM_CHANNEL);
    
    // Esperar duración
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        stopTone();
    }
}

void BuzzerManager::stopTone() {
    ledc_set_duty(BUZZER_PWM_SPEED_MODE, BUZZER_PWM_CHANNEL, 0);
    ledc_update_duty(BUZZER_PWM_SPEED_MODE, BUZZER_PWM_CHANNEL);
    playing = false;
}

void BuzzerManager::playTonePublic(uint32_t frequency, uint32_t duration_ms, uint8_t volume) {
    playTone(frequency, duration_ms, volume);
}

void BuzzerManager::stop() {
    stopTone();
    current_alert_level = ALERT_SAFE;
}

bool BuzzerManager::isPlaying() {
    return playing;
}

bool BuzzerManager::isInitialized() {
    return initialized;
}

void BuzzerManager::playStartupMelody() {
    if (!initialized) return;
    
    DEBUG_INFO("🎵 Reproduciendo melodía de inicio...");
    
    // Melodía ascendente alegre
    playTone(NOTE_C4, 200, 60);
    delay(50);
    playTone(NOTE_E4, 200, 70);
    delay(50);
    playTone(NOTE_G4, 200, 80);
    delay(50);
    playTone(NOTE_C5, 300, 90);
    delay(100);
    playTone(NOTE_E5, 400, 100);
    delay(200);
    
    // Confirmación final
    playTone(BUZZER_FREQ_MID, 300, 80);
    delay(100);
    playTone(BUZZER_FREQ_HIGH, 500, 90);
    
    DEBUG_INFO("✅ Melodía de inicio completada");
}

void BuzzerManager::playShutdownMelody() {
    if (!initialized) return;
    
    DEBUG_INFO("🎵 Reproduciendo melodía de apagado...");
    
    // Melodía descendente
    playTone(NOTE_G5, 200, 80);
    delay(50);
    playTone(NOTE_E5, 200, 70);
    delay(50);
    playTone(NOTE_C5, 200, 60);
    delay(50);
    playTone(NOTE_G4, 400, 50);
    
    DEBUG_INFO("✅ Melodía de apagado completada");
}

void BuzzerManager::playSuccessTone() {
    playTone(BUZZER_FREQ_MID, 150, 70);
    delay(80);
    playTone(BUZZER_FREQ_HIGH, 200, 80);
}

void BuzzerManager::playErrorTone() {
    playTone(BUZZER_FREQ_LOW, 300, 80);
    delay(100);
    playTone(BUZZER_FREQ_LOW, 300, 80);
}

void BuzzerManager::playWarningTone() {
    playTone(BUZZER_FREQ_HIGH, 100, 85);
    delay(50);
    playTone(BUZZER_FREQ_HIGH, 100, 85);
    delay(50);
    playTone(BUZZER_FREQ_HIGH, 100, 85);
}

void BuzzerManager::startAlert(AlertLevel level, float distance) {
    current_alert_level = level;
    last_alert_time = millis();
    
    DEBUG_INFO("🚨 Iniciando alerta nivel " + String(level) + " (dist: " + String(distance) + "m)");
}

void BuzzerManager::stopAlert() {
    if (current_alert_level <= ALERT_CAUTION) {
        current_alert_level = ALERT_SAFE;
        stopTone();
        DEBUG_INFO("✅ Alerta detenida - zona segura");
    }
}

void BuzzerManager::executeAlert() {
    if (current_alert_level == ALERT_SAFE || !initialized) return;
    
    uint32_t current_time = millis();
    uint32_t time_in_alert = current_time - last_alert_time;
    
    // Ejecutar patrón según nivel
    switch (current_alert_level) {
        case ALERT_CAUTION:
            if (time_in_alert % 8000 < 200) {
                playCautionPattern();
            }
            break;
            
        case ALERT_WARNING:
            if (time_in_alert % 5000 < 200) {
                playWarningPattern();
            }
            break;
            
        case ALERT_DANGER:
            if (time_in_alert % 3000 < 300) {
                playDangerPattern();
            }
            break;
            
        case ALERT_EMERGENCY:
            if (time_in_alert % 2000 < 400) {
                playEmergencyPattern();
            }
            break;
            
        default:
            break;
    }
}

void BuzzerManager::playCautionPattern() {
    playTone(BUZZER_FREQ_LOW, 300, 60);
}

void BuzzerManager::playWarningPattern() {
    playTone(BUZZER_FREQ_MID, 150, 75);
    delay(100);
    playTone(BUZZER_FREQ_MID, 150, 75);
}

void BuzzerManager::playDangerPattern() {
    for (int i = 0; i < 3; i++) {
        playTone(BUZZER_FREQ_HIGH, 100, 85);
        delay(50);
    }
}

void BuzzerManager::playEmergencyPattern() {
    // Sirena escalada
    for (int freq = 2000; freq <= 4000; freq += 200) {
        playTone(freq, 30, 95);
    }
}

AlertLevel BuzzerManager::getCurrentAlertLevel() {
    return current_alert_level;
}

void BuzzerManager::setVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    global_volume = volume;
    DEBUG_INFO("🔊 Volumen configurado: " + String(volume) + "%");
}

uint8_t BuzzerManager::getVolume() {
    return global_volume;
}

void BuzzerManager::mute() {
    muted = true;
    stopTone();
    DEBUG_INFO("🔇 Buzzer silenciado");
}

void BuzzerManager::unmute() {
    muted = false;
    DEBUG_INFO("🔊 Buzzer activado");
}

bool BuzzerManager::isMuted() {
    return muted;
}

void BuzzerManager::testAllPatterns() {
    DEBUG_INFO("🧪 === TEST PATRONES DE ALERTA ===");
    
    String pattern_names[] = {"Precaución", "Advertencia", "Peligro", "Emergencia"};
    AlertLevel levels[] = {ALERT_CAUTION, ALERT_WARNING, ALERT_DANGER, ALERT_EMERGENCY};
    
    for (int i = 0; i < 4; i++) {
        DEBUG_INFO("🎯 Test: " + pattern_names[i]);
        current_alert_level = levels[i];
        last_alert_time = millis();
        
        // Ejecutar patrón 2 veces
        for (int j = 0; j < 2; j++) {
            executeAlert();
            delay(1000);
        }
        
        delay(1500);
    }
    
    current_alert_level = ALERT_SAFE;
    DEBUG_INFO("✅ Test de patrones completado");
}

void BuzzerManager::testFrequencyRange() {
    DEBUG_INFO("🧪 === TEST RANGO DE FRECUENCIAS ===");
    
    uint32_t frequencies[] = {1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};
    
    for (int i = 0; i < 9; i++) {
        DEBUG_INFO("🎵 Frecuencia: " + String(frequencies[i]) + " Hz");
        playTone(frequencies[i], 500, 70);
        delay(300);
    }
    
    DEBUG_INFO("✅ Test de frecuencias completado");
}

void BuzzerManager::testVolumeRange() {
    DEBUG_INFO("🧪 === TEST RANGO DE VOLUMEN ===");
    
    uint8_t volumes[] = {10, 25, 50, 75, 100};
    
    for (int i = 0; i < 5; i++) {
        DEBUG_INFO("🔊 Volumen: " + String(volumes[i]) + "%");
        playTone(BUZZER_FREQ_MID, 500, volumes[i]);
        delay(300);
    }
    
    DEBUG_INFO("✅ Test de volumen completado");
}

void BuzzerManager::printStatus() {
    Serial.println(F("\n🎵 === ESTADO DEL BUZZER ==="));
    Serial.println(F("Inicializado: ") + String(initialized ? "SÍ" : "NO"));
    Serial.println(F("Reproduciendo: ") + String(playing ? "SÍ" : "NO"));
    Serial.println(F("Silenciado: ") + String(muted ? "SÍ" : "NO"));
    Serial.println(F("Volumen: ") + String(global_volume) + "%");
    Serial.println(F("Nivel alerta: ") + String(current_alert_level));
    Serial.println(F("Pin: ") + String(BUZZER_PIN));
    Serial.println(F("============================\n"));
}

void BuzzerManager::deinit() {
    stopTone();
    initialized = false;
    current_alert_level = ALERT_SAFE;
    DEBUG_INFO("🎵 Buzzer desinicializado");
}