# Registro de cambios — Catamarán RC

Las entradas describen la secuencia de desarrollo y observaciones del prototipo, no ensayos certificados.

## [0.3.1] — 2026-11-07 — Mejora de respuesta de la tolva

- **Verificado en hardware:** se redujo el período de comunicación maestro de 100 ms a **50 ms** y mejoró perceptiblemente la respuesta del servo.
- Se mantuvo la confirmación de dos tramas consecutivas para abrir la tolva; el cierre se ordena al recibir X liberado.
- Se adoptó una **fuente regulada independiente** para el servo debido al ruido observado cuando se alimentaba desde el ESC. Requiere GND común.

## [0.3.0] — 2026-11-07 — Tolva y dos ESC integrados

- **Verificado en hardware:** botón X abre la tolva y al soltar X se cierra.
- Se preservó una única trama de cuatro bytes y se aprovechó el bit 0 del checksum para la tolva.
- El esclavo conserva dos ESC (GPIO13/GPIO18) y agrega servo (GPIO23).
- Cambio de protocolo para la orden sin mando (`angulo=254`, `velocidad=126`).

## [0.2.0] — 2026-11-07 — UART nativa ESP-IDF en maestra

- **Hallazgo clave:** `Serial2.write()` del firmware maestro compilado mediante una combinación concreta PlatformIO/Bluepad32/ESP-IDF no entregaba paquetes vía XBee, pese a que la UART directa entre ESP32 funcionaba.
- **Verificado en hardware:** `uart_driver_install`, `uart_param_config`, `uart_set_pin` y `uart_write_bytes` en UART2 permitieron comunicación inalámbrica desde PlatformIO; `RX_OK` creció en la esclava.
- **Verificado:** Bluepad32 + UART nativa combinados mantuvieron joystick y gatillos con datos recibidos en la esclava.
- No se modificó la configuración XCTU.

## [0.1.0] — 2026-11-07 — Control de dos brushless

- **Verificado en hardware:** dos ESC a 50 Hz, 1000–1300 µs, control diferencial por joystick, aceleración R2 y reducción de empuje L2.
- Se adoptó cierre a mínimo al perder comunicación después de 400 ms y nuevo armado después de reposo.
- Comunicación cruzada Arduino IDE (maestra) + PlatformIO (esclava) también funcionó, aislando el problema al firmware maestro de PlatformIO.

## Pendiente

- Prueba de pérdida de enlace y de mando **con motores desconectados de hélices**.
- Medición de latencia y tasa de paquetes corruptos por intervalos prolongados.
- Caracterizar corriente de la fuente del servo bajo carga y ajustar los ángulos a la compuerta real.
- Pruebas mecánicas de tolva, estanqueidad y navegación en agua.
