# Manual de Usuario - Lector RFID FDX para Caravanas

## Introducción

El Lector RFID FDX para Caravanas es un dispositivo diseñado específicamente para la identificación de ganado mediante la lectura de chips RFID FDX-B (ISO 11784/11785) integrados en caravanas de identificación animal.

## Características Principales

### ✅ Funcionalidades
- **Lectura automática** de caravanas FDX-B de 134.2kHz
- **Rango de lectura** de 8-15cm (típico)
- **Display LCD** para visualización inmediata
- **Interfaz web** para monitoreo remoto
- **WiFi integrado** para conectividad
- **Almacenamiento** de últimas lecturas
- **Indicadores LED** de estado

### 📋 Especificaciones Técnicas
- **Protocolo**: FDX-B (ISO 11784/11785)
- **Frecuencia**: 134.2kHz
- **Alimentación**: 5V DC
- **Consumo**: 150mA típico
- **Dimensiones**: 80mm x 60mm x 25mm
- **Peso**: 120g aproximadamente

## Instalación y Configuración

### 🔌 Conexión de Alimentación

1. **Conectar la fuente de 5V** al jack de alimentación
2. **Verificar polaridad**: Centro positivo (+), exterior negativo (-)
3. **LED verde** debe encender indicando alimentación correcta

⚠️ **PRECAUCIÓN**: Usar solo fuente de 5V DC. Voltajes mayores pueden dañar el equipo.

### 📶 Configuración WiFi

#### Primera Configuración
1. **Encender el dispositivo**
2. **Buscar red WiFi**: "RFID_Reader_AP"
3. **Conectarse** con password: "rfid2025"
4. **Abrir navegador** e ir a: http://192.168.4.1
5. **Configurar WiFi local** (opcional)

#### Conectar a Red Local
1. En la interfaz web, ir a **"Configuración"**
2. Seleccionar **"WiFi Settings"**
3. Ingresar **SSID** y **contraseña** de su red
4. **Guardar** y reiniciar dispositivo
5. **Obtener nueva IP** del router local

### 🖥️ Display LCD (Opcional)

Si tiene display LCD conectado:
1. **Verificar conexión I2C** (SDA pin 21, SCL pin 22)
2. **Dirección I2C**: 0x27 (configurable)
3. **Pantalla muestra**:
   - Línea 1: Estado del sistema
   - Línea 2: ID del último tag leído

## Operación del Sistema

### 📖 Lectura de Caravanas

#### Método Automático
1. **Acercar la caravana** al lector (8-15cm)
2. **El sistema detecta automáticamente** el tag
3. **LED rojo parpadea** durante la lectura
4. **LED verde se enciende** cuando detecta el tag
5. **Datos se muestran** en LCD y web

#### Método Manual
1. **Presionar botón** de lectura
2. **Acercar caravana** dentro de 5 segundos
3. **Esperar confirmación** visual/auditiva
4. **Revisar datos** en la interfaz

### 🌐 Interfaz Web

#### Acceso a la Interfaz
1. **Conectar a la red WiFi** del dispositivo
2. **Abrir navegador** en http://192.168.4.1
3. **O usar IP local** si está conectado a su red

#### Funciones Principales

**📊 Panel Principal**
- Estado actual del lector
- Último tag detectado
- Información del animal y país
- Timestamp de la lectura

**🔄 Actualización Automática**
- La página se actualiza cada segundo
- No necesita refrescar manualmente
- Datos en tiempo real

**🗑️ Limpiar Datos**
- Botón "Limpiar" borra la última lectura
- Útil para preparar nueva lectura
- No afecta configuraciones

### 📱 Interpretación de Datos

#### Información del Tag FDX-B
```
ID Animal: 123456789012345
Código País: 484 (México)
Timestamp: 14/08/2025 15:30:25
```

**🔢 ID del Animal**: Número único de 15 dígitos
**🌍 Código del País**: Según estándar ISO 3166
**⏰ Timestamp**: Fecha y hora de la lectura

#### Códigos de País Comunes
- **032**: Argentina
- **076**: Brasil  
- **152**: Chile
- **170**: Colombia
- **484**: México
- **858**: Uruguay
- **724**: España

## Solución de Problemas

### ❌ Problemas Comunes

#### No Lee Caravanas
**Síntomas**: LED rojo permanece encendido, no detecta tags
**Soluciones**:
- ✅ Verificar que la caravana tiene chip FDX-B
- ✅ Acercar más la caravana (máximo 15cm)
- ✅ Verificar orientación de la caravana
- ✅ Probar con otra caravana conocida
- ✅ Reiniciar el dispositivo

#### No Se Conecta a WiFi
**Síntomas**: No aparece la red WiFi o no se conecta
**Soluciones**:
- ✅ Verificar que está en modo AP (primera vez)
- ✅ Revisar password: "rfid2025"
- ✅ Reiniciar dispositivo manteniendo botón presionado
- ✅ Verificar que el router acepta nuevos dispositivos

#### Display LCD No Funciona
**Síntomas**: Pantalla en blanco o caracteres extraños
**Soluciones**:
- ✅ Verificar conexiones I2C (SDA/SCL)
- ✅ Verificar dirección I2C (0x27)
- ✅ Revisar alimentación del LCD
- ✅ Verificar contraste del display

#### Lecturas Inconsistentes
**Síntomas**: Lee a veces sí, a veces no
**Soluciones**:
- ✅ Verificar distancia de lectura (8-15cm)
- ✅ Mantener caravana estable durante lectura
- ✅ Evitar interferencias metálicas cercanas
- ✅ Verificar que la caravana no está dañada

### 🔧 Diagnóstico Avanzado

#### Verificar Componentes
1. **LED de alimentación**: Debe estar verde fijo
2. **LEDs de estado**: 
   - Rojo = procesando/error
   - Verde = lectura exitosa
3. **Botón de reset**: Debe reiniciar el sistema
4. **Antena**: Sin daños físicos visibles

#### Verificar Señales (Técnico)
1. **Clock de 134.2kHz** en pin 25 (GPIO25)
2. **Señal de datos** en pin 34 (GPIO34)
3. **Alimentación 3.3V** estable
4. **I2C funcionando** en pins 21/22

### 📞 Soporte Técnico

#### Información para Soporte
Antes de contactar soporte, tenga lista la siguiente información:
- **Modelo del dispositivo**
- **Versión del firmware**
- **Descripción del problema**
- **Pasos reproducir el error**
- **Tipo de caravanas usadas**

#### Contacto
- **Email**: soporte@iotcloudstudio.com
- **Teléfono**: +54 11 xxxx-xxxx
- **Horario**: Lunes a Viernes 9:00-18:00

## Mantenimiento

### 🧼 Limpieza Regular
- **Limpiar superficie** con paño seco
- **No usar líquidos** directamente sobre el dispositivo
- **Verificar conexiones** periódicamente
- **Mantener libre de polvo** la antena

### 🔄 Actualizaciones
- **Firmware**: Se actualiza vía web OTA
- **Configuración**: Respaldable desde interfaz web
- **Logs**: Disponibles para diagnóstico

### 🔋 Cuidados de Alimentación
- **Usar fuente estabilizada** de 5V
- **Evitar picos de voltaje**
- **Desconectar** durante tormentas eléctricas
- **Verificar cables** de alimentación

## Especificaciones Ambientales

### 🌡️ Condiciones de Operación
- **Temperatura**: -10°C a +60°C
- **Humedad**: 0-85% sin condensación
- **Altitud**: 0-2000 metros
- **Protección**: IP40 (uso interior)

### ⚠️ Limitaciones
- **No sumergible** en agua
- **Evitar golpes** y vibraciones fuertes
- **No exponer** a químicos corrosivos
- **Mantener alejado** de fuentes de calor

## Garantía

### 📜 Términos de Garantía
- **Período**: 12 meses desde la compra
- **Cubre**: Defectos de fabricación
- **No cubre**: Daños por uso indebido
- **Reparación**: En centro de servicio autorizado

### 📋 Registro de Producto
Para activar la garantía:
1. **Completar registro** en www.iotcloudstudio.com
2. **Conservar factura** de compra
3. **Número de serie** ubicado en etiqueta del producto

---

## Apéndices

### 📚 Documentos Relacionados
- Manual Técnico de Instalación
- Esquemas Eléctricos
- Protocolo FDX-B (ISO 11784/11785)
- Código de países ISO 3166

### 🔗 Enlaces Útiles
- Portal de soporte: www.iotcloudstudio.com/soporte
- Actualizaciones: www.iotcloudstudio.com/downloads
- Comunidad: forum.iotcloudstudio.com

---

**© 2025 IoT Cloud Studio - Todos los derechos reservados**
