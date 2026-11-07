<#
Uso desde PowerShell (Windows):
  .\scripts\platformio_catamaran.ps1 -Equipo Maestro -Accion Compilar -Proyecto C:\Proyectos\Catamaran_Bluepad32
  .\scripts\platformio_catamaran.ps1 -Equipo Maestro -Accion Cargar -Proyecto C:\Proyectos\Catamaran_Bluepad32 -Puerto COM3
  .\scripts\platformio_catamaran.ps1 -Equipo Esclavo -Accion Monitor -Proyecto C:\Proyectos\Catamaran_esclavo -Puerto COM7

El programa NO flashea por defecto: hay que solicitar -Accion Cargar.
Si PowerShell bloquea scripts, ejecutar cada comando manualmente (ver README).
#>
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Maestro','Esclavo')]
    [string]$Equipo,
    [Parameter(Mandatory=$true)]
    [ValidateSet('Compilar','Cargar','Monitor','Puertos')]
    [string]$Accion,
    [Parameter(Mandatory=$true)]
    [string]$Proyecto,
    [string]$Puerto,
    [string]$CoreDir = 'C:\PIO_Core'
)

$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $Proyecto -PathType Container)) {
    throw "La carpeta de proyecto no existe: $Proyecto"
}

$env:PLATFORMIO_CORE_DIR = $CoreDir
New-Item -ItemType Directory -Path $CoreDir -Force | Out-Null

$pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe'
if (!(Test-Path $pio)) {
    $cmd = Get-Command pio -ErrorAction SilentlyContinue
    if (-not $cmd) { throw 'PlatformIO no encontrado. Instala/extiende PlatformIO IDE en VS Code.' }
    $pio = $cmd.Source
}

$entorno = if ($Equipo -eq 'Maestro') { 'esp32dev' } else { 'esp32doit-devkit-v1' }

Push-Location $Proyecto
try {
    if ($Accion -eq 'Puertos') {
        & $pio device list
    } elseif ($Accion -eq 'Compilar') {
        & $pio run -e $entorno
    } elseif ($Accion -eq 'Cargar') {
        if ([string]::IsNullOrWhiteSpace($Puerto)) { throw 'Para cargar firmware indica -Puerto COMx.' }
        Write-Host 'ADVERTENCIA: Retira las hélices y desconecta la alimentación de potencia de los ESC.' -ForegroundColor Yellow
        & $pio run -e $entorno -t upload --upload-port $Puerto
    } elseif ($Accion -eq 'Monitor') {
        if ([string]::IsNullOrWhiteSpace($Puerto)) { throw 'Para abrir monitor indica -Puerto COMx.' }
        & $pio device monitor -p $Puerto -b 115200
    }
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO terminó con código $LASTEXITCODE" }
} finally {
    Pop-Location
}
