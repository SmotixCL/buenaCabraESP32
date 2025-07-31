/*
 * ============================================================================
 * IMPLEMENTACIÓN MÓDULO BUZZER - Sistema PWM Optimizado
 * ============================================================================
 * 
 * SOLUCIÓN CRÍTICA: Configuración LEDC corregida para ESP32-S3
 * Esta implementación soluciona el error de compilación original.
 * ============================================================================
 */

#include "hardware/buzzer.h"

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
BuzzerController Buzzer;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
BuzzerController::BuzzerController() 
    : initialized(false), current_volume(VOLUME_MID) {
}

BuzzerController::~BuzzerController() {
    end();
}

// ============================================================================
// CONFIGURACIÓN LEDC CORREGIDA - SOLUCIÓN DEL ERROR
// ============================================================================
bool BuzzerController::setupLEDC() {
    DEBUG_PRINTLN(F("🎵 Configurando LEDC para buzzer..."));
    
    // *** CONFIGURACIÓN DEL TIMER - ORDEN CORREGIDO ***
    ledc_timer_config_t timer_conf;
    timer_conf.speed_mode = BUZZER_SPEED_MODE;      // PRIMER CAMPO
    timer_conf.timer_num = BUZZER_TIMER;            // SEGUNDO CAMPO  
    timer_conf.duty_resolution = BUZZER_RESOLUTION; // TERCER CAMPO - CORREGIDO
    timer_conf.freq_hz = FREQ_MID;                  // CUARTO CAMPO
    timer_conf.clk_cfg = LEDC_AUTO_CLK;            // QUINTO CAMPO
    timer_conf.deconfigure = false;                 // SEXTO CAMPO (nuevo)
    
    esp_err_t timer_result = ledc_timer_config(&timer_conf);
    if (timer_result != ESP_OK) {
        DEBUG_PRINTF("❌ Error configurando timer LEDC: %d\n", timer_result);
        return false;
    }
    
    // *** CONFIGURACIÓN DEL CANAL - ORDEN CORREGIDO ***
    ledc_channel_config_t channel_conf;
    channel_conf.speed_mode = BUZZER_SPEED_MODE;    // PRIMER CAMPO
    channel_conf.channel = BUZZER_CHANNEL;          // SEGUNDO CAMPO
    channel_conf.timer_sel = BUZZER_TIMER;          // TERCER CAMPO
    channel_conf.intr_type = LEDC_INTR_DISABLE;     // CUARTO CAMPO
    channel_conf.gpio_num = BUZZER_PIN;             // QUINTO CAMPO - CORREGIDO
    channel_conf.duty = 0;                          // SEXTO CAMPO
    channel_conf.hpoint = 0;                        // SÉPTIMO CAMPO (nuevo)
    
    esp_err_t channel_result = ledc_channel_config(&channel_conf);
    if (channel_result != ESP_OK) {
        DEBUG_PRINTF("❌ Error configurando canal LEDC: %d\n", channel_result);
        return false;
    }
    
    DEBUG_PRINTLN(F("✅ LEDC configurado correctamente"));
    return true;
}

// ============================================================================
// INICIALIZACIÓN Y CONTROL BÁSICO
// ============================================================================
bool BuzzerController::begin() {
    DEBUG_PRINTLN(F("🎵 Inicializando sistema de buzzer..."));
    
    // Configurar pin como salida
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Configurar LEDC con la nueva implementación corregida
    if (!setupLEDC()) {
        DEBUG_PRINTLN(F("❌ Error: Falló configuración LEDC"));
        return false;
    }
    
    initialized = true;
    current_volume = VOLUME_MID;
    
    DEBUG_PRINTLN(F("✅ Sistema de buzzer inicializado correctamente"));
    
    // Test básico
    playTone(FREQ_MID, 200, VOLUME_LOW);
    delay(100);
    
    return true;
}

void BuzzerController::end() {
    if (!initialized) return;
    
    stopTone();
    
    // Limpiar configuración LEDC
    ledc_stop(BUZZER_SPEED_MODE, BUZZER_CHANNEL, 0);
    
    initialized = false;
    DEBUG_PRINTLN(F("🎵 Sistema de buzzer desactivado"));
}

// ============================================================================
// CONTROL BÁSICO DE TONOS
// ============================================================================
void BuzzerController::setFrequency(uint32_t frequency) {
    if (!initialized) return;
    
    ledc_set_freq(BUZZER_SPEED_MODE, BUZZER_TIMER, frequency);
}

void BuzzerController::setVolume(uint8_t volume) {
    if (!initialized) return;
    
    // Calcular duty cycle (0-1023 para 10-bit)
    uint32_t duty = (1023 * CLAMP(volume, 0, 100)) / 100;
    
    ledc_set_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL, duty);
    ledc_update_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL);
}

void BuzzerController::playTone(uint32_t frequency, uint32_t duration_ms, uint8_t volume) {
    if (!initialized) return;
    
    setFrequency(frequency);
    setVolume(volume);
    
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    stopTone();
}

void BuzzerController::playToneAsync(uint32_t frequency, uint8_t volume) {
    if (!initialized) return;
    
    setFrequency(frequency);
    setVolume(volume);
}

void BuzzerController::stopTone() {
    if (!initialized) return;
    
    ledc_set_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL);
}

// ============================================================================
// MELODÍAS PREDEFINIDAS
// ============================================================================
void BuzzerController::playStartupMelody() {
    if (!initialized) return;
    
    DEBUG_PRINTLN(F("🎵 Reproduciendo melodía de inicio..."));
    
    // Melodía ascendente alegre
    playTone(NOTE_C4, 200, VOLUME_LOW);
    delay(50);
    playTone(NOTE_E4, 200, VOLUME_MID);
    delay(50);
    playTone(NOTE_G4, 200, VOLUME_HIGH);
    delay(50);
    playTone(NOTE_C5, 300, VOLUME_HIGH);
    delay(100);
    playTone(NOTE_E5, 400, VOLUME_MAX);
    delay(200);
    
    // Confirmación final
    playTone(FREQ_MID, 300, VOLUME_HIGH);
    delay(100);
    playTone(FREQ_HIGH, 500, VOLUME_MAX);
}

void BuzzerController::playShutdownMelody() {
    if (!initialized) return;
    
    // Melodía descendente
    playTone(NOTE_G5, 200, VOLUME_HIGH);
    delay(50);
    playTone(NOTE_E5, 200, VOLUME_MID);
    delay(50);
    playTone(NOTE_C5, 200, VOLUME_MID);
    delay(50);
    playTone(NOTE_G4, 400, VOLUME_LOW);
}

void BuzzerController::playSuccessTone() {
    if (!initialized) return;
    
    // Tono de éxito alegre
    playTone(FREQ_MID, 150, VOLUME_MID);
    delay(80);
    playTone(FREQ_HIGH, 300, VOLUME_HIGH);
}

void BuzzerController::playErrorTone() {
    if (!initialized) return;
    
    // Tono de error grave
    playTone(FREQ_LOW, 100, VOLUME_HIGH);
    delay(50);
    playTone(FREQ_LOW, 100, VOLUME_HIGH);
    delay(50);
    playTone(FREQ_LOW, 200, VOLUME_HIGH);
}

void BuzzerController::playWarningTone() {
    if (!initialized) return;
    
    // Tono de advertencia medio
    playTone(FREQ_MID, 200, VOLUME_MID);
    delay(100);
    playTone(FREQ_HIGH, 200, VOLUME_HIGH);
}

// ============================================================================
// ALERTAS DE GEOFENCING
// ============================================================================
void BuzzerController::playCautionAlert() {
    if (!initialized) return;
    
    playTone(FREQ_LOW, 300, VOLUME_LOW);
}

void BuzzerController::playWarningAlert() {
    if (!initialized) return;
    
    playTone(FREQ_MID, 150, VOLUME_MID);
    delay(100);
    playTone(FREQ_MID, 150, VOLUME_MID);
}

void BuzzerController::playDangerAlert() {
    if (!initialized) return;
    
    for (int i = 0; i < 3; i++) {
        playTone(FREQ_HIGH, 100, VOLUME_HIGH);
        delay(50);
    }
}

void BuzzerController::playEmergencyAlert() {
    if (!initialized) return;
    
    // Sirena escalada de emergencia
    for (uint32_t freq = 2000; freq <= 4000; freq += 200) {
        playTone(freq, 30, VOLUME_MAX);
    }
}

// ============================================================================
// PATRONES ESPECIALES
// ============================================================================
void BuzzerController::playHeartbeat() {
    if (!initialized) return;
    
    playTone(FREQ_LOW, 50, VOLUME_LOW);
    delay(50);
    playTone(FREQ_LOW, 50, VOLUME_LOW);
}

void BuzzerController::playSOS() {
    if (!initialized) return;
    
    DEBUG_PRINTLN(F("🆘 Reproduciendo SOS"));
    
    // S (3 cortos)
    for (int i = 0; i < 3; i++) {
        playTone(FREQ_HIGH, 100, VOLUME_HIGH);
        delay(100);
    }
    delay(200);
    
    // O (3 largos)
    for (int i = 0; i < 3; i++) {
        playTone(FREQ_HIGH, 300, VOLUME_HIGH);
        delay(100);
    }
    delay(200);
    
    // S (3 cortos)
    for (int i = 0; i < 3; i++) {
        playTone(FREQ_HIGH, 100, VOLUME_HIGH);
        delay(100);
    }
}

void BuzzerController::playConnectionTone() {
    if (!initialized) return;
    
    // Tono de conexión establecida
    playTone(FREQ_LOW, 100, VOLUME_MID);
    delay(50);
    playTone(FREQ_HIGH, 200, VOLUME_MID);
}

void BuzzerController::playBatteryLowTone() {
    if (!initialized) return;
    
    // Patrón de batería baja
    for (int i = 0; i < 5; i++) {
        playTone(FREQ_LOW, 50, VOLUME_LOW);
        delay(200);
    }
}

// ============================================================================
// CONTROL DE VOLUMEN
// ============================================================================
void BuzzerController::setMasterVolume(uint8_t volume) {
    current_volume = CLAMP(volume, 0, 100);
    DEBUG_PRINTF("🔊 Volumen establecido: %d%%\n", current_volume);
}

// ============================================================================
// TEST Y DIAGNÓSTICO
// ============================================================================
void BuzzerController::runSelfTest() {
    if (!initialized) {
        DEBUG_PRINTLN(F("❌ Buzzer no inicializado para self-test"));
        return;
    }
    
    DEBUG_PRINTLN(F("🧪 === SELF-TEST BUZZER ==="));
    
    // Test de frecuencias
    DEBUG_PRINTLN(F("🎵 Test frecuencias..."));
    playTone(FREQ_LOW, 200, VOLUME_MID);
    delay(300);
    playTone(FREQ_MID, 200, VOLUME_MID);
    delay(300);
    playTone(FREQ_HIGH, 200, VOLUME_MID);
    delay(300);
    playTone(FREQ_EMERGENCY, 200, VOLUME_MID);
    delay(500);
    
    // Test de volúmenes
    DEBUG_PRINTLN(F("🔊 Test volúmenes..."));
    for (int vol = 20; vol <= 100; vol += 20) {
        playTone(FREQ_MID, 100, vol);
        delay(200);
    }
    delay(500);
    
    // Test de alertas
    DEBUG_PRINTLN(F("🚨 Test alertas..."));
    playCautionAlert();
    delay(500);
    playWarningAlert();
    delay(500);
    playDangerAlert();
    delay(500);
    playEmergencyAlert();
    delay(500);
    
    // Confirmación final
    playSuccessTone();
    DEBUG_PRINTLN(F("✅ Self-test buzzer completado"));
}

void BuzzerController::playFrequencySweep(uint32_t start_freq, uint32_t end_freq, uint32_t duration_ms) {
    if (!initialized) return;
    
    DEBUG_PRINTF("🎵 Frequency sweep: %lu Hz -> %lu Hz (%lu ms)\n", start_freq, end_freq, duration_ms);
    
    uint32_t steps = duration_ms / 50; // 50ms por paso
    uint32_t freq_step = (end_freq - start_freq) / steps;
    
    for (uint32_t i = 0; i < steps; i++) {
        uint32_t current_freq = start_freq + (i * freq_step);
        playTone(current_freq, 50, VOLUME_MID);
    }
}