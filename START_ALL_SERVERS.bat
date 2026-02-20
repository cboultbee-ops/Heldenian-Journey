@echo off
title Helbreath Server Launcher
color 0A

echo ================================================================================
echo HELBREATH SERVER STARTUP
echo ================================================================================
echo.

REM Check if running as admin
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Must run as Administrator!
    echo Right-click this file and select "Run as administrator"
    pause
    exit
)

echo [1/6] Checking MySQL...
sc query MySQL80 | find "RUNNING" >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] MySQL already running
) else (
    echo Starting MySQL...
    net start MySQL80 >nul 2>&1
    if %errorLevel% equ 0 (
        echo [OK] MySQL started
    ) else (
        net start MySQL >nul 2>&1
        if %errorLevel% equ 0 (
            echo [OK] MySQL started
        ) else (
            echo [WARN] Could not start MySQL - check services.msc
        )
    )
)
timeout /t 2 >nul

echo.
echo [2/6] Starting Login Server...
start "Helbreath Login Server" cmd /k "cd /d "C:\Helbreath Project\Login" && Login.exe"
echo Waiting 3 seconds for Login Server to initialize...
timeout /t 3 >nul

echo.
echo [3/6] Starting Towns Server (port 3002)...
start "Helbreath Towns" cmd /k "cd /d "C:\Helbreath Project\Maps\Towns" && HGserver.exe"
timeout /t 2 >nul

echo.
echo [4/6] Starting Neutrals Server (port 3008)...
start "Helbreath Neutrals" cmd /k "cd /d "C:\Helbreath Project\Maps\Neutrals" && HGserver.exe"
timeout /t 2 >nul

echo.
echo [5/6] Starting Middleland Server (port 3007)...
start "Helbreath Middleland" cmd /k "cd /d "C:\Helbreath Project\Maps\Middleland" && HGserver.exe"
timeout /t 2 >nul

echo.
echo [6/6] Starting Events Server (port 3001)...
start "Helbreath Events" cmd /k "cd /d "C:\Helbreath Project\Maps\Events" && HGserver.exe"
timeout /t 3 >nul

echo.
echo ================================================================================
echo ALL SERVERS STARTED!
echo ================================================================================
echo.
echo Check each server window for ready messages.
echo.
echo Press any key to launch the game client...
pause >nul

echo.
echo Launching client...
cd /d "C:\Helbreath Project\Client"
start Client_d.exe

echo.
echo Done! Have fun playing!
echo Close this window anytime - servers will keep running.
pause
