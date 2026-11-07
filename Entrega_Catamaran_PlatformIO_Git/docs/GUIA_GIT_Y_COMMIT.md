# Subir firmware y bitácora a GitHub con Git

Este paquete está listo para versionarse. **No se ha hecho `git push` a una cuenta remota**: para ello se necesita el repositorio exacto al que debe subirse y sus permisos. No insertar nombres de repositorio inventados.

## Opción 1 — Un solo repositorio del catamarán (recomendado)

1. En GitHub, abrir el repositorio **propio** del proyecto o crear uno. Copiar su URL real (no la de la plantilla upstream de Bluepad32, a menos que sea tu fork).
2. Clonarlo en tu computadora y copiar **el contenido** de `Entrega_Catamaran_PlatformIO_Git` a la raíz del repositorio.
3. Desde PowerShell en la raíz de ese repositorio:

```powershell
git status
git remote -v
# Verifica que 'origin' apunta a TU repositorio, no al proyecto upstream.
git switch -c feat/catamaran-ps4-xbee-esc-tolva
git add README.md CHANGELOG.md .gitignore firmware docs scripts COMMIT_MESSAGE.txt
git diff --cached --stat
git diff --cached --check
git commit -m "feat(catamaran): documentar PS4, XBee, dos ESC y tolva en PlatformIO"
git push -u origin feat/catamaran-ps4-xbee-esc-tolva
```

Después abre un Pull Request para fusionarlo a `main` si tu equipo trabaja con ramas. Si ya existe la rama y está bajo control del equipo, usa su flujo habitual: **no hagas force-push**.

## Opción 2 — Nuevo repositorio local sin historial

Desde la carpeta descomprimida de esta entrega:

```powershell
cd C:\Proyectos\Entrega_Catamaran_PlatformIO_Git
git init -b main
git config user.name "TU_NOMBRE_DE_GIT"
git config user.email "TU_CORREO_ASOCIADO_A_GIT"
git add .
git diff --cached --stat
git commit -m "feat(catamaran): version funcional PS4-XBee-ESC-tolva y documentacion"
git remote add origin <URL-REAL-DE-TU-REPOSITORIO>
git push -u origin main
```

**No copies literalmente** los placeholders `<...>`: sustitúyelos por la URL real del repositorio y tus datos de Git. Si el remoto ya tiene commits, clonarlo primero (opción 1) para evitar conflictos de historial.

## Si los proyectos `Catamaran_Bluepad32` y `Catamaran_esclavo` son repositorios distintos

Puedes versionarlos por separado, pero la documentación cruzada será más difícil de encontrar. Alternativa: mantener un repositorio central con `firmware/maestro` y `firmware/esclavo` como referencias, además de publicar versiones compatibles en los repositorios de desarrollo.

**Muy importante sobre Bluepad32:** si vas a versionar el proyecto maestro completo, conservar los **archivos de plantilla**, `main/CMakeLists.txt`, `components/`, `external/`, `.gitmodules`, `patches/`, `sdkconfig.defaults` y referencias a submódulos. El archivo `sketch.cpp` por sí solo **no reproduce la compilación ESP-IDF + Bluepad32**.

No subir `.pio/`, binarios generados, contraseñas, credenciales Bluetooth privadas, tokens, ni copias de la carpeta global `.platformio/`.

## Verificar qué acabas de publicar

```powershell
git log -1 --oneline
git status
git remote -v
```

Abrir GitHub y verificar que aparecen el código de ambos dispositivos, README, historial de errores, instrucciones de instalación desde laptop nueva y la configuración del esclavo.

## Mensaje de commit ampliado

Consulta `COMMIT_MESSAGE.txt` para un resumen formal del cambio. Este archivo también puede adjuntarse a la descripción del Pull Request.
