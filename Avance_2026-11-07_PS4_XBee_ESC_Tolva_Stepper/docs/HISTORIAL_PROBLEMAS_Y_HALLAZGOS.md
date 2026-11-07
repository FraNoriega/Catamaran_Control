# Bitácora técnica — problemas, evidencias y soluciones

**Criterio:** separar hechos observados de hipótesis. Las fechas corresponden a las sesiones de integración documentadas; el avance actual es del **7 de noviembre de 2026**.

## 1. Base PS4/Bluepad32 y dos ESP32

- **Objetivo:** PS4 → Bluetooth Bluepad32 → ESP32 maestra → XBee PRO S1 → ESP32 esclava → actuadores.
- **Entorno exigido:** VS Code/PlatformIO en ambas placas. La maestra utiliza `framework=espidf` con Arduino como componente; la esclava utiliza Arduino normal bajo PlatformIO.
- **Resultado:** mando conectado y tramas transmitidas/recibidas durante ensayos de dos ESC.

## 2. Problema de `esptool` al compilar/cargar el maestro

- **Síntoma observado:** `esptool: error: unrecognized arguments: --flash-mode --flash-freq 40m --flash-size 4MB ...` durante el proceso de carga/compilación de una versión de la plataforma.
- **Trabajo realizado:** revisión de paquetes de `esptool` y de rutas duplicadas, actualización a una versión compatible (en el equipo se documentó `esptool 5.1.0`) y eliminación/reubicación de una copia anterior conflictiva.
- **Resultado:** volvió a ser posible generar/cargar firmware del maestro. **No puede afirmarse que esto causara por sí solo el problema de recepción XBee.**

## 3. Pérdida de comunicación XBee con código maestro en PlatformIO

- **Síntoma:** monitor maestro incrementaba transmisiones por `Serial2.write()` pero la esclava mostraba `RX_OK:0` o el contador congelado; se había confirmado previamente enlace válido.
- **Prueba cruzada:** **maestro Arduino IDE + esclava PlatformIO** entregó paquetes y comprobó que las conexiones/radios eran capaces de operar. Esa combinación no era aceptable para la entrega académica, pues ambas ESP32 deben programarse con PlatformIO.
- **Prueba mínima en maestro PlatformIO con `Serial2`:** no restauró la recepción.
- **Prueba con UART nativa de ESP-IDF:** `uart_driver_install`, `uart_param_config`, `uart_set_pin`, `uart_write_bytes`, `uart_wait_tx_done` a 115200 en UART2 GPIO17/16. El esclavo pasó a `XBEE:OK`, `RX_OK` creciente.
- **Integración final:** Bluepad32 y UART nativa juntos en maestro **sí funcionaron con ambos dispositivos programados desde PlatformIO**.
- **Conclusión acotada:** en **esta combinación de plantilla, firmware y hardware**, reemplazar `Serial2.write()` por UART nativa **resolvió el síntoma**. No se identificó la causa raíz interna de Arduino `Serial2` y no se concluye que falle universalmente.

## 4. Tolva y ruido de alimentación

- **Versión de control:** botón X para abrir mientras está presionado y cerrar al soltar, codificando el estado en bit0 del checksum de 4 bytes; receptor requiere dos tramas válidas consecutivas para abrir.
- **Problema:** servo alimentado desde BEC del ESC provocó ruido/interferencia y movimiento con retardo.
- **Hallazgo práctico:** fuente regulada externa para el servo y tierra común redujo el problema. No unir positivos de fuentes ni dos BEC.
- **Optimización:** cambio periódico de **100 ms a 50 ms** mejoró perceptiblemente la respuesta de la tolva, sin volver a configurar XBee.

## 5. Controlador ULN2003 y motor 28BYJ-48

- **Identificación:** motor 28BYJ-48 **5 V** y placa ULN2003 con cable de cinco conductores.
- **Proyecto de prueba independiente:** `Catamaran_Prueba_Stepper`, ESP32 + `AccelStepper::HALF4WIRE`, IN1 25, IN2 26, IN3 27, IN4 32.
- **Resultado:** motor ejecutó ida (256 medios pasos), regreso y se detuvo; monitor mostró `PRUEBA FINALIZADA`. El usuario confirmó movimiento físico.

## 6. Integración con mando PS4 por cruceta

- **Protocolo nuevo:** comandos paso a paso 250/251/252 usando el segundo byte 126, checksum con bit0=0; el envío de motores y tolva permaneció separado cada 50 ms, stepper a 100 ms.
- **Primera prueba fallida:** monitor esclavo `RX_M` creciente y `RX_P:0`; monitor maestro todavía mostraba el formato antiguo `TX_IDF` y en el editor había cambios sin guardar.
- **Corrección:** guardar `sketch.cpp` y volver a cargar firmware del maestro con PlatformIO.
- **Resultado confirmado:** monitor maestro `PASO:PARAR`, `TX_M` y `TX_P` creciendo; esclavo `XBEE:OK`, `PS4:OK`, `RX_M` y `RX_P` aumentando, `ERR:0`, posición de motor distinta de cero tras los movimientos. El usuario confirmó que «funcionó».

## 7. Limitaciones y próximos trabajos

- No se ha instalado ni ensayado un carrete con peso real de sonda.
- No existe final de carrera superior, control de *homing* ni límite real de profundidad. ±1024 medios pasos es solo una cota desde el arranque.
- Al pararse el paso a paso, `disableOutputs` elimina par de retención; riesgo de caída de carga si se suspende una sonda.
- No se ha caracterizado el par, capacidad de carga, fiabilidad RF prolongada ni seguridad eléctrica del sistema embarcado.
- La medición real de DS18B20/pH, la calibración del pH y la visualización remota pertenecen a etapas independientes; no están incluidas en este firmware de actuadores.
- Es necesario archivar la carpeta completa y versiones de submódulos de Bluepad32 para reproducibilidad exacta del maestro.
