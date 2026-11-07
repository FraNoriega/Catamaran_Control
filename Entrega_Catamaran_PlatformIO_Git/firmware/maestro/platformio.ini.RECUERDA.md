# Configuración del maestro (referencia, NO reemplazo automático)

El proyecto maestro **no es un proyecto Arduino convencional**. Requiere la plantilla
`esp-idf-arduino-bluepad32-template` con sus submódulos, `CMakeLists.txt`,
`main/`, `components/`, `external/`, `patches/`, y dependencias ESP-IDF.

En el proyecto probado se observó:

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

El archivo real contiene otras entradas, como `board_build.embed_txtfiles` y
varios entornos de placa: **no lo sustituyas por este extracto**.

Durante la reparación de esptool también se agregó temporalmente en
`[env:esp32dev]` una referencia `platform_packages` a esptool 5.1.0.
Ese ajuste fue parte del diagnóstico; no lo traslades ciegamente a una instalación
limpia sin confirmar que sea necesario. Es preferible conservar el
`platformio.ini` completo del proyecto que está funcionando y registrar su commit.

Fuente de la plantilla:
https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template
