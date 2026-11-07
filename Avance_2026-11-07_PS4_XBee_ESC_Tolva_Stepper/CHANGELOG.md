# Registro de cambios del catamarán

## 2026-11-07 — Cruceta PS4 y motor 28BYJ-48 (ensayo sin carga)

- Se añadió `AccelStepper` en ESP32 esclava con ULN2003, IN1 GPIO25, IN2 GPIO26, IN3 GPIO27 e IN4 GPIO32.
- Se extendió el protocolo XBee de 4 bytes con tres códigos reservados: **250 SUBIR**, **251 BAJAR**, **252 PARAR**, manteniendo la trama separada de motores/tolva.
- La maestra mantiene `uart_write_bytes()` sobre UART2 nativa de ESP-IDF, Bluepad32, controles de ESC y tolva, envío de motores a 50 ms; el paso a paso se refresca a 100 ms.
- El esclavo conserva el control diferencial y la tolva; añade `runSpeed()` no bloqueante, guardián de 250 ms, posición relativa con límites ±1024 y mensajes `RX_M`, `RX_P`, `PASO`, `POS`.
- Tras cargar ambos programas desde PlatformIO, se observó `XBEE:OK`, `PS4:OK`, `RX_M`/`RX_P` aumentando, `ERR:0`. El usuario confirmó que el conjunto funcionó.
- **Pendiente:** sensor final de carrera, homing real, carrete, par de retención, pruebas con carga, calibración de profundidad y sensores de pH/temperatura.

## 2026-11-07 — ESC + tolva funcionando en ambas ESP32 con PlatformIO

- UART nativa ESP-IDF recuperó la transmisión XBee del maestro (`Serial2.write` había fallado bajo la plantilla integrada Arduino/ESP-IDF); el esclavo sí conserva `Serial2`.
- Servo de tolva alimentado desde fuente regulada independiente por ruido al usar el BEC del ESC; GND común.
- Reducción del período de 100 a 50 ms mejoró la latencia percibida de tolva, manteniendo confirmación de dos paquetes.
- XBee PRO S1 y su configuración broadcast permanecieron sin modificaciones.

Consulta `docs/HISTORIAL_PROBLEMAS_Y_HALLAZGOS.md` para los fallos, observaciones y límites de las conclusiones.
