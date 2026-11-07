# Plan de pruebas, resultados observados y límites de seguridad

## 1. Prueba de integración **sin carga**

1. **Retirar hélices** de ambos brushless y desconectar la alimentación de potencia de sus ESC.
2. Dejar el eje del 28BYJ-48 **sin carrete, cable ni sonda**, con controlador ULN2003 en 5 V externo regulado y GND común.
3. Mantener el servo desacoplado de la compuerta durante pruebas de recorrido (o verificar que la compuerta no se atasque).
4. Compilar y cargar **esclava** y **maestra** desde PlatformIO (ver `INSTALACION_OTRA_COMPUTADORA.md`).
5. Conectar mando DualShock 4 con el botón PS. Esperar `PS4:OK`, `XBEE:OK`.
6. Observar el monitor maestro: `TX_M` y `TX_P` deben aumentar. En esclavo `RX_M` y `RX_P` deben aumentar.
7. Mantener cruceta **abajo**: monitor esclavo indica `[STEPPER] BAJANDO`, motor gira. Soltar: `[STEPPER] PARADO` y cesa el giro.
8. Mantener cruceta **arriba**: `[STEPPER] SUBIENDO` y gira en sentido contrario. Soltar: se detiene.
9. Mantener **X**: tolva abre; soltar X: tolva cierra. Los motores permanecen sin alimentación de potencia.
10. Desconectar mando durante movimiento y comprobar **en la práctica** el paro; no asumirlo solo por leer el código.
11. Reiniciar sin mover físicamente el eje: `POS` vuelve a cero porque este prototipo **no tiene homing físico**.

## 2. Evidencia histórica

- **Prueba del 28BYJ-48 separado:** usuario confirmó giro y monitor mostró «Movimiento de ida», «Movimiento de regreso», «PRUEBA FINALIZADA».
- **Primera integración:** el monitor esclavo mostró `XBEE:OK`, `RX_M` creciendo, `RX_P:0`; la maestra aún tenía firmware previo (`TX_IDF`) porque el archivo nuevo no estaba guardado/cargado.
- **Tras guardar y volver a cargar maestra:** aparecieron `PASO:PARAR`, `TX_M` y `TX_P` en monitor maestro, y `RX_M`, `RX_P` creciendo en esclava, con `ERR:0`; el usuario confirmó «funcionó».
- **No se probó:** capacidad para levantar el peso real de sonda, carrete, sensores pH/temperatura sumergidos, calibración de profundidad o funcionamiento sostenido sobre el agua.

## 3. Señales de fallo y diagnóstico

| Síntoma | Causa probable / verificación |
|---|---|
| Maestro solo muestra `TX_IDF` | Firmware antiguo: guardar `sketch.cpp` con `Ctrl+S` y volver a cargar `COM3` |
| `TX_P` aumenta, `RX_P` permanece en 0 | Revisar que la esclava tenga **el programa nuevo** y el protocolo correcto; comprobar `RX_M` antes de modificar radios |
| `XBEE:PERDIDO` | No llega trama válida de motores en 400 ms; revisar que **maestra y esclava ejecuten firmwares compatibles** |
| `[STEPPER] BAJANDO`, motor no gira | ULN2003 5 V, GND común, orden IN1/IN2/IN3/IN4, conector blanco |
| Motor deja de girar tras recorrido | Se alcanzó límite relativo de ±1024, propio de la prueba |
| Servo falla/tiembla al conectar ESC | Evitar BEC del ESC que presentó ruido; fuente regulada independiente |
| `PioArduino` y `PlatformIO` aparecen como extensiones a la vez | Preferir entorno único y mantener consistencia; la plataforma PioArduino de compilación no equivale a instalar otra extensión |
| Problema de `esptool --flash-mode ...` | Consultar bitácora de versiones/instalación; no cambiar herramientas sin diagnosticar |

## 4. Riesgos pendientes

- **Retención:** `disableOutputs()` desenergiza las bobinas. Con carga, el carrete puede desenrollarse; se requiere retención mecánica.
- **Cero y límites:** ±1024 representan **posición relativa al arranque**, no centímetros de profundidad ni topes físicos.
- **Final de carrera:** debe añadirse un interruptor superior antes de colocar sonda colgante.
- **Torque:** comprobar peso, rozamiento, radio del tambor y alimentación estable.
- **Protocolo:** checksum pequeño y sin autenticación ni número de secuencia; radios y tolerancia a errores no han sido caracterizados.
- **Seguridad física:** stop software no sustituye un interruptor físico de desconexión de potencia.
