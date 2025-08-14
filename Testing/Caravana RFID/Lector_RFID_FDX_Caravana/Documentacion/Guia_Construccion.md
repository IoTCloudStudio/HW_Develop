# Guía de Construcción - Lector RFID FDX Caravanas

## Resumen del Proyecto

Este documento proporciona instrucciones paso a paso para construir un lector RFID FDX-B capaz de leer caravanas de ganado con chips de identificación de 134.2kHz.

## Herramientas Requeridas

### Herramientas Básicas
- [ ] Soldador de 40W con punta fina
- [ ] Estaño 60/40 o libre de plomo (0.6mm)
- [ ] Flux para soldadura
- [ ] Desoldador o malla desoldadora
- [ ] Multímetro digital
- [ ] Alicates de punta fina
- [ ] Destornilladores de precisión
- [ ] Lupa o microscopio (recomendado)

### Herramientas de Medición
- [ ] Osciloscopio (mínimo 10MHz)
- [ ] Generador de funciones
- [ ] LCR meter (para medir inductancia)
- [ ] Fuente de alimentación regulada 5V/1A

### Materiales Adicionales
- [ ] Alambre esmaltado AWG 26 (2 metros)
- [ ] Tubo PVC Ø80mm (para molde de bobina)
- [ ] Cinta aislante
- [ ] Barniz aislante (opcional)

## Fase 1: Preparación

### 1.1 Verificación de Componentes

Antes de comenzar, verificar que se tienen todos los componentes según la lista:

**Semiconductores:**
- [ ] ESP32-WROOM-32D (x1)
- [ ] LM358N (x1)
- [ ] LM1117-3.3V (x1)
- [ ] 1N4148 (x2)
- [ ] LEDs 3mm (verde y rojo)

**Pasivos:**
- [ ] Resistencias según valores especificados
- [ ] Capacitores (cerámicos y electrolíticos)
- [ ] Capacitor trimmer 22nF

**Conectores:**
- [ ] Jack de alimentación 2.1mm
- [ ] Header para programación
- [ ] Pulsadores 6x6mm (x2)

### 1.2 Preparación del Espacio de Trabajo

1. **Área limpia** y bien iluminada
2. **Base antiestática** para componentes sensibles
3. **Ventilación adecuada** para soldadura
4. **Organización** de componentes por tipo

## Fase 2: Construcción de la Antena

### 2.1 Cálculo de la Bobina

Para 134.2kHz necesitamos aproximadamente 120μH:

```
Inductancia = (d² × n²) / (18d + 40l)
120μH = (3.15² × n²) / (18×3.15 + 40×0.2)

Donde:
- d = 3.15 pulgadas (80mm)
- n = número de vueltas (18-20)
- l = longitud de la bobina
```

### 2.2 Construcción de la Bobina

#### Paso 1: Preparar el Molde
1. **Cortar tubo PVC** de 80mm de diámetro x 10cm de largo
2. **Marcar posiciones** cada 5mm para vueltas uniformes
3. **Hacer pequeñas muescas** para guiar el alambre

#### Paso 2: Bobinado
1. **Fijar un extremo** del alambre con cinta
2. **Bobinar 18 vueltas** siguiendo las marcas
3. **Mantener tensión uniforme** durante el bobinado
4. **Espaciado uniforme** entre vueltas
5. **Fijar el extremo final** con cinta

#### Paso 3: Medición y Ajuste
1. **Medir inductancia** con LCR meter
2. **Objetivo**: 120μH ± 10%
3. **Si es bajo**: Agregar 1-2 vueltas
4. **Si es alto**: Quitar 1-2 vueltas
5. **Remedir** hasta obtener valor correcto

#### Paso 4: Fijación Final
1. **Aplicar barniz aislante** (opcional)
2. **Dejar secar** 24 horas
3. **Retirar del molde** cuidadosamente
4. **Preparar terminales** para conexión

### 2.3 Prueba de la Antena

```
Frecuencia de resonancia = 1 / (2π√LC)

Con C = 100nF + 22nF = 122nF
f = 1 / (2π√(120μH × 122nF)) ≈ 131kHz
```

Ajustar el trimmer de 22nF para obtener exactamente 134.2kHz.

## Fase 3: Ensamblaje del PCB

### 3.1 Soldadura de Componentes SMD (Si aplica)

**Orden recomendado:**
1. **Componentes más pequeños primero** (resistencias, capacitores pequeños)
2. **ICs en package SMD**
3. **Componentes de mayor tamaño**

**Técnica:**
1. **Aplicar flux** en los pads
2. **Pre-estaño** un pad
3. **Posicionar componente** con pinzas
4. **Soldar un pin** para fijar
5. **Soldar resto de pines**
6. **Verificar soldaduras** con lupa

### 3.2 Soldadura de Componentes Through-Hole

#### Orden de Montaje:
1. **Resistencias** (más bajas primero)
2. **Diodos** (verificar polaridad)
3. **Capacitores cerámicos**
4. **Zócalos para ICs** (opcional)
5. **Capacitores electrolíticos** (verificar polaridad)
6. **Conectores**
7. **Pulsadores**
8. **LEDs** (verificar polaridad)

#### Técnicas de Soldadura:
```
Para cada componente:
1. Insertar componente
2. Verificar orientación
3. Doblegar patas para fijar
4. Soldar con estaño suficiente
5. Cortar exceso de patas
6. Inspeccionar soldadura
```

### 3.3 Instalación de ICs

**Si usa zócalos:**
1. **Verificar orientación** del zócalo (muesca)
2. **Soldar todos los pines**
3. **Insertar IC** en zócalo (verificar orientación)

**Soldadura directa:**
1. **Aplicar flux** en pines
2. **Posicionar IC** correctamente
3. **Soldar pin 1** y último pin para fijar
4. **Soldar resto de pines**
5. **Verificar soldaduras** y posibles cortos

## Fase 4: Conexión de la Antena

### 4.1 Preparación de Conexiones

1. **Identificar puntos** de conexión de antena en PCB
2. **Preparar cables** de conexión (AWG 22-24)
3. **Longitud mínima** para evitar interferencias

### 4.2 Conexión del Circuito Resonante

```
Conexiones de antena:
- Terminal 1 de L1 → Punto "ANT+" en PCB
- Terminal 2 de L1 → A través de C1 (100nF) → GND
- Trimmer C2 en paralelo con C1
```

### 4.3 Montaje de la Antena

1. **Fijar antena** a la caja o estructura
2. **Evitar materiales metálicos** cerca de la antena
3. **Orientación**: Plano de la bobina perpendicular al tag
4. **Distancia mínima**: 5cm de otros circuitos

## Fase 5: Programación del ESP32

### 5.1 Configuración del Entorno

#### Arduino IDE:
1. **Instalar ESP32 Core** para Arduino
2. **Configurar board**: ESP32 Dev Module
3. **Configurar puerto** COM correspondiente
4. **Velocidad**: 115200 baud

#### Librerías Requeridas:
```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
```

### 5.2 Carga del Firmware

1. **Conectar ESP32** vía USB
2. **Presionar BOOT** antes de programar
3. **Compilar y cargar** el código
4. **Verificar carga** exitosa en monitor serie
5. **Resetear** dispositivo

### 5.3 Verificación Inicial

**Monitor Serie debe mostrar:**
```
=== Lector RFID FDX-B para Caravanas ===
Hardware Development - IoT Cloud Studio
Clock 134.2kHz configurado en GPIO25
WiFi AP iniciado: RFID_Reader_AP
IP: 192.168.4.1
Sistema listo para lectura de caravanas FDX-B
```

## Fase 6: Pruebas y Calibración

### 6.1 Verificación de Alimentación

1. **Conectar fuente 5V**
2. **Verificar 3.3V** en pin ESP32
3. **LED verde** debe encender
4. **Consumo típico**: 100-150mA

### 6.2 Verificación de Señales

#### Con Osciloscopio:
1. **Pin 25 (Clock)**: Señal cuadrada 134.2kHz
2. **Pin 34 (Data)**: Debe cambiar con proximidad de tags
3. **Antena**: Señal senoidal 134.2kHz

#### Ajuste de Frecuencia:
1. **Conectar osciloscopio** a la antena
2. **Acercar tag FDX-B** para activar
3. **Ajustar trimmer C2** para 134.2kHz exactos
4. **Verificar estabilidad** de la frecuencia

### 6.3 Pruebas Funcionales

#### Test 1: Detección Básica
1. **Encender sistema**
2. **Acercar tag FDX-B** conocido
3. **Verificar detección** en monitor serie
4. **LED rojo** debe parpadear durante lectura
5. **LED verde** debe encender al detectar

#### Test 2: Rango de Lectura
1. **Usar tag de referencia**
2. **Medir distancia máxima** de detección
3. **Objetivo**: 8-15cm típico
4. **Documentar resultados**

#### Test 3: Interfaz Web
1. **Conectar a WiFi** del dispositivo
2. **Acceder a** http://192.168.4.1
3. **Verificar interfaz** carga correctamente
4. **Probar lectura** desde web

### 6.4 Calibración de Sensibilidad

Si la sensibilidad es baja:
1. **Aumentar ganancia** del amplificador (R5, R6)
2. **Ajustar umbrales** en software
3. **Verificar antena** está bien sintonizada
4. **Revisar filtros** RF

## Fase 7: Ensamblaje Final

### 7.1 Preparación de la Caja

1. **Seleccionar caja** plástica adecuada
2. **Hacer orificios** para:
   - Jack de alimentación
   - LEDs indicadores
   - Botones de control
   - Antena (si es externa)
3. **Preparar montajes** internos

### 7.2 Montaje Final

1. **Fijar PCB** dentro de la caja
2. **Conectar elementos** externos
3. **Verificar que no hay cortos**
4. **Cerrar caja** temporalmente para pruebas

### 7.3 Pruebas Finales

1. **Prueba completa** de todas las funciones
2. **Verificar rango** de lectura
3. **Test de durabilidad** (encendido/apagado)
4. **Documentar problemas** encontrados

## Fase 8: Documentación y Entrega

### 8.1 Documentación Técnica

1. **Registrar valores** de componentes finales
2. **Documentar ajustes** realizados
3. **Crear manual** específico del dispositivo
4. **Fotografiar** el ensamblaje

### 8.2 Archivo de Configuración

```json
{
  "device_id": "RFID_FDX_001",
  "antenna_frequency": 134200,
  "antenna_inductance": 120.5,
  "capacitor_trimmer": 18.2,
  "amplifier_gain": 40,
  "detection_threshold": 2.1,
  "build_date": "2025-08-14",
  "firmware_version": "1.0.0"
}
```

## Solución de Problemas Durante la Construcción

### Problema: No hay señal de clock
**Causas posibles:**
- ESP32 no programado correctamente
- Error en conexiones GPIO25
- Problema en el generador PWM

**Soluciones:**
- Verificar código cargado
- Medir con osciloscopio
- Revisar conexiones

### Problema: No detecta tags
**Causas posibles:**
- Antena mal sintonizada
- Amplificador sin ganancia
- Tags no compatibles

**Soluciones:**
- Ajustar capacitor trimmer
- Verificar LM358
- Probar con tags conocidos

### Problema: Detección errática
**Causas posibles:**
- Interferencias electromagnéticas
- Ganancia excesiva
- Problemas de alimentación

**Soluciones:**
- Agregar filtros
- Reducir ganancia
- Verificar fuente estable

## Lista de Verificación Final

### Antes de Cerrar la Caja:
- [ ] Todas las soldaduras verificadas
- [ ] No hay cortos circuitos
- [ ] LEDs funcionan correctamente
- [ ] Clock de 134.2kHz presente
- [ ] Antena sintonizada
- [ ] WiFi funcionando
- [ ] Interfaz web accesible
- [ ] Detecta tags FDX-B
- [ ] Rango de lectura adecuado
- [ ] Consumo dentro de especificaciones

### Documentación Completada:
- [ ] Esquemático final actualizado
- [ ] Lista de componentes usados
- [ ] Valores de calibración
- [ ] Manual de usuario
- [ ] Procedimientos de prueba
- [ ] Certificado de calidad

---

**¡Felicitaciones! Has completado la construcción del Lector RFID FDX para Caravanas**

Para soporte técnico adicional, contactar: soporte@iotcloudstudio.com
