# Instalar y ejecutar desde otra computadora Windows (VS Code + PlatformIO)

## Objetivo

Reproducir **ambas ESP32 exclusivamente en PlatformIO**. El maestro usa **ESP-IDF + Arduino + Bluepad32**, y el esclavo usa **Arduino + ESP32Servo** dentro de PlatformIO. No es necesario utilizar Arduino IDE.

**Aviso de reproducibilidad:** el proyecto maestro original utiliza la plantilla de Bluepad32 con componentes/submódulos. Este paquete incluye el código de aplicación y una referencia de la configuración, **no todos los archivos del framework ni el árbol completo de la plantilla**. Para una reproducción idéntica, clona tu repositorio completo (incluyendo `.gitmodules` y hashes de submódulos), o usa la plantilla del autor y adapta la configuración como se indica más abajo.

## 1. Instalar programas

En la laptop nueva instalar:

1. **Git para Windows** (incluye `git clone`, `git status`, `git push`). https://git-scm.com/downloads/win
2. **Visual Studio Code**. https://code.visualstudio.com/
3. Extensión **PlatformIO IDE** desde el panel Extensiones de VS Code. https://platformio.org/install/ide?install=vscode
4. Los controladores USB-UART necesarios para las placas ESP32 si los puertos COM no aparecen (pueden variar: CP210x/CH340). No instalar un controlador que no corresponda al chip físico.

Abrir PlatformIO Home y completar su inicialización. Si el nombre de usuario de Windows contiene espacios y PlatformIO se queja de la ruta de los paquetes, se puede usar desde el inicio un directorio de Core sin espacios:

```powershell
$env:PLATFORMIO_CORE_DIR = "C:\PIO_Core"
New-Item -ItemType Directory -Force -Path $env:PLATFORMIO_CORE_DIR | Out-Null
```

**Mantener el mismo `PLATFORMIO_CORE_DIR`** en todas las terminales posteriores. En el equipo donde se hizo la prueba se utilizó `D:\PlatformIO_Data`; esta ruta puede no existir en otra laptop.

## 2. Clonar el código

### Opción A — El repositorio GitHub contiene los dos proyectos completos

```powershell
cd C:\Proyectos
git clone --recurse-submodules <URL-DEL-REPOSITORIO> Catamaran
git -C C:\Proyectos\Catamaran submodule update --init --recursive
```

Identificar en ese repositorio las carpetas completas del proyecto maestro y del esclavo. Si el repositorio incluye solo esta entrega de firmware y documentación, usar la opción B.

### Opción B — Dispones solamente de este paquete documental

Clonar la **plantilla oficial** Bluepad32 con sus submódulos para montar el proyecto maestro:

```powershell
cd C:\Proyectos
git clone --recursive https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template.git Catamaran_Bluepad32
```

**Importante:** la rama más reciente de la plantilla puede evolucionar respecto de la utilizada originalmente. La instalación comprobada usaba PioArduino `55.03.31-2` y ESP-IDF 5.5.1. Si la plantilla nueva compila de forma diferente, debe recuperarse el commit de la plantilla y de los submódulos del proyecto original; no basta con reemplazar `sketch.cpp`.

Copiar el maestro del paquete hacia la plantilla:

```powershell
Copy-Item "C:\Proyectos\Entrega_Catamaran_PlatformIO_Git\firmware\maestro\sketch.cpp" `
  "C:\Proyectos\Catamaran_Bluepad32\main\sketch.cpp" -Force
```

Revisar `platformio.ini` original del maestro: **`src_dir=main`, `framework=espidf`, entorno `[env:esp32dev]`**, y conservar todas las dependencias y `CMakeLists.txt` de la plantilla. Consultar `firmware/maestro/platformio.ini.RECUERDA.md`.

Crear el proyecto esclavo copiando la carpeta `firmware/esclavo`:

```powershell
Copy-Item "C:\Proyectos\Entrega_Catamaran_PlatformIO_Git\firmware\esclavo" `
  "C:\Proyectos\Catamaran_esclavo" -Recurse -Force
```

La estructura final debe ser:

```text
C:\Proyectos\Catamaran_Bluepad32\
    platformio.ini
    main\sketch.cpp
    main\CMakeLists.txt
    components\, external\, patches\, etc. (de la plantilla)

C:\Proyectos\Catamaran_esclavo\
    platformio.ini
    src\main.cpp
```

El `platformio.ini` de la esclava ya está en el paquete e incluye:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32@6.5.0
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
lib_deps =
    madhephaestus/ESP32Servo @ ^3.0.0
```

## 3. Localizar PlatformIO en PowerShell

Abre una terminal PowerShell de VS Code. Comprueba la ubicación:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
if (-not (Test-Path $pio)) {
    $pio = (Get-Command pio -ErrorAction Stop).Source
}
& $pio --version
& $pio device list
```

Si `pio` no se encuentra, termina de instalar la extensión PlatformIO y reinicia VS Code.

**Nota:** las rutas `COM3` y `COM7` del computador original **son ejemplos**, no valores universales. Asigna los puertos identificados por `device list` a cada placa.

## 4. Compilar y subir a la ESP32 MAESTRA

Desde la terminal correspondiente al maestro:

```powershell
$env:PLATFORMIO_CORE_DIR = "C:\PIO_Core"
cd C:\Proyectos\Catamaran_Bluepad32
& $pio run -e esp32dev
& $pio run -e esp32dev -t upload --upload-port COM3
& $pio device monitor -p COM3 -b 115200
```

La compilación inicial descargará dependencias y puede tomar más tiempo. `Console.println`/`Console.printf` del maestro se ven en este monitor, **no en `Serial`**. Para subir otra vez el programa, cerrar el monitor con `Ctrl + C`.

En el inicio esperamos `[UART] Driver:ESP_OK | Config:ESP_OK | Pines:ESP_OK` y `[XBEE] UART LISTA`.

**Si el compilador dice que falta `driver/uart.h` / `esp_driver_uart`:** el proyecto ESP-IDF necesita declarar la dependencia de ese componente en `main/CMakeLists.txt`, de acuerdo con la versión del framework. No reemplazar a ciegas `CMakeLists.txt` de la plantilla.

## 5. Compilar y subir a la ESP32 ESCLAVA

En otra terminal PowerShell:

```powershell
$env:PLATFORMIO_CORE_DIR = "C:\PIO_Core"
cd C:\Proyectos\Catamaran_esclavo
& $pio run -e esp32doit-devkit-v1
& $pio run -e esp32doit-devkit-v1 -t upload --upload-port COM7
& $pio device monitor -p COM7 -b 115200
```

Se usa `Serial` de Arduino en la esclava. Verificar `XBEE:OK` y que el contador `RX_OK` aumente mientras el mando está conectado.

## 6. Conectar el mando PS4

- Si ya está emparejado, encender ambas ESP32 y presionar **PS**; durante las pruebas se reconectó así, sin reinicio físico.
- Si no se empareja, activar modo de emparejamiento con **SHARE + PS**, según el estado del mando.
- El pequeño botón físico trasero es un **reinicio de recuperación**, no parte del uso diario.
- No activar `BP32.forgetBluetoothKeys()` en `setup()` porque elimina claves de emparejamiento previas.

## 7. Validar funcionamiento con seguridad

Antes de alimentar los motores: retirar hélices y verificar que las dos ESP32 comparten enlace XBee con el mando conectado. La fuente externa regulada del servo debe tener GND común con la esclava. El esclavo ordena inicialmente ESC a 1000 µs.

Ver resultados esperados y casos de prueba en [PROTOCOLO_Y_COMPROBACION.md](PROTOCOLO_Y_COMPROBACION.md). No realizar pruebas en agua ni con hélices hasta validar los fallos seguros por pérdida de mando y radio.

## 8. Resolución de problemas frecuentes

| Síntoma | Diagnóstico recomendado |
|---|---|
| `pio` o `platformio.exe` no encontrado | Instalar/inicializar PlatformIO, consultar `Get-Command pio`, comprobar `penv/Scripts`. |
| Error de ruta con espacios en Windows | Configurar `PLATFORMIO_CORE_DIR` a `C:\PIO_Core` **antes de instalar dependencias**; usar proyecto sin espacios en su ruta. |
| `esptool: unrecognized arguments --flash-mode` | Revisar **la versión cargada** de `esptool`, no solo `pip show`: `python -m esptool version` y `import esptool; print(esptool.__file__)`. En el PC original se importaba 4.5.1 desde una instalación editable vieja; se reparó y funcionó 5.1.0. No borrar paquetes de forma indiscriminada. |
| `esptool version` dice módulo ausente | Puede haber una instalación editable apuntando a carpeta movida; reinstalar `esptool` correctamente en el entorno Python afectado. No es necesaria una reinstalación total de PlatformIO. |
| `TX_IDF` aumenta pero `RX_OK` se queda fijo | `TX_IDF` confirma el envío local. Revisar `UART` nativa de maestro (no usar `Serial2.write` allí), código y firmware realmente flasheado, y solo luego el resto del enlace. |
| No sale `PS4:OK` | Verificar placa compatible Bluepad32, emparejamiento y claves. Evitar olvidar claves en cada inicio. |
| Servo interfiere con ESC / se reinicia ESP32 | Utilizar fuente regulada independiente para servo, GND común y cables de alimentación apropiados. |
| Tolva responde tarde | Verificar `PERIODO_TX=50` y que el esclavo usa 2 confirmaciones de apertura. La demora restante depende de la mecánica del servo. |
| Programa compila pero se observa código viejo | Guardar `Ctrl+S`, subir el firmware al **COM correcto**, confirmar `SUCCESS` y reiniciar placa. |

## Fuentes oficiales

- Bluepad32 ESP-IDF + Arduino en PlatformIO: https://bluepad32.readthedocs.io/en/latest/plat_arduino/
- Plantilla upstream y submódulos: https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template
- UART ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html
