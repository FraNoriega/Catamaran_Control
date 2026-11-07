# Conexiones, energía y medidas preventivas

## Conexiones XBee / HW-186

**Importante:** En los adaptadores HW-186 usados en este proyecto se comprobó por continuidad que el terminal **TX del HW-186** conduce a **DIN (entrada RX) del XBee** y el terminal **RX del HW-186** conduce a **DOUT (salida TX) del XBee**. Esta nomenclatura puede variar entre placas de otros fabricantes: estas correspondencias se refieren al adaptador particular probado.

| HW-186 | ESP32 maestra | ESP32 esclava |
|---|---|---|
| `5V` del adaptador | `VIN/5V` | `VIN/5V` |
| `GND` del adaptador | `GND` | `GND` |
| `TX` (hacia DIN XBee) | `GPIO17` | `GPIO17` |
| `RX` (desde DOUT XBee) | `GPIO16` | `GPIO16` |

- UART2 115200 bps, 8N1, sin RTS/CTS implementados por firmware.
- La **radio XBee requiere 3,3 V a su propio módulo**; los 5 V del cuadro son **solo para la entrada regulada del adaptador HW-186**, cuya salida al XBee fue verificada durante el proyecto.
- No conectar 5 V directamente a DIN, DOUT ni VCC del XBee. No cambiar ni cruzar cables durante pruebas de firmware sin apagar.
- No conectar directamente las ESP32 para operación normal: la comunicación de producción usa los dos radios.

## Actuadores en ESP32 esclava

| Función | Pin ESP32 esclava | Señal |
|---|---|---|
| ESC izquierdo | GPIO13 | Pulsos tipo servo, 50 Hz, 1000–1300 µs durante ensayos |
| ESC derecho | GPIO18 | Pulsos tipo servo, 50 Hz, 1000–1300 µs durante ensayos |
| Servo tolva | GPIO23 | PWM tipo servo 50 Hz, posiciones provisionales 20°/90° |

## Alimentación del servo

El servo produjo **ruido perceptible** cuando se alimentó del BEC de uno de los ESC. La versión que funcionó mejor usa una **fuente externa regulada** adecuada a la corriente y al voltaje nominales del servo:

```text
Fuente servo +V  ---> cable rojo SERVO
Fuente servo GND ---> cable negro/marrón SERVO
Fuente servo GND ---> GND ESP32 esclava (referencia común)
GPIO23 ESP32      ---> cable amarillo/naranja de SEÑAL del servo
```

**No unir** el positivo de esta fuente externa con la salida BEC de un ESC. Si ambos ESC tienen BEC, no paralelizar sus cables rojos sin una solución de aislamiento aprobada. Alimentar la ESP32 por USB y BEC simultáneamente puede crear rutas indeseadas: documentar un sistema de alimentación único antes de la versión final.

## Seguridad antes de las pruebas

- **Retirar las hélices**, sujetar mecánicamente motores y mantener libres los ejes.
- Realizar la primera comprobación con potencia de motores desconectada y observar `RX_OK`/estados por serial.
- No asumir que 1000 µs detiene o arma todos los modelos de ESC: comprobar su manual y rango.
- `R2` acciona propulsión unidireccional; `L2` disminuye la consigna, no ofrece reversa física.
- La compuerta se abre al mantener X. Un servo bloqueado o mal alimentado puede sobrecalentarse.
- Probar el servo **sin acoplar la tolva** hasta confirmar que 20° y 90° son adecuados.
- `TIMEOUT_XBEE=400` ms ordena mínimo y cierre tras pérdida de tramas; no equivale a parada mecánica ni eléctrica de emergencia.
- Antes de probar en agua: validar protección ante desconexión del control, corte del maestro, interferencias y reinicios de alimentación, además de seguridad eléctrica y estanqueidad.
