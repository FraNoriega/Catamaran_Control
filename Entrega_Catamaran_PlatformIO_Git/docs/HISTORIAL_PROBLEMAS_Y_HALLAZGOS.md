# Bitácora de problemas, pruebas, evidencias y hallazgos

**Alcance:** ESP32 maestra con DualShock 4/Bluepad32, enlace XBee PRO Series 1 por HW-186, ESP32 esclava, dos ESC brushless y servo de tolva. Se documentan hechos reportados durante las pruebas del proyecto, no diagnósticos instrumentales definitivos.

## Resumen cronológico

| N.º | Situación observada | Evidencia y prueba realizada | Hallazgo o solución | Estado |
|---:|---|---|---|---|
| 1 | Reconexión irregular del mando PS4 | Se logró conectar después de reiniciarlo en una prueba anterior. En pruebas posteriores volvió a conectar únicamente presionando PS. | No se requiere reinicio físico en cada arranque. No ejecutar `BP32.forgetBluetoothKeys()` en cada `setup()` porque borra vínculos guardados. | Resuelto operativamente |
| 2 | XBee transmitía mensajes en XCTU, pero la ESP32 esclava no recibía | Los módulos intercambiaron textos en XCTU con la misma configuración. Más adelante el monitor del esclavo mostró `Total bytes:0`. | La configuración XCTU no era inherentemente inválida; ausencia de bytes no prueba daño del módulo. | Corregido a nivel de firmware maestro en el montaje final |
| 3 | Dudábamos del protocolo de 4 bytes | Enlace directo GPIO17 maestro → GPIO16 esclavo + GND mostró `FF 5A 7E D8`, etc. | La generación y recepción UART directa funcionaban. | Verificado |
| 4 | Al añadir la tolva hubo pérdida de comunicación | Se ensayó transmitir dos tramas (`FF ...` motores y `FE ...` tolva), luego se simplificó. | Se preservó **una sola trama de cuatro bytes**, usando bit 0 del checksum como estado de tolva. No se demostró que la segunda trama fuera por sí sola la causa del corte. | Simplificado y posteriormente verificado |
| 5 | Comunicación con los mismos XBee: maestro Arduino IDE sí, maestro PlatformIO no | Prueba cruzada Arduino IDE (maestra) + PlatformIO (esclava): `XBEE:OK`, `RX_OK` creciente, `ERRORES:0` en captura. Cambiando solo firmware maestro a PlatformIO con `Serial2.write()`: recepción detenida. | El fallo se concentró en el **firmware/entorno del maestro**. No se probó una falla universal de `Serial2` en todos los ESP32/PlatformIO. | Aislado |
| 6 | Error de compilación del maestro tras actualizar herramientas | `esptool: error: unrecognized arguments --flash-mode ...`; `pip show esptool` daba 5.1.0, pero `python -m esptool version` cargaba 4.5.1 desde una instalación editable en `...\.platformio\packages\tool-esptoolpy`. | Había conflicto de rutas/versiones Python; se reparó la instalación. Después se observó `esptool v5.1.0`, generación de `firmware.bin` y `SUCCESS`. | Resuelto en equipo de prueba |
| 7 | Prueba mínima sin Bluepad32 tampoco recibía mediante `Serial2` del maestro ESP-IDF | La transmisión local imprimía TX; la esclava reportaba `RX_OK:0` o bytes cero en distintos intentos. | Imprimir TX local **no confirma entrega por radio**. | Diagnosticado |
| 8 | UART nativa de ESP-IDF en maestro | `uart_driver_install()`, `uart_param_config()`, `uart_set_pin()` y `uart_write_bytes()`. Captura: `[TX IDF] ... Bytes:4 | Fin:ESP_OK`; simultáneamente `XBEE:OK`, `RX_OK` creciendo. | **Solución técnica verificada:** usar UART nativa de ESP-IDF en ESP32 maestra y mantener `Serial2` en esclava. | Resuelto y reproducido |
| 9 | Integración Bluepad32 + UART nativa | Maestra: `PS4:OK` y `TX_IDF` creciente; esclava: `XBEE:OK`, `RX_OK` creciente y ángulo/velocidad coincidentes. | Ambas placas funcionan en PlatformIO sin cambiar cableado ni XCTU. | Verificado |
| 10 | Control de dos ESC | Motores respondieron a R2 y joystick, limitados a 1300 µs. | GPIO13 izquierdo, GPIO18 derecho, 50 Hz, giro diferencial. | Verificado en ensayo inicial |
| 11 | Se añadió servo de tolva, con alimentación compartida con ESC | Se percibió ruido eléctrico al alimentarlo desde un ESC. | **Solución práctica verificada:** fuente externa regulada y GND común; no mezclar positivos ni BEC. | Mejora observada |
| 12 | Servo de tolva respondía con retraso | Maestro transmitía cada 100 ms; esclava exigía dos paquetes de apertura. Al reducir el envío a 50 ms el usuario informó: «funciona mucho mejor». | Mantener **PERIODO_TX = 50 ms** y las dos confirmaciones de apertura; servo mueve hasta los ángulos provisionales. | Mejorado y verificado subjetivamente |

## Configuración XBee preservada

Se usaron XBee PRO Series 1 / IEEE 802.15.4 con UART a 115200 baudios, 8N1, modo transparente (`AP=0`), PAN ID `0x1234`, canal `0x17`. En las pruebas documentadas:

| Parámetro | XBee maestro | XBee esclavo |
|---|---|---|
| `CE` | `1` | `0` |
| `MY` | `0x10` | `0x20` |
| `DH` | `0` | `0` |
| `DL` | `0xFFFF` broadcast | `0x10` |
| `ID` (PAN) | `0x1234` | `0x1234` |
| `CH` | `0x17` | `0x17` |
| `AP` | `0` | `0` |
| `BD` | `115200` | `115200` |
| `RO` | `0` | `0` |

**No se cambiaron estos valores** durante la solución final. El usuario observó latencia mayor al ensayar unicast anteriormente, por lo que prefirió mantener broadcast. Ese resultado es específico de este montaje y no demuestra que unicast sea siempre más lento en otras aplicaciones.

## Lecciones de depuración

- No cambiar varias dimensiones a la vez: firmware maestro, parser esclavo, alimentación y parámetros XCTU; se pierde trazabilidad.
- `TX` o `ESP_OK` en UART solo informa del envío local. **Recepción comprobada:** `XBEE:OK`, `RX_OK` creciente, datos correlacionados.
- Un valor alto `RX_OK` puede estar **congelado** y ser histórico. Comparar al menos dos lecturas separadas en el tiempo.
- Diferenciar `BYTES:0`, `ENCABEZADOS:0`, `RX_OK:0` y `ERRORES` creciente. No equivalen al mismo fallo.
- El parser no debe reiniciarse indiscriminadamente cuando aparece `0xFF` dentro de una trama: el checksum u otros bytes de datos pueden valer `0xFF`.
- Conservar la última versión funcional antes de agregar motores, sensores o la tolva.
- Evitar reinstalaciones masivas de PlatformIO cuando el compilador funciona y hay evidencia de diferencias de firmware.

## Riesgos y tareas que siguen abiertas

1. El checksum XOR/no criptográfico usado aquí (máscara `& 0xFE`) es débil: ciertas corrupciones pueden coincidir con la suma esperada. Recomendada para evolución: CRC, secuencia, autenticidad de tramas si el control será crítico.
2. El bit de apertura se valida mediante dos tramas consecutivas, pero eso no equivale a identificación de mensajes con número de secuencia.
3. El cierre de servo es una **orden**, no garantía mecánica de compuerta cerrada si falla la fuente, el servo, el mecanismo o hay un atasco.
4. Se necesita verificar físicamente paro tras desconexión PS4, pérdida XBee, recuperación, y no-arranque inesperado.
5. No se ha cuantificado latencia end-to-end, pérdida de paquetes ni corriente pico del servo.
6. Aún no se han validado navegación en agua, estanqueidad, probióticos reales o dosificación cuantitativa.
