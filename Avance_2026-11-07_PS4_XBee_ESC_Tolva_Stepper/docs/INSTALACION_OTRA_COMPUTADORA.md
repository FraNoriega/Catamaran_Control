# Instalar, compilar y ejecutar en otra laptop Windows

**Meta:** ejecutar las **dos ESP32 desde VS Code + PlatformIO**, conservando Bluepad32, UART nativa ESP-IDF, enlace XBee, dos brushless, tolva y 28BYJ-48. **No necesitas Arduino IDE.**

> Hay **dos proyectos de PlatformIO diferentes**. ESP32 maestra: plantilla completa **ESP-IDF con Arduino como componente y Bluepad32**. ESP32 esclava: proyecto ordinario `framework=arduino` con bibliotecas `ESP32Servo` y `AccelStepper`. La estructura de la maestra debe conseguirse de la plantilla original o de una copia completa del proyecto de prueba (incluidos submódulos), porque esta entrega contiene únicamente `sketch.cpp`.

## A. Instalar herramientas

1. **Git for Windows**: https://git-scm.com/downloads/win
2. **Visual Studio Code**: https://code.visualstudio.com/
3. Extensión **PlatformIO IDE** desde el Marketplace de VS Code. Abre su `PIO Home` y deja que termine la primera instalación.
4. **Drivers USB-serial de tu placa** si Windows no reconoce el puerto COM (puede ser CP210x, CH340 u otro; mira el chip de la placa).
5. Acceso a Internet en el primer `Build` para descargar framework y bibliotecas.

Si VS Code muestra simultáneamente extensiones «PlatformIO IDE» y «PioArduino IDE», para reproducir esta prueba mantén un solo gestor de proyectos habilitado. **La plataforma de compilación PioArduino que usa el maestro sí puede funcionar dentro de PlatformIO IDE**; no es lo mismo que una segunda extensión.

## B. Obtener los archivos del repositorio

Abre **PowerShell**. Ejemplo sin espacios en la ruta:

```powershell
New-Item -ItemType Directory -Force C:\Proyectos | Out-Null
cd C:\Proyectos
git clone https://github.com/FraNoriega/Catamaran_Control.git
```

Busca dentro del clon la carpeta de este avance: `Avance_2026-11-07_PS4_XBee_ESC_Tolva_Stepper`. Si GitHub la muestra dentro de `avances/`, usa esa ruta. No confundas el `platformio.ini` de la raíz del repositorio con el del maestro: el de la raíz corresponde a un proyecto anterior y puede tener otras funciones.

En los siguientes ejemplos asumimos que los archivos están en:

```powershell
$avance = 'C:\Proyectos\Catamaran_Control\Avance_2026-11-07_PS4_XBee_ESC_Tolva_Stepper'
```

**Ajusta `$avance` a la ruta real después de subir la carpeta a GitHub.**

## C. Configurar PlatformIO Core sin espacios (opcional)

Si tu carpeta de usuario de Windows contiene espacios y ESP-IDF falla por esa causa, usa una carpeta específica:

```powershell
New-Item -ItemType Directory -Force C:\PlatformIO_Data | Out-Null
[Environment]::SetEnvironmentVariable('PLATFORMIO_CORE_DIR','C:\PlatformIO_Data','User')
$env:PLATFORMIO_CORE_DIR = 'C:\PlatformIO_Data'
```

Cierra VS Code y vuelve a abrirlo para que recoja la variable. Una laptop que ya compila correctamente puede conservar su directorio de datos de PlatformIO; **no** cambies rutas que funcionan sin necesidad.

## D. Preparar el proyecto MAESTRO (Bluepad32)

En PowerShell:

```powershell
cd C:\Proyectos
git clone --recurse-submodules https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template.git Catamaran_Bluepad32
```

> La plantilla puede actualizarse con el tiempo. En el equipo original se utilizó una integración PioArduino ESP32 (`55.03.31-2` como referencia documentada), ESP-IDF + Arduino como componente, y Bluepad32. **La manera más fiel de reproducir** el resultado es copiar el proyecto completo que ya funcionó, incluida la versión exacta de `platformio.ini`, submódulos y `sdkconfig.defaults`. Si utilizas la plantilla más reciente, verifica compatibilidad en la primera compilación.

Copia **solo** el código del maestro de esta entrega:

```powershell
Copy-Item "$avance\firmware\maestro\sketch.cpp" `
  'C:\Proyectos\Catamaran_Bluepad32\main\sketch.cpp' -Force
```

Verifica que el proyecto maestro contenga `main/main.c`, `main/CMakeLists.txt`, `components`, `external`, `platformio.ini`, `CMakeLists.txt` y `.gitmodules` donde corresponda. **No borres esos archivos ni cambies `framework = espidf` a `arduino`.**

En la configuración del maestro debe existir el entorno **`esp32dev`**. Si aparece un error al enlazar `driver/uart.h`, revisa en `main/CMakeLists.txt` la declaración de dependencia del componente `esp_driver_uart` para la versión concreta de ESP-IDF. No borres Bluepad32 ni los submódulos.

## E. Preparar el proyecto ESCLAVO

```powershell
Copy-Item "$avance\firmware\esclavo" 'C:\Proyectos\Catamaran_esclavo' -Recurse -Force
```

Debe quedar:

```text
C:\Proyectos\Catamaran_esclavo\
├── platformio.ini
└── src\
    └── main.cpp
```

El `platformio.ini` fija la plataforma **`espressif32@6.5.0`**, la placa `esp32doit-devkit-v1`, el framework Arduino y las bibliotecas:

```ini
lib_deps =
    madhephaestus/ESP32Servo @ ^3.0.0
    waspinator/AccelStepper @ ^1.64
```

Si PlatformIO ya está instalado y tienes una plantilla válida en `D:\Catamaran_esclavo`, basta copiar `main.cpp` y agregar `AccelStepper` en `lib_deps`; **guarda una copia del firmware antiguo antes de modificarlo**.

## F. Compilar desde VS Code (recomendado)

1. **File → Open Folder** → `C:\Proyectos\Catamaran_Bluepad32`. Espera a que PlatformIO inicialice los componentes.
2. En la barra inferior de PlatformIO usa **Build**. Selecciona `esp32dev` cuando corresponda.
3. Conecta la ESP32 maestra por USB; en **Upload** selecciona su puerto COM.
4. Abre **Serial Monitor** a **115200**.
5. Abre una **segunda ventana de VS Code** con `C:\Proyectos\Catamaran_esclavo`, compila entorno `esp32doit-devkit-v1`, sube a la placa esclava y abre su monitor.
6. Antes de pulsar **Upload**, cierra el monitor serie que esté usando ese mismo puerto.

**Orden recomendado para primera prueba:** esclava y luego maestra, sin potencia de los ESC ni carga en el paso a paso.

## G. Compilar y subir por PowerShell (alternativa)

Para hallar el ejecutable, primero prueba:

```powershell
$pio = (Get-Command pio -ErrorAction SilentlyContinue).Source
if (-not $pio) { $pio = (Get-Command platformio -ErrorAction SilentlyContinue).Source }
if (-not $pio) {
    $candidatos = @(
      "$env:PLATFORMIO_CORE_DIR\penv\Scripts\platformio.exe",
      "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
    )
    $pio = $candidatos | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $pio) { throw 'PlatformIO CLI no encontrado: termina la instalación desde VS Code.' }
& $pio --version
& $pio device list
```

Anota el puerto **de cada ESP32**. En el equipo donde se ensayó eran **COM3** (maestra) y **COM7** (esclava), pero en otra laptop es normal que cambien.

**Esclava** (ejemplo con COM7):

```powershell
cd C:\Proyectos\Catamaran_esclavo
& $pio run -e esp32doit-devkit-v1
& $pio run -e esp32doit-devkit-v1 -t upload --upload-port COM7
& $pio device monitor -p COM7 -b 115200
```

**Maestra** (otra terminal, ejemplo con COM3):

```powershell
cd C:\Proyectos\Catamaran_Bluepad32
& $pio run -e esp32dev
& $pio run -e esp32dev -t upload --upload-port COM3
& $pio device monitor -p COM3 -b 115200
```

**IMPORTANTE:** en cada terminal nueva se debe volver a establecer `$pio` (el bloque de búsqueda de arriba) y, si hace falta, `$env:PLATFORMIO_CORE_DIR`. Usa `Ctrl+C` para cerrar el monitor serie antes de una nueva carga.

## H. Conectar PS4 y verificar

Presiona **PS** para reconectar el mando que ya estaba emparejado; cuando se necesita un emparejamiento nuevo, consulta las instrucciones de Bluepad32 y del mando. El firmware **no llama a `BP32.forgetBluetoothKeys()`** para evitar borrar las claves en cada reinicio.

En el monitor maestra espera, por ejemplo:

```text
[MAESTRO] PS4:OK | ANG:89 | VEL:126 | TOLVA:CERRAR | PASO:PARAR | TX_M:802 | TX_P:398
```

En el monitor esclava espera valores crecientes:

```text
[ESTADO] XBEE:OK | PS4:OK | ESC:SI | ANG:89 | VEL:126 | TOLVA:CERRADA | PASO:PARADO | POS:0 | RX_M:9959 | RX_P:869 | ERR:0
```

Los contadores son **ejemplos** de una sesión anterior, no valores iniciales obligatorios. Si los motores no están alimentados eléctricamente, `ESC:SI` significa *lógica de armado habilitada*, no que haya propulsión.

**Prueba sin carga:** cruceta arriba y abajo produce dos sentidos de giro; suelta cruceta y se detiene; X abre/cierra servo. `POS` es relativo y se reinicia a cero al encender la placa. Si `RX_P:0` pero `RX_M` crece, confirma que **el maestro tenga el programa NUEVO guardado y subido** (con `PASO`, `TX_M` y `TX_P`).

## I. Solucionar problemas frecuentes

| Problema | Acción recomendada |
|---|---|
| Error de proyecto al instalar PioArduino IDE | Usa PlatformIO IDE con los entornos anteriores; no necesitas ambas extensiones a la vez |
| ESP-IDF falla por espacios en ruta | Usa `C:\Proyectos` y, si procede, `C:\PlatformIO_Data` |
| Bluepad32 no compila en proyecto Arduino común | Abre la **plantilla ESP-IDF completa**, no el `.cpp` aislado |
| Fallos `esptool: unrecognized arguments --flash-mode` | Hubo un conflicto de versión en el equipo original. Revisar qué `esptool` está invocando la plataforma PioArduino y los paquetes instalados; no copiar parches a ciegas |
| `XBEE:PERDIDO` | Verificar que maestro y esclavo tengan los códigos correctos y XBee 115200. No cambiar XCTU sin motivo |
| `RX_M` aumenta pero `RX_P:0` | La maestra puede seguir ejecutando firmware antiguo: guardar y **volver a subir** a COM correspondiente |
| Los botones cambian en monitor pero motor no gira | Comprobar ULN2003 5 V, tierra compartida y pines IN1..IN4 |
| Servo tiembla / lentitud | Mantener fuente regulada independiente del ESC que produjo ruido |
| Puerto ocupado al subir | Cerrar Serial Monitor de ese puerto con `Ctrl+C` |

## J. Límites de reproducibilidad

Los códigos están tomados de la versión del proyecto que se confirmó funcional mediante monitores y pruebas descritas, pero los **binarios exactos**, las bibliotecas cacheadas, los submódulos y el `platformio.ini` completo **del maestro original** no están en esta carpeta. Para reproducibilidad estricta, archivar desde la laptop original el proyecto completo `Catamaran_Bluepad32` con `.gitmodules`, `git submodule status` y versión de plataforma, además del proyecto esclavo. No subir a GitHub carpetas `.pio`, `build` ni secretos.
