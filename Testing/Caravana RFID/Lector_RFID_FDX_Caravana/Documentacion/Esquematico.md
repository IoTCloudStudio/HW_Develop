# Esquemático del Lector RFID FDX para Caravanas

## Diagrama de Bloques

```
[Antena] → [Amplificador] → [Filtros] → [Comparador] → [ESP32]
    ↑           LM358        RC          Schmitt      Digital
[Clock Gen] ←────────────────────────────────────────┘
```

## Circuito Principal

### 1. Antena Resonante (134.2kHz)

```
           L1 (120μH)
    +──────┬─────┬──────+
    │      │     │      │
    │      │     C1     │  
    │   RF_OUT  100nF   │
    │      │     │      │
    │      │     C2     │
    │      │    22nF    │
    │      │   (trim)   │
    +──────┴─────┴──────+
           GND

RF_OUT → Pin amplificador
```

**Componentes:**
- L1: Bobina 120μH, Ø80mm, 18 vueltas, AWG 26
- C1: 100nF, cerámico, 5%, 50V
- C2: 22nF trimmer, ajuste fino

### 2. Amplificador y Acondicionador (LM358)

```
              +5V
               │
               R5 (100kΩ)
               │
    RF_IN ──┬──┤+
            │  │  IC1A
    ┌───────┤  │  LM358
    │   R6  └──┤-
    │  100kΩ   │
    │          └── AMP_OUT
    │
    └── C3 (1nF) ── GND

    AMP_OUT ──┬── R3 (1.6kΩ) ──┬── FILT_OUT
              │                 │
              │                 C4 (1nF)
              │                 │
              └─────────────────┴── GND
```

### 3. Comparador Schmitt (LM358)

```
              +5V
               │
               R7 (10kΩ)    R8 (10kΩ)
               │            │
    FILT_OUT ──┤+           ├── +3.3V
               │  IC1B      │
               │  LM358     R9 (10kΩ)
               │            │
    ┌──────────┤-           │
    │          │            │
    │          └── DATA_OUT ┴── GND
    │
    └── R10 (1kΩ) ── GND
```

### 4. Generador de Clock (ESP32)

```
ESP32 GPIO25 ── R11 (220Ω) ── CLOCK_OUT ──┐
                                          │
                                          └── A la antena
```

### 5. Interfaz ESP32

```
ESP32 Connections:

GPIO34 (ADC1_CH6) ← DATA_IN  (señal demodulada)
GPIO25 (DAC1)     → CLOCK_OUT (134.2kHz)
GPIO21 (I2C_SDA)  ↔ Display opcional
GPIO22 (I2C_SCL)  ↔ Display opcional
GPIO0  (BOOT)     ← Button (pull-up interno)
GPIO2  (LED)      → LED estado

Power:
VIN  ← +5V
GND  ← GND
3V3  → +3.3V (para lógica)
```

### 6. Alimentación

```
+5V_IN ──┬── C5 (100μF) ──┬── LM1117-3.3V ──┬── C6 (100μF) ── +3.3V
         │               │                  │
         │               │                  └── C7 (100nF) ── +3.3V
         │               │
         └───────────────┴──────────────────────────────────── GND
```

### 7. Circuito de Reset y Programación

```
                     +3.3V
                       │
                    R12 (10kΩ)
                       │
ESP32 EN ──────────────┤
                       │
SW1 (Reset) ───────────┴── GND

ESP32 GPIO0 ────┬── R13 (10kΩ) ── +3.3V
                │
SW2 (Boot) ─────┴── GND
```

## Conexiones Detalladas

### Pinout ESP32-WROOM-32

| Pin | GPIO | Función | Conexión |
|-----|------|---------|----------|
| 1 | GND | Ground | GND común |
| 2 | 3V3 | Power | +3.3V |
| 3 | EN | Enable | Reset circuit |
| 4 | VP | ADC | NC |
| 5 | VN | ADC | NC |
| 6 | GPIO34 | Input | DATA_IN |
| 7 | GPIO35 | Input | NC |
| 8 | GPIO32 | I/O | NC |
| 9 | GPIO33 | I/O | NC |
| 10 | GPIO25 | DAC | CLOCK_OUT |
| 11 | GPIO26 | I/O | NC |
| 12 | GPIO27 | I/O | NC |
| 13 | GPIO14 | I/O | NC |
| 14 | GPIO12 | I/O | NC |
| 15 | GND | Ground | GND común |
| 16 | GPIO13 | I/O | NC |
| 17 | GPIO9 | Flash | NC |
| 18 | GPIO10 | Flash | NC |
| 19 | GPIO11 | Flash | NC |
| 20 | VCC | Power | +3.3V |
| 21 | GPIO6 | Flash | NC |
| 22 | GPIO7 | Flash | NC |
| 23 | GPIO8 | Flash | NC |
| 24 | GPIO15 | I/O | NC |
| 25 | GPIO2 | I/O | LED_STATUS |
| 26 | GPIO0 | I/O | BOOT |
| 27 | GPIO4 | I/O | NC |
| 28 | GPIO16 | I/O | NC |
| 29 | GPIO17 | I/O | NC |
| 30 | GPIO5 | I/O | NC |
| 31 | GPIO18 | I/O | NC |
| 32 | GPIO19 | I/O | NC |
| 33 | GND | Ground | GND común |
| 34 | GPIO21 | I/O | I2C_SDA |
| 35 | GPIO3 | UART | RX |
| 36 | GPIO1 | UART | TX |
| 37 | GPIO22 | I/O | I2C_SCL |
| 38 | GPIO23 | I/O | NC |

## Consideraciones de Diseño

### 1. Blindaje EMI
- Plano de tierra continuo
- Filtros de alimentación
- Separación física de circuitos analógicos y digitales

### 2. Layout PCB
- Antena en una esquina de la PCB
- Componentes críticos cerca del ESP32
- Trazas diferencias balanceadas
- Vías de costura en el plano de tierra

### 3. Montaje
- Componentes SMD preferiblemente
- Antena: bobinado externo o PCB spiral
- Conectores para programación
- Indicadores LED visibles

### 4. Testing Points
- TP1: RF_OUT (señal de antena)
- TP2: AMP_OUT (salida amplificador)
- TP3: FILT_OUT (señal filtrada)
- TP4: DATA_OUT (señal digital)
- TP5: CLOCK_OUT (clock de activación)
- TP6: +3.3V
- TP7: GND

## Notas de Implementación

1. **Ajuste de la antena**: El trimmer C2 debe ajustarse para resonancia exacta en 134.2kHz
2. **Ganancia del amplificador**: Puede necesitar ajuste según el tamaño de la antena
3. **Umbrales del comparador**: Ajustar en software para optimizar detección
4. **Clock de activación**: Debe ser estable y preciso para activar los tags FDX-B
5. **Filtros**: Críticos para eliminar ruido y señales espurias

## Herramientas de Medición Requeridas

- Osciloscopio (min 10MHz BW)
- Generador de funciones
- Multímetro digital
- Analizador de espectro (opcional)
- Tags FDX-B de prueba
