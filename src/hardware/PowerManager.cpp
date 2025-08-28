#include "PowerManager.h"
#include "../core/Logger.h"
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include <soc/rtc.h>

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

PowerManager::PowerManager(uint8_t batteryPin) : vbatPin(batteryPin),
                                                 initialized(false),
                                                 lowPowerModeEnabled(false),
                                                 startTime(0),
                                                 lowBatteryCallback(nullptr),
                                                 criticalBatteryCallback(nullptr),
                                                 currentSample(0),
                                                 samplesReady(false)
{
    // Inicializar muestras de batería
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++)
    {
        batterySamples[i] = 0.0f;
    }
    startTime = millis();
}

Result PowerManager::init()
{
    if (initialized)
    {
        return Result::SUCCESS;
    }

    LOG_I("🔋 Inicializando Power Manager...");

    // Configurar pin de batería
    pinMode(vbatPin, INPUT);

    // Configurar ADC para batería
    analogReadResolution(12);
    analogSetPinAttenuation(vbatPin, ADC_11db);
    adcAttachPin(vbatPin);

    // Control ADC necesario para Heltec V3 (sin esto valores raw no tienen sentido)
    pinMode(ADC_CTRL_PIN, OUTPUT);
    digitalWrite(ADC_CTRL_PIN, HIGH); // Activar ADC
    delay(100);                       // Tiempo para activarse

    // Leer muestra inicial de batería
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++)
    {
        batterySamples[i] = readVoltageRaw();
        delay(10);
    }
    samplesReady = true;

    initialized = true;
    // Leer estado inicial (necesitamos que initialized sea true)
    readBattery();

    LOG_INIT("Power Manager", true);
    LOG_BATTERY(batteryStatus.voltage, batteryStatus.percentage);

    return Result::SUCCESS;
}

bool PowerManager::isInitialized() const
{
    return initialized;
}

// ============================================================================
// LECTURA DE BATERÍA
// ============================================================================

void PowerManager::readBattery()
{
    if (!initialized)
    {
        return;
    }

    // Tomar nueva muestra
    batterySamples[currentSample] = readVoltageRaw();
    currentSample = (currentSample + 1) % BATTERY_SAMPLES;

    // Calcular promedio
    float voltage = calculateAverageVoltage();

    // Actualizar estado de batería
    batteryStatus.voltage = voltage;
    batteryStatus.percentage = calculateBatteryPercentage(voltage);
    batteryStatus.lastReading = millis();

    // Determinar estados críticos
    bool wasLow = batteryStatus.low;
    bool wasCritical = batteryStatus.critical;

    batteryStatus.low = (voltage <= BATTERY_LOW);
    batteryStatus.critical = (voltage <= BATTERY_CRITICAL);

    // Detectar carga (aumento significativo de voltaje)
    static float lastVoltage = voltage;
    batteryStatus.charging = (voltage > lastVoltage + 0.1f);
    lastVoltage = voltage;

    // Triggear callbacks si hay cambios
    if ((batteryStatus.low && !wasLow) || (batteryStatus.critical && !wasCritical))
    {
        triggerBatteryCallbacks();
    };

    LOG_D("🔋 Batería: %.2fV (%d%%) %s%s",
          voltage, batteryStatus.percentage,
          batteryStatus.low ? "LOW " : "",
          batteryStatus.critical ? "CRITICAL " : "");
}

BatteryStatus PowerManager::getBatteryStatus() const
{
    return batteryStatus;
}

float PowerManager::getVoltage() const
{
    return batteryStatus.voltage;
}

uint8_t PowerManager::getPercentage() const
{
    return batteryStatus.percentage;
}

bool PowerManager::isLow() const
{
    return batteryStatus.low;
}

bool PowerManager::isCritical() const
{
    return batteryStatus.critical;
}

// ============================================================================
// GESTIÓN DE ENERGÍA
// ============================================================================

void PowerManager::enableLowPowerMode()
{
    if (lowPowerModeEnabled)
        return;

    LOG_I("🔋 Activando modo bajo consumo");

    // Reducir frecuencia CPU
    setCpuFrequencyMhz(80);

    // Configurar GPIO para bajo consumo
    configurePinsForSleep();

    lowPowerModeEnabled = true;
}

void PowerManager::disableLowPowerMode()
{
    if (!lowPowerModeEnabled)
        return;

    LOG_I("⚡ Desactivando modo bajo consumo");

    // Restaurar frecuencia CPU
    setCpuFrequencyMhz(240);

    // Restaurar configuración de pines
    restorePinsAfterSleep();

    lowPowerModeEnabled = false;
}

void PowerManager::prepareForDeepSleep(uint64_t sleepTimeUs)
{
    LOG_I("😴 Preparando para deep sleep");

    // Configurar pines para deep sleep
    configurePinsForSleep();

    // Configurar timer si se especifica tiempo
    if (sleepTimeUs > 0)
    {
        esp_sleep_enable_timer_wakeup(sleepTimeUs);
        LOG_I("⏰ Deep sleep por %llu segundos", sleepTimeUs / 1000000ULL);
    }

    // Habilitar hold de GPIO durante sleep
    gpio_deep_sleep_hold_en();

    LOG_I("💤 Entrando en deep sleep...");
    delay(100); // Asegurar que el mensaje se imprima

    esp_deep_sleep_start();
}

void PowerManager::wakeFromDeepSleep()
{
    LOG_I("⏰ Despertando de deep sleep");

    // Deshabilitar hold de GPIO
    gpio_deep_sleep_hold_dis();

    // Restaurar configuración normal
    restorePinsAfterSleep();

    // Reinicializar sistemas si es necesario
    if (lowPowerModeEnabled)
    {
        disableLowPowerMode();
    }
}

// ============================================================================
// WATCHDOG
// ============================================================================

void PowerManager::enableWatchdog(uint32_t timeoutSeconds)
{
#if ENABLE_WATCHDOG
    esp_task_wdt_init(timeoutSeconds, true);
    esp_task_wdt_add(NULL);
    LOG_I("🐕 Watchdog habilitado (%lu segundos)", timeoutSeconds);
#endif
}

void PowerManager::disableWatchdog()
{
#if ENABLE_WATCHDOG
    esp_task_wdt_delete(NULL);
    esp_task_wdt_deinit();
    LOG_I("🐕 Watchdog deshabilitado");
#endif
}

void PowerManager::feedWatchdog()
{
#if ENABLE_WATCHDOG
    esp_task_wdt_reset();
#endif
}

// ============================================================================
// INFORMACIÓN DEL SISTEMA
// ============================================================================

uint32_t PowerManager::getUptime() const
{
    return (millis() - startTime) / 1000;
}

uint32_t PowerManager::getFreeHeap() const
{
    return ESP.getFreeHeap();
}

float PowerManager::getCPUTemperature() const
{
    // ESP32-S3 no tiene sensor interno de temperatura
    // Retornar estimación basada en carga y tiempo
    uint32_t uptime = getUptime();
    float baseTemp = 25.0f;
    float uptimeEffect = (uptime > 3600) ? 10.0f : (uptime * 10.0f / 3600.0f);

    return baseTemp + uptimeEffect;
}

// ============================================================================
// CALLBACKS
// ============================================================================

void PowerManager::setBatteryLowCallback(BatteryCallback callback)
{
    lowBatteryCallback = callback;
}

void PowerManager::setBatteryCriticalCallback(BatteryCallback callback)
{
    criticalBatteryCallback = callback;
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

float PowerManager::readVoltageRaw()
{
    // Activar ADC temporalmente
    digitalWrite(37, HIGH);
    delay(10);

    // Calculo de raw con analogRead usa las configuraciones hechas en init y activadas con digitalWrite
    int raw = analogRead(vbatPin);

    // Desactivar ADC para ahorrar energía
    digitalWrite(37, LOW);

    // Calcular con factor 4.9 correcto según documentación
    float voltage = (raw * VBAT_REFERENCE * VBAT_DIVIDER) / VBAT_RESOLUTION;

    return voltage; // Retornamos el voltage calculado
}

float PowerManager::calculateAverageVoltage()
{
    if (!samplesReady)
        return readVoltageRaw();

    float sum = 0.0f;
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++)
    {
        sum += batterySamples[i];
    }
    return sum / BATTERY_SAMPLES;
}

void PowerManager::triggerBatteryCallbacks()
{
    if (batteryStatus.critical && criticalBatteryCallback)
    {
        criticalBatteryCallback(batteryStatus);
    }
    else if (batteryStatus.low && lowBatteryCallback)
    {
        lowBatteryCallback(batteryStatus);
    }
}

void PowerManager::configurePinsForSleep()
{
    // Configurar pines no utilizados para bajo consumo
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << EXP_PIN_1) | (1ULL << EXP_PIN_2) |
                        (1ULL << EXP_PIN_3) | (1ULL << EXP_PIN_4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config);

    // Poner pines en bajo
    digitalWrite(EXP_PIN_1, LOW);
    digitalWrite(EXP_PIN_2, LOW);
    digitalWrite(EXP_PIN_3, LOW);
    digitalWrite(EXP_PIN_4, LOW);
}

void PowerManager::restorePinsAfterSleep()
{
    // Restaurar configuración normal de pines
    pinMode(EXP_PIN_1, OUTPUT);
    pinMode(EXP_PIN_2, OUTPUT);
    pinMode(EXP_PIN_3, OUTPUT);
    pinMode(EXP_PIN_4, OUTPUT);
}

float PowerManager::calculateBatteryPercentage(float voltage)
{
    // Curva no lineal más precisa para Li-ion bajo carga
    if (voltage <= 3.2)
        return 0;
    if (voltage >= 4.0)
        return 100;

    // Mapeo con puntos de inflexión típicos
    if (voltage >= 3.7)
    {
        // 3.7V-4.0V = 50%-100% (subida rápida)
        return 50 + ((voltage - 3.7) / 0.3) * 50;
    }
    else
    {
        // 3.2V-3.7V = 0%-50% (caída gradual)
        return ((voltage - 3.2) / 0.5) * 50;
    }
}
