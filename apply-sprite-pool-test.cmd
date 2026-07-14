@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0apply-sprite-pool-test.ps1"
if errorlevel 1 exit /b 1
