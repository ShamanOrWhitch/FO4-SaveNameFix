@echo off
title FO4 short save names
if "%~1"=="" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Patch-SaveName.ps1"
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Patch-SaveName.ps1" -ExePath "%~1"
)
echo.
pause
