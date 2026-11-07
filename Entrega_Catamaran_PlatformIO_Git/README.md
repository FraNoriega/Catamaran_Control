# Catamarán RC — control PS4, XBee, dos motores brushless y tolva

**Estado:** prototipo funcional comprobado con **ambas ESP32 programadas en PlatformIO**. Última mejora verificada: envío periódico a **50 ms**, que redujo la demora percibida de la tolva.

> Este repositorio documenta el firmware y los hallazgos de la integración. Los dos archivos de código reproducen la **última versión descrita y probada en la conversación**; antes de sustituir archivos de una placa en uso, compáralos con la copia exacta que tienes en tu laptop. No se han extraído los archivos originales del disco `D:`.

## Descripción

```text
DualShock 4 --Bluetooth/Bluepad32--> ESP32 MAESTRA (PlatformIO, ESP-IDF)
                                       |
                              UART2 nativa ESP-IDF
                            GPIO17 TX / GPIO16 RX
                                       |
                              HW-186 + XBee PRO S1
                                       ))) enlace RF (((
                              HW-186 + XBee PRO S1
                                       |
                            GPIO16 RX / GPIO17 TX
                                       |
                       ESP32 ESCLAVA (PlatformIO, Arduino)
                           |             |             |
                      GPIO13 ESC L  GPIO18 ESC R   GPIO23 SERVO
                                                    tolva
```

La maestra utiliza **`uart_write_bytes()` de ESP-IDF**, no `Serial2.write()`. Es la corrección que recuperó el enlace inalámbrico en el proyecto maestro de PlatformIO. La esclava sí utiliza `Serial2`.

## Funciones comprobadas

| Entrada del mando | Acción |
|---|---|
| Joystick izquierdo, eje X | Dirección diferencial: reduce la potencia del motor del lado del giro |
| R2 | Aumenta el empuje de los dos brushless |
| L2 | Reduce la demanda de empuje; no es reversa ni frenado eléctrico activo |
| Mantener X (cruz) | Abrir la compuerta de la tolva |
| Soltar X | Cerrar la compuerta |
| Desconectar mando o perder datos durante más de 400 ms | Ordenar 1000 µs a ambos ESC y cierre de tolva, con alimentación del esclavo y servo disponible |

**Rango de ensayo de los ESC:** 1000–1300 µs. **Servo tolva:** 20° (cerrado) y 90° (abierto), posiciones provisionales. **Período de UART:** transmisión cada 50 ms o inmediatamente al cambiar la orden. **Armado:** 800 ms de señal neutra; rearmado exigido después de pérdida de enlace.

## Archivos

```text
README.md
CHANGELOG.md
.gitignore
firmware/
  maestro/sketch.cpp                # Copiar a Catamaran_Bluepad32/main/sketch.cpp
  maestro/platformio.ini.RECUERDA.md
  esclavo/platformio.ini           # Proyecto Arduino/PlatformIO normal
  esclavo/src/main.cpp             # Dos ESC + servo tolva

docs/
  HISTORIAL_PROBLEMAS_Y_HALLAZGOS.md
  INSTALACION_OTRA_LAPTOP_WINDOWS.md
  CONEXIONES_Y_SEGURIDAD.md
  PROTOCOLO_Y_COMPROBACION.md
  GUIA_GIT_Y_COMMIT.md
scripts/
  platformio_catamaran.ps1
COMMIT_MESSAGE.txt
```

## Antes de programar

1. Retirar hélices y desconectar la potencia de los ESC para la primera validación.
2. No alterar XCTU: se conserva la configuración XBee usada durante las pruebas exitosas.
3. No alimentar el servo desde el BEC del ESC que presentó ruido; usar fuente regulada independiente con **GND común** con la ESP32. No unir positivos de fuentes o de dos BEC.
4. Confirmar los puertos COM reales. En las pruebas fueron **COM3** para maestro y **COM7** para esclavo; en otra laptop pueden cambiar.
5. No usar `BP32.forgetBluetoothKeys()` en cada arranque. Para reconectar normalmente basta presionar **PS**.

## Instalación y ejecución en otro equipo

**Ver [docs/INSTALACION_OTRA_LAPTOP_WINDOWS.md](docs/INSTALACION_OTRA_LAPTOP_WINDOWS.md)** para instrucciones completas de Git, VS Code, PlatformIO, dependencias, clonación recursiva de Bluepad32, creación de ambos proyectos, compilación, carga, monitores serie y problemas comunes.

### Ejecutar en una laptop con los proyectos ya preparados

```powershell
# Maestro: PowerShell en carpeta del proyecto completo Bluepad32
cd D:\Catamaran_Bluepad32
$env:PLATFORMIO_CORE_DIR = "D:\PlatformIO_Data"  # Ruta del equipo probado; ajustar a la laptop
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio run -e esp32dev -t upload --upload-port COM3
& $pio device monitor -p COM3 -b 115200
```

```powershell
# Esclavo: usar otra terminal, y Ctrl+C para cerrar el monitor antes de subir firmware
cd D:\Catamaran_esclavo
$env:PLATFORMIO_CORE_DIR = "D:\PlatformIO_Data"
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio run -e esp32doit-devkit-v1 -t upload --upload-port COM7
& $pio device monitor -p COM7 -b 115200
```

Si esa nueva computadora no tiene `D:`, seguir la guía de instalación: **no copiar literalmente las rutas del computador original**.

## Evidencias y límites

**Se observó funcionando:** PS4 conectado, UART RF a través de XBee, contador `RX_OK` creciente, dos ESC controlados por R2/joystick, servo abre al mantener X y cierra al soltar, y respuesta mejorada con 50 ms. **No se considera validado:** parada de emergencia independiente, fiabilidad estadística del enlace, operación prolongada bajo carga, funcionamiento en el agua, estanqueidad y exactitud de dosificación.

Los mensajes `TX_IDF` indican transmisión por UART local, **no confirman por sí solos recepción radio**. La prueba de extremo a extremo requiere `XBEE:OK`, `RX_OK` creciente y coincidencia de valores en el esclavo.

## Referencias técnicas

- Plantilla Bluepad32 ESP-IDF + Arduino: https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template
- Uso de la plantilla en PlatformIO: https://bluepad32.readthedocs.io/en/latest/plat_arduino/
- Documentación oficial de ESP-IDF UART: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html

## Cambios y reproducibilidad

- [CHANGELOG.md](CHANGELOG.md): avances de integración.
- [docs/HISTORIAL_PROBLEMAS_Y_HALLAZGOS.md](docs/HISTORIAL_PROBLEMAS_Y_HALLAZGOS.md): fallos observados, evidencia y solución o hipótesis pendiente.
- [docs/GUIA_GIT_Y_COMMIT.md](docs/GUIA_GIT_Y_COMMIT.md): subir esta entrega a un repositorio Git propio.
