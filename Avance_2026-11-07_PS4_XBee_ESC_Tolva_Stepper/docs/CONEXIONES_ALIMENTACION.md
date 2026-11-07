# Conexiones eléctricas y alimentación

## 1. Vista del sistema

Dos ESP32 clásicas (maestra y esclava), dos XBee PRO S1 con adaptadores HW-186, dos ESC para brushless, un servo de tolva y un motor 28BYJ-48 **5 V** con placa ULN2003.

### ESP32 MAESTRA

| Función | GPIO / vínculo |
|---|---|
| Mando PS4 | Bluetooth, a través de Bluepad32 (sin cable) |
| UART2 RX hacia XBee | GPIO16 |
| UART2 TX hacia XBee | GPIO17 |
| XBee HW-186 alimentación | 5 V de alimentación **del adaptador compatible**, según montaje validado |
| XBee HW-186 tierra | GND de ESP32 maestra |

### ESP32 ESCLAVA

| Dispositivo | Función en la esclava | Conexión |
|---|---|---|
| XBee HW-186 | RX2 | GPIO16 |
| XBee HW-186 | TX2 | GPIO17 |
| ESC izquierdo | Señal PWM | GPIO13 |
| ESC derecho | Señal PWM | GPIO18 |
| Servo de tolva | Señal PWM | GPIO23 |
| ULN2003 IN1 | Fase 1 | GPIO25 |
| ULN2003 IN2 | Fase 2 | GPIO26 |
| ULN2003 IN3 | Fase 3 | GPIO27 |
| ULN2003 IN4 | Fase 4 | GPIO32 |
| Todos los módulos | Referencia de señal | GND común de la ESP32 esclava |

**Importante sobre el HW-186:** durante la prueba se confirmó el cableado ESP32 **GPIO17 → borne marcado TX** del adaptador, y ESP32 **GPIO16 → borne marcado RX**; el propio adaptador lleva esas señales al DIN/DOUT del XBee conforme a su diseño. Es un hallazgo específico de los adaptadores usados: **no extrapolar la etiqueta TX/RX a otros adaptadores** sin consultar su placa o pinout. En un XBee desnudo la UART de transmisión de la ESP32 debe llegar a **DIN** y la recepción salir de **DOUT**. Verifica además los niveles lógicos soportados por el adaptador; el XBee como módulo no debe alimentarse directamente a 5 V si su placa no incorpora regulación.

## 2. ULN2003 + 28BYJ-48 de 5 V

```text
ESP32 ESCLAVA                 ULN2003                      MOTOR 28BYJ-48
GPIO25 --------------------> IN1                Conector blanco de 5 hilos
GPIO26 --------------------> IN2                enchufado a la ULN2003
GPIO27 --------------------> IN3
GPIO32 --------------------> IN4
GND ----------------------- GND o -
5 V regulados EXTERNOS ---- VCC o +   (motor de 5 V, no de 12 V)
```

**Sin carga y sin carrete** para los ensayos iniciales. El sketch declara las fases en el orden `AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4`; **no cambiarlo** si funcionó en el módulo probado.

La alimentación de la ULN2003 debe suministrar la corriente de las bobinas del paso a paso. **Nunca** alimentar el motor desde los GPIO de ESP32. El regulador/fuente debe tener corriente suficiente para evitar reinicios por caída de tensión.

## 3. Servo de tolva

```text
GPIO23 ESP32 esclava ------- cable de SEÑAL servo
GND ESP32 esclava ---------- GND de fuente del servo
Fuente regulada servo + ---- V+ del servo (ver etiqueta del servo)
Fuente regulada servo GND -- GND del servo
```

Durante las pruebas, alimentarlo desde el BEC de un ESC produjo ruido y comportamiento con retraso. Con una fuente externa la respuesta mejoró; **no unir los positivos** de fuentes separadas ni los cables rojos de dos BEC entre sí. El código usa `TOLVA_CERRADA=20` y `TOLVA_ABIERTA=90` como puntos iniciales, que se deben comprobar mecánicamente.

## 4. ESC y motores brushless

- Señal izquierda GPIO13; señal derecha GPIO18; 50 Hz y pulsos entre **1000 y 1300 µs** en el código de pruebas.
- Alimentación de potencia de ESC desde su batería/sistema apropiado; **no** desde 5 V de la ESP32.
- GND de señal ESC con GND de referencia del controlador. Si hay fuentes aisladas/compartidas, diseñar y verificar una referencia eléctrica apropiada; no unir indiscriminadamente masas de alta potencia sin analizar la distribución de corriente.
- Para **compilar, subir, probar XBee y probar el paso a paso**, quitar las hélices y desconectar la potencia de los ESC.

## 5. Parámetros XBee PRO S1 utilizados

| Parámetro | Maestro | Esclavo |
|---|---|---|
| Baudrate | 115200 | 115200 |
| Canal CH | 0x17 | 0x17 |
| PAN ID | 0x1234 | 0x1234 |
| API AP | 0 (transparente) | 0 (transparente) |
| RO | 0 | 0 |
| CE | 1 | 0 |
| MY | 0x10 | 0x20 |
| DL | 0xFFFF (broadcast) | 0x10 |

Estos son los parámetros que se utilizaron durante las pruebas exitosas. **No cambiarlos solo por cambiar de computadora**: XCTU configura los módulos de radio, no el firmware del ESP32. El enlace radio funcionó con esta configuración; antes, pasar de broadcast a unicast había introducido retraso perceptible en las pruebas particulares.

## 6. Pendientes antes de instalar la sonda

El 28BYJ-48 tiene reductora y puede mover un carrete pequeño, pero su capacidad útil depende del radio del carrete, masa de la sonda, fricción y corriente disponible. El programa deja de alimentar las bobinas cuando el motor se detiene, así que **no mantiene una sonda suspendida**. Se necesitan un freno/retención mecánica apropiados, final de carrera superior y un procedimiento de *homing*; usar una guía de cable resistente al agua y al desgaste. No depender únicamente de los límites ±1024 del software para la seguridad física.
