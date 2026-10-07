@echo off
cmake --build "%~dp0out\build\win-amd64-release" --config Release
if errorlevel 1 (
    echo Game build failed. Setup was not created.
    pause
    exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Build-Setup.ps1"
pause
