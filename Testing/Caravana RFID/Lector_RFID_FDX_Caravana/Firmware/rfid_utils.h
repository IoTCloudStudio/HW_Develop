/*
 * Librerías adicionales para el Lector RFID FDX-B
 * Funciones de utilidad y configuración avanzada
 */

#ifndef RFID_FDX_UTILS_H
#define RFID_FDX_UTILS_H

#include <Arduino.h>

// ==================== CONFIGURACIONES AVANZADAS ====================

// Configuración de filtros digitales
#define FILTER_SAMPLES          5       // Número de muestras para filtro promedio
#define DEBOUNCE_TIME          50       // Tiempo de debounce para botones (ms)
#define TAG_TIMEOUT           5000      // Tiempo para considerar tag perdido (ms)

// Configuración de almacenamiento
#define MAX_STORED_TAGS        100      // Máximo número de tags almacenados
#define EEPROM_SIZE           4096      // Tamaño de EEPROM para configuración

// ==================== ESTRUCTURAS DE DATOS ====================

struct RFIDConfig {
    float antennaFrequency;     // Frecuencia de antena en Hz
    uint16_t amplifierGain;     // Ganancia del amplificador
    uint16_t detectionThreshold;// Umbral de detección
    uint8_t filterSamples;      // Muestras para filtro
    bool autoSave;              // Auto-guardar configuración
    char deviceName[32];        // Nombre del dispositivo
};

struct TagHistory {
    uint64_t tagId;            // ID del tag
    uint16_t countryCode;      // Código del país
    uint32_t timestamp;        // Momento de detección
    uint8_t signalStrength;    // Fuerza de la señal (0-255)
    bool valid;                // Entrada válida
};

struct SystemStats {
    uint32_t totalReadings;    // Total de lecturas
    uint32_t successfulReads;  // Lecturas exitosas
    uint32_t errorCount;       // Número de errores
    uint32_t uptime;           // Tiempo de funcionamiento
    float avgSignalStrength;   // Fuerza promedio de señal
};

// ==================== CLASE PRINCIPAL ====================

class RFIDUtils {
private:
    RFIDConfig config;
    TagHistory tagHistory[MAX_STORED_TAGS];
    SystemStats stats;
    uint8_t historyIndex;
    uint32_t lastConfigSave;
    
    // Filtro digital para señales
    float signalFilter[FILTER_SAMPLES];
    uint8_t filterIndex;
    
public:
    RFIDUtils();
    
    // Configuración
    bool loadConfig();
    bool saveConfig();
    void setDefaultConfig();
    RFIDConfig getConfig() { return config; }
    void setConfig(RFIDConfig newConfig);
    
    // Manejo de tags
    bool addTagReading(uint64_t tagId, uint16_t countryCode, uint8_t signalStrength);
    TagHistory getLastTag();
    TagHistory* getTagHistory(uint8_t count);
    bool isTagRecent(uint64_t tagId, uint32_t timeWindow = 3000);
    
    // Estadísticas
    SystemStats getStats() { return stats; }
    void updateStats(bool successful, uint8_t signalStrength);
    void resetStats();
    
    // Filtros y procesamiento
    float applySignalFilter(float newSample);
    bool validateFDXData(uint64_t tagId, uint16_t countryCode);
    uint8_t calculateSignalStrength(uint32_t* pulses, uint16_t count);
    
    // Utilidades
    String formatTagId(uint64_t tagId);
    String getCountryName(uint16_t countryCode);
    String formatTimestamp(uint32_t timestamp);
    bool isValidCountryCode(uint16_t code);
    
    // Diagnóstico
    void printDiagnostics();
    bool selfTest();
};

// ==================== IMPLEMENTACIÓN ====================

RFIDUtils::RFIDUtils() {
    historyIndex = 0;
    filterIndex = 0;
    lastConfigSave = 0;
    
    // Inicializar filtro
    for (int i = 0; i < FILTER_SAMPLES; i++) {
        signalFilter[i] = 0.0;
    }
    
    // Inicializar estadísticas
    memset(&stats, 0, sizeof(SystemStats));
    
    // Inicializar historial
    for (int i = 0; i < MAX_STORED_TAGS; i++) {
        tagHistory[i].valid = false;
    }
    
    setDefaultConfig();
}

void RFIDUtils::setDefaultConfig() {
    config.antennaFrequency = 134200.0;
    config.amplifierGain = 40;
    config.detectionThreshold = 128;
    config.filterSamples = FILTER_SAMPLES;
    config.autoSave = true;
    strcpy(config.deviceName, "RFID_FDX_Reader");
}

bool RFIDUtils::loadConfig() {
    EEPROM.begin(EEPROM_SIZE);
    
    // Verificar magic number para validar EEPROM
    uint32_t magic;
    EEPROM.get(0, magic);
    
    if (magic != 0xDEADBEEF) {
        Serial.println("EEPROM no inicializada, usando configuración por defecto");
        setDefaultConfig();
        return saveConfig();
    }
    
    // Cargar configuración
    EEPROM.get(4, config);
    
    Serial.println("Configuración cargada desde EEPROM");
    return true;
}

bool RFIDUtils::saveConfig() {
    EEPROM.begin(EEPROM_SIZE);
    
    // Escribir magic number
    uint32_t magic = 0xDEADBEEF;
    EEPROM.put(0, magic);
    
    // Escribir configuración
    EEPROM.put(4, config);
    
    EEPROM.commit();
    lastConfigSave = millis();
    
    Serial.println("Configuración guardada en EEPROM");
    return true;
}

void RFIDUtils::setConfig(RFIDConfig newConfig) {
    config = newConfig;
    
    if (config.autoSave) {
        saveConfig();
    }
}

bool RFIDUtils::addTagReading(uint64_t tagId, uint16_t countryCode, uint8_t signalStrength) {
    // Validar datos
    if (!validateFDXData(tagId, countryCode)) {
        return false;
    }
    
    // Verificar si no es un duplicado reciente
    if (isTagRecent(tagId)) {
        return false;
    }
    
    // Agregar al historial
    TagHistory newTag;
    newTag.tagId = tagId;
    newTag.countryCode = countryCode;
    newTag.timestamp = millis();
    newTag.signalStrength = signalStrength;
    newTag.valid = true;
    
    tagHistory[historyIndex] = newTag;
    historyIndex = (historyIndex + 1) % MAX_STORED_TAGS;
    
    // Actualizar estadísticas
    updateStats(true, signalStrength);
    
    Serial.printf("Tag agregado: %s, País: %s\\n", 
                  formatTagId(tagId).c_str(), 
                  getCountryName(countryCode).c_str());
    
    return true;
}

TagHistory RFIDUtils::getLastTag() {
    uint8_t lastIndex = (historyIndex > 0) ? historyIndex - 1 : MAX_STORED_TAGS - 1;
    return tagHistory[lastIndex];
}

bool RFIDUtils::isTagRecent(uint64_t tagId, uint32_t timeWindow) {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < MAX_STORED_TAGS; i++) {
        if (tagHistory[i].valid && 
            tagHistory[i].tagId == tagId && 
            (currentTime - tagHistory[i].timestamp) < timeWindow) {
            return true;
        }
    }
    return false;
}

void RFIDUtils::updateStats(bool successful, uint8_t signalStrength) {
    stats.totalReadings++;
    
    if (successful) {
        stats.successfulReads++;
        
        // Calcular promedio de señal (promedio móvil)
        float alpha = 0.1;  // Factor de suavizado
        stats.avgSignalStrength = stats.avgSignalStrength * (1 - alpha) + signalStrength * alpha;
    } else {
        stats.errorCount++;
    }
    
    stats.uptime = millis();
}

float RFIDUtils::applySignalFilter(float newSample) {
    // Filtro promedio móvil
    signalFilter[filterIndex] = newSample;
    filterIndex = (filterIndex + 1) % FILTER_SAMPLES;
    
    float sum = 0;
    for (int i = 0; i < FILTER_SAMPLES; i++) {
        sum += signalFilter[i];
    }
    
    return sum / FILTER_SAMPLES;
}

bool RFIDUtils::validateFDXData(uint64_t tagId, uint16_t countryCode) {
    // Verificar que el tag ID no sea 0
    if (tagId == 0) {
        return false;
    }
    
    // Verificar código de país válido
    if (!isValidCountryCode(countryCode)) {
        return false;
    }
    
    // Verificar rango del tag ID (máximo 38 bits para FDX-B)
    if (tagId > 0x3FFFFFFFFF) {  // 2^38 - 1
        return false;
    }
    
    return true;
}

uint8_t RFIDUtils::calculateSignalStrength(uint32_t* pulses, uint16_t count) {
    if (count == 0) return 0;
    
    // Calcular la consistencia de los pulsos como medida de calidad
    uint32_t avgPulse = 0;
    for (uint16_t i = 0; i < count; i++) {
        avgPulse += pulses[i];
    }
    avgPulse /= count;
    
    // Calcular desviación estándar
    uint32_t variance = 0;
    for (uint16_t i = 0; i < count; i++) {
        uint32_t diff = (pulses[i] > avgPulse) ? pulses[i] - avgPulse : avgPulse - pulses[i];
        variance += diff * diff;
    }
    variance /= count;
    
    uint32_t stdDev = sqrt(variance);
    
    // Convertir a escala 0-255 (menos desviación = mejor señal)
    uint8_t quality = 255 - constrain(stdDev * 10, 0, 255);
    
    return quality;
}

String RFIDUtils::formatTagId(uint64_t tagId) {
    char buffer[20];
    sprintf(buffer, "%015llu", tagId);
    return String(buffer);
}

String RFIDUtils::getCountryName(uint16_t countryCode) {
    switch (countryCode) {
        case 32: return "Argentina";
        case 76: return "Brasil";
        case 152: return "Chile";
        case 170: return "Colombia";
        case 484: return "México";
        case 858: return "Uruguay";
        case 724: return "España";
        case 250: return "Francia";
        case 276: return "Alemania";
        case 380: return "Italia";
        case 528: return "Países Bajos";
        case 826: return "Reino Unido";
        case 840: return "Estados Unidos";
        default: return "País " + String(countryCode);
    }
}

String RFIDUtils::formatTimestamp(uint32_t timestamp) {
    uint32_t seconds = timestamp / 1000;
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    char buffer[20];
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, secs);
    return String(buffer);
}

bool RFIDUtils::isValidCountryCode(uint16_t code) {
    // Lista de códigos de país válidos comunes para FDX-B
    uint16_t validCodes[] = {
        32,   // Argentina
        76,   // Brasil  
        152,  // Chile
        170,  // Colombia
        484,  // México
        858,  // Uruguay
        724,  // España
        250,  // Francia
        276,  // Alemania
        380,  // Italia
        528,  // Países Bajos
        826,  // Reino Unido
        840,  // Estados Unidos
        36,   // Australia
        124,  // Canadá
        554,  // Nueva Zelanda
        0     // Fin de lista
    };
    
    for (int i = 0; validCodes[i] != 0; i++) {
        if (code == validCodes[i]) {
            return true;
        }
    }
    
    // También aceptar códigos en rango válido ISO 3166
    return (code > 0 && code < 900);
}

void RFIDUtils::printDiagnostics() {
    Serial.println("\\n=== DIAGNÓSTICO DEL SISTEMA ===");
    Serial.printf("Dispositivo: %s\\n", config.deviceName);
    Serial.printf("Frecuencia antena: %.1f Hz\\n", config.antennaFrequency);
    Serial.printf("Ganancia amplificador: %d dB\\n", config.amplifierGain);
    Serial.printf("Umbral detección: %d\\n", config.detectionThreshold);
    
    Serial.println("\\n--- Estadísticas ---");
    Serial.printf("Total lecturas: %lu\\n", stats.totalReadings);
    Serial.printf("Lecturas exitosas: %lu\\n", stats.successfulReads);
    Serial.printf("Errores: %lu\\n", stats.errorCount);
    Serial.printf("Tasa éxito: %.1f%%\\n", 
                  stats.totalReadings > 0 ? (float)stats.successfulReads / stats.totalReadings * 100 : 0);
    Serial.printf("Señal promedio: %.1f\\n", stats.avgSignalStrength);
    Serial.printf("Tiempo funcionamiento: %s\\n", formatTimestamp(stats.uptime).c_str());
    
    Serial.println("\\n--- Último Tag ---");
    TagHistory lastTag = getLastTag();
    if (lastTag.valid) {
        Serial.printf("ID: %s\\n", formatTagId(lastTag.tagId).c_str());
        Serial.printf("País: %s\\n", getCountryName(lastTag.countryCode).c_str());
        Serial.printf("Señal: %d/255\\n", lastTag.signalStrength);
        Serial.printf("Tiempo: %s\\n", formatTimestamp(lastTag.timestamp).c_str());
    } else {
        Serial.println("No hay tags en historial");
    }
    
    Serial.println("===============================\\n");
}

bool RFIDUtils::selfTest() {
    Serial.println("Iniciando auto-test...");
    
    bool testPassed = true;
    
    // Test 1: Verificar configuración
    if (config.antennaFrequency < 130000 || config.antennaFrequency > 140000) {
        Serial.println("ERROR: Frecuencia de antena fuera de rango");
        testPassed = false;
    }
    
    // Test 2: Verificar EEPROM
    if (!saveConfig() || !loadConfig()) {
        Serial.println("ERROR: Problema con EEPROM");
        testPassed = false;
    }
    
    // Test 3: Verificar filtro
    for (int i = 0; i < 10; i++) {
        float filtered = applySignalFilter(100.0 + random(-10, 10));
        if (filtered < 90 || filtered > 110) {
            Serial.println("ERROR: Filtro de señal no funciona correctamente");
            testPassed = false;
            break;
        }
    }
    
    // Test 4: Verificar validación
    if (!validateFDXData(123456789, 32)) {
        Serial.println("ERROR: Validación de datos no funciona");
        testPassed = false;
    }
    
    if (validateFDXData(0, 999)) {
        Serial.println("ERROR: Validación acepta datos inválidos");
        testPassed = false;
    }
    
    Serial.printf("Auto-test %s\\n", testPassed ? "PASADO" : "FALLIDO");
    return testPassed;
}

#endif // RFID_FDX_UTILS_H
