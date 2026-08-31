# Lector RFID FDX para Caravanas - Documentación de Diseño KiCad

## Descripción General
Este proyecto de KiCad implementa un lector RFID FDX-B completo para la identificación de caravanas de ganado operando a 134.2kHz según el estándar ISO 11784/11785.

## Archivos del Proyecto

### Archivos Principales
- **RFID_FDX_Reader.kicad_pro** - Archivo de proyecto principal
- **RFID_FDX_Reader.kicad_sch** - Esquemático del circuito
- **RFID_FDX_Reader.kicad_pcb** - Diseño del PCB

### Componentes Principales del Circuito

#### 1. Microcontrolador ESP32-WROOM-32 (U1)
- **Función**: Control principal, procesamiento de datos FDX-B y conectividad WiFi/Bluetooth
- **Alimentación**: 3.3V
- **Pines clave**:
  - GPIO34: Entrada de datos demodulados (DATA_IN)
  - GPIO25: Salida de clock 134.2kHz (CLOCK_OUT)  
  - GPIO21/22: I2C para display opcional
  - GPIO2: LED de estado

#### 2. Amplificador Operacional LM358 (U2)
- **Función**: Amplificación de señal de antena
- **Configuración**: Amplificador no inversor con ganancia de 100x
- **Alimentación**: Dual rail (+3.3V, GND)
- **Ancho de banda**: 1kHz - 1MHz

#### 3. Circuito de Antena Resonante
- **L1**: Bobina de antena 120µH
- **C1**: Capacitor principal de sintonía 100nF
- **C2**: Capacitor de ajuste fino 22nF (trimmer)
- **Frecuencia de resonancia**: 134.2kHz ± 1%

#### 4. Filtros y Acondicionamiento
- **R3**: Resistencia de filtro HPF 1.6kΩ
- Filtros pasivos para eliminar ruido y harmonics

### Especificaciones del PCB

#### Características Físicas
- **Dimensiones**: 150mm x 100mm (tamaño A)
- **Capas**: 2 capas (superior e inferior)
- **Grosor**: 1.6mm (estándar)
- **Material**: FR4 con εr = 4.5

#### Consideraciones de Diseño
1. **Plano de tierra**: Zona de cobre en capa inferior para minimizar EMI
2. **Separación de señales**: Trazas analógicas separadas de digitales
3. **Impedancia**: Trazas de 50Ω para señales RF
4. **Clearance**: 0.2mm mínimo entre trazas

#### Distribución de Componentes
- **Zona de RF** (izquierda): Antena, capacitores de sintonía
- **Zona analógica** (centro): Amplificador LM358, filtros
- **Zona digital** (derecha): ESP32, conectores, LEDs

### Conexiones Críticas

#### Circuito de Antena
```
L1 (120µH) → C1 (100nF) || C2 (22nF) → GND
         ↓
      Entrada amplificador (Pin 3, U2)
```

#### Amplificador
```
Entrada antena → Pin 3 (U2) [No invertido]
Pin 2 (U2) ← R3 (1.6kΩ) ← Salida Pin 1 (U2)
Pin 1 (U2) → GPIO34 (ESP32) [Señal amplificada]
```

#### Alimentación
```
+3.3V → ESP32 (Pin 2), LM358 (Pin 8)
GND → ESP32 (Pin 1,15,38,39), LM358 (Pin 4), Capacitores
```

### Ajustes y Calibración

#### Sintonización de Antena
1. **Paso 1**: Conectar L1 con valor calculado (120µH)
2. **Paso 2**: Ajustar C2 (trimmer) para resonancia exacta en 134.2kHz
3. **Paso 3**: Verificar factor Q > 50 con analizador de impedancia
4. **Paso 4**: Optimizar alcance de lectura (objetivo: 10-15cm)

#### Ganancia del Amplificador
- **Valor actual**: 100x (40dB)
- **Ajuste**: Modificar valor de resistencia de realimentación
- **Rango recomendado**: 50x - 200x según aplicación

### Fabricación

#### Requisitos del PCB
- **Acabado superficial**: HASL o ENIG
- **Máscara de soldadura**: Verde estándar
- **Serigrafía**: Blanca en capa superior
- **Vías**: 0.8mm diámetro, 0.4mm perforación

#### Lista de Componentes para Soldadura
- Todos los componentes son THT (Through-Hole Technology)
- ESP32 en formato SMD (montaje superficial)
- Requiere soldadura de precisión para ESP32
- Componentes pasivos estándar 1/4W

### Testing y Validación

#### Puntos de Test Recomendados
1. **TP1**: Salida de antena (antes del amplificador)
2. **TP2**: Salida del amplificador (entrada ESP32)
3. **TP3**: Señal de clock 134.2kHz
4. **TP4**: Alimentación +3.3V
5. **TP5**: Tierra analógica

#### Herramientas de Medición
- Osciloscopio (mín. 10MHz BW)
- Analizador de espectro
- Generador de funciones
- Multímetro de precisión
- Tags FDX-B de referencia

### Notas de Diseño

#### Consideraciones EMC
- Filtros de alimentación incluidos
- Plano de tierra continuo
- Blindaje opcional para antena
- Separación adecuada entre circuitos

#### Optimizaciones Futuras
1. **Versión 2.0**: Antena PCB integrada
2. **Mejoras**: Amplificador diferencial
3. **Opciones**: Display LCD integrado
4. **Conectividad**: Puerto Ethernet adicional

## Contacto
**Desarrollado por**: IoT Cloud Studio  
**Fecha**: Agosto 2025  
**Versión**: 1.0  
**Licencia**: Propietaria
