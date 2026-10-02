# Monitor de frecuencia cardíaca y SpO₂ con ESP32 y MAX30102

Proyecto de un monitor de frecuencia cardíaca y saturación de oxígeno (SpO₂) realizado con un ESP32, un sensor MAX30102 y una pantalla TFT ST7789V3.

## Descripción

El proyecto permite adquirir la señal PPG mediante el sensor MAX30102 y procesarla con el ESP32 para obtener la frecuencia cardíaca en BPM y una estimación de la saturación de oxígeno (SpO₂).

La pantalla TFT muestra los valores de BPM y SpO₂ junto con la señal PPG en tiempo real.

El sistema también detecta cuando no hay un dedo colocado sobre el sensor y permite visualizar la señal procesada mediante el Serial Plotter del Arduino IDE.

## Componentes

- ESP32
- Sensor MAX30102
- Pantalla TFT ST7789V3 de 1.69"
- Cables de conexión
- Cable USB
- Computador para la programación

## Conexiones

### MAX30102 → ESP32

| MAX30102 | ESP32 |
|---|---|
| SDA | GPIO32 |
| SCL | GPIO33 |
| GND | GND |
| VCC | Alimentación del módulo |

### TFT ST7789V3 → ESP32

| TFT | ESP32 |
|---|---|
| CS | GPIO5 |
| DC | GPIO2 |
| RST | GPIO4 |
| SCLK | GPIO18 |
| MOSI | GPIO23 |
| GND | GND |
| VCC | Alimentación de la pantalla |

> La tensión de alimentación puede depender del módulo MAX30102 y de la versión de la pantalla utilizada. Se recomienda verificar las especificaciones de cada módulo antes de realizar la conexión.

## Diagrama de conexiones

A continuación se presenta el diagrama de conexiones utilizado en el montaje del sistema.

![Diagrama de conexiones](./DIAGRAMA%20DE%20CONEXIONES.png)

## Funcionamiento

El MAX30102 utiliza luz roja e infrarroja para obtener las señales ópticas utilizadas por el sistema.

El ESP32 recibe estas señales mediante comunicación I²C y las procesa para obtener la frecuencia cardíaca, estimar el SpO₂ y generar la señal PPG que se muestra en la pantalla.

La señal PPG pasa por diferentes etapas de filtrado para reducir ruido, variaciones de la línea base e interferencias. Además, la gráfica utiliza un ajuste automático de escala para facilitar su visualización.

La pantalla TFT muestra:

- Frecuencia cardíaca en BPM.
- Saturación de oxígeno SpO₂.
- Señal PPG en tiempo real.

Cuando no se detecta un dedo sobre el MAX30102, el sistema muestra un mensaje indicando que se debe colocar el dedo y reinicia las mediciones anteriores.

## Procesamiento de las mediciones

A partir de las señales obtenidas por el MAX30102, el ESP32 realiza tres procesos principales:

### ❤️ Frecuencia cardíaca (BPM)

La señal infrarroja se utiliza para detectar cada latido. A partir del tiempo entre latidos se calcula la frecuencia cardíaca y se promedian varias mediciones para obtener un valor más estable.

### 🩸 Saturación de oxígeno (SpO₂)

Para estimar el SpO₂ se utilizan conjuntamente las señales roja e infrarroja. Las muestras se almacenan y procesan en bloques, descartando mediciones que no sean válidas y estabilizando el resultado antes de mostrarlo.

### 📈 Señal PPG

La señal infrarroja también se procesa para obtener la forma de onda PPG. Antes de visualizarla se aplican filtros para reducir ruido, eliminar variaciones de la línea base y disminuir interferencias.

La señal resultante se muestra en tiempo real en la pantalla TFT y también puede observarse mediante el Serial Plotter del Arduino IDE.

## Serial Plotter

El proyecto permite visualizar la señal PPG desde el Serial Plotter del Arduino IDE.

Para utilizarlo:

1. Conectar el ESP32 al computador.
2. Cargar el programa.
3. Abrir el Serial Plotter.
4. Seleccionar una velocidad de 115200 baudios.
5. Colocar el dedo sobre el MAX30102.

La señal PPG procesada comenzará a mostrarse en tiempo real.

## Librerías

Para el funcionamiento del proyecto se utilizan librerías para:

- Comunicación I²C.
- Comunicación SPI.
- Control de la pantalla ST7789.
- Manejo del sensor MAX30102.
- Detección de frecuencia cardíaca.
- Cálculo de SpO₂.

Las librerías correspondientes deben estar instaladas en el Arduino IDE antes de compilar el proyecto.

## Código

El código fuente completo se encuentra disponible en la sección **Code** de este repositorio.

En él se encuentra la configuración del MAX30102 y de la pantalla TFT, así como el procesamiento de la señal PPG, cálculo de BPM, estimación de SpO₂ y visualización de los datos.

## Archivos

- Archivo `.ino`: código utilizado para el funcionamiento del sistema.
- `DIAGRAMA DE CONEXIONES.png`: diagrama de conexiones del montaje.
- `README.md`: documentación general del proyecto.

## Posibles mejoras

- Almacenamiento de las mediciones.
- Comunicación mediante Bluetooth.
- Envío de datos mediante Wi-Fi.
- Interfaz web para visualizar las mediciones.
- Registro histórico de BPM y SpO₂.
- Indicador de calidad de la señal.

## Nota

Este proyecto fue desarrollado con fines académicos y experimentales para estudiar la adquisición y procesamiento de señales PPG mediante el MAX30102.

Los valores de BPM y SpO₂ obtenidos son estimaciones y el sistema no corresponde a un dispositivo médico certificado. No debe utilizarse para diagnóstico o toma de decisiones médicas.
