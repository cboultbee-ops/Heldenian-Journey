@echo off
title Helbreath Firewall & Defender Setup
color 0B

echo ================================================================================
echo HELBREATH - FIREWALL ^& WINDOWS DEFENDER SETUP
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

echo ========================================
echo [1/3] Windows Defender Exclusions
echo ========================================
echo.

echo Adding folder exclusion for Helbreath Project...
powershell -Command "Add-MpPreference -ExclusionPath 'C:\Helbreath Project'" 2>nul
if %errorLevel% equ 0 (
    echo [OK] Folder exclusion added: C:\Helbreath Project
) else (
    echo [WARN] Could not add folder exclusion - may already exist
)

echo Adding process exclusions...
powershell -Command "Add-MpPreference -ExclusionProcess 'Login.exe'" 2>nul
powershell -Command "Add-MpPreference -ExclusionProcess 'HGserver.exe'" 2>nul
powershell -Command "Add-MpPreference -ExclusionProcess 'Client_d.exe'" 2>nul
echo [OK] Process exclusions added: Login.exe, HGserver.exe, Client_d.exe

echo.
echo ========================================
echo [2/3] Firewall Rules
echo ========================================
echo.

REM Remove old rules first (ignore errors if they don't exist)
netsh advfirewall firewall delete rule name="Helbreath Login TCP" >nul 2>&1
netsh advfirewall firewall delete rule name="Helbreath Login UDP" >nul 2>&1
netsh advfirewall firewall delete rule name="Helbreath GateServer TCP" >nul 2>&1
netsh advfirewall firewall delete rule name="Helbreath Towns TCP" >nul 2>&1
netsh advfirewall firewall delete rule name="Helbreath Neutrals TCP" >nul 2>&1
netsh advfirewall firewall delete rule name="Helbreath Middleland TCP" >nul 2>&1
netsh advfirewall firewall delete rule name="Helbreath Events TCP" >nul 2>&1

echo Adding Login Server rules (port 4000)...
netsh advfirewall firewall add rule name="Helbreath Login TCP" dir=in action=allow protocol=TCP localport=4000 >nul
netsh advfirewall firewall add rule name="Helbreath Login UDP" dir=in action=allow protocol=UDP localport=4000 >nul
echo [OK] Login Server: port 4000 TCP/UDP

echo Adding GateServer rule (port 5656)...
netsh advfirewall firewall add rule name="Helbreath GateServer TCP" dir=in action=allow protocol=TCP localport=5656 >nul
echo [OK] GateServer: port 5656 TCP

echo Adding Towns Server rule (port 3002)...
netsh advfirewall firewall add rule name="Helbreath Towns TCP" dir=in action=allow protocol=TCP localport=3002 >nul
echo [OK] Towns: port 3002 TCP

echo Adding Neutrals Server rule (port 3008)...
netsh advfirewall firewall add rule name="Helbreath Neutrals TCP" dir=in action=allow protocol=TCP localport=3008 >nul
echo [OK] Neutrals: port 3008 TCP

echo Adding Middleland Server rule (port 3007)...
netsh advfirewall firewall add rule name="Helbreath Middleland TCP" dir=in action=allow protocol=TCP localport=3007 >nul
echo [OK] Middleland: port 3007 TCP

echo Adding Events Server rule (port 3001)...
netsh advfirewall firewall add rule name="Helbreath Events TCP" dir=in action=allow protocol=TCP localport=3001 >nul
echo [OK] Events: port 3001 TCP

echo.
echo ========================================
echo [3/3] Port Proxy Verification
echo ========================================
echo.

echo Current port proxy rules:
netsh interface portproxy show all

echo.
echo ================================================================================
echo SETUP COMPLETE!
echo ================================================================================
echo.
echo Windows Defender: Helbreath Project folder excluded
echo Firewall Rules:  Ports 4000, 5656, 3001, 3002, 3007, 3008 opened
echo Port Proxies:    10.0.0.168 forwarding to 172.16.0.1
echo.
echo If port proxies are missing, run these commands as Administrator:
echo   netsh interface portproxy add v4tov4 listenaddress=10.0.0.168 listenport=4000 connectaddress=172.16.0.1 connectport=4000
echo   netsh interface portproxy add v4tov4 listenaddress=10.0.0.168 listenport=5656 connectaddress=172.16.0.1 connectport=5656
echo   netsh interface portproxy add v4tov4 listenaddress=10.0.0.168 listenport=3002 connectaddress=172.16.0.1 connectport=3002
echo   netsh interface portproxy add v4tov4 listenaddress=10.0.0.168 listenport=3008 connectaddress=172.16.0.1 connectport=3008
echo   netsh interface portproxy add v4tov4 listenaddress=10.0.0.168 listenport=3007 connectaddress=172.16.0.1 connectport=3007
echo   netsh interface portproxy add v4tov4 listenaddress=10.0.0.168 listenport=3001 connectaddress=172.16.0.1 connectport=3001
echo.
pause
