# Catamarán — Control inalámbrico con ESP32 y DualShock 4 (PS4)

**Estado de la prueba:** comunicación Bluetooth validada ✅  
**Placa utilizada:** ESP32 DOIT DevKit V1 / ESP32 clásico (`esp32dev`)  
**Entorno:** Windows + Visual Studio Code + PlatformIO + ESP-IDF + Arduino como componente + Bluepad32  
**Última comprobación:** 6 de noviembre de 2026

> Esta etapa comprueba la **conexión del control PS4 y la recepción de botones**. No implica que los ESC, motores o sistemas de propulsión del catamarán hayan sido ensayados. Durante las pruebas de software, mantener los motores desconectados.

## Índice

1. [Objetivo y resultado obtenido](#1-objetivo-y-resultado-obtenido)
2. [Requisitos para una computadora nueva](#2-requisitos-para-una-computadora-nueva)
3. [Descargar el proyecto y sus submódulos](#3-descargar-el-proyecto-y-sus-submódulos)
4. [Configurar PlatformIO](#4-configurar-platformio)
5. [Código y estructura del proyecto](#5-código-y-estructura-del-proyecto)
6. [Compilar, cargar y monitorear](#6-compilar-cargar-y-monitorear)
7. [Emparejar el control PS4](#7-emparejar-el-control-ps4)
8. [Solución de errores conocidos](#8-solución-de-errores-conocidos)
9. [Publicar y mantener el proyecto en GitHub](#9-publicar-y-mantener-el-proyecto-en-github)
10. [Referencias y notas](#10-referencias-y-notas)

## 1. Objetivo y resultado obtenido

Este repositorio contiene el proyecto de prueba que permite al ESP32 **detectar un control Sony DualShock 4 mediante Bluetooth** e imprimir mensajes cuando se presiona el botón **X (cruz)**.

La prueba ejecutada en la placa física mostró:

```text
Control conectado!
Indice: 0
Modelo: DualShock 4
Boton X (Cruz) presionado -> Comunicacion OK!
```

La compilación del entorno `esp32dev` terminó con `SUCCESS`, se cargó el firmware al ESP32 y el monitor serie mostró eventos reales del mando. **No confundir un `SUCCESS` de compilación con la prueba de conexión:** ambas verificaciones deben hacerse.

### Configuración que funcionó

| Elemento | Configuración de referencia |
|---|---|
| Microcontrolador | ESP32 DOIT DevKit V1 / ESP32 clásico |
| Entorno de PlatformIO | `esp32dev` |
| Plataforma | PioArduino `55.03.31-2` (fijada en `platformio.ini`) |
| Framework | `espidf` (con Arduino como componente) |
| ESP-IDF observado en la compilación | `5.5.1` |
| Biblioteca para mandos | Bluepad32 nativa en el proyecto, **no** *Bluepad32 for NINA-W10 boards* |
| Velocidad del monitor serie | `115200` baudios |
| Equipo de pruebas | Windows con VS Code, PlatformIO y Git |

Estas versiones son las que aparecieron en la experiencia documentada; para repetirla, **conservar el `platformio.ini` y los submódulos del repositorio**. No es necesario instalar manualmente Arduino IDE ni añadir Bluepad32 en `lib_deps`.

## 2. Requisitos para una computadora nueva

- **Windows 10/11**, conexión a Internet (para la primera instalación) y acceso a un puerto USB.
- [Visual Studio Code](https://code.visualstudio.com/).
- Extensión **PlatformIO IDE** para VS Code.
- [Git for Windows](https://git-scm.com/downloads/win) para clonar el proyecto con submódulos.
- Cable **USB de datos** y un **ESP32 clásico** (para reproducir exactamente esta prueba).
- Control **DualShock 4 (PS4)** con batería.

**No es obligatorio** instalar la extensión ESP-IDF ni escoger un *CMake Kit* para compilar **este proyecto mediante PlatformIO**. Si ya tienes esas extensiones instaladas, evita mezclar sus terminales y configuraciones con las tareas de PlatformIO.

### Evitar rutas con espacios

ESP-IDF puede fallar si encuentra espacios en rutas relevantes. En la prueba inicial se detectó:

```text
Error: Detected a whitespace character in project paths.
```

Usa, por ejemplo, una ruta de proyecto **sin espacios**:

```text
D:\Catamaran_Bluepad32
```

Si tu computadora no tiene disco `D:`, puedes usar `C:\Proyectos\Catamaran_Bluepad32`.

También conviene fijar una carpeta de datos de PlatformIO sin espacios, **en cada computadora**, especialmente si el nombre de usuario de Windows contiene espacios. En PowerShell:

```powershell
New-Item -ItemType Directory -Force 'C:\PlatformIO_Data'
$env:PLATFORMIO_CORE_DIR = 'C:\PlatformIO_Data'
[Environment]::SetEnvironmentVariable('PLATFORMIO_CORE_DIR', 'C:\PlatformIO_Data', 'User')
```

Cierra y vuelve a abrir VS Code para que recoja la variable persistente. Si ya utilizas otra unidad, puedes elegir `D:\PlatformIO_Data`. Verifica la ruta con **PlatformIO Core CLI** o:

```powershell
pio system info
```

Busca `Core Directory`. Si `pio` no se reconoce, utiliza **PlatformIO IDE → PROJECT TASKS** para las tareas de compilación, o la ruta del ejecutable realmente instalado (véase la sección 6). **No guardes una ruta absoluta `core_dir` específica de tu computadora en el `platformio.ini` compartido**, porque haría menos portable el repositorio.

## 3. Descargar el proyecto y sus submódulos

### Opción A — Clonar este proyecto desde tu propio GitHub (recomendado)

Una vez publicado el repositorio, reemplaza `TU_USUARIO` por el propietario real:

```powershell
cd D:\
git clone --recurse-submodules https://github.com/TU_USUARIO/Catamaran_Bluepad32.git
cd D:\Catamaran_Bluepad32
```

**Importante:** la URL de arriba es una **plantilla** y no representa un repositorio que ya esté publicado. Copia la URL real del botón **Code → HTTPS** de tu repositorio.

Si clonaste sin `--recurse-submodules`, ejecuta dentro del proyecto:

```powershell
git submodule update --init --recursive
```

### Opción B — Comenzar desde la plantilla oficial

Si todavía no has publicado tu proyecto, puedes descargar la plantilla que sirvió de base:

```powershell
cd D:\
git clone --recursive https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template.git Catamaran_Bluepad32
cd D:\Catamaran_Bluepad32
```

**Esta opción descarga la plantilla original**, no tu código personalizado. Después deberás llevar allí tu versión funcional de `main/sketch.cpp` y conservar la configuración probada.

Luego, en VS Code, abre **File → Open Folder** y selecciona la carpeta completa del proyecto, **no únicamente** el archivo `.cpp`.

## 4. Configurar PlatformIO

Instala la extensión **PlatformIO IDE** desde el Marketplace de VS Code. Asegúrate de abrir como proyecto la carpeta que contiene `platformio.ini`.

La estructura esencial de `platformio.ini` usada en la prueba fue:

```ini
[platformio]
src_dir = main

[env]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.31-2/platform-espressif32.zip
framework = espidf
monitor_speed = 115200
monitor_filters = direct

[env:esp32dev]
board = esp32dev
```

**No reemplaces el archivo real por este fragmento:** el archivo original de la plantilla incluye otras configuraciones (`board_build.embed_txtfiles` y entornos adicionales). Lo anterior sirve para reconocer la configuración principal. En la primera compilación PlatformIO descargará el framework y las herramientas necesarias.

**Advertencia clave:** **no** añadir esta dependencia al proyecto:

```ini
; INCORRECTO para este ESP32 con la plantilla nativa:
; lib_deps = https://github.com/ricardoquesada/bluepad32-arduino.git
```

Eso llevó a resolver la biblioteca **Bluepad32 for NINA-W10 boards**, cuya interfaz y archivos SPI no corresponden al uso de Bluetooth nativo en el ESP32 probado.

## 5. Código y estructura del proyecto

Estructura principal:

```text
Catamaran_Bluepad32/
├── platformio.ini           # Configuración de PlatformIO
├── CMakeLists.txt           # Proyecto ESP-IDF
├── sdkconfig.defaults       # Valores base ESP-IDF / Bluepad32
├── .gitmodules              # Registro de submódulos Git
├── components/              # Componentes (incl. Arduino y Bluepad32)
├── external/                # Dependencias externas (incl. BTstack)
└── main/
    ├── CMakeLists.txt       # Incluye main.c y sketch.cpp
    ├── main.c               # Inicialización de Bluepad32 / BTstack
    └── sketch.cpp           # Lógica de conexión y lectura del mando
```

**No borres** `main/main.c`, los `CMakeLists.txt`, `components/` ni `external/`. Son parte de la integración que permitió compilar Bluepad32 con ESP-IDF.

### Ejemplo mínimo de `main/sketch.cpp`

Este es el programa de referencia para verificar la conexión y el botón X. Si tu repositorio ya contiene una versión funcional, **consérvala**; este ejemplo es para comparar o restaurar la prueba básica.

```cpp
#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            Console.println("=========================");
            Console.println("Control conectado!");
            Console.printf("Indice: %d\n", i);
            Console.printf("Modelo: %s\n", ctl->getModelName());
            Console.println("=========================");
            break;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            Console.println("Control desconectado!");
            break;
        }
    }
}

void setup() {
    Console.println("Iniciando ESP32...");
    Console.println("Buscando control PS4...");
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableVirtualDevice(false);
}

void loop() {
    bool dataUpdated = BP32.update();
    if (dataUpdated) {
        for (auto ctl : myControllers) {
            if (ctl && ctl->isConnected() && ctl->hasData() && ctl->isGamepad()) {
                if (ctl->a()) {
                    Console.println("Boton X (Cruz) presionado -> Comunicacion OK!");
                }
            }
        }
    }
    delay(15);
}
```

Se utiliza `Console` en lugar de `Serial`, porque la consola interactiva de Bluepad32 está habilitada en la configuración de referencia. El método `ctl->a()` corresponde al botón X del DualShock 4 en la asignación utilizada. Si mantienes presionado X, es normal ver varios mensajes seguidos: la lectura se repite mientras está pulsado.

> En esta versión de Bluepad32 nativa, `BP32.update()` puede devolver `bool` y `Controller` dispone de `hasData()`. Los errores previos de esos métodos correspondían a una **biblioteca incompatible**, no a un fallo de la lógica del mando.

## 6. Compilar, cargar y monitorear

Abre una terminal **PowerShell** en VS Code, situada en la carpeta del proyecto. **No uses ESP-IDF Terminal** para estos comandos de PlatformIO.

### 6.1 Comprobar la instalación

Si tienes la CLI de PlatformIO disponible en `PATH`:

```powershell
pio --version
```

Si `pio` no se reconoce, usa las tareas de la extensión (**PlatformIO → PROJECT TASKS → esp32dev**) o localiza `platformio.exe` en tu instalación. En la computadora original se utilizó:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" --version
```

La ruta del ejecutable **puede variar entre computadoras**. En adelante, los ejemplos usan `pio` para simplificar; si no lo tienes en `PATH`, reemplázalo por `& "RUTA_REAL\platformio.exe"` o usa los botones de PlatformIO.

### 6.2 Compilar (Build)

```powershell
pio run -e esp32dev
```

Equivalente gráfico: **PlatformIO → PROJECT TASKS → esp32dev → General → Build**.

Resultado esperado al final:

```text
Environment    Status
esp32dev       SUCCESS
```

El binario suele quedar en:

```text
.pio\build\esp32dev\firmware.bin
```

**Observación:** durante la compilación se vieron advertencias de Kconfig de BTstack y sobre certificados de RainMaker, pero **no impidieron** la creación correcta de `firmware.bin`. Una advertencia no significa `SUCCESS` ni debe ocultar otros errores.

### 6.3 Conectar el ESP32 y cargar (Upload)

Conecta el ESP32 mediante un **cable USB de datos**. En esta etapa mantén desconectados los ESC y motores.

```powershell
pio run -e esp32dev -t upload
```

Si no detecta el puerto:

```powershell
pio device list
```

Identifica el puerto (por ejemplo, `COM5`, **solo como ejemplo**) y configúralo en `platformio.ini` si es necesario:

```ini
[env:esp32dev]
board = esp32dev
; upload_port = COM5    ; descomentar y adaptar únicamente si falla la detección
```

En ciertas placas puede ser necesario mantener presionado **BOOT** durante el inicio de la carga y soltarlo cuando comience la escritura.

### 6.4 Abrir el monitor serie

```powershell
pio device monitor -b 115200
```

Si hay más de un puerto, especifica el correcto:

```powershell
pio device monitor -p COM5 -b 115200
```

Cierra el monitor antes de un nuevo `Upload`, porque el puerto COM solo debe estar ocupado por un programa a la vez. Sal del monitor con `Ctrl+C`.

## 7. Emparejar el control PS4

1. Enciende el ESP32 con el firmware cargado y abre el monitor serie a **115200**.
2. Mantén presionados **SHARE + PS** en el mando hasta que la barra luminosa parpadee rápidamente.
3. Observa el monitor hasta que aparezcan `Control conectado!` y `Modelo: DualShock 4`.
4. Presiona **X (cruz)**. Debe aparecer `Boton X (Cruz) presionado -> Comunicacion OK!`.

En la prueba también aparecieron mensajes de diagnóstico internos del mando como:

```text
Device is ready
DS4: Calibration report received
DS4: Failed to create virtual device
```

El último mensaje **no impidió la conexión en esta prueba concreta**, porque inmediatamente se reconoció `DualShock 4` y se recibieron pulsaciones. Si no aparece `Control conectado!` o no llegan los botones, entonces sí hay que investigar el emparejamiento.

**No ejecutar `BP32.forgetBluetoothKeys()` automáticamente en cada arranque**: borra vínculos Bluetooth anteriores y puede obligar a emparejar de nuevo. Úsalo solo si necesitas restablecer las claves.

## 8. Solución de errores conocidos

Esta sección recoge **errores que aparecieron durante el proceso real** y cómo diagnosticarlos en otra computadora.

### 8.1 `Bluepad32 for NINA-W10 boards` / `PINS_COUNT` / `NINA_GPIO0`

**Síntomas:**

```text
'PINS_COUNT' was not declared
'NINA_GPIO0' was not declared
```

**Causa:** se instaló la variante Bluepad32 diseñada para usar un módulo NINA como coprocesador.

**Solución:** partir de la plantilla oficial `esp-idf-arduino-bluepad32-template`, clonar sus submódulos y conservar `framework = espidf`. No instalar manualmente la librería NINA-W10 en `lib_deps`.

### 8.2 `void value not ignored` en `BP32.update()` / `hasData()` inexistente

**Síntomas:**

```text
bool dataUpdated = BP32.update();
error: void value not ignored as it ought to be
'class Controller' has no member named 'hasData'
```

**Causa en la prueba original:** API de la biblioteca incorrecta. **No** corregir estas dos líneas a ciegas antes de verificar la biblioteca.

### 8.3 `pio : El término 'pio' no se reconoce`

Estás usando PowerShell sin `pio` en el `PATH`. Usa **PROJECT TASKS** de PlatformIO o invoca la ruta real de `platformio.exe`. No es un error del código.

### 8.4 `No module named 'urllib3'` y bibliotecas faltantes

Se observó una instalación incompleta del Python interno de PlatformIO (`urllib3`, `certifi`, `pyelftools`, `uvicorn`, `anyio`, `typing-extensions`). En un equipo nuevo **no es necesario instalar todo manualmente si PlatformIO se instala correctamente**.

Para diagnosticar dependencias de una instalación existente, usa **el Python del mismo entorno de PlatformIO**, no el Python global:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m pip check
```

En la experiencia original hubo que ajustar `uvicorn` a un rango compatible con ese Core (`>=0.16,<0.41`) y `urllib3` a `<2` para el instalador usado. **Son medidas de reparación de esa instalación, no pasos obligatorios para todas las computadoras.** Si la instalación nueva falla, conserva el mensaje completo para determinar la versión y paquete que causa el error.

### 8.5 `PermissionError [WinError 5]` o `os error 32` (archivo en uso)

Otros procesos de Python/PlatformIO/VS Code pueden bloquear un archivo mientras se actualizan los entornos virtuales. También puede influir un antivirus o un permiso insuficiente.

**Procedimiento prudente:** guardar el proyecto, cerrar monitores y tareas activas, cerrar VS Code, reiniciar Windows si es necesario y repetir una sola compilación. Evitar varias instalaciones simultáneas. **No borrar todo `.platformio` ni desactivar el antivirus como primera medida.**

### 8.6 `Error: Detected a whitespace character in project paths`

Mover el proyecto a una carpeta **sin espacios** (ej. `D:\Catamaran_Bluepad32`) y, si es necesario, configurar **`PLATFORMIO_CORE_DIR`** hacia `C:\PlatformIO_Data` o `D:\PlatformIO_Data`, antes de volver a compilar. Ver el apartado 2.

### 8.7 `Failed to install Python dependencies into penv`

Es un mensaje general que **no explica la causa por sí mismo**. Revisa las líneas anteriores, en particular `permission denied`, `ModuleNotFoundError`, `WinError`, `uv` o incompatibilidades de versiones. Para ampliar el diagnóstico:

```powershell
pio run -e esp32dev -v
```

Si necesitas guardar la salida en un archivo:

```powershell
pio run -e esp32dev -v *> compilacion.log
```

### 8.8 Subrayados rojos en `#include "sdkconfig.h"`

Si el proyecto **compila y carga correctamente**, pueden ser solo advertencias de IntelliSense/CMake Tools que aún no han indexado todos los archivos generados por ESP-IDF. La terminal de **Build** tiene prioridad sobre estos subrayados. Abrir únicamente la carpeta del proyecto y reconstruir el índice puede ayudar.

### 8.9 El mando se conecta pero no imprime `X`

Verifica que el monitor esté a `115200`, que hayas cargado el firmware actualizado y que la función `loop()` lea `BP32.update()` y `ctl->a()`. Asegúrate de que el dispositivo reportado sea un gamepad y esté conectado. Algunas variantes de control pueden tener un mapeo distinto.

## 9. Publicar y mantener el proyecto en GitHub

**Este archivo debe llamarse `README.md` y quedar en la raíz del proyecto** para que GitHub lo muestre automáticamente al abrir el repositorio.

Antes de publicar, revisa el contenido de la carpeta y **no subas binarios ni entornos locales**. En la plantilla original puede ser útil añadir estas reglas al archivo `.gitignore` existente:

```gitignore
.pio/
managed_components/
compilacion.log
```

También evita subir claves, tokens, contraseñas, datos personales o archivos `.env` con credenciales. **Sí conserva** los archivos fuente (`main/`), `platformio.ini`, `sdkconfig.defaults`, `.gitmodules`, `components/`, `external/` y los archivos de configuración necesarios. Para reproducibilidad, revisar y versionar `dependencies.lock` cuando corresponda.

### Si ya tienes un repositorio en GitHub

Desde la carpeta local:

```powershell
git status
git remote -v
```

Comprueba que `origin` apunte **a tu propio repositorio**, no accidentalmente a la plantilla oficial. Agrega `README.md` y los cambios deseados:

```powershell
git add README.md
git add main/sketch.cpp platformio.ini
git status
git commit -m "Documentar instalacion de Bluepad32 y prueba PS4"
git push
```

Si la rama aún no tiene seguimiento remoto, quizás necesites `git push -u origin main` (ajusta el nombre de rama si no es `main`).

### Si tu carpeta es un clon de la plantilla oficial y aún no has creado tu repositorio

1. En GitHub, crea un repositorio **vacío** para el proyecto (por ejemplo, `Catamaran_Bluepad32`), sin inicializarlo de nuevo con archivos que ya tienes localmente.
2. En la carpeta local, comprueba el remoto actual con `git remote -v`.
3. Si `origin` corresponde a `ricardoquesada/esp-idf-arduino-bluepad32-template`, conserva esa referencia como `upstream` y añade tu propio repositorio como `origin`:

   ```powershell
   git remote rename origin upstream
   git remote add origin https://github.com/TU_USUARIO/Catamaran_Bluepad32.git
   ```

4. Revisa `.gitignore` y `git status`; sube los archivos fuente y esta documentación:

   ```powershell
   git add README.md main/sketch.cpp platformio.ini .gitignore
   git status
   git commit -m "Guardar prueba funcional ESP32 y DualShock 4"
   git push -u origin main
   ```

**No copies literalmente `TU_USUARIO`**: reemplázalo por la URL HTTPS real que GitHub te da. Si hay cambios adicionales en el proyecto, revísalos con `git status` y agrégalos conscientemente. Los submódulos se almacenan como referencias Git; quien clone el repo deberá ejecutar `git clone --recurse-submodules`.

## 10. Referencias y notas

- [Plantilla oficial ESP-IDF + Arduino + Bluepad32](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template)
- [Proyecto Bluepad32](https://github.com/ricardoquesada/bluepad32)
- [PlatformIO — ESP-IDF](https://docs.platformio.org/en/latest/frameworks/espidf.html)
- [Documentación de PlatformIO](https://docs.platformio.org/)
- [ESP-IDF — documentación de Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)

**Próxima etapa del catamarán:** implementar lectura de joysticks y control de ESC con calibración, límites de señal, estado neutro y parada de seguridad por pérdida de enlace. Estos elementos **no forman parte de la prueba Bluetooth ya validada** y deben verificarse por separado, inicialmente sin hélices ni carga peligrosa.

---

**Resumen de reproducción:** instalar VS Code + PlatformIO + Git → clonar el proyecto y submódulos → evitar rutas con espacios → abrir la carpeta del `platformio.ini` → `Build` → `Upload` → monitor a `115200` → SHARE + PS → botón X → comprobar mensaje de comunicación.
