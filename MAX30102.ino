#include <Wire.h>
#include <SPI.h>
#include <math.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// =====================================================
//                    TFT ST7789V3
// =====================================================

#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_SCLK  18
#define TFT_MOSI  23

Adafruit_ST7789 tft(
  TFT_CS,
  TFT_DC,
  TFT_RST
);

// =====================================================
//                     MAX30102
// =====================================================

#define MAX_SDA   32
#define MAX_SCL   33

MAX30105 sensor;

// =====================================================
//                      COLORES
// =====================================================

#define NEGRO    0x0000
#define VERDE    0x07E0
#define BLANCO   0xFFFF
#define ROJO     0xF800
#define GRIS     0x4208
#define CELESTE  0x07FF

// =====================================================
//                     PANTALLA
// =====================================================

#define ANCHO       280
#define ALTO        240

#define Y_SUPERIOR   92
#define Y_INFERIOR  225
#define Y_CENTRO    160

int xActual = 0;
int yAnterior = Y_CENTRO;

// =====================================================
//                 MUESTREO MAX30102
// =====================================================

const float FS = 400.0;

#define MUESTRAS_PIXEL 4

float sumaGrafica = 0;
int contadorGrafica = 0;

// =====================================================
//                 SERIAL PLOTTER
// =====================================================

#define MUESTRAS_PLOTTER 16

int contadorPlotter = 0;

// =====================================================
//                 DETECCION DE DEDO
// =====================================================

#define UMBRAL_DEDO 15000

bool dedoPresente = false;
bool mensajeDedoMostrado = false;

// =====================================================
//              FILTRO MEDIANA DE 3
// =====================================================

float muestraM1 = 0;
float muestraM2 = 0;

float mediana3(
  float a,
  float b,
  float c
)
{
  if (a > b)
  {
    float t = a;
    a = b;
    b = t;
  }

  if (b > c)
  {
    float t = b;
    b = c;
    c = t;
  }

  if (a > b)
  {
    float t = a;
    a = b;
    b = t;
  }

  return b;
}

// =====================================================
//                  LINEA BASE
// =====================================================

float lineaBase = 0;

float alphaBase = 0;

// =====================================================
//                  NOTCH 60 Hz
// =====================================================

float notchB0 = 0;
float notchB1 = 0;
float notchB2 = 0;

float notchA1 = 0;
float notchA2 = 0;

float notchX1 = 0;
float notchX2 = 0;

float notchY1 = 0;
float notchY2 = 0;

// =====================================================
//                 PASA BAJAS
// =====================================================

float alphaLP = 0;

float lp1 = 0;
float lp2 = 0;

// =====================================================
//                 AUTOESCALA ESTABLE
// =====================================================

float amplitudReferencia = 1000.0;

// =====================================================
//                       BPM
// =====================================================

long ultimoLatido = 0;

float bpmInstantaneo = 0;

int bpmPromedio = 0;

#define NUM_BPM 4

byte historialBPM[NUM_BPM];

byte indiceBPM = 0;
byte cantidadBPM = 0;

// =====================================================
//                       SpO2
// =====================================================

#define SPO2_BUFFER 100

uint32_t irRing[SPO2_BUFFER];
uint32_t redRing[SPO2_BUFFER];

uint32_t irOrdenado[SPO2_BUFFER];
uint32_t redOrdenado[SPO2_BUFFER];

int posicionRing = 0;
int cantidadRing = 0;

int muestrasNuevasSpO2 = 0;

int decimadorSpO2 = 0;

int32_t spo2Calculado = 0;
int8_t spo2Valido = 0;

int32_t hrAlgoritmo = 0;
int8_t hrValido = 0;

// =====================================================
//             ESTABILIZACION SpO2
// =====================================================

#define NUM_SPO2 5

int historialSpO2[NUM_SPO2];

int indiceSpO2 = 0;
int cantidadSpO2 = 0;

int spo2Mostrar = 0;

// =====================================================
//             TEMPORIZADOR SpO2
// =====================================================

#define NUEVAS_MUESTRAS_SPO2 100

// =====================================================
//                  SERIAL PLOTTER
// =====================================================

#define PLOT_MIN -42
#define PLOT_MAX  42

float ultimaSenalPPG = 0;

// =====================================================
//                  TEMPORIZADORES TFT
// =====================================================

unsigned long ultimaActualizacionNumeros = 0;

// =====================================================
//                    PROTOTIPOS
// =====================================================

void configurarFiltros();

float filtrarPPG(float entrada);

void procesarBPM(long valorIR);

void agregarMuestraSpO2(
  uint32_t ir,
  uint32_t red
);

void calcularSpO2();

void dibujarInterfaz();

void actualizarNumeros();

void dibujarPPG(float senal);

void sinDedo();

void reiniciarFiltros();

int obtenerMedianaSpO2();

// =====================================================
//                       SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(700);

  SPI.begin(
    TFT_SCLK,
    -1,
    TFT_MOSI,
    TFT_CS
  );

  tft.init(
    240,
    280
  );

  tft.setRotation(1);

  tft.fillScreen(NEGRO);

  dibujarInterfaz();

  Wire.begin(
    MAX_SDA,
    MAX_SCL
  );

  Wire.setClock(
    400000
  );

  if (
    !sensor.begin(
      Wire,
      I2C_SPEED_FAST
    )
  )
  {
    tft.fillScreen(NEGRO);

    tft.setTextColor(
      ROJO,
      NEGRO
    );

    tft.setTextSize(2);

    tft.setCursor(
      25,
      85
    );

    tft.print(
      "MAX30102"
    );

    tft.setCursor(
      25,
      115
    );

    tft.print(
      "NO ENCONTRADO"
    );

    Serial.println(
      "ERROR: MAX30102 NO ENCONTRADO"
    );

    while (1);
  }

  byte brilloLED = 0x1F;

  byte promedioMuestras = 1;

  byte modoLED = 2;

  int frecuenciaMuestreo = 400;

  int anchoPulso = 411;

  int rangoADC = 4096;

  sensor.setup(
    brilloLED,
    promedioMuestras,
    modoLED,
    frecuenciaMuestreo,
    anchoPulso,
    rangoADC
  );

  sensor.setPulseAmplitudeRed(
    0x1F
  );

  sensor.setPulseAmplitudeIR(
    0x1F
  );

  sensor.setPulseAmplitudeGreen(
    0
  );

  configurarFiltros();
}

// =====================================================
//                       LOOP
// =====================================================

void loop()
{
  sensor.check();

  while (
    sensor.available()
  )
  {
    uint32_t valorIR =
      sensor.getFIFOIR();

    uint32_t valorRojo =
      sensor.getFIFORed();

    sensor.nextSample();

    dedoPresente =
      valorIR >
      UMBRAL_DEDO;

    if (!dedoPresente)
    {
      sinDedo();

      continue;
    }

    if (mensajeDedoMostrado)
    {
      tft.fillRect(
        0,
        40,
        ANCHO,
        40,
        NEGRO
      );

      mensajeDedoMostrado = false;

      actualizarNumeros();
    }

    // =================================================
    // BPM
    // =================================================

    procesarBPM(
      valorIR
    );

    // =================================================
    // FILTRAR SEÑAL PPG
    // =================================================

    float ppg =
      filtrarPPG(
        (float)valorIR
      );

    // =================================================
    // SERIAL PLOTTER
    // =================================================

    contadorPlotter++;

    if (
      contadorPlotter >=
      MUESTRAS_PLOTTER
    )
    {
      contadorPlotter = 0;

      float gananciaPlotter =
        42.0 /
        amplitudReferencia;

      float senalPlotter =
        ppg *
        gananciaPlotter;

      senalPlotter =
        constrain(
          senalPlotter,
          PLOT_MIN,
          PLOT_MAX
        );

      Serial.print(
        "MIN:"
      );

      Serial.print(
        PLOT_MIN
      );

      Serial.print(
        "\t"
      );

      Serial.print(
        "MAX:"
      );

      Serial.print(
        PLOT_MAX
      );

      Serial.print(
        "\t"
      );

      Serial.print(
        "PPG:"
      );

      Serial.println(
        senalPlotter,
        2
      );
    }

    // =================================================
    // GRAFICA TFT
    // =================================================

    sumaGrafica +=
      ppg;

    contadorGrafica++;

    if (
      contadorGrafica >=
      MUESTRAS_PIXEL
    )
    {
      float promedioGrafica =
        sumaGrafica /
        MUESTRAS_PIXEL;

      sumaGrafica = 0;

      contadorGrafica = 0;

      ultimaSenalPPG =
        promedioGrafica;

      dibujarPPG(
        promedioGrafica
      );
    }

    // =================================================
    // SpO2
    // =================================================

    decimadorSpO2++;

    if (
      decimadorSpO2 >=
      4
    )
    {
      decimadorSpO2 = 0;

      agregarMuestraSpO2(
        valorIR,
        valorRojo
      );
    }

    // =================================================
    // ACTUALIZAR NUMEROS
    // =================================================

    if (
      millis() -
      ultimaActualizacionNumeros >=
      250
    )
    {
      ultimaActualizacionNumeros =
        millis();

      actualizarNumeros();
    }
  }
}

// =====================================================
//               CONFIGURAR FILTROS
// =====================================================

void configurarFiltros()
{
  float dt =
    1.0 /
    FS;

  float fcBase =
    0.45;

  float rcBase =
    1.0 /
    (
      2.0 *
      PI *
      fcBase
    );

  alphaBase =
    dt /
    (
      rcBase +
      dt
    );

  float f0 =
    60.0;

  float Q =
    12.0;

  float w0 =
    2.0 *
    PI *
    f0 /
    FS;

  float alpha =
    sin(w0) /
    (
      2.0 *
      Q
    );

  float a0 =
    1.0 +
    alpha;

  notchB0 =
    1.0 /
    a0;

  notchB1 =
    (
      -2.0 *
      cos(w0)
    ) /
    a0;

  notchB2 =
    1.0 /
    a0;

  notchA1 =
    (
      -2.0 *
      cos(w0)
    ) /
    a0;

  notchA2 =
    (
      1.0 -
      alpha
    ) /
    a0;

  float fcLP =
    8.0;

  float rcLP =
    1.0 /
    (
      2.0 *
      PI *
      fcLP
    );

  alphaLP =
    dt /
    (
      rcLP +
      dt
    );
}

// =====================================================
//                   FILTRAR PPG
// =====================================================

float filtrarPPG(
  float entrada
)
{
  float mediana =
    mediana3(
      muestraM2,
      muestraM1,
      entrada
    );

  muestraM2 =
    muestraM1;

  muestraM1 =
    entrada;

  if (
    lineaBase ==
    0
  )
  {
    lineaBase =
      mediana;
  }

  lineaBase =
    lineaBase +
    alphaBase *
    (
      mediana -
      lineaBase
    );

  float ac =
    mediana -
    lineaBase;

  float notch =
    notchB0 * ac +
    notchB1 * notchX1 +
    notchB2 * notchX2 -
    notchA1 * notchY1 -
    notchA2 * notchY2;

  notchX2 =
    notchX1;

  notchX1 =
    ac;

  notchY2 =
    notchY1;

  notchY1 =
    notch;

  lp1 =
    lp1 +
    alphaLP *
    (
      notch -
      lp1
    );

  lp2 =
    lp2 +
    alphaLP *
    (
      lp1 -
      lp2
    );

  return lp2;
}

// =====================================================
//                    BPM
// =====================================================

void procesarBPM(
  long valorIR
)
{
  if (
    checkForBeat(
      valorIR
    )
  )
  {
    long ahora =
      millis();

    long intervalo =
      ahora -
      ultimoLatido;

    if (
      ultimoLatido == 0
    )
    {
      ultimoLatido =
        ahora;

      return;
    }

    if (
      intervalo <
      450
    )
    {
      return;
    }

    float nuevoBPM =
      60000.0 /
      intervalo;

    if (
      nuevoBPM <
      40
      ||
      nuevoBPM >
      150
    )
    {
      return;
    }

    if (
      cantidadBPM >= 2
    )
    {
      if (
        fabs(
          nuevoBPM -
          bpmPromedio
        ) >
        25
      )
      {
        return;
      }
    }

    ultimoLatido =
      ahora;

    bpmInstantaneo =
      nuevoBPM;

    historialBPM[
      indiceBPM
    ] =
      (byte)nuevoBPM;

    indiceBPM++;

    indiceBPM %=
      NUM_BPM;

    if (
      cantidadBPM <
      NUM_BPM
    )
    {
      cantidadBPM++;
    }

    int suma = 0;

    for (
      int i = 0;
      i < cantidadBPM;
      i++
    )
    {
      suma +=
        historialBPM[i];
    }

    bpmPromedio =
      suma /
      cantidadBPM;
  }
}

// =====================================================
//               BUFFER SpO2
// =====================================================

void agregarMuestraSpO2(
  uint32_t ir,
  uint32_t red
)
{
  irRing[
    posicionRing
  ] =
    ir;

  redRing[
    posicionRing
  ] =
    red;

  posicionRing++;

  if (
    posicionRing >=
    SPO2_BUFFER
  )
  {
    posicionRing = 0;
  }

  if (
    cantidadRing <
    SPO2_BUFFER
  )
  {
    cantidadRing++;
  }

  muestrasNuevasSpO2++;

  if (
    cantidadRing ==
    SPO2_BUFFER
    &&
    muestrasNuevasSpO2 >=
    NUEVAS_MUESTRAS_SPO2
  )
  {
    calcularSpO2();

    muestrasNuevasSpO2 = 0;
  }
}

// =====================================================
//                  CALCULAR SpO2
// =====================================================

void calcularSpO2()
{
  for (
    int i = 0;
    i < SPO2_BUFFER;
    i++
  )
  {
    int indice =
      (
        posicionRing +
        i
      )
      %
      SPO2_BUFFER;

    irOrdenado[i] =
      irRing[indice];

    redOrdenado[i] =
      redRing[indice];
  }

  uint32_t irMin =
    irOrdenado[0];

  uint32_t irMax =
    irOrdenado[0];

  uint32_t redMin =
    redOrdenado[0];

  uint32_t redMax =
    redOrdenado[0];

  for (
    int i = 1;
    i < SPO2_BUFFER;
    i++
  )
  {
    if (
      irOrdenado[i] <
      irMin
    )
    {
      irMin =
        irOrdenado[i];
    }

    if (
      irOrdenado[i] >
      irMax
    )
    {
      irMax =
        irOrdenado[i];
    }

    if (
      redOrdenado[i] <
      redMin
    )
    {
      redMin =
        redOrdenado[i];
    }

    if (
      redOrdenado[i] >
      redMax
    )
    {
      redMax =
        redOrdenado[i];
    }
  }

  if (
    irMax - irMin <
    500
    ||
    redMax - redMin <
    300
  )
  {
    spo2Valido = 0;

    return;
  }

  maxim_heart_rate_and_oxygen_saturation(
    irOrdenado,
    SPO2_BUFFER,
    redOrdenado,
    &spo2Calculado,
    &spo2Valido,
    &hrAlgoritmo,
    &hrValido
  );

  if (
    !spo2Valido
    ||
    spo2Calculado <
    80
    ||
    spo2Calculado >
    100
  )
  {
    spo2Valido = 0;

    return;
  }

  historialSpO2[
    indiceSpO2
  ] =
    spo2Calculado;

  indiceSpO2++;

  indiceSpO2 %=
    NUM_SPO2;

  if (
    cantidadSpO2 <
    NUM_SPO2
  )
  {
    cantidadSpO2++;
  }

  spo2Mostrar =
    obtenerMedianaSpO2();
}

// =====================================================
//             OBTENER MEDIANA SpO2
// =====================================================

int obtenerMedianaSpO2()
{
  int datos[NUM_SPO2];

  for (
    int i = 0;
    i < cantidadSpO2;
    i++
  )
  {
    datos[i] =
      historialSpO2[i];
  }

  for (
    int i = 0;
    i < cantidadSpO2 - 1;
    i++
  )
  {
    for (
      int j = i + 1;
      j < cantidadSpO2;
      j++
    )
    {
      if (
        datos[j] <
        datos[i]
      )
      {
        int temp =
          datos[i];

        datos[i] =
          datos[j];

        datos[j] =
          temp;
      }
    }
  }

  if (
    cantidadSpO2 == 0
  )
  {
    return 0;
  }

  if (
    cantidadSpO2 % 2 == 1
  )
  {
    return datos[
      cantidadSpO2 / 2
    ];
  }

  return (
    datos[
      cantidadSpO2 / 2 - 1
    ]
    +
    datos[
      cantidadSpO2 / 2
    ]
  )
  /
  2;
}

// =====================================================
//                DIBUJAR PPG
// =====================================================

void dibujarPPG(
  float senal
)
{
  float amplitud =
    fabs(
      senal
    );

  if (
    amplitud >
    amplitudReferencia
  )
  {
    amplitudReferencia =
      amplitudReferencia *
      0.80
      +
      amplitud *
      0.20;
  }
  else
  {
    amplitudReferencia =
      amplitudReferencia *
      0.97
      +
      amplitud *
      0.03;
  }

  if (
    amplitudReferencia <
    800
  )
  {
    amplitudReferencia =
      800;
  }

  if (
    amplitudReferencia >
    30000
  )
  {
    amplitudReferencia =
      30000;
  }

  float ganancia =
    42.0 /
    amplitudReferencia;

  int y =
    Y_CENTRO +
    (
      senal *
      ganancia
    );

  y =
    constrain(
      y,
      Y_SUPERIOR,
      Y_INFERIOR
    );

  if (
    xActual >=
    ANCHO
  )
  {
    xActual = 0;

    yAnterior =
      Y_CENTRO;
  }

  tft.drawFastVLine(
    xActual,
    Y_SUPERIOR,
    Y_INFERIOR -
    Y_SUPERIOR,
    NEGRO
  );

  tft.drawPixel(
    xActual,
    Y_CENTRO,
    GRIS
  );

  if (
    xActual >
    0
  )
  {
    tft.drawLine(
      xActual - 1,
      yAnterior,
      xActual,
      y,
      VERDE
    );
  }

  yAnterior =
    y;

  xActual++;
}

// =====================================================
//                DIBUJAR INTERFAZ
// =====================================================

void dibujarInterfaz()
{
  tft.fillScreen(
    NEGRO
  );

  tft.setTextColor(
    VERDE,
    NEGRO
  );

  tft.setTextSize(2);

  tft.setCursor(
    5,
    5
  );

  tft.print(
    "MAX30102"
  );

  tft.setTextSize(1);

  tft.setCursor(
    5,
    29
  );

  tft.print(
    "BPM"
  );

  tft.setCursor(
    165,
    29
  );

  tft.print(
    "SpO2"
  );

  tft.drawFastHLine(
    0,
    Y_CENTRO,
    ANCHO,
    GRIS
  );
}

// =====================================================
//              ACTUALIZAR NUMEROS
// =====================================================

void actualizarNumeros()
{
  tft.fillRect(
    0,
    40,
    110,
    38,
    NEGRO
  );

  tft.fillRect(
    155,
    40,
    125,
    38,
    NEGRO
  );

  tft.setTextColor(
    BLANCO,
    NEGRO
  );

  tft.setTextSize(3);

  tft.setCursor(
    5,
    45
  );

  if (
    bpmPromedio >
    0
  )
  {
    tft.print(
      bpmPromedio
    );
  }
  else
  {
    tft.print(
      "--"
    );
  }

  tft.setCursor(
    160,
    45
  );

  if (
    spo2Valido
    &&
    spo2Mostrar >=
    80
    &&
    spo2Mostrar <=
    100
  )
  {
    tft.print(
      spo2Mostrar
    );

    tft.print(
      "%"
    );
  }
  else
  {
    tft.print(
      "--%"
    );
  }
}

// =====================================================
//                   SIN DEDO
// =====================================================

void sinDedo()
{
  bpmPromedio = 0;

  spo2Valido = 0;

  spo2Mostrar = 0;

  cantidadBPM = 0;

  indiceBPM = 0;

  ultimoLatido = 0;

  cantidadRing = 0;

  posicionRing = 0;

  muestrasNuevasSpO2 = 0;

  decimadorSpO2 = 0;

  cantidadSpO2 = 0;

  indiceSpO2 = 0;

  contadorPlotter = 0;

  for (
    int i = 0;
    i < NUM_SPO2;
    i++
  )
  {
    historialSpO2[i] = 0;
  }

  reiniciarFiltros();

  if (!mensajeDedoMostrado)
  {
    tft.fillRect(
      0,
      40,
      ANCHO,
      40,
      NEGRO
    );

    tft.setTextColor(
      ROJO,
      NEGRO
    );

    tft.setTextSize(2);

    tft.setCursor(
      45,
      50
    );

    tft.print(
      "COLOQUE EL DEDO"
    );

    mensajeDedoMostrado = true;
  }
}

// =====================================================
//               REINICIAR FILTROS
// =====================================================

void reiniciarFiltros()
{
  lineaBase = 0;

  muestraM1 = 0;
  muestraM2 = 0;

  notchX1 = 0;
  notchX2 = 0;

  notchY1 = 0;
  notchY2 = 0;

  lp1 = 0;
  lp2 = 0;

  sumaGrafica = 0;

  contadorGrafica = 0;

  amplitudReferencia =
    1000.0;
}
