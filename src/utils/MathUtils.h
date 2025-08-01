#pragma once
#include <Arduino.h>

/*
 * ============================================================================
 * UTILIDADES MATEMÁTICAS - COLLAR GEOFENCING
 * ============================================================================
 */

namespace MathUtils {
    
    // ========================================================================
    // CONSTANTES MATEMÁTICAS
    // ========================================================================
    constexpr double PI_D = 3.14159265358979323846;
    constexpr float PI_F = 3.14159265f;
    constexpr double EARTH_RADIUS_M = 6371000.0;    // Radio de la Tierra en metros
    constexpr double DEG_TO_RAD = PI_D / 180.0;
    constexpr double RAD_TO_DEG = 180.0 / PI_D;
    
    // ========================================================================
    // FUNCIONES DE DISTANCIA GEOGRÁFICA
    // ========================================================================
    
    /**
     * Calcula la distancia entre dos puntos usando la fórmula de Haversine
     * @param lat1 Latitud del primer punto (grados)
     * @param lon1 Longitud del primer punto (grados)
     * @param lat2 Latitud del segundo punto (grados)
     * @param lon2 Longitud del segundo punto (grados)
     * @return Distancia en metros
     */
    inline double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
        double dLat = (lat2 - lat1) * DEG_TO_RAD;
        double dLon = (lon2 - lon1) * DEG_TO_RAD;
        
        double a = sin(dLat / 2) * sin(dLat / 2) +
                   cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) *
                   sin(dLon / 2) * sin(dLon / 2);
        
        double c = 2 * atan2(sqrt(a), sqrt(1 - a));
        
        return EARTH_RADIUS_M * c;
    }
    
    /**
     * Aproximación rápida de distancia para distancias cortas (<1km)
     * Más eficiente pero menos precisa que Haversine
     */
    inline float fastDistance(double lat1, double lon1, double lat2, double lon2) {
        float dLat = (lat2 - lat1) * 111000.0f; // ~111km por grado de latitud
        float dLon = (lon2 - lon1) * 111000.0f * cos(lat1 * DEG_TO_RAD);
        return sqrt(dLat * dLat + dLon * dLon);
    }
    
    /**
     * Calcula el bearing (rumbo) entre dos puntos
     * @return Bearing en grados (0-360)
     */
    inline double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
        double dLon = (lon2 - lon1) * DEG_TO_RAD;
        double lat1_rad = lat1 * DEG_TO_RAD;
        double lat2_rad = lat2 * DEG_TO_RAD;
        
        double y = sin(dLon) * cos(lat2_rad);
        double x = cos(lat1_rad) * sin(lat2_rad) - sin(lat1_rad) * cos(lat2_rad) * cos(dLon);
        
        double bearing = atan2(y, x) * RAD_TO_DEG;
        return fmod(bearing + 360.0, 360.0); // Normalizar a 0-360
    }
    
    // ========================================================================
    // FUNCIONES DE INTERPOLACIÓN Y FILTRADO
    // ========================================================================
    
    /**
     * Filtro promedio móvil simple
     */
    template<typename T, size_t N>
    class MovingAverage {
    private:
        T samples[N];
        size_t index;
        size_t count;
        T sum;
        
    public:
        MovingAverage() : index(0), count(0), sum(0) {
            for (size_t i = 0; i < N; i++) {
                samples[i] = 0;
            }
        }
        
        T update(T newSample) {
            if (count < N) {
                samples[count] = newSample;
                sum += newSample;
                count++;
                return sum / count;
            } else {
                sum = sum - samples[index] + newSample;
                samples[index] = newSample;
                index = (index + 1) % N;
                return sum / N;
            }
        }
        
        T getAverage() const {
            return count > 0 ? sum / count : 0;
        }
        
        void reset() {
            index = count = 0;
            sum = 0;
        }
    };
    
    /**
     * Filtro de Kalman simple para suavizar lecturas de sensores
     */
    class SimpleKalman {
    private:
        float q; // process noise covariance
        float r; // measurement noise covariance
        float x; // value
        float p; // estimation error covariance
        float k; // kalman gain
        
    public:
        SimpleKalman(float q = 0.1f, float r = 1.0f) : q(q), r(r), x(0), p(1), k(0) {}
        
        float update(float measurement) {
            // prediction update
            p = p + q;
            
            // measurement update
            k = p / (p + r);
            x = x + k * (measurement - x);
            p = (1 - k) * p;
            
            return x;
        }
        
        float getValue() const { return x; }
        void reset(float initialValue = 0) { x = initialValue; p = 1; }
    };
    
    // ========================================================================
    // FUNCIONES DE VALIDACIÓN Y LÍMITES
    // ========================================================================
    
    /**
     * Limita un valor entre min y max
     */
    template<typename T>
    inline T clamp(T value, T min_val, T max_val) {
        return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
    }
    
    /**
     * Mapea un valor de un rango a otro
     */
    template<typename T>
    inline T map(T value, T in_min, T in_max, T out_min, T out_max) {
        return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
    
    /**
     * Valida coordenadas GPS
     */
    inline bool isValidLatitude(double lat) {
        return lat >= -90.0 && lat <= 90.0;
    }
    
    inline bool isValidLongitude(double lon) {
        return lon >= -180.0 && lon <= 180.0;
    }
    
    inline bool isValidCoordinate(double lat, double lon) {
        return isValidLatitude(lat) && isValidLongitude(lon);
    }
    
    // ========================================================================
    // FUNCIONES DE CONVERSIÓN
    // ========================================================================
    
    /**
     * Convierte grados a radianes
     */
    inline double toRadians(double degrees) {
        return degrees * DEG_TO_RAD;
    }
    
    /**
     * Convierte radianes a grados
     */
    inline double toDegrees(double radians) {
        return radians * RAD_TO_DEG;
    }
    
    /**
     * Convierte metros a pies
     */
    inline float metersToFeet(float meters) {
        return meters * 3.28084f;
    }
    
    /**
     * Convierte pies a metros
     */
    inline float feetToMeters(float feet) {
        return feet * 0.3048f;
    }
    
    /**
     * Convierte km/h a m/s
     */
    inline float kmhToMs(float kmh) {
        return kmh / 3.6f;
    }
    
    /**
     * Convierte m/s a km/h
     */
    inline float msToKmh(float ms) {
        return ms * 3.6f;
    }
    
    // ========================================================================
    // FUNCIONES DE GEOMETRÍA
    // ========================================================================
    
    /**
     * Verifica si un punto está dentro de un círculo
     */
    inline bool isPointInCircle(double pointLat, double pointLon, 
                               double centerLat, double centerLon, double radius) {
        double distance = haversineDistance(pointLat, pointLon, centerLat, centerLon);
        return distance <= radius;
    }
    
    /**
     * Calcula el área de un círculo
     */
    inline double circleArea(double radius) {
        return PI_D * radius * radius;
    }
    
    /**
     * Calcula la circunferencia de un círculo
     */
    inline double circleCircumference(double radius) {
        return 2 * PI_D * radius;
    }
    
    // ========================================================================
    // FUNCIONES ESTADÍSTICAS
    // ========================================================================
    
    /**
     * Calcula la media de un array
     */
    template<typename T>
    inline T mean(const T* values, size_t count) {
        if (count == 0) return 0;
        
        T sum = 0;
        for (size_t i = 0; i < count; i++) {
            sum += values[i];
        }
        return sum / count;
    }
    
    /**
     * Calcula la desviación estándar
     */
    template<typename T>
    inline T standardDeviation(const T* values, size_t count) {
        if (count <= 1) return 0;
        
        T avg = mean(values, count);
        T sum = 0;
        
        for (size_t i = 0; i < count; i++) {
            T diff = values[i] - avg;
            sum += diff * diff;
        }
        
        return sqrt(sum / (count - 1));
    }
    
    /**
     * Encuentra el valor mínimo en un array
     */
    template<typename T>
    inline T findMin(const T* values, size_t count) {
        if (count == 0) return 0;
        
        T minVal = values[0];
        for (size_t i = 1; i < count; i++) {
            if (values[i] < minVal) {
                minVal = values[i];
            }
        }
        return minVal;
    }
    
    /**
     * Encuentra el valor máximo en un array
     */
    template<typename T>
    inline T findMax(const T* values, size_t count) {
        if (count == 0) return 0;
        
        T maxVal = values[0];
        for (size_t i = 1; i < count; i++) {
            if (values[i] > maxVal) {
                maxVal = values[i];
            }
        }
        return maxVal;
    }
    
    // ========================================================================
    // FUNCIONES DE PRECISIÓN
    // ========================================================================
    
    /**
     * Redondea a un número específico de decimales
     */
    inline double roundToDecimals(double value, int decimals) {
        double multiplier = pow(10.0, decimals);
        return round(value * multiplier) / multiplier;
    }
    
    /**
     * Verifica si dos valores flotantes son aproximadamente iguales
     */
    inline bool approximately(double a, double b, double epsilon = 1e-9) {
        return abs(a - b) < epsilon;
    }
    
} // namespace MathUtils
