# Protocolo inalámbrico, interpretación y pruebas

## La trama de cuatro bytes

```text
Byte 0       Byte 1           Byte 2         Byte 3
0xFF         angulo           velocidad      ((angulo + velocidad) & 0xFE) | tolva
cabecera     0..180 o 254    0..253         bit0 = 0/1
```

- `angulo`: 0–180 según joystick. `254` indica **mando desconectado**, y debe ir con velocidad `126`.
- `velocidad`: 0–253. La zona neutral está alrededor de `126`, con R2 aumentando el valor y L2 reduciéndolo.
- `tolva`: 0 = cerrado; 1 = solicitado abrir mientras X esté pulsado.
- `checksum` ocupa bits 7..1 del cuarto byte. El bit 0 tiene un uso diferente, por lo que el receptor valida `(chk & 0xFE) == ((ang + vel) & 0xFE)`.
- **No se modifica XBee XCTU.** Los radios actúan como enlace transparente de bytes.

## Ejemplos de tramas

| Modo | Hexadecimal | Interpretación |
|---|---|---|
| Centrado, tolva cerrada | `FF 59 7E D6` | Ángulo 89°, velocidad 126, cerrado |
| Centrado, tolva abierta | `FF 59 7E D7` | Ángulo 89°, velocidad 126, abierto |
| Sin mando PS4 | `FF FE 7E 7C` | Estado especial de desconexión, cerrar/paro |

## Indicadores del monitor

**Maestro (`Console`, COM3 en pruebas):**

```text
[UART] Driver:ESP_OK | Config:ESP_OK | Pines:ESP_OK
[MAESTRO] PS4:OK | ANG:89 | VEL:126 | TOLVA:CERRAR | TX_IDF:979
```

**Esclavo (`Serial`, COM7 en pruebas):**

```text
[ESCLAVO] XBEE:OK | PS4:OK | ARMADO:SI | ANG:89 | VEL:126 | ESC_L:1000 | ESC_R:1000 | TOLVA:CERRADA | RX_OK:4437 | ERRORES:7
```

`TX_IDF` creciente **no basta** para confirmar comunicación inalámbrica. `RX_OK` debe seguir aumentando al mover el mando y los valores deben corresponderse. `ERRORES` es acumulado desde el arranque y puede mantenerse igual si no hay errores nuevos.

## Prueba sugerida de aceptación, sin hélices

1. Compilar y cargar ambos proyectos **desde PlatformIO**. Mantener desconectada la potencia de los ESC durante la primera verificación de enlace.
2. Abrir los dos monitores 115200, emparejar el PS4 con el botón PS.
3. Confirmar `PS4:OK`, `XBEE:OK` y `RX_OK` creciente sin necesidad de reiniciar físicamente el mando.
4. Mover joystick: observar cambio de ángulo en ambos monitores.
5. Pulsar R2/L2: observar cambio de velocidad recibida.
6. Alimentar el **servo solo desde su fuente regulada externa** y probarlo sin carga: presionar X y soltar X.
7. Comprobar la pérdida de señal y su recuperación (primero sin alimentar los ESC). Solo después evaluar los motores, fijados y sin hélices.

## Límites del protocolo y seguridad

El checksum es de uso didáctico, no un CRC robusto. XBee broadcast puede perder mensajes. La parada después de 400 ms depende de que la placa esclava, su temporizador y el ESC sigan operativos. Para aplicaciones reales debe contemplarse un paro independiente, prueba de watchdog y alimentación estable.
