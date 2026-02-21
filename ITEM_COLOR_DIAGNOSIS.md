# Item Color Fix — Diagnosis & Next Steps

## How Item Colors Work (Verified)

The color system is a pipeline:

1. **Item has `m_cItemColor`** — a byte value (0-14) stored per item
   - 0 = no color (grey/default)
   - 1 = Indigo Blue
   - 2 = Custom Weapon Color
   - 3 = Gold
   - 4 = Crimson
   - 5 = Green (Poisoning)
   - 6 = Gray
   - 7 = Aqua
   - 8 = Pink
   - etc.

2. **When item is equipped**, the server packs the color into `m_iApprColor` (HG.cpp line 8725-8728):
   - Weapon color → bits 28-31
   - Shield color → bits 24-27
   - Armor color → bits 20-23
   - Mantle color → bits 16-19
   - etc.

3. **Server sends `m_iApprColor`** to the client (HG.cpp line 1202)

4. **Client unpacks** the color into per-slot values (Game.cpp line 7925-7932)

5. **Client renders** using `PutSpriteRGB` with the color lookup tables (Game.cpp line 774-789)

**ALL of this code is intact and correct.**

## The Problem

The items Cameron has in his inventory most likely have `m_cItemColor = 0`. This happens when:
- Items were created without enchantment prefix (plain items)
- Items exist from the original database without color data
- Items were inserted via SQL without setting the color field

## How to Diagnose

### Step 1: Add debug logging to check item colors at login
In HG.cpp, after line 4881 (where `bEquipItemHandler` is called during login), add:

```cpp
// DEBUG: Log item color for equipped items
if (m_pClientList[iClientH]->m_bIsItemEquipped[b] == TRUE) {
    wsprintf(cTxt, "(DEBUG) Equipped item[%d] '%s' color=%d attr=0x%08X",
        b, item->m_cName, item->m_cItemColor, item->m_dwAttribute);
    PutLogList(cTxt);
}
```

Also after ALL items are loaded, add before the function returns:
```cpp
wsprintf(cTxt, "(DEBUG) Player '%s' final m_iApprColor = 0x%08X",
    m_pClientList[iClientH]->m_cCharName, m_pClientList[iClientH]->m_iApprColor);
PutLogList(cTxt);
```

### Step 2: Check the log output
Restart HGserver and log in. Check the server log for the debug lines. If all colors are 0, the items need their attribute and color set.

### Step 3: Test with /createitem
Once GM commands are working, create a colored item:
```
/createitem GiantSword 1048576 0 1
```
This creates a Poisoning Giant Sword (prefix 1, shifted by 20 bits = 1048576). The `AdminOrder_CreateItem` function at HG.cpp line 32710-32719 maps:
- Prefix 1 → `m_cItemColor = 5` (Green)
- Prefix 6 → `m_cItemColor = 2` (Custom weapon)
- Prefix 8 → `m_cItemColor = 3` (Gold)

Equip it. If it shows green, the color system works and the issue is purely that existing items lack color data.

## If Colors Still Don't Show After Creating a Colored Item

Then the problem is in the client rendering. Check:

1. **`m_cDetailLevel` setting** — If this is 0 (Game.cpp line 7915), ALL colors are forced to 0. Check `Client\Settings.cfg` or `client.ini` for `DetailLevel`. It should be 1 or higher. Press **Ctrl+D** in-game to cycle detail level.

2. **`PutSpriteRGB` function** — If this function is broken or falls through to `PutSpriteFast` (no color tinting), nothing will show color. Check the DDraw sprite rendering code.

## Common /createitem Attribute Values for Colored Items

### Weapons:
```
/createitem GiantSword 1048576 0 1       — Poisoning (Green, prefix 1)
/createitem GiantSword 6291456 0 1       — Ancient (prefix 6, color 2)
/createitem GiantSword 8388608 0 1       — Light (Gold, prefix 8)
/createitem GiantSword 9437184 0 1       — Sharp (prefix 9, color 8/Pink)
```

### Armor:
```
/createitem PlatemailArmor 1048576 0 1   — Poisoning (Green)
/createitem PlatemailArmor 8388608 0 1   — Knight (Gold)
```

### How the attribute hex works:
Bits 20-23 hold the prefix type. Shift left by 20:
- Prefix 1 << 20 = 1048576   = 0x00100000
- Prefix 2 << 20 = 2097152   = 0x00200000
- Prefix 3 << 20 = 3145728   = 0x00300000
- Prefix 5 << 20 = 5242880   = 0x00500000
- Prefix 6 << 20 = 6291456   = 0x00600000
- Prefix 7 << 20 = 7340032   = 0x00700000
- Prefix 8 << 20 = 8388608   = 0x00800000
- Prefix 9 << 20 = 9437184   = 0x00900000
- Prefix 10 << 20 = 10485760 = 0x00A00000

## Build
- HGserver in **Debug** mode only
- Copy to all 4 map dirs
- Restart servers, test
