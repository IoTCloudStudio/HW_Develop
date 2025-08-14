# Especificaciones Técnicas - Lector RFID FDX Caravanas

## Cálculos de Diseño de Antena

### Frecuencia de Resonancia: 134.2kHz

La frecuencia de resonancia se calcula como:
```
f = 1 / (2π √(LC))
```

Donde:
- f = 134.2kHz
- L = Inductancia de la bobina (μH)
- C = Capacitancia total (nF)

### Diseño de Bobina

**Configuración recomendada para caravanas:**
- **Diámetro**: 80-100mm (mayor alcance)
- **Número de vueltas**: 15-20 vueltas
- **Cable**: AWG 26-28 (0.4-0.32mm)
- **Inductancia calculada**: ~100-150μH

**Cálculo de inductancia (bobina circular):**
```
L = (d² × n²) / (18d + 40l)
```
Donde:
- d = diámetro en pulgadas
- n = número de vueltas
- l = longitud de la bobina en pulgadas

### Capacitor de Sintonía

Para L = 120μH y f = 134.2kHz:
```
C = 1 / (4π²f²L)
C = 1 / (4π² × (134200)² × 120×10⁻⁶)
C ≈ 117nF
```

**Componentes recomendados:**
- Capacitor principal: 100nF (cerámico, 5%)
- Capacitor de ajuste: 22nF variable (trimmer)

## Circuito Amplificador

### LM358 Configuración

**Etapa 1 - Amplificador de Instrumentación:**
- Ganancia: 100x (40dB)
- Ancho de banda: 100Hz - 500kHz
- Resistencia de entrada: >1MΩ

**Etapa 2 - Comparador Schmitt:**
- Umbral superior: 2.5V
- Umbral inferior: 1.0V
- Histéresis: 1.5V

### Filtros

**Filtro Pasa-Altos (HPF):**
- Frecuencia de corte: 100kHz
- Configuración: RC simple
- R = 1.6kΩ, C = 1nF

**Filtro Pasa-Bajos (LPF):**
- Frecuencia de corte: 200kHz
- Configuración: RC simple
- R = 800Ω, C = 1nF

## Especificaciones Eléctricas

### Alimentación
- **Voltaje de entrada**: 5V ± 0.25V
- **Consumo típico**: 150mA
- **Consumo en standby**: 50mA
- **Regulación 3.3V**: LM1117-3.3V

### Señales Digitales
- **Clock output**: 3.3V CMOS, 134.2kHz
- **Data input**: 3.3V CMOS, TTL compatible
- **Nivel lógico alto**: >2.4V
- **Nivel lógico bajo**: <0.8V

### Rango de Operación
- **Distancia de lectura**: 8-15cm (caravanas estándar)
- **Temperatura operación**: -20°C a +70°C
- **Humedad**: 0-95% no condensante

## Lista de Componentes

### Semiconductores
| Componente | Valor | Cantidad | Descripción |
|------------|-------|----------|-------------|
| ESP32-WROOM-32 | - | 1 | Microcontrolador principal |
| LM358N | - | 1 | Amplificador operacional dual |
| LM1117-3.3V | 3.3V | 1 | Regulador de voltaje |
| 1N4148 | - | 2 | Diodos de protección |

### Pasivos
| Componente | Valor | Cantidad | Descripción |
|------------|-------|----------|-------------|
| R1, R2 | 10kΩ | 2 | Pull-up resistors |
| R3 | 1.6kΩ | 1 | Filtro HPF |
| R4 | 800Ω | 1 | Filtro LPF |
| R5, R6 | 100kΩ | 2 | Ganancia amplificador |
| R7 | 1kΩ | 1 | Resistor de referencia |
| C1 | 100nF | 1 | Capacitor principal antena |
| C2 | 22nF | 1 | Capacitor ajuste (trimmer) |
| C3, C4 | 1nF | 2 | Capacitores de filtro |
| C5, C6 | 100μF | 2 | Capacitores de alimentación |
| C7, C8 | 22pF | 2 | Capacitores del cristal |

### Otros
| Componente | Valor | Cantidad | Descripción |
|------------|-------|----------|-------------|
| L1 | 120μH | 1 | Bobina de antena |
| X1 | 32.768kHz | 1 | Cristal para RTC |
| SW1 | - | 1 | Pulsador reset |
| LED1 | Verde | 1 | LED estado |
| LED2 | Rojo | 1 | LED error |

## Consideraciones de PCB

### Layout Guidelines
1. **Separación de planos**: Analógico y digital separados
2. **Blindaje**: Cobre en GND alrededor de la antena
3. **Vías**: Múltiples vías para conexiones GND
4. **Componentes críticos**: Cerca del ESP32

### Routing
- **Trazas de antena**: Anchas (0.5mm mínimo)
- **Trazas de señal**: 0.2mm típico
- **Longitud diferencial**: Mantener balanceadas
- **Impedancia**: 50Ω para señales de clock

### Mechanical
- **Tamaño PCB**: 60mm x 80mm (máximo)
- **Grosor**: 1.6mm estándar
- **Capas**: 4 capas (señal/GND/VCC/señal)
- **Finish**: HASL o ENIG

## Testing y Validación

### Pruebas de Laboratorio
1. **Frecuencia de resonancia**: Verificar 134.2kHz ± 1%
2. **Ganancia del amplificador**: Medir 40dB ± 2dB
3. **Sensibilidad**: Detectar señal de -60dBm
4. **Rango dinámico**: 40dB mínimo

### Pruebas de Campo
1. **Distancia de lectura**: Mínimo 8cm con caravanas estándar
2. **Velocidad de lectura**: <500ms por identificación
3. **Tasa de error**: <0.1% en condiciones normales
4. **Interferencia**: Resistencia a EMI de 50Hz/60Hz

## Notas de Fabricación

### Ensamblaje
- Soldar primero componentes SMD
- Bobina de antena: Bobinado manual o pre-fabricada
- Ajuste de capacitor trimmer después del ensamblaje
- Programación del ESP32: Via USB o ISP

### Calibración
1. Conectar generador de señales a la antena
2. Ajustar trimmer para resonancia a 134.2kHz
3. Verificar amplificación con osciloscopio
4. Probar con caravana conocida
5. Calibrar umbrales de detección en software
