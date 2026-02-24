# Equilibrium Project — Master Implementation Task: Combat & Abilities Update

**Date:** February 23, 2026
**Author:** Cameron Boultbee (design) + Claude (technical spec)
**For:** Code Bot Implementation
**Project:** C:\Helbreath Project\Sources\
**Toolchain:** Visual Studio 2022, v143, Win32

---

## What This Document Covers

This is a single prioritized task list covering four interconnected systems that together form a major combat and abilities update for the Equilibrium server. Each section references a detailed implementation document. **Read the referenced document before implementing each section.**

The four systems are:

1. **Special Abilities UI System** — Colored circles above the UI bar with cooldown timers
2. **New Spells & Ability Chains** — Minor Berzerk, Speed, and their unlocked abilities
3. **Spell Targeting Guides** — Visual overlays showing where spells will land
4. **AoE Zone Mechanics** — Inner/outer zone damage and directional knockback

These systems share code (rendering utilities, packet infrastructure, buff management) so the implementation order matters.

---

## Implementation Priority Order

### Phase 1: Foundation (Do First)

These are shared systems that everything else depends on.

#### 1A. Alpha Blending Rendering Utility
**Reference:** `IMPLEMENT_SPELL_TARGETING_GUIDE.md` → Part 5.2

Build the `DrawAlphaRect()` function first. Both the spell targeting guides AND the AoE targeting circles AND the special ability circles all need semi-transparent colored overlays drawn onto tiles or screen regions. Getting this working once means all three visual systems can use it.

**Deliverable:** A working `DrawAlphaRect(x, y, w, h, r, g, b, alpha)` function that handles both 16-bit and 32-bit DirectDraw surfaces.

**⚠️ QUESTION FOR CAMERON:** Does the current client use 16-bit or 32-bit color surfaces? Check the DirectDraw surface initialization code — this determines which pixel format branch we need. If using DgVoodoo2 wrapper, it may be presenting as 32-bit even if the original code was 16-bit.

#### 1B. Special Ability Data Structures & Packet Infrastructure
**Reference:** `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` → Parts 1.1, 1.2, 4

Add the `SpecialAbility` struct, the `CGame` class members, and define the new packet types (`CYCMD_SPECIAL_ABILITY_ACTIVATE`, `CYCMD_SPECIAL_ABILITY_RESULT`). This packet infrastructure is used by all three abilities (Berzerk, Speed Burst, Glacial Strike).

**⚠️ QUESTION FOR CAMERON:** What is the current highest packet ID / command ID in use? The new ability packets need IDs that don't conflict with existing ones. Check the packet/command `#define` list in the shared headers.

#### 1C. Speed Buff Framework (Server)
**Reference:** `IMPLEMENT_SPEED_AND_GLACIAL_STRIKE.md` → Parts 1.3, 1.4, 1.5

Add the speed buff fields (`m_bSpeedBuff`, `m_dwSpeedBuffStartTime`, `m_dwSpeedBuffDuration`, `m_fSpeedMultiplier`) to the client structure and hook them into the movement speed calculation. Both the Speed spell, Speed Burst ability, and Glacial Strike sprint phase all use this same buff system.

**⚠️ QUESTION FOR CAMERON:** How does the server currently calculate movement speed/frequency? Is it a timer between allowed movements (like `MOVEMENTFREQUENCY` in the Server Manager), a speed multiplier, or something else? The speed buff needs to integrate with whatever system is already in place. Check `CGame::Process` or the movement packet handler for how the server decides whether enough time has passed for a player to move.

---

### Phase 2: Special Abilities UI (Client-Side)

#### 2A. Ability Circle Rendering
**Reference:** `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` → Parts 1.3, 1.4, 1.5

Implement `InitSpecialAbilities()`, `DrawSpecialAbilityCircles()`, and `UpdateSpecialAbilityTimers()`. This draws the colored circles above the bottom UI bar with cooldown sweeps, timers, and tooltips.

**⚠️ QUESTION FOR CAMERON:** What is the exact Y pixel position of the top edge of the bottom UI bar (the area with bags, character page, HP bar)? The ability circles need to sit immediately above this. Also — does the UI bar position change between windowed and fullscreen modes, or is it always relative to the bottom of the screen?

#### 2B. Click Handling & Network Send
**Reference:** `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` → Parts 1.6, 4.1

Implement `HandleSpecialAbilityClick()` for detecting clicks on circles, and `SendSpecialAbilityActivation()` to send the activation packet to the server.

**⚠️ QUESTION FOR CAMERON:** Where in the client code is mouse click processing done for UI elements? There's likely a function that checks if a click hit a UI button before it falls through to game world interaction. The ability circle click check needs to be added near the top of that chain so it's checked before other UI elements.

---

### Phase 3: New Spells (Server-Side)

#### 3A. Minor Berzerk Spell
**Reference:** `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` → Part 2

Add Minor Berzerk as a purchasable spell. Requires 32 Intelligence. Provides **half** the damage bonus that regular Berzerk provides when cast as a spell.

**⚠️ QUESTION FOR CAMERON:** What is the current Berzerk spell's exact damage bonus? Is it a flat number added to damage, a percentage multiplier, or something more complex? This needs to be known precisely to implement "half of Berzerk." Check the spell effect processing in the server — search for where SPELL_ID_BERZERK is handled in combat/damage code.

**⚠️ QUESTION FOR CAMERON:** What is the current Berzerk spell ID number? And what spell ID range is available for new spells? Check `Magic.cfg` or wherever spell definitions live.

#### 3B. Speed Spell
**Reference:** `IMPLEMENT_SPEED_AND_GLACIAL_STRIKE.md` → Parts 1.1–1.5

Add Speed as a purchasable spell. Requires 62 Intelligence. Cast on OTHER players only (not self). Gives +50% movement speed for 7 seconds. Uses the same visual effect as Invisibility but does NOT make the target invisible.

**⚠️ QUESTION FOR CAMERON:** What is the Invisibility spell's visual effect ID / packet? We need to send the same visual but without setting the invisibility state flag. Find where Invisibility applies its shimmer/transparency effect versus where it sets the actual invisibility flag — they should be separable.

#### 3C. Add Spells to Shops
Both Minor Berzerk and Speed need to be added to the spell shop listings. Minor Berzerk goes wherever Berzerk is sold. Speed goes wherever mid-tier spells are sold.

**⚠️ QUESTION FOR CAMERON:** How are spell shops configured? Is it a config file listing which spells each NPC sells, or is it hardcoded? And are spell shops the same for both Aresden and Elvine, or do they have different spell inventories?

---

### Phase 4: Ability Activation Logic (Server-Side)

#### 4A. Berzerk / Super Berzerk Ability
**Reference:** `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` → Parts 3.1–3.6

Server processes ability 0 activation:
- Player has Minor Berzerk only → cast regular Berzerk on them, yellow hue
- Player has Minor Berzerk + Berzerk → activate Super Berzerk: Berzerk bonuses + 25% extra damage + 20% extra super attack damage, purple hue, 15 seconds duration

10-minute cooldown, tracked server-side.

#### 4B. Speed Burst Ability
**Reference:** `IMPLEMENT_SPEED_AND_GLACIAL_STRIKE.md` → Parts 2.1–2.4

Server processes ability 1 activation:
- Player has Speed spell → gives self +50% movement speed for 10 seconds
- Uses the same speed buff framework from Phase 1C
- 10-minute cooldown

#### 4C. Glacial Strike Ability
**Reference:** `IMPLEMENT_SPEED_AND_GLACIAL_STRIKE.md` → Parts 3.1–3.11

Server processes ability 2 activation. This is the most complex ability:
- Player has Ice Strike spell → 3-second sprint (+50% speed) → if within 1 tile of enemy → unblockable freeze for 7 seconds
- Freeze prevents movement, attacking, and casting
- Super attack animation plays on hit (visual only, no super attack damage)
- Goes on cooldown even if the sprint expires without hitting anyone
- 10-second freeze immunity after being unfrozen (prevents chain-freezing)

**⚠️ QUESTION FOR CAMERON:** Does the server have an existing stun or paralyze mechanic? If so, the freeze can use the same underlying system (lock movement, lock attack, lock cast). If not, we need to add movement/attack/cast lock flags to the client structure and check them in every relevant handler. Also — what is the current Ice Strike spell ID?

**⚠️ QUESTION FOR CAMERON:** For the "within 1 tile" proximity check that runs every server tick during the 3-second sprint — how often does the server tick? Is it fast enough that a sprinting player won't overshoot an enemy between ticks? If the tick rate is slow (e.g., 100ms+), we might need to check along the movement path rather than just the current position.

---

### Phase 5: Spell Targeting Guide — Line Corridor (Client-Side)

**Reference:** `IMPLEMENT_SPELL_TARGETING_GUIDE.md` → Full document

For Blizzard, Earth Shock Wave, and other long-range line spells: show a 4-lane corridor overlay while aiming.

- 2 outer lanes: dark blue
- 2 inner lanes: light blue (sweet spot)
- Updates in real-time as mouse moves
- Snaps to 8 directions for clean grid alignment
- Renders above ground, below characters

Uses `DrawAlphaRect()` from Phase 1A.

**⚠️ QUESTION FOR CAMERON:** Which spells specifically should have the corridor guide? You mentioned Blizzard and Earth Shock Wave — are there others? What about Energy Bolt, Fire Strike, Lightning Bolt? Essentially: which spells travel in a line from the caster across the screen?

**⚠️ QUESTION FOR CAMERON:** The corridor currently shows where the spell WILL land, but does the server actually calculate damage differently based on center vs. edge hit for these line spells? Or is this purely a visual guide for now, with zone-based damage being a future addition? The visual is ready either way — just want to know if the code bot should also modify line spell damage calculations.

---

### Phase 6: AoE Targeting Circle + Zone Knockback (Client + Server)

**Reference:** `IMPLEMENT_AOE_TARGETING_CIRCLE_KNOCKBACK.md` → Full document

For Meteor Strike, Mass Magic Missile, Mass Ice Strike: show a targeting circle while aiming, and apply zone-based mechanics on impact.

#### 6A. Client: AoE Targeting Circle Rendering
Dark outer ring + light blue inner circle, follows mouse, pulse effect, drawn above ground tiles.

#### 6B. Server: Zone Detection & Damage Modification
On spell impact, determine which zone each target is in:
- **Inner zone (light blue):** Base damage, NO knockback (even if damage > 40)
- **Outer zone (dark ring):** Base damage + 10% bonus, ALWAYS directional knockback

#### 6C. Server: Pool Ball Knockback
Targets in the outer zone are launched away from the spell center in the direction determined by their position relative to center. Snapped to 8 directions. Checks for tile passability and stops at walls/obstacles. Default 3 tiles for players, 2 for NPCs.

#### 6D. Client: Knockback Animation
Smooth slide with ease-out deceleration when a player gets knocked back. 300ms animation.

**⚠️ QUESTION FOR CAMERON:** The existing flying mechanic triggers when damage exceeds 40. For these three AoE spells, the new system OVERRIDES the normal flying check entirely (inner zone never flies, outer zone always flies regardless of damage). Does this override feel right, or should outer zone flying still require the > 40 damage threshold? Choosing "always flies in outer zone" makes the knockback more predictable and strategic.

**⚠️ QUESTION FOR CAMERON:** Should the knockback distance (3 tiles) scale with anything — spell power, magic level, damage dealt? Or should it always be a fixed 3 tiles? Fixed is simpler and more predictable for PvP.

**⚠️ QUESTION FOR CAMERON:** What are the actual current AoE radii for Meteor Strike, Mass Magic Missile, and Mass Ice Strike? I estimated 4 tiles for Meteor and 3 for the others. Check the spell definitions for the actual area-of-effect values.

---

### Phase 7: Visual Effects & Polish

#### 7A. Purple Hue for Super Berzerk
**Reference:** `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` → Part 3.5

Find where the yellow Berzerk tint is applied to character sprites and add a purple variant: `RGB(160, 32, 240)`.

**⚠️ QUESTION FOR CAMERON:** How does the current Berzerk yellow hue work? Is it a color tint applied to the sprite surface, a palette swap, or an overlay sprite drawn on top of the character? This determines how to implement the purple variant.

#### 7B. Freeze Visual Effect
**Reference:** `IMPLEMENT_SPEED_AND_GLACIAL_STRIKE.md` → Part 3.10

Blue tint on frozen characters + optional ice particle effects.

#### 7C. Glacial Strike Sprint Trail
Ice particles trailing behind the player during the 3-second sprint phase.

#### 7D. Speed Buff Visual (Invisibility Shimmer)
The shimmer/transparency effect from Invisibility, applied without the actual invisibility flag.

---

## Quick Reference: All New Spell/Ability IDs Needed

| Item | Type | ID Needed |
|------|------|-----------|
| Minor Berzerk | Spell | New spell ID |
| Speed | Spell | New spell ID |
| Ability Activate | Packet | New command ID |
| Ability Result | Packet | New command ID |
| Super Berzerk Effect | Visual Effect | New effect ID |
| Speed Buff Effect | Visual Effect | Reuse Invisibility visual |
| Glacial Sprint Effect | Visual Effect | New effect ID |
| Glacial Freeze Effect | Visual Effect | New effect ID |
| Knockback | Packet | New command ID |

## Quick Reference: All New Client Structure Fields

```cpp
// Speed buff (shared by Speed spell, Speed Burst ability, Glacial Strike sprint)
bool  m_bSpeedBuff;
DWORD m_dwSpeedBuffStartTime;
DWORD m_dwSpeedBuffDuration;
float m_fSpeedMultiplier;

// Super Berzerk
bool  m_bSuperBerzerk;
DWORD m_dwSuperBerzerkStartTime;
DWORD m_dwSuperBerzerkDuration;

// Glacial Strike
bool  m_bGlacialStrikeActive;
DWORD m_dwGlacialStrikeStartTime;
bool  m_bGlacialStrikeUsed;

// Freeze state
bool  m_bFrozen;
DWORD m_dwFrozenStartTime;
DWORD m_dwFrozenDuration;
DWORD m_dwLastFreezeEndTime;

// Ability cooldowns (server-authoritative)
DWORD m_dwLastAbility0Time;  // Berzerk / Super Berzerk
DWORD m_dwLastAbility1Time;  // Speed Burst
DWORD m_dwLastAbility2Time;  // Glacial Strike
```

## Quick Reference: Detailed Documents

| Document | Contents |
|----------|----------|
| `IMPLEMENT_SPECIAL_ABILITIES_BERZERK.md` | Ability circle UI system + Minor Berzerk + Super Berzerk |
| `IMPLEMENT_SPEED_AND_GLACIAL_STRIKE.md` | Speed spell + Speed Burst ability + Glacial Strike freeze ability |
| `IMPLEMENT_SPELL_TARGETING_GUIDE.md` | Line corridor targeting overlay for Blizzard/Earth Shock Wave |
| `IMPLEMENT_AOE_TARGETING_CIRCLE_KNOCKBACK.md` | AoE circle targeting + zone damage + pool ball knockback |

---

## Questions Summary for Cameron

Before the code bot begins, these questions should be answered and added to this document:

1. **16-bit or 32-bit surfaces?** — Determines alpha blending pixel format
2. **Highest current packet/command ID?** — For new packet IDs
3. **Movement speed mechanism?** — Timer-based, multiplier, or other?
4. **Bottom UI bar Y position?** — For placing ability circles
5. **Mouse click processing function?** — Where to add circle click checks
6. **Berzerk damage bonus — exact value and type?** — Flat or percentage?
7. **Current Berzerk spell ID?** — And available spell ID range
8. **Invisibility visual effect ID?** — To reuse for Speed spell
9. **Spell shop configuration method?** — Config file or hardcoded?
10. **Existing stun/paralyze system?** — For freeze implementation
11. **Ice Strike spell ID?** — Current ID number
12. **Server tick rate?** — For Glacial Strike proximity checks
13. **Which line spells get corridor guide?** — Beyond Blizzard and Earth Shock Wave
14. **Line spell zone damage — visual only or gameplay too?** — For corridor guide
15. **Flying override behavior in AoE zones?** — Always fly in outer, or require > 40?
16. **Knockback distance — fixed or scaling?** — 3 tiles fixed vs. variable
17. **Actual AoE radii?** — For Meteor Strike, Mass MM, Mass Ice Strike
18. **Berzerk yellow hue implementation?** — Tint, palette swap, or overlay?
19. **Spell shops same for Aresden and Elvine?** — Or different inventories?
