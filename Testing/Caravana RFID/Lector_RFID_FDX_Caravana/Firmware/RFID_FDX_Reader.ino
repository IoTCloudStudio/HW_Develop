/*
 * Lector RFID FDX-B para Caravanas de Ganado
 * 
 * Firmware para ESP32 que implementa la lectura de tags FDX-B
 * según estándar ISO 11784/11785 para identificación animal
 * 
 * Hardware Development - IoT Cloud Studio
 * 2025
 */

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ==================== CONFIGURACIÓN DE PINES ====================
#define DATA_PIN        34    // GPIO34 - Entrada de datos demodulados
#define CLOCK_PIN       25    // GPIO25 - Salida de clock 134.2kHz
#define LED_STATUS_PIN  2     // GPIO2 - LED de estado
#define BOOT_PIN        0     // GPIO0 - Botón de boot
#define I2C_SDA_PIN     21    // GPIO21 - I2C SDA para LCD
#define I2C_SCL_PIN     22    // GPIO22 - I2C SCL para LCD

// ==================== CONSTANTES DEL PROTOCOLO FDX-B ====================
#define FDX_FREQUENCY       134200    // 134.2kHz
#define FDX_BIT_RATE        4190      // ~4.19 kbps
#define FDX_BIT_DURATION    240       // 240 microsegundos por bit
#define FDX_SHORT_PULSE     120       // Pulso corto (0 lógico)
#define FDX_LONG_PULSE      240       // Pulso largo (1 lógico)
#define FDX_HEADER_SIZE     11        // 10 ceros + 1 uno
#define FDX_TOTAL_BITS      128       // Bits totales del frame
#define FDX_DATA_BITS       64        // Bits de datos útiles

#define PULSE_BUFFER_SIZE   512       // Buffer para pulsos capturados
#define BIT_BUFFER_SIZE     256       // Buffer para bits decodificados

// ==================== TOLERANCIAS DE TIMING ====================
#define PULSE_MIN_TIME      80        // Tiempo mínimo de pulso válido (μs)
#define PULSE_MAX_TIME      400       // Tiempo máximo de pulso válido (μs)
#define SHORT_LONG_BOUNDARY 180       // Límite entre pulso corto y largo (μs)

// ==================== VARIABLES GLOBALES ====================
volatile bool dataAvailable = false;
volatile uint32_t pulseBuffer[PULSE_BUFFER_SIZE];
volatile uint16_t pulseIndex = 0;
volatile uint32_t lastInterruptTime = 0;
volatile bool capturing = false;

uint8_t bitBuffer[BIT_BUFFER_SIZE];
uint16_t bitCount = 0;
uint64_t currentTagId = 0;
uint16_t countryCode = 0;
bool tagDetected = false;
uint32_t lastTagTime = 0;

// ==================== OBJETOS Y CONFIGURACIÓN ====================
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Display LCD 16x2

// Configuración WiFi
const char* ssid = "RFID_Reader_AP";
const char* password = "rfid2025";

// ==================== ESTRUCTURA DE DATOS ====================
struct FDXTag {
    uint64_t animalId;      // ID del animal (38 bits)
    uint16_t countryCode;   // Código del país (10 bits)
    uint8_t application;    // Bit de aplicación
    uint8_t checksum;       // Bits de verificación
    uint32_t timestamp;     // Momento de lectura
    bool valid;             // Tag válido
};

FDXTag lastTag = {0, 0, 0, 0, 0, false};

// ==================== FUNCIONES DE INTERRUPCIÓN ====================

// ISR para captura de pulsos de datos
void IRAM_ATTR dataISR() {
    uint32_t currentTime = micros();
    uint32_t pulseWidth = currentTime - lastInterruptTime;
    lastInterruptTime = currentTime;
    
    // Verificar si el pulso está dentro del rango válido
    if (pulseWidth > PULSE_MIN_TIME && pulseWidth < PULSE_MAX_TIME) {
        if (pulseIndex < PULSE_BUFFER_SIZE - 1) {
            pulseBuffer[pulseIndex++] = pulseWidth;
        }
        
        // Si tenemos suficientes pulsos, marcar datos disponibles
        if (pulseIndex >= FDX_TOTAL_BITS * 2) {
            dataAvailable = true;
            capturing = false;
            detachInterrupt(digitalPinToInterrupt(DATA_PIN));
        }
    }
}

// ==================== GENERADOR DE CLOCK ====================

void setupClock() {
    // Configurar PWM para generar 134.2kHz
    ledcSetup(0, FDX_FREQUENCY, 8);  // Canal 0, 134.2kHz, 8-bit resolution
    ledcAttachPin(CLOCK_PIN, 0);
    ledcWrite(0, 128);  // 50% duty cycle
    
    Serial.println("Clock 134.2kHz configurado en GPIO25");
}

void startClock() {
    ledcWrite(0, 128);  // Activar clock
}

void stopClock() {
    ledcWrite(0, 0);    // Parar clock
}

// ==================== DECODIFICACIÓN FDX-B ====================

bool decodePulses() {
    if (pulseIndex < FDX_TOTAL_BITS) {
        return false;
    }
    
    bitCount = 0;
    uint16_t i = 0;
    
    // Convertir pulsos a bits según protocolo FDX-B
    while (i < pulseIndex - 1 && bitCount < BIT_BUFFER_SIZE) {
        uint32_t pulse1 = pulseBuffer[i];
        uint32_t pulse2 = pulseBuffer[i + 1];
        
        // FDX-B: Pulso largo = 1, dos pulsos cortos = 0
        if (pulse1 > SHORT_LONG_BOUNDARY) {
            // Pulso largo = bit 1
            bitBuffer[bitCount++] = 1;
            i++;
        } else if (pulse1 < SHORT_LONG_BOUNDARY && pulse2 < SHORT_LONG_BOUNDARY) {
            // Dos pulsos cortos = bit 0
            bitBuffer[bitCount++] = 0;
            i += 2;
        } else {
            // Patrón inválido, avanzar
            i++;
        }
    }
    
    return (bitCount >= FDX_TOTAL_BITS);
}

bool findFDXHeader() {
    // Buscar secuencia de header: 10 ceros seguidos de un 1
    for (uint16_t i = 0; i <= bitCount - FDX_HEADER_SIZE; i++) {
        bool headerFound = true;
        
        // Verificar 10 ceros
        for (uint8_t j = 0; j < 10; j++) {
            if (bitBuffer[i + j] != 0) {
                headerFound = false;
                break;
            }
        }
        
        // Verificar el 1 que sigue
        if (headerFound && bitBuffer[i + 10] == 1) {
            // Header encontrado, mover bits al inicio
            for (uint16_t k = 0; k < bitCount - i; k++) {
                bitBuffer[k] = bitBuffer[i + k];
            }
            bitCount -= i;
            return true;
        }
    }
    
    return false;
}

bool parseFDXData() {
    if (bitCount < FDX_DATA_BITS) {
        return false;
    }
    
    // Saltar header (11 bits)
    uint16_t dataStart = 11;
    uint64_t animalId = 0;
    uint16_t country = 0;
    
    // Extraer ID del animal (38 bits) - bits 11-48
    for (uint8_t i = 0; i < 38; i++) {
        animalId = (animalId << 1) | bitBuffer[dataStart + i];
    }
    
    // Extraer código del país (10 bits) - bits 49-58
    for (uint8_t i = 0; i < 10; i++) {
        country = (country << 1) | bitBuffer[dataStart + 38 + i];
    }
    
    // Verificar si los datos son válidos (no todo ceros)
    if (animalId != 0 || country != 0) {
        currentTagId = animalId;
        countryCode = country;
        
        // Actualizar estructura del tag
        lastTag.animalId = animalId;
        lastTag.countryCode = country;
        lastTag.application = bitBuffer[dataStart + 48];
        lastTag.timestamp = millis();
        lastTag.valid = true;
        
        return true;
    }
    
    return false;
}

// ==================== FUNCIONES DE LECTURA ====================

void startReading() {
    // Limpiar buffers
    pulseIndex = 0;
    bitCount = 0;
    dataAvailable = false;
    capturing = true;
    
    // Configurar interrupción para captura de datos
    lastInterruptTime = micros();
    attachInterrupt(digitalPinToInterrupt(DATA_PIN), dataISR, CHANGE);
    
    // Activar clock por un período corto para energizar el tag
    startClock();
    delay(100);  // 100ms de activación
    stopClock();
    
    Serial.println("Iniciando lectura RFID...");
}

bool processReading() {
    if (!dataAvailable) {
        return false;
    }
    
    dataAvailable = false;
    Serial.printf("Pulsos capturados: %d\\n", pulseIndex);
    
    // Decodificar pulsos a bits
    if (!decodePulses()) {
        Serial.println("Error: No se pudieron decodificar los pulsos");
        return false;
    }
    
    Serial.printf("Bits decodificados: %d\\n", bitCount);
    
    // Buscar header FDX-B
    if (!findFDXHeader()) {
        Serial.println("Error: Header FDX-B no encontrado");
        return false;
    }
    
    Serial.println("Header FDX-B encontrado");
    
    // Parsear datos del tag
    if (!parseFDXData()) {
        Serial.println("Error: No se pudieron parsear los datos del tag");
        return false;
    }
    
    tagDetected = true;
    lastTagTime = millis();
    
    Serial.printf("Tag FDX-B detectado!\\n");
    Serial.printf("ID Animal: %llu\\n", currentTagId);
    Serial.printf("Código País: %u\\n", countryCode);
    
    return true;
}

// ==================== DISPLAY LCD ====================

void setupLCD() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    lcd.setCursor(0, 0);
    lcd.print("RFID FDX Reader");
    lcd.setCursor(0, 1);
    lcd.print("Iniciando...");
    
    delay(2000);
    lcd.clear();
}

void updateLCD() {
    lcd.clear();
    
    if (tagDetected && (millis() - lastTagTime < 5000)) {
        lcd.setCursor(0, 0);
        lcd.print("Tag Detectado:");
        lcd.setCursor(0, 1);
        
        // Mostrar los últimos 8 dígitos del ID
        char idStr[17];
        sprintf(idStr, "%08llu", currentTagId % 100000000);
        lcd.print(idStr);
    } else {
        lcd.setCursor(0, 0);
        lcd.print("RFID FDX Lector");
        lcd.setCursor(0, 1);
        lcd.print("Esperando tag...");
    }
}

// ==================== SERVIDOR WEB ====================

void setupWebServer() {
    // Página principal
    server.on("/", HTTP_GET, []() {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Lector RFID FDX-B</title>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: Arial; margin: 20px; background-color: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .header { text-align: center; color: #333; }
        .status { padding: 15px; margin: 10px 0; border-radius: 5px; }
        .detected { background-color: #d4edda; border: 1px solid #c3e6cb; }
        .waiting { background-color: #fff3cd; border: 1px solid #ffeaa7; }
        .data { margin: 10px 0; }
        .btn { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }
        .btn:hover { background-color: #0056b3; }
    </style>
    <script>
        function refreshData() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status').innerHTML = updateStatus(data);
                });
        }
        
        function updateStatus(data) {
            if (data.tagDetected) {
                return `
                    <div class='status detected'>
                        <h3>🐄 Tag Detectado</h3>
                        <div class='data'><strong>ID Animal:</strong> ${data.animalId}</div>
                        <div class='data'><strong>Código País:</strong> ${data.countryCode}</div>
                        <div class='data'><strong>Tiempo:</strong> ${new Date(data.timestamp).toLocaleString()}</div>
                    </div>
                `;
            } else {
                return `
                    <div class='status waiting'>
                        <h3>📡 Esperando Tag...</h3>
                        <div class='data'>Acerque la caravana al lector</div>
                    </div>
                `;
            }
        }
        
        setInterval(refreshData, 1000); // Actualizar cada segundo
    </script>
</head>
<body>
    <div class='container'>
        <h1 class='header'>🐄 Lector RFID FDX-B para Caravanas</h1>
        <div id='status'>
            <div class='status waiting'>
                <h3>📡 Esperando Tag...</h3>
                <div class='data'>Acerque la caravana al lector</div>
            </div>
        </div>
        <button class='btn' onclick='refreshData()'>🔄 Actualizar</button>
        <button class='btn' onclick='fetch("/api/clear")'>🗑️ Limpiar</button>
    </div>
</body>
</html>
        )";
        server.send(200, "text/html", html);
    });
    
    // API de estado
    server.on("/api/status", HTTP_GET, []() {
        DynamicJsonDocument doc(1024);
        
        doc["tagDetected"] = tagDetected && (millis() - lastTagTime < 5000);
        doc["animalId"] = String(currentTagId);
        doc["countryCode"] = countryCode;
        doc["timestamp"] = lastTag.timestamp;
        doc["uptime"] = millis();
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
    
    // API para limpiar datos
    server.on("/api/clear", HTTP_GET, []() {
        tagDetected = false;
        currentTagId = 0;
        countryCode = 0;
        lastTag = {0, 0, 0, 0, 0, false};
        
        server.send(200, "application/json", "{\"status\":\"cleared\"}");
    });
    
    server.begin();
    Serial.println("Servidor web iniciado en http://192.168.4.1");
}

// ==================== CONFIGURACIÓN INICIAL ====================

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Lector RFID FDX-B para Caravanas ===");
    Serial.println("Hardware Development - IoT Cloud Studio");
    
    // Configurar pines
    pinMode(DATA_PIN, INPUT_PULLUP);
    pinMode(LED_STATUS_PIN, OUTPUT);
    pinMode(BOOT_PIN, INPUT_PULLUP);
    
    // LED de inicio
    digitalWrite(LED_STATUS_PIN, HIGH);
    
    // Configurar LCD
    setupLCD();
    
    // Configurar generador de clock
    setupClock();
    
    // Configurar WiFi como Access Point
    WiFi.softAP(ssid, password);
    Serial.printf("WiFi AP iniciado: %s\\n", ssid);
    Serial.printf("IP: %s\\n", WiFi.softAPIP().toString().c_str());
    
    // Configurar servidor web
    setupWebServer();
    
    // LED listo
    digitalWrite(LED_STATUS_PIN, LOW);
    
    Serial.println("Sistema listo para lectura de caravanas FDX-B");
    Serial.println("Conecte a WiFi: " + String(ssid));
    Serial.println("Acceda a: http://192.168.4.1");
}

// ==================== BUCLE PRINCIPAL ====================

void loop() {
    // Manejar servidor web
    server.handleClient();
    
    // Verificar si se presionó el botón
    static bool lastButtonState = HIGH;
    bool currentButtonState = digitalRead(BOOT_PIN);
    
    if (lastButtonState == HIGH && currentButtonState == LOW) {
        // Botón presionado, iniciar lectura
        startReading();
        digitalWrite(LED_STATUS_PIN, HIGH);
    }
    lastButtonState = currentButtonState;
    
    // Procesar datos si están disponibles
    if (dataAvailable) {
        bool success = processReading();
        digitalWrite(LED_STATUS_PIN, success ? LOW : HIGH);
        
        if (success) {
            updateLCD();
        }
    }
    
    // Actualizar LCD periódicamente
    static uint32_t lastLCDUpdate = 0;
    if (millis() - lastLCDUpdate > 2000) {
        updateLCD();
        lastLCDUpdate = millis();
    }
    
    // Limpiar detección antigua
    if (tagDetected && (millis() - lastTagTime > 10000)) {
        tagDetected = false;
        Serial.println("Tag expirado, limpiando datos");
    }
    
    delay(10);
}
