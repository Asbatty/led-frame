# Build the LED frame firmware and install it over Wi-Fi.
#   powershell -ExecutionPolicy Bypass -File tools\flash.ps1            # build + update over Wi-Fi
#   powershell -ExecutionPolicy Bypass -File tools\flash.ps1 -Usb COM5  # first install / recovery (hold BOOT)
#   powershell -ExecutionPolicy Bypass -File tools\flash.ps1 -BuildOnly
param(
  [string]$Frame = "matrix.local",   # or the frame's IP, e.g. 192.168.1.50
  [string]$Usb = "",
  [switch]$BuildOnly
)
$ErrorActionPreference = "Continue"  # compiler notes go to stderr; real failures are checked below
$root = Split-Path $PSScriptRoot -Parent
$sketch = Join-Path $root "firmware\LedFrame"
$build = Join-Path $root "build"
$fqbn = "esp32:esp32:esp32:PartitionScheme=min_spiffs"
$cli = "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
if (-not (Test-Path $cli)) { $cli = "arduino-cli" }
if (-not (Test-Path (Join-Path $sketch "secrets.h"))) { throw "Missing firmware\LedFrame\secrets.h (copy secrets.example.h and fill it in)" }

Write-Host "Building..."
$log = & $cli compile --fqbn $fqbn --output-dir $build $sketch 2>&1 | ForEach-Object { "$_" }
$ok = $LASTEXITCODE -eq 0
$log | Where-Object { $_ -match "error:|Sketch uses" } | ForEach-Object { "  $_" }
if (-not $ok) { throw "Build failed" }
if ($BuildOnly) { return }

if ($Usb) {
  Write-Host "Uploading over USB on $Usb (hold BOOT if it sticks on Connecting...)"
  & $cli upload --fqbn $fqbn -p $Usb --input-dir $build $sketch
  return
}

# Find the frame (mDNS name first, then the IP it reports)
$state = curl.exe -s -m 6 "http://$Frame/api/state"
if (-not ($state -match '"ip":"([^"]+)"')) { throw "Can't reach the frame at $Frame. Is it on and on Wi-Fi? Try -Frame <ip>." }
$ip = $Matches[1]
$pw = ([regex]::Match((Get-Content (Join-Path $sketch "secrets.h") -Raw), 'OTA_PASSWORD\s+"([^"]+)"')).Groups[1].Value

Write-Host "Updating frame at $ip over Wi-Fi..."
$r = curl.exe -s -m 180 -u "admin:$pw" -F "fw=@$build\LedFrame.ino.bin" "http://$ip/update"
if ($r -notmatch "Updated") { throw "Update failed: $r" }
Start-Sleep 15
$after = curl.exe -s -m 8 "http://$ip/api/state"
if ($after -match '"fw":"([^"]+)"') { Write-Host "Done. Frame is running firmware $($Matches[1])." } else { Write-Host "Uploaded; the frame is still restarting." }
