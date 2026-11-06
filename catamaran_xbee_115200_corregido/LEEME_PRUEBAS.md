# Catamarán – PS4 + ESP32 maestro + XBee S1 PRO + ESP32 esclavo + dos ESC

## Lo que NO se debe modificar

**NO reconfigures los XBee en XCTU.** Se respeta tu configuración existente de PAN ID, canal, destinos, direcciones, modo transparente, etc. Ambos módulos ya trabajaban con tus códigos originales.

Conexión original en **ambas ESP32**: RX2 = GPIO16 conectado a DOUT del XBee; TX2 = GPIO17 conectado a DIN del XBee; masa común; alimentación XBee mediante regulador de 3,3 V capaz de soportar su consumo. Baud de XBee en ambos códigos: **115200, 8N1**. El monitor USB también va a 115200, pero es un puerto distinto.

## Qué cambió en el software

Se conserva **exactamente la estructura heredada de 4 bytes**: `[0xFF, ángulo, velocidad, (ángulo+velocidad)&0xFE]`. En el programa original, ángulo estaba entre 0 y 180 y velocidad entre 0 y 253. Ahora velocidad 126 = paro y 253 = máxima orden de prueba; el esclavo **solo habilita avance**. Se reservan dos ángulos fuera del rango usado por el joystick: 253 indica gatillos presionados sin empuje neto (mantener apagado, no rearmar); 254 indica PS4 ausente (mantener apagado, no rearmar). Estas reservas NO cambian XCTU y permiten diferenciar reposo real de falta de mando.

## Archivos

- `MAESTRO_XBEE_PS4.cpp` → copiar el contenido a **D:/Catamaran_Bluepad32/main/sketch.cpp**, conservando el resto de la plantilla Bluepad32 (main.c, components, submódulos, CMakeLists.txt y platformio.ini). No instalar la biblioteca Bluepad32 para NINA-W10.
- `ESCLAVO_DOBLE_ESC.cpp` → copiar a **D:/Catamaran_esclavo/src/main.cpp**.
- `platformio_esclavo.ini` → copiar sus líneas al `platformio.ini` del esclavo, si no existe una configuración válida.

## Comprobación antes de cargar

Confirmar que ambos archivos tienen `XBEE_BAUD = 115200` y `XBEE_RX = 16`, `XBEE_TX = 17`. Verificar también **que la salida de compilación sea SUCCESS y que Upload termine correctamente**: guardar un archivo no actualiza la ESP32.

## Secuencia sugerida

1. **Sin hélices y sin alimentar los ESC de potencia**, conectar solo la ESP32 esclava por USB a la computadora.
2. Desde terminal **PowerShell**, dentro de `D:/Catamaran_esclavo`, cerrar cualquier monitor previo con Ctrl+C y ejecutar:

   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32doit-devkit-v1 -t upload
   & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor -b 115200
   ```

   Si la placa aparece en puerto diferente, usar `-p COM4` sustituyendo COM4 por el puerto correcto.
3. Repetir desde `D:/Catamaran_Bluepad32` con la **ESP32 maestra** (desconectar el esclavo del USB durante la carga para evitar elegir el COM incorrecto):

   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32dev -t upload
   & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor -b 115200
   ```

4. Encender **ambas ESP32** con sus XBee (una puede estar alimentada por un cargador USB regulado), emparejar el mando PS4. Dejar el monitor serie del esclavo abierto.
5. Observar `Paquetes:` creciendo y `Radio:OK`, luego `PS4:OK`. Con gatillos sueltos por 700 ms debe aparecer `Habilitado:SI`. Con R2 aumenta `VEL`; con L2 se reduce; con el joystick horizontal cambia `ANG`. Si no hay paquetes, `Radio:PERDIDA` y ESC_L=ESC_R=1000.

## Diagnóstico (SIN tocar XCTU)

- `TX=...` **crece en el maestro**, pero `Paquetes:0` en esclavo → verificar primero que ambos programas fueron subidos, la conexión cruzada RX/TX, GND, alimentación XBee y que la radio original estaba en modo transparente. Para comprobar una configuración ya existente, **leer** XCTU sin escribir cambios.
- `Paquetes:` crece y `PS4:SIN SENAL` → emparejar el mando y verificar que el maestro muestre `PS4:OK`.
- `Radio:OK PS4:OK Habilitado:NO` → soltar R2 y L2 por 700 ms. Si fueron accionados a la vez, no bastará con que sus fuerzas se cancelen: la nueva trama reservada 253 conserva el bloqueo.
- `Errores:` crece mucho → llegan bytes, pero fallan la validación; revisar conexión o confirmar que **ambos** tienen esta misma versión del programa. A diferencia del prototipo previo de 8 bytes `[A5,5A,...]`, este código **recibe el protocolo anterior de 4 bytes**.
- `Radio:PERDIDA` con `Paquetes:>0` → se recibieron datos antes pero dejaron de llegar; revisar alimentación, distancia, módulos o si el maestro se apagó.

## Control de motores y límites

- **R2:** acelera ambos motores de forma progresiva (rampa 12 µs/20 ms).
- **L2:** reduce la orden de empuje solicitada por R2. **No es frenado activo ni inversión de giro.**
- **Joystick izquierdo horizontal:** reduce el empuje de uno de los motores para girar. En centro ambos empujan por igual. Sin avance, no gira sobre sí mismo.
- **Pérdida de enlace:** orden de mínimo 1000 µs al cumplirse 400 ms sin trama válida.
- **Tras reconexión:** ambos gatillos deben estar sueltos durante 700 ms antes de permitir empuje.
- PWM ESC izquierdo = GPIO13; derecho = GPIO18. ESC unidireccionales de 50 Hz. Prueba limitada por software a un máximo de 1600 µs, no presupone calibración a 2000 µs.

**Seguridad:** antes de conectar potencia a los ESC, comprobar que ambos reciben 1000 µs al inicio y ante pérdida del maestro. Usar baterías y reguladores adecuados, compartir GND de señales, no alimentar ESC/motores por el pin 3V3 o 5V del ESP32 y probar primero sin hélices. El mínimo PWM no equivale a paro mecánico instantáneo.

## Nota de compatibilidad

Conservar los 4 bytes mantiene interoperabilidad básica con el enlace transparente que ya funcionaba. Sin embargo, el checksum heredado de 1 byte es débil: un error de bytes puede pasar la comprobación. Para uso real en agua, una versión posterior debería añadir CRC fuerte, número de secuencia y botón de paro de emergencia, **sin necesidad de reconfigurar XCTU**. De momento el propósito de estos archivos es verificar la radio y el sentido de los controles sin modificar parámetros previamente funcionales.
