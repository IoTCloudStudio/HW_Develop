# Lector RFID FDX para Caravanas de Ganado

## Descripción
Circuito electrónico completo para lectura de caravanas RFID FDX-B (ISO 11784/11785) de 134.2kHz utilizado en identificación de ganado.

## Características Técnicas
- **Frecuencia**: 134.2kHz (FDX-B)
- **Protocolo**: ISO 11784/11785
- **Alcance**: 10-15cm (típico para caravanas)
- **Microcontrolador**: ESP32
- **Conectividad**: WiFi/Bluetooth
- **Alimentación**: 5V/3.3V
- **Display**: LCD 16x2 opcional + interfaz web

## Especificaciones del Sistema

### Hardware Principal
1. **Antena resonante** optimizada para 134.2kHz
2. **Amplificador de señal** con LM358
3. **Acondicionador de señal** con filtros
4. **ESP32** para procesamiento y conectividad
5. **Circuito de clock** para activación de tags

### Componentes Principales
- ESP32-WROOM-32 (microcontrolador principal)
- LM358N (amplificador operacional dual)
- Bobina de antena personalizada
- Capacitor de sintonía variable
- Resistencias de precisión
- Cristal de 32.768kHz para clock del sistema
- Regulador de voltaje 3.3V

## Estructura del Proyecto

```
Lector_RFID_FDX_Caravana/
├── PCB/                    # Archivos de diseño PCB
├── Firmware/               # Código fuente del microcontrolador
├── Documentacion/          # Esquemas, cálculos y especificaciones
└── README.md
```

## Principio de Funcionamiento

### FDX-B Protocol
El protocolo FDX-B utiliza:
- **Modulación**: Biphase (Manchester diferencial)
- **Estructura**: 128 bits total
  - 11 bits de header (10 ceros + 1 uno)
  - 38 bits ID del animal
  - 10 bits código país
  - 1 bit aplicación
  - 8 bits de control
  - Resto bits adicionales/reserva

### Características de la Señal
- **Bit rate**: 134.2kHz / 32 = ~4.19 kbps
- **Duración bit**: ~240μs
- **Pulso corto**: ~120μs (representa 0 lógico)
- **Pulso largo**: ~240μs (representa 1 lógico)

## Notas de Diseño
- Diseño optimizado para lectura confiable de caravanas de ganado
- Antena de mayor tamaño para aumentar el rango de lectura
- Filtros mejorados para reducir interferencias
- Interfaz web para monitoreo remoto
- Almacenamiento local de lecturas

## Estado del Desarrollo
- [x] Análisis de requerimientos
- [ ] Diseño del esquemático
- [ ] Diseño de PCB
- [ ] Desarrollo de firmware
- [ ] Pruebas de laboratorio
- [ ] Pruebas de campo

## Autor
Hardware Development - IoT Cloud Studio
