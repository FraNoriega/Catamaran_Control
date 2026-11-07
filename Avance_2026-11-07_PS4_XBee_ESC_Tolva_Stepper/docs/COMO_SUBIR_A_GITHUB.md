# Subir esta carpeta a GitHub arrastrándola desde Windows

**Repositorio:** https://github.com/FraNoriega/Catamaran_Control

No hace falta usar Git o PowerShell para subir los archivos de este avance. La interfaz web de GitHub permite arrastrar una carpeta local.

1. **Descomprime el ZIP** de esta entrega en Windows (clic derecho → *Extraer todo*).
2. Asegúrate de ver la carpeta `Avance_2026-11-07_PS4_XBee_ESC_Tolva_Stepper` y **dentro** `README.md`, `CHANGELOG.md`, `firmware` y `docs`.
3. Abre el repositorio de GitHub y entra en la rama principal `main` (o en una nueva rama si ya la creaste).
4. Pulsa **Add file → Upload files**.
5. Arrastra **la carpeta extraída**, NO el ZIP. GitHub debe mostrar rutas como `Avance_2026-11-07_PS4_XBee_ESC_Tolva_Stepper/firmware/maestro/sketch.cpp`.
6. Revisa los archivos antes de guardar. Deben permanecer bajo esa carpeta: **no reemplaces** los `README.md`, `platformio.ini` o `src/main.cpp` antiguos que ya están en la raíz del repositorio.
7. Escribe el mensaje de commit:

```text
feat(catamaran): integrar stepper por PS4 y documentar reproducibilidad
```

8. Si GitHub muestra **Create a new branch for this commit**, úsala y propone un pull request para que el equipo pueda revisar el avance antes de integrarlo. Nombre sugerido: `feat/ps4-xbee-esc-tolva-stepper-2026-11-07`.
9. Pulsa **Propose changes** o **Commit changes** según la opción elegida. Si creaste rama, completa el pull request cuando el equipo apruebe los cambios.
10. Comprueba en GitHub que los dos archivos `.cpp` y la guía se vean como **archivos normales** (no como un ZIP único).

## Qué NO subir

- `*.zip` del paquete cuando se pueden subir los archivos separados.
- `.pio/`, `.vscode` con rutas personales, `build/`, `*.bin`, `*.elf`, temporales de compilación.
- Claves personales, contraseñas, credenciales o tokens.
- Copias de librerías descargadas del sistema si se gestionan con submódulos/dependencias.

## Cómo obtenerlo después en otra laptop

```powershell
git clone https://github.com/FraNoriega/Catamaran_Control.git
```

Sigue `docs/INSTALACION_OTRA_COMPUTADORA.md` dentro de la carpeta del avance. Para compilar la **maestra** necesitarás, además, la plantilla ESP-IDF/Arduino/Bluepad32 completa; los archivos de esta entrega por sí solos no la sustituyen.
