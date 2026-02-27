@echo off
REM Скрипт для компиляции ssh-copy-id.exe
REM Использует GCC (MinGW) если доступен

setlocal

echo === Компиляция ssh-copy-id.exe ===
echo.

REM Проверка наличия GCC
where gcc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Найден GCC. Компиляция...
    gcc -O2 -Wall -Wextra -o ssh-copy-id.exe ssh-copy-id.c -lws2_32
    if %ERRORLEVEL% equ 0 (
        echo.
        echo Успешно! Создан файл ssh-copy-id.exe
    ) else (
        echo.
        echo Ошибка компиляции!
        exit /b 1
    )
    goto :check_result
)

REM Проверка наличия cl (MSVC)
where cl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Найден MSVC. Компиляция...
    cl /O2 /W3 ssh-copy-id.c ws2_32.lib /Fe:ssh-copy-id.exe
    if %ERRORLEVEL% equ 0 (
        echo.
        echo Успешно! Создан файл ssh-copy-id.exe
    ) else (
        echo.
        echo Ошибка компиляции!
        exit /b 1
    )
    goto :check_result
)

echo ОШИБКА: Не найден компилятор C (GCC или MSVC)
echo.
echo Для компиляции установите один из следующих компиляторов:
echo   - MinGW-w64: https://www.mingw-w64.org/
echo   - MSVC (Visual Studio Build Tools): https://visualstudio.microsoft.com/
echo.
echo Или используйте предварительно скомпилированный ssh-copy-id.exe
exit /b 1

:check_result
if exist ssh-copy-id.exe (
    echo.
    echo === Готово ===
    echo Для проверки выполните: ssh-copy-id.exe -h
    echo.
    echo Для установки скопируйте ssh-copy-id.exe в директорию из PATH:
    echo   copy ssh-copy-id.exe C:\Windows\
    echo или добавьте текущую директорию в PATH
)

endlocal
