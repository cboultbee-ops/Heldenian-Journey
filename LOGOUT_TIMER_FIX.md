# Logout Timer Fix — Implementation Guide

## Problem Summary

The logout timer in the Helbreath client was not functioning. When a player initiated logout (via ESC or the F12 SysMenu), the countdown was supposed to tick from N seconds down to 0 before disconnecting, but instead the logout completed almost instantly regardless of the configured timer value.

## Root Cause

In `Sources/Client/Game.cpp`, the variable `m_dwLogOutCountTime` (a DWORD storing the last tick timestamp) was **never initialized** when logout began. It was only updated *during* countdown ticks.

The countdown logic (in the main game loop) works like this:

```cpp
if (m_cLogOutCount > 0) {
    dwTime = timeGetTime();
    if ((dwTime - m_dwLogOutCountTime) > 1000) {  // 1 second elapsed?
        m_dwLogOutCountTime = dwTime;
        m_cLogOutCount--;
        // ... display countdown ...
    }
}
```

Because `m_dwLogOutCountTime` was uninitialized (defaulted to 0), the expression `dwTime - m_dwLogOutCountTime` evaluated to the current system uptime in milliseconds — always far greater than 1000. This caused the countdown to burn through all ticks in a single frame, making logout appear instant.

## The Fix

Added `m_dwLogOutCountTime = timeGetTime();` at **both** locations where logout is initiated:

### Location 1: F12 SysMenu "Log Out" button click
**File**: `Sources/Client/Game.cpp`
**Function**: `DlgBoxClick_SysMenu` (search for `m_cLogOutCount = m_cLogOutResCount`)

```cpp
// BEFORE (broken):
m_cLogOutCount = m_cLogOutResCount;

// AFTER (fixed):
m_cLogOutCount = m_cLogOutResCount;
m_dwLogOutCountTime = timeGetTime();  // <-- ADD THIS LINE
```

### Location 2: ESC key / Observer mode logout
**File**: `Sources/Client/Game.cpp`
**Function**: Key handler or observer mode section (search for the other `m_cLogOutCount = m_cLogOutResCount`)

```cpp
// BEFORE (broken):
m_cLogOutCount = m_cLogOutResCount;

// AFTER (fixed):
m_cLogOutCount = m_cLogOutResCount;
m_dwLogOutCountTime = timeGetTime();  // <-- ADD THIS LINE
```

## How to Find the Exact Lines

1. Search `Game.cpp` for all occurrences of `m_cLogOutCount = m_cLogOutResCount`
2. There are exactly **2** locations where this assignment starts the logout countdown
3. Immediately after each one, add `m_dwLogOutCountTime = timeGetTime();`
4. Do NOT modify the countdown tick logic itself (where `m_cLogOutCount--` happens) — that already updates `m_dwLogOutCountTime` correctly

## Related Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `m_cLogOutCount` | char | Current countdown value (ticks down from start to 0) |
| `m_cLogOutResCount` | char | The configured logout timer in seconds (from `GM.cfg` `logout-timer`) |
| `m_dwLogOutCountTime` | DWORD | Timestamp of last countdown tick (must be initialized at logout start) |

## Server Manager UI Change

The Helbreath Server Manager (`server_manager.py`) was also updated:

- **Removed** the "Instant Logout (Testing)" checkbox from the Dev Toggles section
- **Added** it underneath the "Logout Timer (seconds)" entry field in the Movement & Dash Speed section
- When the checkbox is **checked**: sets logout timer to `1` second
- When the checkbox is **unchecked**: sets logout timer to `10` seconds
- The checkbox auto-detects its state from the current timer value on load (timer <= 1 = checked)

### Server Manager Code Location
**File**: `server_manager.py`
**Section**: Look for `logout-timer` in the `_build_gameplay_tab` method

The relevant config key in `Client/GM.cfg`:
```
logout-timer = 10
```

## Build & Test

1. Build client: `MSBuild Sources\Client\Client.vcxproj /p:Configuration=Release /p:Platform=Win32`
2. Output: `Client\Play_Me.exe`
3. Set `logout-timer = 10` in `Client\GM.cfg`
4. Launch client, log in, press ESC or click Log Out from F12 menu
5. Verify: countdown displays "10... 9... 8..." ticking once per second
6. Verify: with `logout-timer = 1`, logout happens after 1 second
