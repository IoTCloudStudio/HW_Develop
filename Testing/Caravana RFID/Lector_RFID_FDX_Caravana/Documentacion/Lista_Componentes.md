# Lista de Componentes - Lector RFID FDX Caravanas

## Componentes Principales

### Semiconductores
| Ref | Componente | Valor/Modelo | Cantidad | Precio Est. | Proveedor |
|-----|------------|--------------|----------|-------------|-----------|
| U1 | ESP32-WROOM-32 | ESP32-WROOM-32D | 1 | $8.00 | Mouser/DigiKey |
| U2 | LM358N | LM358N | 1 | $0.50 | Mouser/DigiKey |
| U3 | LM1117-3.3V | LM1117T-3.3 | 1 | $1.00 | Mouser/DigiKey |
| D1,D2 | Diodo | 1N4148 | 2 | $0.20 | Mouser/DigiKey |
| LED1 | LED Verde | 3mm Verde | 1 | $0.15 | Local |
| LED2 | LED Rojo | 3mm Rojo | 1 | $0.15 | Local |

### Resistencias (1/4W, 5%)
| Ref | Valor | Cantidad | Descripción |
|-----|-------|----------|-------------|
| R1,R2 | 10kΩ | 2 | Pull-up GPIO |
| R3 | 1.6kΩ | 1 | Filtro HPF |
| R4 | 800Ω | 1 | Filtro LPF |
| R5,R6 | 100kΩ | 2 | Ganancia amplificador |
| R7 | 1kΩ | 1 | Referencia comparador |
| R8,R9,R10 | 10kΩ | 3 | Divisor de tensión |
| R11 | 220Ω | 1 | Limitador clock |
| R12,R13 | 10kΩ | 2 | Pull-up botones |
| R14,R15 | 330Ω | 2 | Limitador LEDs |

### Capacitores
| Ref | Valor | Voltaje | Tipo | Cantidad | Descripción |
|-----|-------|---------|------|----------|-------------|
| C1 | 100nF | 50V | Cerámico | 1 | Capacitor principal antena |
| C2 | 22nF | 50V | Trimmer | 1 | Ajuste fino resonancia |
| C3,C4 | 1nF | 50V | Cerámico | 2 | Filtros RF |
| C5,C6 | 100μF | 16V | Electrolítico | 2 | Filtros alimentación |
| C7,C8 | 100nF | 16V | Cerámico | 2 | Desacoplo alimentación |
| C9,C10 | 22pF | 50V | Cerámico | 2 | Cristal RTC |

### Inductores
| Ref | Valor | Corriente | Tipo | Cantidad | Descripción |
|-----|-------|-----------|------|----------|-------------|
| L1 | 120μH | 1A | Bobina aire | 1 | Antena principal |
| L2 | 10μH | 500mA | SMD | 1 | Filtro alimentación |

### Conectores y Mecánicos
| Ref | Componente | Modelo | Cantidad | Descripción |
|-----|------------|--------|----------|-------------|
| J1 | Conector USB | Micro USB | 1 | Programación |
| J2 | Conector alimentación | Jack 2.1mm | 1 | Entrada 5V |
| J3 | Conector I2C | Header 2x2 | 1 | Display LCD |
| SW1 | Pulsador | 6x6mm | 1 | Reset |
| SW2 | Pulsador | 6x6mm | 1 | Boot/Lectura |
| X1 | Cristal | 32.768kHz | 1 | RTC (opcional) |

### Display (Opcional)
| Ref | Componente | Modelo | Cantidad | Precio Est. |
|-----|------------|--------|----------|-------------|
| LCD1 | Display LCD | 16x2 I2C | 1 | $5.00 |

## Herramientas Requeridas

### Para Construcción
- Soldador 40W con punta fina
- Estaño 0.6mm (60/40 o libre de plomo)
- Flux para soldadura
- Desoldador o malla desoldadora
- Multímetro digital
- Alicates de precisión
- Destornilladores de precisión

### Para Pruebas
- Osciloscopio (min 10MHz BW)
- Generador de funciones
- Fuente de alimentación 5V/1A
- Tags FDX-B de prueba
- Analizador de espectro (opcional)

## Bobina de Antena - Especificaciones

### Construcción Manual
- **Diámetro interno**: 80mm
- **Wire gauge**: AWG 26 (0.4mm)
- **Número de vueltas**: 18-20 vueltas
- **Inductancia objetivo**: 120μH ± 10%
- **Resistencia DC**: <2Ω
- **Factor Q**: >50 @ 134.2kHz

### Materiales para Bobina
- Alambre esmaltado AWG 26: 2 metros
- Tubo PVC Ø80mm: 10cm (como molde)
- Cinta aislante
- Barniz aislante (opcional)

### Procedimiento de Bobinado
1. Marcar el tubo PVC cada 5mm
2. Bobinar 18 vueltas uniformemente espaciadas
3. Fijar extremos con cinta
4. Medir inductancia con LCR meter
5. Ajustar número de vueltas si es necesario
6. Aplicar barniz protector

## PCB Especificaciones

### Características Físicas
- **Tamaño**: 80mm x 60mm
- **Grosor**: 1.6mm
- **Capas**: 4 capas (Signal/GND/VCC/Signal)
- **Material**: FR4 estándar
- **Acabado**: HASL o ENIG
- **Color**: Verde (estándar)

### Especificaciones Eléctricas
- **Impedancia**: 50Ω ± 10%
- **Ancho de pista mínimo**: 0.2mm
- **Vía mínimo**: 0.3mm
- **Espaciado mínimo**: 0.15mm
- **Grosor de cobre**: 35μm (1oz)

## Costos Estimados

### Componentes Electrónicos
| Categoría | Costo Unitario | Costo 10 unidades |
|-----------|----------------|-------------------|
| Semiconductores | $10.00 | $90.00 |
| Pasivos | $3.00 | $25.00 |
| Conectores | $2.00 | $18.00 |
| Mecánicos | $1.50 | $12.00 |
| **TOTAL COMPONENTES** | **$16.50** | **$145.00** |

### PCB y Fabricación
| Item | Costo Unitario | Costo 10 unidades |
|------|----------------|-------------------|
| PCB (4 capas) | $8.00 | $35.00 |
| Stencil SMD | $15.00 | $15.00 |
| Ensamblaje | $5.00 | $40.00 |
| **TOTAL FABRICACIÓN** | **$28.00** | **$90.00** |

### Costo Total por Unidad
- **Prototipo**: $44.50
- **Producción (10 unidades)**: $23.50
- **Producción (100 unidades)**: $18.00 (estimado)

## Proveedores Recomendados

### Componentes Electrónicos
- **Mouser Electronics**: Semiconductores, precisión
- **DigiKey**: Stock amplio, entrega rápida
- **LCSC**: Componentes asiáticos, económicos
- **Local (Buenos Aires)**: Electrónica Patito, Gandhi

### PCB Fabricación
- **JLCPCB**: Económico, buena calidad
- **PCBWay**: Profesional, opciones avanzadas
- **Local**: Asembli, ControlTech

### Encapsulado/Caja
- **Hammond Manufacturing**: Cajas plásticas
- **Takachi**: Cajas profesionales
- **Impresión 3D**: Diseño personalizado

## Notas de Compra

1. **Componentes críticos**: Ordenar 20% extra de componentes SMD pequeños
2. **Bobina**: Considerar compra de inductores comerciales si disponibles
3. **PCB**: Ordenar al menos 5 unidades para pruebas
4. **Herramientas**: Verificar disponibilidad de osciloscopio antes del montaje
5. **Tags de prueba**: Conseguir tags FDX-B reales para validación

## Tiempo de Desarrollo Estimado

- **Diseño PCB**: 2 semanas
- **Fabricación PCB**: 1-2 semanas
- **Ensamblaje**: 1 día por prototipo
- **Pruebas y depuración**: 1-2 semanas
- **Validación final**: 1 semana
- **TOTAL**: 6-8 semanas

## Lista de Verificación Pre-Pedido

- [ ] Verificar disponibilidad de todos los componentes
- [ ] Confirmar especificaciones de la bobina de antena
- [ ] Validar pinout del ESP32-WROOM-32
- [ ] Revisar datasheet del LM358 para configuración
- [ ] Confirmar especificaciones del regulador LM1117
- [ ] Verificar compatibilidad del display LCD I2C
- [ ] Reservar tiempo para pruebas con tags reales
