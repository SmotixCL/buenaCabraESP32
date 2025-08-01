#pragma once
#include <Arduino.h>

/*
 * ============================================================================
 * UTILIDADES DE CADENAS - COLLAR GEOFENCING
 * ============================================================================
 */

namespace StringUtils {
    
    // ========================================================================
    // FORMATEO DE COORDENADAS GPS
    // ========================================================================
    
    /**
     * Formatea una coordenada GPS con dirección cardinal
     * @param coordinate Coordenada en grados decimales
     * @param isLatitude true si es latitud, false si es longitud
     * @param decimals Número de decimales a mostrar
     * @return String formateado (ej: "12.3456°N")
     */
    inline String formatCoordinate(double coordinate, bool isLatitude, int decimals = 4) {
        char direction;
        double absCoord = abs(coordinate);
        
        if (isLatitude) {
            direction = (coordinate >= 0) ? 'N' : 'S';
        } else {
            direction = (coordinate >= 0) ? 'E' : 'W';
        }
        
        return String(absCoord, decimals) + "°" + direction;
    }
    
    /**
     * Formatea coordenadas GPS en formato DMS (Grados, Minutos, Segundos)
     */
    inline String formatCoordinateDMS(double coordinate, bool isLatitude) {
        char direction;
        double absCoord = abs(coordinate);
        
        if (isLatitude) {
            direction = (coordinate >= 0) ? 'N' : 'S';
        } else {
            direction = (coordinate >= 0) ? 'E' : 'W';
        }
        
        int degrees = (int)absCoord;
        double minutesFloat = (absCoord - degrees) * 60.0;
        int minutes = (int)minutesFloat;
        double seconds = (minutesFloat - minutes) * 60.0;
        
        return String(degrees) + "°" + String(minutes) + "'" + 
               String(seconds, 2) + "\"" + direction;
    }
    
    // ========================================================================
    // FORMATEO DE TIEMPO Y DURACIÓN
    // ========================================================================
    
    /**
     * Formatea tiempo en milisegundos a formato legible
     * @param ms Tiempo en milisegundos
     * @return String formateado (ej: "1h 23m 45s")
     */
    inline String formatDuration(uint32_t ms) {
        uint32_t seconds = ms / 1000;
        uint32_t minutes = seconds / 60;
        uint32_t hours = minutes / 60;
        uint32_t days = hours / 24;
        
        String result = "";
        
        if (days > 0) {
            result += String(days) + "d ";
            hours %= 24;
        }
        
        if (hours > 0) {
            result += String(hours) + "h ";
            minutes %= 60;
        }
        
        if (minutes > 0) {
            result += String(minutes) + "m ";
            seconds %= 60;
        }
        
        if (seconds > 0 || result.length() == 0) {
            result += String(seconds) + "s";
        }
        
        result.trim();
        return result;
    }
    
    /**
     * Formatea uptime del sistema
     */
    inline String formatUptime(uint32_t uptimeSeconds) {
        return formatDuration(uptimeSeconds * 1000);
    }
    
    /**
     * Formatea timestamp a formato HH:MM:SS
     */
    inline String formatTime(uint32_t timestamp) {
        uint32_t totalSeconds = timestamp / 1000;
        uint32_t hours = (totalSeconds / 3600) % 24;
        uint32_t minutes = (totalSeconds / 60) % 60;
        uint32_t seconds = totalSeconds % 60;
        
        char buffer[9];
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
        return String(buffer);
    }
    
    // ========================================================================
    // FORMATEO DE VALORES NUMÉRICOS
    // ========================================================================
    
    /**
     * Formatea voltaje con unidades
     */
    inline String formatVoltage(float voltage, int decimals = 2) {
        return String(voltage, decimals) + "V";
    }
    
    /**
     * Formatea porcentaje
     */
    inline String formatPercentage(uint8_t percentage) {
        return String(percentage) + "%";
    }
    
    /**
     * Formatea distancia con unidades apropiadas
     */
    inline String formatDistance(float meters) {
        if (meters < 1000) {
            return String(meters, 1) + "m";
        } else {
            return String(meters / 1000.0, 2) + "km";
        }
    }
    
    /**
     * Formatea frecuencia con unidades
     */
    inline String formatFrequency(float freq) {
        if (freq < 1000) {
            return String(freq, 1) + "Hz";
        } else if (freq < 1000000) {
            return String(freq / 1000.0, 1) + "kHz";
        } else {
            return String(freq / 1000000.0, 1) + "MHz";
        }
    }
    
    /**
     * Formatea tamaño de memoria
     */
    inline String formatMemorySize(uint32_t bytes) {
        if (bytes < 1024) {
            return String(bytes) + "B";
        } else if (bytes < 1024 * 1024) {
            return String(bytes / 1024.0, 1) + "KB";
        } else {
            return String(bytes / (1024.0 * 1024.0), 1) + "MB";
        }
    }
    
    // ========================================================================
    // FORMATEO DE ARRAYS DE BYTES
    // ========================================================================
    
    /**
     * Convierte array de bytes a string hexadecimal
     */
    inline String bytesToHex(const uint8_t* data, size_t length, bool uppercase = true, const char* separator = "") {
        String result = "";
        const char* format = uppercase ? "%02X" : "%02x";
        
        for (size_t i = 0; i < length; i++) {
            char buffer[3];
            snprintf(buffer, sizeof(buffer), format, data[i]);
            result += buffer;
            
            if (i < length - 1 && strlen(separator) > 0) {
                result += separator;
            }
        }
        
        return result;
    }
    
    /**
     * Convierte string hexadecimal a array de bytes
     */
    inline size_t hexToBytes(const String& hexString, uint8_t* buffer, size_t maxLength) {
        String cleanHex = hexString;
        cleanHex.replace(":", "");
        cleanHex.replace(" ", "");
        cleanHex.replace("-", "");
        
        size_t length = min((size_t)(cleanHex.length() / 2), maxLength);
        
        for (size_t i = 0; i < length; i++) {
            String byteString = cleanHex.substring(i * 2, i * 2 + 2);
            buffer[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        }
        
        return length;
    }
    
    // ========================================================================
    // VALIDACIÓN DE STRINGS
    // ========================================================================
    
    /**
     * Verifica si una string es un número válido
     */
    inline bool isNumeric(const String& str) {
        if (str.length() == 0) return false;
        
        bool hasDecimal = false;
        int startIndex = 0;
        
        // Permitir signo negativo al inicio
        if (str.charAt(0) == '-' || str.charAt(0) == '+') {
            startIndex = 1;
            if (str.length() == 1) return false;
        }
        
        for (int i = startIndex; i < str.length(); i++) {
            char c = str.charAt(i);
            
            if (c == '.') {
                if (hasDecimal) return false; // Más de un punto decimal
                hasDecimal = true;
            } else if (!isdigit(c)) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Verifica si una string contiene solo caracteres alfanuméricos
     */
    inline bool isAlphanumeric(const String& str) {
        for (int i = 0; i < str.length(); i++) {
            if (!isalnum(str.charAt(i))) {
                return false;
            }
        }
        return true;
    }
    
    // ========================================================================
    // MANIPULACIÓN DE STRINGS
    // ========================================================================
    
    /**
     * Capitaliza la primera letra de cada palabra
     */
    inline String toTitleCase(String str) {
        bool capitalize = true;
        
        for (int i = 0; i < str.length(); i++) {
            if (str.charAt(i) == ' ') {
                capitalize = true;
            } else if (capitalize && isalpha(str.charAt(i))) {
                str.setCharAt(i, toupper(str.charAt(i)));
                capitalize = false;
            } else {
                str.setCharAt(i, tolower(str.charAt(i)));
            }
        }
        
        return str;
    }
    
    /**
     * Trunca una string a una longitud máxima y añade "..."
     */
    inline String truncate(const String& str, int maxLength, const String& suffix = "...") {
        if (str.length() <= maxLength) {
            return str;
        }
        
        int truncateLength = maxLength - suffix.length();
        if (truncateLength <= 0) {
            return suffix.substring(0, maxLength);
        }
        
        return str.substring(0, truncateLength) + suffix;
    }
    
    /**
     * Centra un texto en un ancho específico
     */
    inline String center(const String& str, int width, char fillChar = ' ') {
        if (str.length() >= width) {
            return str;
        }
        
        int totalPadding = width - str.length();
        int leftPadding = totalPadding / 2;
        int rightPadding = totalPadding - leftPadding;
        
        String result = "";
        for (int i = 0; i < leftPadding; i++) {
            result += fillChar;
        }
        result += str;
        for (int i = 0; i < rightPadding; i++) {
            result += fillChar;
        }
        
        return result;
    }
    
    /**
     * Pads un string a la izquierda
     */
    inline String padLeft(const String& str, int width, char fillChar = ' ') {
        if (str.length() >= width) {
            return str;
        }
        
        String result = "";
        for (int i = 0; i < width - str.length(); i++) {
            result += fillChar;
        }
        result += str;
        
        return result;
    }
    
    /**
     * Pads un string a la derecha
     */
    inline String padRight(const String& str, int width, char fillChar = ' ') {
        if (str.length() >= width) {
            return str;
        }
        
        String result = str;
        for (int i = 0; i < width - str.length(); i++) {
            result += fillChar;
        }
        
        return result;
    }
    
    // ========================================================================
    // FUNCIONES ESPECIALIZADAS PARA EL COLLAR
    // ========================================================================
    
    /**
     * Genera un ID único para el dispositivo basado en MAC
     */
    inline String generateDeviceId() {
        uint64_t mac = ESP.getEfuseMac();
        char deviceId[13];
        snprintf(deviceId, sizeof(deviceId), "%012llX", mac);
        return String(deviceId);
    }
    
    /**
     * Formatea información de posición para transmisión
     */
    inline String formatPositionPayload(double lat, double lng, float alt, uint8_t sats) {
        return String(lat, 6) + "," + String(lng, 6) + "," + 
               String(alt, 1) + "," + String(sats);
    }
    
    /**
     * Crea un mensaje de status del sistema
     */
    inline String createStatusMessage(uint32_t uptime, float batteryV, uint8_t batteryP, 
                                    float distance, const String& alertLevel) {
        return "UP:" + formatUptime(uptime) + 
               " BAT:" + formatVoltage(batteryV) + "(" + formatPercentage(batteryP) + ")" +
               " DIST:" + formatDistance(distance) +
               " ALERT:" + alertLevel;
    }
    
} // namespace StringUtils
