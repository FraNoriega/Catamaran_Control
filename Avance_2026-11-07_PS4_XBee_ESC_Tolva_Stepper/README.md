# Avance: catamarán PS4 + XBee + 2 ESC + tolva + sonda paso a paso

**Fecha del avance:** 7 de noviembre de 2026.  
**Repositorio de destino:** https://github.com/FraNoriega/Catamaran_Control  
**Entorno obligatorio:** **VS Code + PlatformIO** para las DOS ESP32.  
**Estado:** la persona que realizó la prueba confirmó que el motor 28BYJ-48 se movió y que la integración de mando/XBee, ESC, tolva y órdenes de motor paso a paso funcionó sin carga. En la captura se observaron `XBEE:OK`, `PS4:OK`, contadores `RX_M` y `RX_P` crecientes y `ERR:0`.

> **Alcance:** esta es una entrega de **código y documentación del avance**. No incluye el árbol completo de la plantilla ESP-IDF + Arduino + Bluepad32 utilizada por la ESP32 maestra, ni los paquetes descargados por PlatformIO. Se indica cómo obtenerlos. Los archivos `.cpp` se reconstruyeron desde la última versión intercambiada y confirmada en el proyecto; no se extrajeron directamente del disco local `D:` del equipo de pruebas. En una nueva computadora deben compilarse y validarse con su propia configuración.

## Arquitectura

```text
Mando DualShock 4 (PS4)
       │ Bluetooth / Bluepad32
       ▼
ESP32 MAESTRA — VS Code / PlatformIO (ESP-IDF + Arduino)
  GPIO17 TX2 / GPIO16 RX2
       │ UART2 nativa ESP-IDF, 115200, 8N1
       ▼
Adaptador HW-186 + XBee PRO S1  ))) radio (((  XBee PRO S1 + HW-186
                                                     │
                                       GPIO16 RX2 / GPIO17 TX2
                                                     ▼
                                     ESP32 ESCLAVA — PlatformIO Arduino
                                     ├─ GPIO13 → señal ESC izquierdo → brushless 1
                                     ├─ GPIO18 → señal ESC derecho   → brushless 2
                                     ├─ GPIO23 → señal servomotor de tolva
                                     └─ ULN2003 (IN1 25, IN2 26, IN3 27, IN4 32)
                                        └─ motor 28BYJ-48, 5 V, sin carga
```

## Controles PS4

| Control | Resultado |
|---|---|
| Joystick izquierdo, eje X | Dirección por reducción diferencial de empuje |
| R2 | Aumenta orden de empuje de los ESC |
| L2 | Reduce orden de empuje; **no** invierte el sentido de giro |
| Mantener **X** (cruz) | Abrir tolva (tras 2 paquetes válidos) |
| Soltar **X** | Cerrar tolva |
| Cruceta **↑** | Girar paso a paso en sentido «subir» |
| Cruceta **↓** | Girar paso a paso en sentido «bajar» |
| Soltar cruceta o pulsar ↑↓ a la vez | Parar paso a paso |
| Pérdida de mando / enlace | Señal de 1000 µs a los ESC, ordenar cierre de tolva y detener paso a paso (requiere alimentación operativa) |

> La etiqueta «SUBIR»/«BAJAR» es lógica; según el montaje del carrete puede que haya que invertir el signo de la velocidad o las conexiones. **No invertir nada hasta comprobar el sentido mecánico real.**

## Firmware entregado

```text
Avance_2026-11-07_PS4_XBee_ESC_Tolva_Stepper/
├── README.md                               ← estás aquí
├── CHANGELOG.md
├── firmware/
│   ├── maestro/
│   │   ├── sketch.cpp                      ← copiar a main/sketch.cpp de la plantilla completa
│   │   └── REQUISITOS_PLANTILLA.md
│   └── esclavo/
│       ├── platformio.ini                  ← proyecto completo del esclavo
│       └── src/
│           └── main.cpp
└── docs/
    ├── INSTALACION_OTRA_COMPUTADORA.md     ← empezar aquí en otra laptop
    ├── CONEXIONES_ALIMENTACION.md
    ├── PROTOCOLO_XBEE.md
    ├── PRUEBAS_Y_SEGURIDAD.md
    ├── HISTORIAL_PROBLEMAS_Y_HALLAZGOS.md
    └── COMO_SUBIR_A_GITHUB.md
```

### Parámetros principales

| Elemento | Valor de esta versión |
|---|---|
| Comunicación XBee | 115200 baudios, 8N1, modo transparente (AP=0) |
| Maestro (UART) | **`uart_write_bytes()` + `uart_wait_tx_done()`** ESP-IDF; **no** `Serial2.write()` |
| Esclavo (UART) | `Serial2.begin(115200, SERIAL_8N1, 16, 17)` |
| Trama de aplicación | **4 bytes** `[FF][ángulo/comando][velocidad][checksum/bit tolva]` |
| Refresco de motores/tolva | Cambio de valor o cada **50 ms** |
| Refresco de cruceta | Cambio de estado o cada **100 ms** |
| Guardián de enlace motores | 400 ms |
| Guardián de órdenes de paso a paso | 250 ms, además del guardián general |
| ESC | 50 Hz; 1000–1300 µs durante el ensayo |
| Tolva | 20° cerrado, 90° abierto (**ajustar a la mecánica**) |
| Paso a paso | `AccelStepper::HALF4WIRE`, 220 medios pasos/s |
| Límites de prueba | −1024 a +1024 medios pasos **relativos al arranque** |

## Empezar rápidamente

1. Lee **[docs/CONEXIONES_ALIMENTACION.md](docs/CONEXIONES_ALIMENTACION.md)**; retira hélices y desconecta potencia de ESC antes de programar.
2. Sigue **[docs/INSTALACION_OTRA_COMPUTADORA.md](docs/INSTALACION_OTRA_COMPUTADORA.md)** para instalar VS Code, Git y PlatformIO y preparar las **dos carpetas de proyectos independientes**.
3. Abre cada carpeta por separado en VS Code, compila, sube el firmware y abre los monitores a 115200.
4. Presiona **PS** para conectar el DualShock 4 (sin borrar cada vez las claves Bluetooth).
5. Comprueba `TX_M`, `TX_P`, `RX_M` y `RX_P`; prueba cruceta **sin carga**, luego X; no energices los ESC en esta verificación.

## Advertencias antes de instalar sonda o carrete

**No está listo para elevar una sonda real automáticamente.** Al parar, este ensayo ejecuta `motorPaso.disableOutputs()` y por tanto pierde el par de retención; una sonda suspendida podría caer. El contador de posición se inicia en cero al encender, sin sensor de referencia (*homing*). Se requiere al menos final de carrera superior, diseño mecánico que sostenga carga aun sin energía, prueba de torque, control de profundidad calibrado y protección contra agua/humedad. Los sensores DS18B20 y pH-4502C **no están incluidos en este firmware**, porque el presente avance únicamente ensaya la elevación y los actuadores.

La comprobación del prototipo **no equivale** a validación de seguridad, confiabilidad, autonomía, estanqueidad, control de profundidad real ni uso con hélices instaladas.

## Documentos de este avance

- [Instalar y ejecutar en otra laptop](docs/INSTALACION_OTRA_COMPUTADORA.md)
- [Conexiones y fuentes externas](docs/CONEXIONES_ALIMENTACION.md)
- [Formato de tramas XBee y ejemplos](docs/PROTOCOLO_XBEE.md)
- [Pruebas, criterios de éxito y pendientes](docs/PRUEBAS_Y_SEGURIDAD.md)
- [Bitácora de problemas y soluciones](docs/HISTORIAL_PROBLEMAS_Y_HALLAZGOS.md)
- [Subir la carpeta a GitHub desde navegador](docs/COMO_SUBIR_A_GITHUB.md)

## Referencias para reproducir la plantilla

- Plantilla original Bluepad32: https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template
- ESP-IDF UART: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html
- AccelStepper: https://www.airspayce.com/mikem/arduino/AccelStepper/
- PlatformIO: https://docs.platformio.org/
