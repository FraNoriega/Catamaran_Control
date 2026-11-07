# ESP32 MAESTRA: requiere plantilla Bluepad32 completa

El archivo `sketch.cpp` de esta carpeta **NO es un proyecto PlatformIO autónomo**. Se debe colocar en:

```text
Catamaran_Bluepad32/
├── platformio.ini
├── CMakeLists.txt
├── sdkconfig.defaults
├── components/          (submódulos/paquetes de la plantilla)
├── external/            (submódulos)
└── main/
    ├── CMakeLists.txt
    ├── main.c
    └── sketch.cpp      ← copiar el de este avance
```

Usar la plantilla `ricardoquesada/esp-idf-arduino-bluepad32-template`, clonada con `--recurse-submodules`. Mantener los `CMakeLists.txt`, `main.c` y archivos de configuración existentes. El entorno se llama `[env:esp32dev]`, usa `framework=espidf` con Arduino como componente y la versión de PioArduino/Bluepad32 empleada en el proyecto. **No reemplazar su `platformio.ini` por el del esclavo**.

La maestra usa `Console.printf()` y el controlador UART nativo de ESP-IDF (`driver/uart.h`) sobre UART2, RX=GPIO16 y TX=GPIO17. `Serial2.write()` **no funcionó** en el montaje probando desde PlatformIO; no reemplazar la UART nativa sin una nueva prueba.

Consultar `../../docs/INSTALACION_OTRA_COMPUTADORA.md` para los pasos completos y la nota sobre el conflicto histórico de `esptool`.
