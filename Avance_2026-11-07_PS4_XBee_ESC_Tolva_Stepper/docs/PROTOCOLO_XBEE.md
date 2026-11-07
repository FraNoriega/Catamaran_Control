# Protocolo de cuatro bytes: ESC, tolva y paso a paso

## 1. Enlace

XBee PRO S1 en modo transparente (`AP=0`), UART 115200 8N1. El maestro **no** usa Arduino `Serial2.write()`; utiliza el controlador nativo ESP-IDF en UART2 (`uart_write_bytes()`). El esclavo recibe con Arduino `Serial2`.

Cada comando consta de **4 bytes**, sin separadores adicionales:

| Posición | Dato |
|---|---|
| 0 | Encabezado **0xFF** |
| 1 | Ángulo **0–180**, **254 = PS4 desconectado**, o **250–252 = paso a paso** |
| 2 | Velocidad ESC **0–253**; para comandos del stepper **126** |
| 3 | `((byte1 + byte2) & 0xFE) \| bit0`, donde bit0=1 significa **tolva abierta** en tramas de motores; bit0=0 en tramas stepper |

El byte 3 conserva el checksum de los bits altos y utiliza el bit menos significativo para la tolva; el control de la tolva se interpreta **solo** en tramas de motores. El checksum no es un CRC robusto; puede aceptar ciertas corrupciones. No usar este formato sin protecciones adicionales en una misión crítica.

## 2. Tramas de motores + tolva

- Motor en reposo con joystick centrado: byte1 suele ser **89** por el redondeo de `map()`; byte2 **126**.
- X presionado: cambia el bit0 del byte3; el esclavo exige **dos órdenes válidas de apertura**.
- Desconexión PS4: byte1 **254**, byte2 **126**, bit0=0; el esclavo ordena parar y cerrar.
- Motor/tolva se actualizan al cambiar el mando o al menos cada **50 ms**.

Ejemplos:

| Acción | HEX bytes | Explicación |
|---|---|---|
| Neutro, tolva cerrada | `FF 59 7E D6` | 89+126=215; 215 & FE = 214 (`D6`) |
| Neutro, tolva abierta | `FF 59 7E D7` | Misma suma, bit0=1 |
| PS4 desconectado | `FF FE 7E 7C` | (254+126)=380; 0x7C tras máscara 8 bits |

## 3. Tramas del paso a paso

Se reservan valores de «ángulo» fuera del rango 0–180, sin interferir con motores/tolva:

| Cruceta | Byte1 | Byte2 | Byte3 | Trama |
|---|---|---|---|---|
| Arriba / subir | 250 (`FA`) | 126 (`7E`) | 120 (`78`) | `FF FA 7E 78` |
| Abajo / bajar | 251 (`FB`) | 126 (`7E`) | 120 (`78`) | `FF FB 7E 78` |
| Soltar / parar | 252 (`FC`) | 126 (`7E`) | 122 (`7A`) | `FF FC 7E 7A` |

El maestro transmite comandos stepper al cambiar el estado o cada **100 ms**. El esclavo deja de mover el motor si pasan más de **250 ms sin orden stepper**, si caduca el enlace general de **400 ms** o si recibe desconexión de PS4.

## 4. Receptor del esclavo

El esclavo examina paquetes de cuatro bytes con validación de encabezado/checksum y tipo de comando. `RX_M` cuenta tramas válidas de motores y tolva, `RX_P` comandos de paso a paso; `ERR` cuenta paquetes rechazados. En los ensayos con la versión nueva se observó `RX_M` y `RX_P` creciendo con `ERR:0`.

> Importante: un `TX_M` o `TX_P` que aumenta en la maestra confirma transmisión por UART local, **no garantiza** recepción RF. Siempre comprobar el contador correspondiente en la esclava.
