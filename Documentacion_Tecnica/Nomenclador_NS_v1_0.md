### Documentación del Sistema de Codificación de Números de Serie para Trazabilidad equipos

#### **Introducción**
Este sistema de codificación de números de serie permite la trazabilidad completa de los equipos, incluyendo la identificación del fabricante, modelo, lote de producción, y las piezas específicas utilizadas. El número de serie contiene información estructurada que facilita su identificación a lo largo del ciclo de vida del producto.

#### **Estructura del Número de Serie**
La estructura del número de serie se basa en un formato alfanumérico compuesto por los siguientes elementos:

```
[Identificador del Fabricante][Identificador del Modelo][Lote de Producción][ID de Piezas]
```

#### **Componentes del Número de Serie**

1. **Identificador del Fabricante (1-4 dígitos)**
   - **Descripción:** Código único que identifica al fabricante o marca en la base de datos. 
   - **Ejemplo:** 
     - `"1"` corresponde a IoT en la base de datos.
   
2. **Lote de Producción (4-6 caracteres)**
   - **Descripción:** Identificador único para el lote de fabricación de la PCB, agrupando PCBs producidas bajo las mismas condiciones.
   - **Ejemplo:** 
     - `"0123"` para un lote específico.

3. **Identificador del Modelo (1-4 dígitos)**
   - **Descripción:** Código único que identifica al modelo específico del producto en la base de datos.
   - **Ejemplo:** 
     - `"8"` corresponde a 3G CONNECT.
     - `"9"` corresponde a 4G CONNECT.
     - `"10"` corresponde a 4G CONNECT 4.

4. **ID de Piezas (5-12 caracteres)**
   - **Descripción:** Identificador numérico que puede ser un número secuencial o un código relacionado con las piezas montadas en la PCB.
   - **Ejemplo:** 
     - `"010002002054"` como identificador de piezas.

#### **Ejemplo de Número de Serie Completo**
```
1-0123-9-010002002054
```
- **1**: Identificador del fabricante, que representa IoT.
- **0123**: Lote de producción.
- **9**: Identificador del modelo, que representa 4G CONNECT.
- **010002002054**: ID de piezas.

#### **Uso del Sistema**
El número de serie debe generarse automáticamente durante el proceso de fabricación de cada PCB. El código generado se imprime en la PCB o se almacena digitalmente para su trazabilidad y control de calidad.

#### **Almacenamiento de Información Adicional**
La información detallada, como la descripción completa del fabricante, modelo y datos adicionales sobre las piezas, se almacenaran en la base de datos vinculada. El número de serie será la clave principal para acceder a esta información.

#### **Conclusión**
Este sistema garantiza que todas las PCBs electrónicas puedan rastrearse con precisión, proporcionando un control efectivo sobre la fabricación, la calidad y las piezas utilizadas.