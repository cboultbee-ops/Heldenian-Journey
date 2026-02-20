# Dallon's Setup Guide — Helbreath Project

**Date:** February 20, 2026  
**From:** Cameron  
**Repo:** `cboultbee-ops/Helbreath-Project`

> **Give this file to your code bot.** It contains everything needed to get you up and running with the latest changes.

---

## Step 1: Pull Latest Changes

```cmd
cd "C:\Helbreath Project"
git pull origin master
```

If you don't have the repo yet:
```cmd
git clone https://github.com/cboultbee-ops/Helbreath-Project.git "C:\Helbreath Project"
```

---

## Step 2: Read the Project Briefing

The file `C:\Helbreath Project\CLAUDE_BRIEFING.md` contains the complete project structure, build instructions, network config, database info, and all changes made so far. **Read it fully before doing anything else.**

---

## Step 3: Compile All Three Binaries

You **must** compile these yourself — exe files are not tracked in git.

### Prerequisites
- **Visual Studio 2022** with C++ Desktop Development workload
- **v143 toolset** (comes with VS2022)
- **Win32/x86 platform** (NOT x64)

### A. Game Server (HGserver.exe)

**CRITICAL: Must be DEBUG build. If you get a ~1.4MB exe, you built Release — it WILL crash.**

Open PowerShell (not cmd, not bash) and run:
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'C:\Helbreath Project\Sources\HG\HGserver.vcxproj' /p:Configuration=Debug /p:Platform=Win32 /verbosity:minimal
```

This outputs to `C:\Helbreath Project\Maps\HGserver.exe` (~2.9MB).

Then copy to all 4 map server directories:
```powershell
Copy-Item "C:\Helbreath Project\Maps\HGserver.exe" "C:\Helbreath Project\Maps\Towns\HGserver.exe" -Force
Copy-Item "C:\Helbreath Project\Maps\HGserver.exe" "C:\Helbreath Project\Maps\Neutrals\HGserver.exe" -Force
Copy-Item "C:\Helbreath Project\Maps\HGserver.exe" "C:\Helbreath Project\Maps\Middleland\HGserver.exe" -Force
Copy-Item "C:\Helbreath Project\Maps\HGserver.exe" "C:\Helbreath Project\Maps\Events\HGserver.exe" -Force
```

### B. Login Server (Login.exe)

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'C:\Helbreath Project\Sources\Login\Login.vcxproj' /p:Configuration=Debug /p:Platform=Win32 /verbosity:minimal
```

This outputs to `C:\Helbreath Project\Login\Login.exe` (~240KB).

### C. Client (Play_Me.exe)

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'C:\Helbreath Project\Sources\Client\Client.vcxproj' /p:Configuration=Release /p:Platform=Win32 /verbosity:minimal
```

This outputs to `C:\Helbreath Project\Client\Play_Me.exe` (~1.5MB).

**Note:** The Server Manager launches `Play_Me.exe`, not the old `Client_d.exe`. If you don't compile the client, the Server Manager's "Launch Client" button won't work. You can temporarily edit `server_manager.py` to point back to `Client_d.exe` if needed, but you'll miss the new client features (bag display, fast logout, etc.).

---

## Step 4: Database Setup

You need MySQL 8.0 running locally.

| Setting | Value |
|---------|-------|
| Host | 127.0.0.1 |
| Port | 3306 |
| Database | `helbreath` |
| User | `root` |
| Password | `alpha123` |

If you need to import the database, check for a `.sql` dump file in the repo or ask Cameron for one.

---

## Step 5: Network Configuration

The server uses IP `172.16.0.1`. If your machine doesn't have this IP, you have two options:

### Option A: Add the virtual IP (recommended — matches Cameron's setup)
Open an **admin** command prompt:
```cmd
netsh interface ip add address "Ethernet" 172.16.0.1 255.255.255.0
```
(Replace "Ethernet" with your actual network adapter name — run `ipconfig` to check.)

Then add port proxies from your real LAN IP to the virtual IP:
```cmd
netsh interface portproxy add v4tov4 listenport=4000 listenaddress=YOUR_LAN_IP connectport=4000 connectaddress=172.16.0.1
netsh interface portproxy add v4tov4 listenport=3002 listenaddress=YOUR_LAN_IP connectport=3002 connectaddress=172.16.0.1
netsh interface portproxy add v4tov4 listenport=3008 listenaddress=YOUR_LAN_IP connectport=3008 connectaddress=172.16.0.1
netsh interface portproxy add v4tov4 listenport=3007 listenaddress=YOUR_LAN_IP connectport=3007 connectaddress=172.16.0.1
netsh interface portproxy add v4tov4 listenport=3001 listenaddress=YOUR_LAN_IP connectport=3001 connectaddress=172.16.0.1
```

### Option B: Change all config files to use your actual LAN IP
Update these files (replace `172.16.0.1` with your IP everywhere):
- `C:\Helbreath Project\Client\GM.cfg` — `log-server-address`
- `C:\Helbreath Project\Login\LServer.cfg` — `external-address` and `permitted-address`
- `C:\Helbreath Project\Maps\Towns\GServer.cfg` — all address fields
- `C:\Helbreath Project\Maps\Neutrals\GServer.cfg` — all address fields
- `C:\Helbreath Project\Maps\Middleland\GServer.cfg` — all address fields
- `C:\Helbreath Project\Maps\Events\GServer.cfg` — all address fields

---

## Step 6: Windows Defender Exclusion

Add an exclusion for the project folder so Defender doesn't block the server executables:

1. Open Windows Security → Virus & threat protection → Manage settings
2. Scroll to Exclusions → Add or remove exclusions
3. Add folder: `C:\Helbreath Project`

---

## Step 7: Start the Server

Use the Server Manager GUI:
```cmd
python "C:\Helbreath Project\server_manager.py"
```

Click **"Start All Servers"** then **"Launch Client"**.

Or start manually:
1. Run `C:\Helbreath Project\Login\Login.exe`
2. Wait 3 seconds
3. Run `C:\Helbreath Project\Maps\Towns\HGserver.exe`
4. Run `C:\Helbreath Project\Maps\Neutrals\HGserver.exe`
5. Run `C:\Helbreath Project\Maps\Middleland\HGserver.exe`
6. Run `C:\Helbreath Project\Maps\Events\HGserver.exe`
7. Run `C:\Helbreath Project\Client\Play_Me.exe`

Login: account **CamB**, password **password**, character **toke**

---

## Step 8: What's New (Changes Made Feb 20, 2026)

### Server-side (HG.cpp + LoginServer.cpp)
- **Recall damage timer toggle** — configurable in Settings.cfg (`recall-damage-timer = 0` or `1`)
- **Recall costs 0 MP** — Recall spell is free to cast
- **Cross-server map transitions fixed** — walking between city and Middleland no longer disconnects
- **Magic mastery encoding fix** — spells now work correctly (was converting ASCII/binary wrong)

### Client-side (Game.cpp + Wmain.cpp)
- **Bag inventory display** — shows item count and weight at bottom of bag window
- **Fast logout** — reads `logout-timer` from GM.cfg (1 = instant, 10 = normal)
- **Logout cancels on movement** — moving during countdown resets it
- **Debug hover text removed** — no more extra green text over characters/enemies

### Server Manager (server_manager.py)
- **Scrollable gameplay tuning tab** — fits in normal window width
- **Recall damage timer toggle** — ON/OFF button
- **Instant logout toggle** — ON/OFF button
- **Launches Play_Me.exe** instead of Client_d.exe

---

## Build Gotchas / Common Issues

| Problem | Solution |
|---------|----------|
| HGserver.exe is ~1.4MB | You built Release — rebuild as **Debug** |
| "Connection Lost" on character enter | HGserver build config is wrong — must be Debug |
| Server Manager can't find Play_Me.exe | Compile the client first (Step 3C) |
| MySQL connection failed | Make sure MySQL service is running, check credentials in LServer.cfg |
| "Access denied" starting servers | Run as administrator, add Windows Defender exclusion |
| Norton 360 blocks servers | Switch to Windows Defender or add exclusions in Norton |
| Login server won't start | Check LServer.cfg has correct `permitted-address` entries for your IP |
| Can't connect from client | Verify `Client\GM.cfg` has correct `log-server-address` matching your setup |
| Compile errors about afxres.h | `Sources\Login\res\Resources.rc` should use `#include <windows.h>` not `afxres.h` |
| MSBuild /p: flags not working | Use **PowerShell**, not cmd or bash — bash mangles the /p: switches |
