# Catamarán: PS4 + ESP32 maestro + XBee S1 PRO + ESP32 esclavo + 2 ESC

**Estado:** prototipo de control a probar en banco. Los archivos suministrados todavía no han sido compilados sobre las placas reales.

## 1. Arquitectura

```text
DualShock 4 --(Bluetooth)--> ESP32 MAESTRO --UART2--> XBee S1 PRO ))) RF ((( XBee S1 PRO --UART2--> ESP32 ESCLAVO
                                                                                                         | GPIO13 (PWM 50 Hz) --> ESC izquierdo --> Brushless izquierdo
                                                                                                         | GPIO18 (PWM 50 Hz) --> ESC derecho   --> Brushless derecho
```

El ESP32 maestro debe utilizar el proyecto **ya funcional** basado en `esp-idf-arduino-bluepad32-template` (Bluepad32 nativo, ESP-IDF + componente Arduino). El esclavo no requiere Bluepad32 y usa un proyecto **Arduino para ESP32** independiente en PlatformIO.

## 2. Conexiones de los dos módulos XBee S1 PRO

Realizar las mismas conexiones UART tanto en el maestro como en el esclavo:

| XBee S1 PRO, pines del módulo (sin adaptador) | ESP32 |
|---|---|
| 1 VCC | Alimentación regulada de 3,3 V para XBee PRO |
| 10 GND | GND |
| 2 DOUT (TX) | GPIO16 (RX2) |
| 3 DIN (RX) | GPIO17 (TX2) |

**Importante:** el módulo XBee desnudo **no soporta 5 V en VCC**. Los PRO pueden requerir varios cientos de mA mientras transmiten: usa fuente/regulador 3,3 V con margen suficiente y desacoplo local; **no presupongas** que el 3V3 de tu ESP32 puede alimentarlo. Si usas una placa adaptadora, identifica sus pines y si incorpora regulador. Los niveles UART del ESP32 y del módulo son de 3,3 V. Se requiere GND común.

## 3. Configuración de ambos XBee con XCTU

Para módulos **legacy XBee S1 / XBee-PRO S1 802.15.4** (no Zigbee/XBee3), utiliza XCTU y configura:

| Parámetro | XBee maestro | XBee esclavo |
|---|---|---|
| `AP` (modo de operación) | `0` (transparente) | `0` |
| `BD` (UART) | `3` (9600 baud) | `3` (9600 baud) |
| `ID` (PAN) | `1234` (hex) | `1234` (hex) |
| `CH` (canal) | `C` (hex) | `C` (hex) |
| `MY` (origen 16-bit) | `1` | `2` |
| `DH` (destino alto) | `0` | `0` |
| `DL` (destino bajo) | `2` | `1` |
| `SM` (modo sueño) | `0` (desactivado) | `0` (desactivado) |

Guarda la configuración (`Write`). Si los radios ya funcionaban a `115200`, puedes mantenerlo: en **ambos** códigos cambia `XBEE_BAUD` a `115200` y asegúrate de que `BD` en XCTU coincida. Los dos radios deben compartir canal/PAN y apuntarse uno al otro. Los valores de esta tabla son **un ejemplo de configuración explícita**, no valores asumidos de tus módulos existentes.

## 4. ESC y motores (ESP32 esclavo)

| Elemento | Conexión |
|---|---|
| Señal del ESC izquierdo | GPIO13 |
| Señal del ESC derecho | GPIO18 |
| GND de señal de ambos ESC | GND ESP32 (referencia común) |
| Alimentación de potencia de ESC | Batería/cableado de potencia adecuado a cada ESC y motor |

**No alimentar los motores desde el ESP32**. Si ambos ESC tienen un BEC de 5 V por el cable rojo del conector de servo, **no conectes en paralelo ambas salidas BEC** ni a la alimentación de 3,3 V del ESP32. Diseña una alimentación única y compatible con tu placa; para pruebas con USB, mantén aislados esos cables rojos si no se necesitan. Usa interruptor de emergencia físico y retira las hélices durante las pruebas iniciales.

El código supone ESC **unidireccionales**, de 50 Hz, con `1000 us` en reposo y un máximo limitado inicialmente a `1600 us` hasta verificar su calibración real. Confirmar rangos y secuencia de armado según el modelo de ESC.

## 5. Programa maestro (el ESP32 que YA se conecta al PS4)

1. Abre en VS Code el proyecto funcional `Catamaran_Bluepad32` (plantilla ESP-IDF + Bluepad32).
2. Haz copia de seguridad de `main/sketch.cpp`.
3. Copia el contenido de `MAESTRO_XBEE_PS4.cpp` a `main/sketch.cpp`.
4. Conserva el `main/main.c`, `main/CMakeLists.txt`, `platformio.ini` y los submódulos de la plantilla; **no** instales `Bluepad32 for NINA-W10 boards`.
5. Compila en PowerShell: `pio run -e esp32dev` si `pio` está en PATH, o usa la ruta completa de tu instalación de PlatformIO.
6. Carga: `pio run -e esp32dev -t upload`.
7. Monitor: `pio device monitor -b 115200` y comprueba mensajes de `Console`.

No borres claves Bluetooth en cada arranque; tu control PS4 ya quedó emparejado.

## 6. Programa esclavo (otro ESP32)

1. Crea **un proyecto nuevo** de PlatformIO para la placa `esp32doit-devkit-v1` con framework `arduino`.
2. Copia `platformio_esclavo.ini` como `platformio.ini` de ese proyecto.
3. Copia `ESCLAVO_DOBLE_ESC.cpp` a `src/main.cpp`.
4. Compila con `pio run -e esp32doit-devkit-v1`.
5. Sube el programa con `pio run -e esp32doit-devkit-v1 -t upload`.
6. Abre el monitor por USB con `pio device monitor -b 115200`.

Como el esclavo usa Arduino ordinario, **no es necesario** portar la plantilla Bluepad32 a este segundo ESP32.

## 7. Mapa de controles

- **Joystick izquierdo, eje X:** dirección diferencial. Hacia la izquierda reduce la velocidad del motor izquierdo; hacia la derecha reduce la del motor derecho. No hay movimiento lateral puro.
- **R2:** aumentar aceleración adelante proporcionalmente.
- **L2:** reducir la aceleración pedida por R2 (se resta a la orden). L2 solo no invierte la marcha; al soltar R2 se deja de impulsar.
- **Ambos gatillos sueltos:** orden de reposo `1000 us` en ambos ESC.

Control diferencial, en porcentajes de la orden neta:

| Situación | Izquierdo | Derecho |
|---|---:|---:|
| Dirección centrada | 100 % | 100 % |
| Dirección completamente izquierda | 0 % | 100 % |
| Dirección completamente derecha | 100 % | 0 % |

**Señal de giro solo si hay aceleración:** con los dos motores detenidos, mover el joystick de dirección no hace girar el catamarán sobre su eje. Se requerirían ESC reversibles para maniobras de giro contrarrotante.

## 8. Protocolo y seguridad

Se transmite un paquete cada 50 ms con 8 bytes: `A5 5A R2 L2 DIR FLAGS SEQ CRC8`. `R2` y `L2` van de `0` a `255`, `DIR` va de `0` (izq.) a `200` (der.) y `100` es centro. `FLAGS bit0` indica que hay datos Bluetooth recientes; `SEQ` es un contador; se verifica `CRC-8/ATM` para rechazar paquetes dañados.

- Si no llega ningún paquete válido por **400 ms**, salida de ambos ESC a `1000 us` inmediatamente.
- Si la conexión Bluetooth deja de entregar datos recientes (umbral en maestro de **250 ms**), el maestro transmite la señal de mando inválido y el esclavo detiene los ESC.
- Al iniciar o recuperar el enlace, ambos gatillos deben estar liberados durante **700 ms** para habilitar potencia; evita una aceleración automática al volver la señal.
- La aceleración está limitada a incrementos de **12 us cada 20 ms**; disminuirla ocurre inmediatamente a nivel de señal. El motor y la hélice siguen teniendo inercia: **esto no equivale a frenado mecánico/activo**.
- La salida máxima de prueba está limitada a **1600 us** y se puede ajustar más adelante tras confirmar modelo, polaridad, suministro y rango de ESC.

Estos controles son salvaguardas de software para pruebas, **no sustituyen** un paro físico, una prueba de alcance ni un sistema de seguridad certificado.

## 9. Secuencia de pruebas recomendada

1. **Retira las hélices** y no conectes aún las etapas de potencia. Comprueba alimentación de XBee y cableado UART.
2. Programa ambos ESP32 y configura los XBee con XCTU. Conecta cada XBee a su ESP32.
3. Abre el monitor del esclavo. Sin PS4 debe indicar que no está habilitado, con salidas de `1000 us`.
4. Enciende y conecta el PS4 al maestro. Comprueba que las lecturas de R2, L2 y dirección varían en su monitor.
5. En el esclavo verifica `Radio:OK`, `PS4:OK` y que tras soltar gatillos por 700 ms aparezca `Habilitado:SI`.
6. Con hélices retiradas y ESC alimentados según su manual, prueba **R2 con muy poca presión**: ambos ESC reciben una señal creciente; con joystick a izquierda la señal del ESC izquierdo debe reducirse, y viceversa.
7. Aprieta **L2** mientras mantienes R2 para confirmar que la señal baja. Suelta R2: los dos ESC deben ordenar reposo.
8. Apaga/desconecta el mando PS4: las dos salidas deben pasar a `1000 us`. Repite cortando el enlace XBee para comprobar el timeout.
9. Vuelve a conectar el mando manteniendo R2 presionado: los motores **no deben reactivarse** hasta soltar ambos gatillos por 700 ms.
10. **Solo después** de verificar sentido de giro, alimentación, calibración, failsafe y paro físico, planifica una prueba de bajo riesgo en agua.

## 10. Errores habituales

- `Radio:PERDIDA`: verificar `CH`, `ID`, `DH`, `DL`, `MY`, `AP=0`, `BD` y TX/RX cruzados; tierra compartida y regulador suficiente.
- `PS4:SIN SENAL`: verificar que el PS4 realmente conecte a la ESP32 maestra y que el código maestro compilado sea el de la plantilla funcional.
- ESC hace beeps pero no gira: verificar armado y rangos de pulso, además de alimentación y masa; no asumir que el rango de un modelo sirve para otro.
- Joystick produce giro inverso: comprobar correspondencia física izquierda/derecha y orientación de hélices; no invertir sin revisar mecánica y cableado.
- `Failed to install Python dependencies`, `whitespace character in project paths`: en Windows usar ruta de proyecto y directorio de herramientas PlatformIO sin espacios (este problema ocurrió en la puesta en marcha original).
- El monitor del maestro usa `Console` de Bluepad32; no confundirlo con `Serial` del proyecto Arduino estándar del esclavo.

### Referencias

- Plantilla Bluepad32: https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template
- Bluepad32 API Arduino: https://bluepad32.readthedocs.io/en/latest/programmers_guide_arduino/
- Digi XBee-PRO S1 legacy: https://docs.digi.com/resources/documentation/digidocs/pdfs/90000982.pdf
- ESP32Servo: https://github.com/madhephaestus/ESP32Servo
