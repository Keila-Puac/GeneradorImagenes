@echo off
echo ============================================
echo  Compilando Generador de Imagenes por Capas
echo ============================================

g++ -std=c++14 -o generador.exe main.cpp

if %errorlevel% == 0 (
    echo [OK] Compilacion exitosa: generador.exe
    echo.
    echo Para ejecutar: generador.exe
) else (
    echo [ERROR] Fallo la compilacion.
)
pause
