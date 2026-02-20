@echo off
title Helbreath Server Shutdown
color 0C

echo ================================================================================
echo HELBREATH SERVER SHUTDOWN
echo ================================================================================
echo.

echo Closing all Helbreath server processes...
echo.

taskkill /IM Client_d.exe /F >nul 2>&1 && echo [OK] Client closed || echo [--] Client not running
taskkill /IM HGserver.exe /F >nul 2>&1 && echo [OK] Game Servers closed || echo [--] Game Servers not running
taskkill /IM Login.exe /F >nul 2>&1 && echo [OK] Login Server closed || echo [--] Login Server not running

echo.
echo Closing leftover server console windows...
taskkill /F /FI "WINDOWTITLE eq Helbreath Login Server" >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq Helbreath Towns" >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq Helbreath Neutrals" >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq Helbreath Middleland" >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq Helbreath Events" >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq Helbreath Server Launcher" >nul 2>&1
echo [OK] Console windows closed

echo.
echo All Helbreath processes stopped.
echo.
echo MySQL is still running (shared service, not stopped automatically).
echo To stop MySQL: net stop MySQL80
echo.
timeout /t 3 >nul
exit
