### Estructura de Tópico MQTT

**Formato:**
```
[TipoTópico]/[Modelo]/[NroSerie]/[TipoMQTT]
```

Donde cada segmento del tópico se codifica de la siguiente manera:

1. **Tipo Tópico**: 1 byte
   - 0: Testing
   - 1: Productivo

2. **Modelo**: 2 bytes
   - Codificación específica para cada tipo de dispositivo (puede ser un código hex o cualquier otra codificación eficiente).

3. **Nro Serie**: 4 bytes
   - Identificación única del fabricante del dispositivo. Podría ser un número secuencial o un UUID comprimido.

4. **Tipo MQTT**: 1 byte
   - 0: Datos (eventos)
   - 1: Envío de comandos (cambios de estados)

### Ejemplo de Codificación

Supongamos que tenemos los siguientes valores:

- Tipo Tópico: 1 (Productivo)
- Modelo: 0x01A2 (Dispositivo específico)
- Nro Serie: 0x12345678
- Tipo MQTT: 0 (Datos)

El tópico MQTT sería:

```
1/01A2/12345678/0
```

#### Desglose del Tópico
- **1**: Indica que es un entorno productivo.
- **01A2**: Código de modelo del dispositivo.
- **12345678**: Número de serie único del dispositivo.
- **0**: Indica que es un mensaje de datos.

### Consideraciones de Implementación

Para implementar esta estructura de tópicos, se pueden seguir los siguientes pasos:

1. **Definir la Codificación para cada Segmento:**
   - Asignar códigos específicos y únicos para cada modelo de dispositivo.
   - Utilizar un formato estándar para los números de serie que permita identificar de manera única cada dispositivo.

2. **Generar y Publicar Tópicos:**
   - Al generar un tópico, concatenar los valores codificados utilizando los delimitadores definidos.
   - Ejemplo en Python:
     ```python
     tipo_topico = 1  # Productivo
     modelo = "01A2"  # Ejemplo de modelo codificado
     nro_serie = "12345678"  # Número de serie del dispositivo
     tipo_mqtt = 0  # Datos

     topico_mqtt = f"{tipo_topico}/{modelo}/{nro_serie}/{tipo_mqtt}"
     print(topico_mqtt)  # Salida: 1/01A2/12345678/0
     ```

3. **Suscripción a Tópicos:**
   - Al suscribirse a tópicos, se puede utilizar el símbolo `+` para suscribirse a múltiples dispositivos o modelos. Por ejemplo:
     - `1/01A2/+/0` se suscribirá a todos los dispositivos del modelo `01A2` que envíen datos.
     - `1/+/+/1` se suscribirá a todos los dispositivos en entorno productivo que envíen comandos.

### Beneficios de la Estructura Propuesta

- **Minimización de Bytes Transmitidos:**
  - La estructura propuesta es compacta y utiliza un número mínimo de bytes para representar toda la información necesaria.
- **Claridad y Organización:**
  - A pesar de la compresión, la estructura del tópico sigue siendo legible y fácil de interpretar.
- **Flexibilidad:**
  - Permite la suscripción a múltiples dispositivos y tipos de mensajes mediante el uso de comodines (`+`).

### Documentación de Referencia

- **Formato de Tópico MQTT:**
  ```
  [TipoTópico]/[Modelo]/[NroSerie]/[TipoMQTT]
  ```
- **Codificación de Segmentos:**
  - `TipoTópico`: 0 = Testing, 1 = Productivo
  - `Modelo`: Código de 2 bytes
  - `NroSerie`: Código de 4 bytes
  - `TipoMQTT`: 0 = Datos, 1 = Comandos

Esta estructura permite optimizar el uso del ancho de banda y facilita la administración y monitoreo de múltiples dispositivos en diferentes entornos y con distintos tipos de mensajes.

### Estructura del Mensaje MQTT

**Formato del mensaje JSON:**
```json
{
    "L": 12345654,
    "T": 125,
    "C": 125,
    "D": 25,
    "V": 100
}
```

Cada campo en el mensaje tiene la siguiente descripción:

1. **Log ID ("L")**
   - **Descripción**: Identificador del log.
   - **Opcional**: No.
   - **Tipo de variable**: Unsigned Long (4 bytes).
   - **Valor min/máx**: 0-4294967295.
   - **Ejemplo**: `"L": 123`.

2. **Timestamp ("T")**
   - **Descripción**: Timestamp en tiempo de época (Epoch time).
   - **Opcional**: No.
   - **Tipo de variable**: Unsigned Long (4 bytes).
   - **Valor min/máx**: 0-4294967295.
   - **Ejemplo**: `"T": 1609459200`.

3. **Código de operación ("C")**
   - **Descripción**: Código de la operación realizada.
   - **Opcional**: No.
   - **Tipo de variable**: Unsigned Char/Byte (1 byte).
   - **Valor min/máx**: 0-255.
   - **Ejemplo**: `"C": 101`.

4. **Dato ("D")**
   - **Descripción**: Valor del dato asociado a la operación.
   - **Opcional**: Sí.
   - **Tipo de variable**: Boolean (1 byte) / Char (1 byte) / Byte (1 byte) / Word (2 bytes) / Unsigned Long (4 bytes) / Float.
   - **Valor min/máx**:
     - Boolean: 0-1.
     - Char: -128 a 127.
     - Byte: 0-255.
     - Word: 0-65535.
     - Unsigned Long: 0-4294967295.
     - Float: -3.4028e+38 a 3.4028e+38.
   - **Ejemplo**: `"D": 25`.

5. **Versión del protocolo ("V")**
   - **Descripción**: Versión del protocolo utilizado.
   - **Opcional**: No.
   - **Tipo de variable**: Unsigned Int (2 bytes).
   - **Valor min/máx**: 100-65535.
   - **Ejemplo**: `"V": 100`.

### Ejemplos de Mensajes

1. **Mensaje completo:**
   ```json
   {"L": 12345654, "T": 1609459200, "C": 101, "D": 25, "V": 100}
   ```
   - **Descripción**:
     - `Log ID`: 12345654
     - `Timestamp`: 1609459200 (01-01-2021 00:00:00 GMT)
     - `Código de operación`: 101 (Temperatura)
     - `Dato`: 25 (Temperatura en °C)
     - `Versión del protocolo`: 100 (V0.1.0)

2. **Mensaje sin datos opcionales ("D"):**
   ```json
   {"L": 12345654, "T": 1640995200, "C": 102, "V": 100}
   ```
   - **Descripción**:
     - `Log ID`: 12345654
     - `Timestamp`: 1640995200 (01-01-2022 00:00:00 GMT)
     - `Código de operación`: 102 (Humedad)
     - `Versión del protocolo`: 100 (V0.1.0)

### Consideraciones Adicionales

- **Optimización del tamaño**: Utilizar tipos de datos compactos y adecuados para minimizar el tamaño del mensaje.
- **Codificación clara**: Asegurarse de que cada campo esté claramente documentado y que los valores sean interpretables.
- **Versionamiento**: Mantener una versión del protocolo para asegurar la compatibilidad y evolución del sistema.

### Descripción de Campos y Valores

- **Log ID (L)**:
  - Identificador único del log, no opcional, tipo Unsigned Long (4 bytes).

- **Timestamp (T)**:
  - Marca temporal en formato epoch, no opcional, tipo Unsigned Long (4 bytes).

- **Código de operación (C)**:
  - Código que identifica la operación realizada, no opcional, tipo Unsigned Char/Byte (1 byte).
  - **Códigos de operación**:
    - 101: Temperatura.
    - 102: Humedad.
    - 103: Presión.

- **Dato (D)**:
  - Valor del dato asociado a la operación, opcional, con diferentes tipos de variables permitidos.
  - Ejemplos:
    - `101`: Temperatura (Char) - 25 (25°C).
    - `102`: Humedad (Unsigned Char) - 50 (50%).

- **Versión del protocolo (V)**:
  - Versión del protocolo en uso, no opcional, tipo Unsigned Int (2 bytes).
  - **Versiones**:
    - V0.1.0 = 100
    - V1.0.0 = 1000
    - V1.2.3 = 1203

### Ejemplo Adicional

Para un mensaje que reporta la humedad con un Log ID de 789, un timestamp de 1640995200, un código de operación de 102, y una versión de protocolo 100:
```json
{"L": 789, "T": 1640995200, "C": 102, "D": 50, "V": 100}
```
- **Descripción**:
  - `Log ID`: 789
  - `Timestamp`: 1640995200 (01-01-2022 00:00:00 GMT)
  - `Código de operación`: 102 (Humedad)
  - `Dato`: 50 (50% de humedad)
  - `Versión del protocolo`: 100 (V0.1.0)

Esta estructura optimiza la transmisión y proporciona una clara documentación para cada campo, asegurando una interpretación precisa y eficiente de los mensajes MQTT.