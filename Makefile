# Makefile для ssh-copy-id для Windows
# Поддерживает GCC (MinGW) и MSVC

CC_GCC = gcc
CC_MSVC = cl

# Определяем компилятор
ifdef USE_MSVC
    CC = $(CC_MSVC)
    CFLAGS = /O2 /W3
    LDFLAGS = ws2_32.lib
    EXE_OUT = /Fe:
    OBJ_OUT = /Fo:
else
    CC = $(CC_GCC)
    CFLAGS = -O2 -Wall -Wextra
    LDFLAGS = -lws2_32
    EXE_OUT = -o
    OBJ_OUT = -o
endif

TARGET = ssh-copy-id.exe
SRC = ssh-copy-id.c

.PHONY: all clean install help

all: $(TARGET)

$(TARGET): $(SRC)
ifdef USE_MSVC
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) $(EXE_OUT)$(TARGET)
else
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) $(EXE_OUT)$(TARGET)
endif

clean:
	del /Q $(TARGET) 2>nul || rm -f $(TARGET)

install: $(TARGET)
	@echo Для установки скопируйте $(TARGET) в директорию из PATH
	@echo Например: copy $(TARGET) C:\Windows\

help:
	@echo Доступные цели:
	@echo   all      - Скомпилировать ssh-copy-id.exe (по умолчанию)
	@echo   clean    - Удалить скомпилированный файл
	@echo   install  - Показать инструкцию по установке
	@echo   help     - Показать эту справку
	@echo.
	@echo Для компиляции с MSVC используйте: nmake /f Makefile USE_MSVC=1
	@echo Для компиляции с GCC используйте: mingw32-make или make
