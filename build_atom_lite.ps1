param(
    [string]$Port = "COM8"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    Write-Host "PlatformIO CLI (pio) was not found."
    Write-Host "Install PlatformIO Core or PlatformIO IDE, then run this script again."
    exit 1
}

pio run -e m5stack_atom_lite
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Build complete:"
Write-Host ".pio\build\m5stack_atom_lite\firmware.bin"
Write-Host ""
Write-Host "To upload to $Port:"
Write-Host "pio run -e m5stack_atom_lite -t upload --upload-port $Port"
