# Quick Fix: Magic Mastery Database Values

## Problem
ALL spells fail with: `Magic XX rejected: mastery not learned (mastery=49)`

The magic mastery values in the database were set to `49` instead of `1`. The server code checks `m_cMagicMastery[sType] != 1` — it must be exactly `1` to mean "learned". Any other value (including 49) is rejected.

## Fix
Update the character save data in the database for character "toke" (account CamB). The magic mastery bytes are stored at byte offsets 243-342 (100 bytes, one per spell) in the character data blob.

Every byte in that range that is currently `49` (0x31) needs to be changed to `1` (0x01).

After updating the database, restart the server so the character reloads with the corrected values.

## Verify
After fix, the debug log should show `mastery=1` and spells should cast successfully. Test with Berserk and Recall.

## After Fix
Delete this file.
