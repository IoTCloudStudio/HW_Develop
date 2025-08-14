# Lector RFID FDX Caravana - Proyecto KiCad

## Descripción
Este proyecto contiene los archivos de diseño para un lector RFID FDX-B (ISO 11784/11785) capaz de leer caravanas de ganado a 134.2kHz.

## Estructura de Archivos

### Archivos Principales
- `RFID_FDX_Reader.kicad_pro` - Archivo de proyecto principal
- `RFID_FDX_Reader.kicad_sch` - Esquemático del circuito
- `RFID_FDX_Reader.kicad_pcb` - Layout del PCB (a crear)

### Librerías Personalizadas
- `RFID_Custom.kicad_sym` - Símbolos personalizados para el proyecto
- `RFID_Custom.pretty/` - Huellas (footprints) personalizadas

## Características del Diseño

### Especificaciones Técnicas
- **Frecuencia**: 134.2kHz ± 0.1%
- **Protocolo**: FDX-B (ISO 11784/11785)
- **Alcance**: 8-15cm (dependiendo del tamaño de la caravana)
- **Alimentación**: 5V DC, 500mA máximo
- **Interfaz**: WiFi 802.11b/g/n, Punto de acceso integrado

### Componentes Principales
1. **ESP32-WROOM-32D** - Microcontrolador principal con WiFi
2. **LM358** - Amplificador operacional dual para acondicionamiento de señal
3. **LM1117-3.3V** - Regulador de voltaje para ESP32
4. **Antena 134.2kHz** - Bobina resonante de 15 vueltas (~120µH)
5. **Circuito de sintonización** - Capacitores variables para ajuste de frecuencia

### Bloques Funcionales

#### 1. Antena y Circuito Resonante
- Bobina de 15 vueltas, diámetro 20mm
- Capacitor de sintonización variable (220-470pF)
- Resistor de carga 2.2kΩ
- Frecuencia de resonancia: 134.2kHz

#### 2. Amplificación y Filtrado
- Amplificador no inversor (LM358A) con ganancia ~100
- Filtro paso banda centrado en 134.2kHz
- Detector de envolvente con diodo Schottky

#### 3. Procesamiento Digital
- ESP32 con ADC de 12 bits
- Detección de bits mediante análisis temporal
- Decodificación del protocolo FDX-B
- Validación de CRC y paridad

#### 4. Alimentación
- Entrada: 5V DC via conector micro-USB o jack 2.1mm
- Regulación a 3.3V para ESP32
- Protección contra polaridad inversa
- Indicadores LED de estado

#### 5. Interfaces
- WiFi punto de acceso (192.168.4.1)
- Servidor web integrado
- Display LCD I2C opcional (SDA=21, SCL=22)
- LEDs indicadores (Power, Status, Detection)

## Instrucciones de Uso del Proyecto KiCad

### 1. Abrir el Proyecto
```bash
# Desde terminal
cd /home/jmrg/Git/HW_Develop/Productivo/Lector_RFID_FDX_Caravana/KiCad
kicad RFID_FDX_Reader.kicad_pro
```

### 2. Verificar Librerías
1. Ir a `Preferences` > `Manage Symbol Libraries`
2. Agregar la librería `RFID_Custom.kicad_sym` si no está presente
3. Ir a `Preferences` > `Manage Footprint Libraries`
4. Agregar la librería `RFID_Custom.pretty` si no está presente

### 3. Editar el Esquemático
1. Abrir `RFID_FDX_Reader.kicad_sch`
2. Verificar todas las conexiones
3. Ejecutar ERC (Electrical Rules Check)
4. Corregir cualquier error o advertencia

### 4. Crear el Layout PCB
1. Desde el esquemático: `Tools` > `Update PCB from Schematic`
2. Importar todos los componentes al PCB
3. Realizar el ruteo de las pistas
4. Ejecutar DRC (Design Rules Check)

### 5. Generar Archivos de Fabricación
1. `File` > `Plot` para generar Gerbers
2. `File` > `Fabrication Outputs` > `Drill Files`
3. Configurar capas según especificaciones del fabricante

## Consideraciones de Diseño

### Layout PCB
- **Plano de tierra continuo** para minimizar ruido
- **Separación digital/analógico** clara
- **Antena alejada** de componentes digitales
- **Vías de stitching** para unir planos de tierra
- **Decoupling capacitors** cerca de cada IC

### Ruteo Crítico
- Mantener trazas de antena cortas y simétricas
- Usar trazas diferenciales para señales balanceadas
- Evitar loops de corriente en circuitos analógicos
- Mantener trazas de cristal cortas y alejadas de switching

### Consideraciones de EMI/EMC
- Filtros en líneas de alimentación
- Ferrite beads en líneas digitales críticas
- Shielding opcional para sección analógica
- Test points para debugging y calibración

## Pruebas y Calibración

### 1. Verificación de Alimentación
- Verificar 5V y 3.3V en test points
- Medir consumo de corriente
- Verificar ripple en alimentación

### 2. Calibración de Antena
- Medir frecuencia de resonancia con generador de señales
- Ajustar capacitor de sintonización para 134.2kHz
- Verificar factor Q de la antena

### 3. Pruebas de Recepción
- Usar caravanas de prueba conocidas
- Verificar alcance de lectura
- Medir SNR de la señal recibida

### 4. Pruebas de Software
- Verificar conectividad WiFi
- Probar interfaz web
- Validar decodificación FDX-B

## Documentación Adicional

### Archivos de Soporte
- `../Documentacion/Manual_Construccion.md` - Guía detallada de ensamblaje
- `../Documentacion/Manual_Usuario.md` - Instrucciones de operación
- `../Firmware/RFID_FDX_Reader.ino` - Código fuente del firmware

### Referencias Técnicas
- ISO 11784: Radio frequency identification of animals
- ISO 11785: Technical concept for RFID of animals
- ESP32 Datasheet
- LM358 Datasheet

## Licencia
Este proyecto está bajo licencia MIT. Ver archivo LICENSE para detalles.

## Contacto
Para soporte técnico o consultas sobre el proyecto, consultar la documentación adicional o revisar el código fuente incluido.

---
**Versión**: 1.0  
**Fecha**: 2024  
**Autor**: Proyecto HW_Develop  
**Estado**: Listo para fabricación
