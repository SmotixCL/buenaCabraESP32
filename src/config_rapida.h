/*
 * CONFIGURACIONES RÁPIDAS PARA EL COLLAR V3.0 - RADIOLIB
 * Sistema de buzzer mejorado con alertas progresivas no detenibles
 */

#ifndef CONFIG_RAPIDA_H
#define CONFIG_RAPIDA_H

// ============================================================================
// CONFIGURACIÓN ACTUAL - CABRAS (RADIO MEDIO, SENSIBILIDAD ALTA)
// ============================================================================
#define GEOFENCE_ALERT_RADIUS   20.0    // Radio de detección de proximidad
#define SAFE_DISTANCE          15.0     // >15m = zona completamente segura
#define CAUTION_DISTANCE       10.0     // 10-15m = precaución ligera
#define WARNING_DISTANCE        5.0     // 5-10m = advertencia media
// DANGER_DISTANCE es 0-5m = peligro máximo
// EMERGENCY es <0m = fuera de geocerca

// ============================================================================
// CONFIGURACIONES ALTERNATIVAS OPTIMIZADAS
// ============================================================================

// GANADO BOVINO - Radio grande, menos sensible (más tolerante)
/*
#define GEOFENCE_ALERT_RADIUS   40.0    // Detección temprana para ganado grande
#define SAFE_DISTANCE          30.0     // Mayor zona segura
#define CAUTION_DISTANCE       20.0     // Precaución más tardía
#define WARNING_DISTANCE       10.0     // Advertencia solo cerca del límite
*/

// OVEJAS - Radio medio, sensibilidad balanceada
/*
#define GEOFENCE_ALERT_RADIUS   25.0    // Balance entre sensibilidad y practicidad
#define SAFE_DISTANCE          18.0     // Zona segura razonable
#define CAUTION_DISTANCE       12.0     // Precaución moderada
#define WARNING_DISTANCE        6.0     // Advertencia oportuna
*/

// ULTRA SENSIBLE - Radio pequeño, muy reactivo (mascotas)
/*
#define GEOFENCE_ALERT_RADIUS   10.0    // Máxima sensibilidad
#define SAFE_DISTANCE           8.0     // Zona segura muy pequeña
#define CAUTION_DISTANCE        6.0     // Precaución inmediata
#define WARNING_DISTANCE        3.0     // Advertencia muy temprana
*/

// MODO STEALTH - Solo alertas críticas (uso nocturno)
/*
#define GEOFENCE_ALERT_RADIUS   5.0     // Mínima detección
#define SAFE_DISTANCE           4.0     // Solo alertar en límite
#define CAUTION_DISTANCE        3.0     // Precaución mínima
#define WARNING_DISTANCE        1.0     // Solo peligro real
*/

// ============================================================================
// CONFIGURACIÓN DE VOLUMEN Y POTENCIA DEL BUZZER PWM
// ============================================================================

// VOLUMEN MODERADO (RECOMENDADO PARA INICIO)
#define BUZZER_VOLUME_CAUTION   60      // 60% para alertas de precaución
#define BUZZER_VOLUME_WARNING   80      // 80% para advertencias
#define BUZZER_VOLUME_DANGER    95      // 95% para peligro
#define BUZZER_VOLUME_EMERGENCY 100     // 100% para emergencia

// VOLUMEN BAJO (AMBIENTES SENSIBLES AL RUIDO)
/*
#define BUZZER_VOLUME_CAUTION   40      // 40% volumen bajo
#define BUZZER_VOLUME_WARNING   55      // 55% volumen bajo
#define BUZZER_VOLUME_DANGER    70      // 70% volumen bajo  
#define BUZZER_VOLUME_EMERGENCY 85      // 85% volumen bajo
*/

// VOLUMEN ALTO (CAMPO ABIERTO, RUIDO AMBIENTAL)
/*
#define BUZZER_VOLUME_CAUTION   75      // 75% volumen alto
#define BUZZER_VOLUME_WARNING   90      // 90% volumen alto
#define BUZZER_VOLUME_DANGER    100     // 100% volumen alto
#define BUZZER_VOLUME_EMERGENCY 100     // 100% volumen máximo
*/

// ============================================================================
// FRECUENCIAS OPTIMIZADAS PARA MÁXIMA PENETRACIÓN SONORA
// ============================================================================

// FRECUENCIAS ESTÁNDAR (PROBADAS Y OPTIMIZADAS)
#define FREQ_PENETRATION  2730  // Frecuencia de máxima penetración en aire
#define FREQ_ATTENTION    3400  // Frecuencia de máxima sensibilidad humana
#define FREQ_EMERGENCY    4000  // Frecuencia de máxima urgencia

// FRECUENCIAS GRAVES (MAYOR ALCANCE, MENOS MOLESTO)
/*
#define FREQ_PENETRATION  1800  // Más grave, mayor alcance
#define FREQ_ATTENTION    2200  // Frecuencia media-grave
#define FREQ_EMERGENCY    2800  // Emergencia menos aguda
*/

// FRECUENCIAS AGUDAS (MÁXIMA ATENCIÓN, MENOR ALCANCE)
/*
#define FREQ_PENETRATION  3500  // Más agudo, máxima atención
#define FREQ_ATTENTION    4200  // Muy llamativo
#define FREQ_EMERGENCY    5000  // Extremadamente agudo
*/

// ============================================================================
// CONFIGURACIÓN DE TIMING DE ALERTAS PROGRESIVAS
// ============================================================================

// TIMING ESTÁNDAR (RECOMENDADO)
#define ESCALATION_TIME_MS      60000   // 1 minuto para escalación automática
#define CAUTION_REPEAT_MS       8000    // Repetir precaución cada 8 segundos
#define WARNING_REPEAT_MS       5000    // Repetir advertencia cada 5 segundos
#define DANGER_REPEAT_MS        3000    // Repetir peligro cada 3 segundos
#define EMERGENCY_REPEAT_MS     2000    // Sirena cada 2 segundos

// TIMING RÁPIDO (RESPUESTA INMEDIATA)
/*
#define ESCALATION_TIME_MS      30000   // 30 segundos para escalación
#define CAUTION_REPEAT_MS       5000    // Precaución cada 5 segundos
#define WARNING_REPEAT_MS       3000    // Advertencia cada 3 segundos
#define DANGER_REPEAT_MS        2000    // Peligro cada 2 segundos
#define EMERGENCY_REPEAT_MS     1000    // Sirena cada 1 segundo
*/

// TIMING LENTO (MENOS INTRUSIVO)
/*
#define ESCALATION_TIME_MS      120000  // 2 minutos para escalación
#define CAUTION_REPEAT_MS       15000   // Precaución cada 15 segundos
#define WARNING_REPEAT_MS       10000   // Advertencia cada 10 segundos
#define DANGER_REPEAT_MS        5000    // Peligro cada 5 segundos
#define EMERGENCY_REPEAT_MS     3000    // Sirena cada 3 segundos
*/

// ============================================================================
// CONFIGURACIÓN LORAWAN PARA CHILE
// ============================================================================

// REGIÓN AU915 - CONFIGURACIÓN PARA CHILE
#define LORAWAN_REGION          AU915
#define LORAWAN_SUB_BAND        2       // Sub-banda 2 (canales 8-15) para TTN
#define LORAWAN_DATA_RATE       5       // DR5 = SF7BW125 (más rápido)
#define LORAWAN_TX_POWER        20      // 20 dBm (máximo permitido en Chile)

// INTERVALOS DE TRANSMISIÓN
#define TX_INTERVAL_NORMAL      600     // 10 minutos en operación normal
#define TX_INTERVAL_ALERT       30      // 30 segundos cuando hay alerta
#define TX_INTERVAL_EMERGENCY   15      // 15 segundos en emergencia

// ============================================================================
// CONFIGURACIÓN DE TESTING Y DEBUG
// ============================================================================

// Descomentar para habilitar diferentes modos
#define DEBUG_MODE                      // Logs detallados + simulación de movimiento
#define CALIBRATION_MODE                // Test automático al inicio
// #define VERBOSE_LOGS                 // Logs extra verbosos
// #define OLED_ALWAYS_ON              // OLED siempre encendido (gasta más batería)
// #define GPS_SIMULATION_MODE          // Usar coordenadas simuladas
// #define BUZZER_TEST_ON_BOOT         // Test completo de buzzer al arrancar

// ============================================================================
// CONFIGURACIÓN DE GESTIÓN DE ENERGÍA
// ============================================================================

// NIVELES DE BATERÍA (EN VOLTIOS)
#define BATTERY_FULL            4.1     // Batería llena
#define BATTERY_GOOD            3.8     // Batería buena
#define BATTERY_LOW             3.6     // Batería baja (modo conservación)
#define BATTERY_CRITICAL        3.3     // Batería crítica (modo emergencia)

// MODOS DE AHORRO
#define POWER_SAVE_THRESHOLD    3.7     // Voltaje para activar ahorro
#define DEEP_SLEEP_DURATION     1800    // 30 minutos de deep sleep normal
#define EMERGENCY_SLEEP_DURATION 3600   // 1 hora en modo emergencia energética

// ============================================================================
// CONFIGURACIÓN PARA FUTURAS EXPANSIONES
// ============================================================================

// PINS RESERVADOS PARA SISTEMA DE DESCARGA ELÉCTRICA
#define HIGH_VOLTAGE_ENABLE     32      // Pin para habilitar sistema HV
#define HIGH_VOLTAGE_CONTROL    33      // Pin para controlar pulso HV
#define SAFETY_MONITOR          25      // Pin para monitoreo de seguridad

// PARÁMETROS DE SEGURIDAD PARA FUTURO SISTEMA HV
#define HV_MAX_DURATION_MS      1000    // Máximo 1 segundo de activación
#define HV_MIN_INTERVAL_MS      30000   // Mínimo 30 segundos entre activaciones
#define HV_BATTERY_MIN_VOLTAGE  3.5     // Voltaje mínimo para activar HV

// ============================================================================
// INSTRUCCIONES DE USO Y PERSONALIZACIÓN
// ============================================================================

/*
GUÍA DE CONFIGURACIÓN RÁPIDA:

1. SELECCIONAR PERFIL DE ANIMAL:
   - Descomentar la sección correspondiente al tipo de animal
   - Ajustar distancias según el tamaño y comportamiento

2. CONFIGURAR VOLUMEN:
   - Elegir configuración de volumen según el ambiente
   - Ambientes ruidosos = volumen alto
   - Ambientes urbanos = volumen moderado

3. AJUSTAR TIMING:
   - Animales nerviosos = timing lento
   - Situaciones críticas = timing rápido

4. CONFIGURAR LORAWAN:
   - Verificar que la región AU915 sea correcta para tu país
   - Ajustar sub-banda según tu proveedor LoRaWAN

TESTING Y CALIBRACIÓN:

1. Compilar: pio run
2. Cargar: pio run --target upload
3. Monitor: pio device monitor --baud 115200

COMANDOS DE DOWNLINK:
- 0x01: Activar alerta manual (testing)
- 0x02: Cargar nueva geocerca
- 0x03: Test completo del buzzer
- 0x04: Configurar parámetros

PERSONALIZACIÓN AVANZADA:
- Modificar melodías en playStartupMelody()
- Ajustar patrones de alerta en executeProgressiveAlert()
- Cambiar frecuencias de emergencia según necesidades
- Configurar pins de expansión para futuras mejoras

SOLUCIÓN DE PROBLEMAS:
- Si OLED no funciona: verificar conexiones I2C en pins 17, 18, 21
- Si buzzer es muy bajo: aumentar valores de volumen
- Si alertas no suenan: verificar definiciones de distancias
- Si LoRaWAN falla: verificar claves y configuración de región
*/

#endif // CONFIG_RAPIDA_H