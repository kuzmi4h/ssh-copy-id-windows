@echo off
REM ssh-copy-id для Windows
REM Обёртка для запуска PowerShell скрипта

setlocal enabledelayedexpansion

REM Получение пути к директории скрипта
set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%ssh-copy-id.ps1"

REM Проверка существования PowerShell скрипта
if not exist "%PS_SCRIPT%" (
    echo Ошибка: Не найден скрипт %PS_SCRIPT%
    exit /b 1
)

REM Проверка PowerShell
where powershell >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Ошибка: PowerShell не найден
    exit /b 1
)

REM Запуск PowerShell скрипта с передачей всех аргументов
powershell -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*

exit /b %ERRORLEVEL%
