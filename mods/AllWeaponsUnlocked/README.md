# AllWeaponsUnlocked

Puts nearly every weapon that a story-mode shop lists on sale from the start of the game, instead of after the story mission that normally releases it. The few exceptions are listed below. Weapons still have to be bought: nothing is handed out and nothing is marked as owned.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after AllWeaponsUnlocked 1.1.0 (author not recorded in the mod's files, see Credits)
- **Status:** In use, as of 2026-10-12 (the file in the game folder is identical to `build/AllWeaponsUnlocked.asi`, and the ASI loader's log lists it as loaded)
- **Game build:** 1.0.1491.50, story mode

An `.asi` plugin is a small Windows library that an ASI loader loads into the game. ScriptHookRDR2 lets such a plugin call the game's own script commands.

## What it does

- The game keeps two flags for each weapon. "Visible" means shops offer it. "Unlocked" is the game's record that the player owns that type of weapon.
- Once a second the plugin marks 126 weapon names as visible. Shops read only that flag when they decide whether a listed weapon can be bought. Buying a weapon unlocks it, as in the unmodded game.
- It repeats this every second because a new game and a mission replay hide every weapon again.
- A name the game has no unlock entry for is skipped.
- It never sets "unlocked", gives no weapon and changes no price or shop list. A shop's other rules still apply: the M1899 Pistol and the LeMat Revolver are still sold only in Saint Denis, and the Evans Repeater still needs its story mission.
- Only 31 of the 126 names are in a story-mode shop list. The other 95 are gang members' weapons, mission items, kit such as the camera, Online-only weapons and alternative names. They are never sold in story mode, so the flag changes nothing for them in shops.

## Changes from the original

The original came as a binary without source. The source here was written from its disassembly and then changed.

1. **Marks weapons visible only.** The original marked every weapon unlocked and visible, every frame. The game's scripts use "unlocked" to judge how far the player has come, so with everything unlocked they:
   - armed enemies and lawmen from the full weapon pool from the first fight (train guards in the prologue, ambush gangs, a sniper in the Valentine church tower),
   - stopped withholding looted thrown weapons and picked-up guns,
   - put the Ornate Dagger on sale at the Saint Denis fence without the vampire hunt,
   - and, most likely, kept all of that in the save.

   This build leaves "unlocked" to the game. The one enemy rule that still changes is listed under Known limits.
2. **Adds 30 names the original left out.** Four are sold by story-mode gunsmiths: M1899 Pistol, Evans Repeater, LeMat Revolver and High Roller Revolver. The rest are Online-only weapons, alternative names of weapons already listed, two story items that are never sold, and three names with no weapon record in the game files that could be read. Those have no effect in story-mode shops.
3. **Drops 17 entries that are not weapons:** "unarmed" and 16 animal attacks.
4. **Skips weapons without an unlock entry.** The original passed an empty entry to the game for the 16 animal attacks.
5. **Re-applies once a second** instead of every frame.
6. **No C runtime.** The built file is 3 KB instead of 118 KB and imports nothing but six ScriptHookRDR2 functions.

## Requirements

- An ASI loader (`dinput8.dll` in the game folder).
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). Loaded here with ScriptHookRDR2 V2, version 1.9.

## Install

1. Run `make` in this folder, or use the built file that is already in `build/`.
2. Copy `build/AllWeaponsUnlocked.asi` into the game's main folder, next to `RDR2.exe`.
3. Do not copy the `AllWeaponsUnlocked.asi` at the top of this folder. It has the same name but is the original 1.1.0.

To uninstall, delete `AllWeaponsUnlocked.asi` from the game folder. Weapons already made visible are expected to stay visible in that save (see Known limits).

## Build and test

- Tools: `make`, `clang++` with the `x86_64-pc-windows-msvc` target, `lld` and `llvm-dlltool`. No Windows SDK is needed.
- `make` builds `build/ScriptHookRDR2.lib` from `src/ScriptHookRDR2.def`, then `build/AllWeaponsUnlocked.asi`.
- There is no `make test`. The plugin is one loop of game calls. A compile-time check in the source makes sure the name hash gives the value found in the original for the Cattleman Revolver.

## Files

| File | What it is |
| --- | --- |
| `AllWeaponsUnlocked.asi` | The original author's binary, version 1.1.0 (117,760 bytes, linked 2022-01-27). Kept for reference, not installed. |
| `src/AllWeaponsUnlocked.cpp` | Source of this build, with the list of 126 weapon names. |
| `src/ScriptHookRDR2.def` | The six functions imported from `ScriptHookRDR2.dll`. |
| `Makefile` | Cross-builds the plugin on Linux. |
| `build/AllWeaponsUnlocked.asi` | Built from this repo's source (3,072 bytes). This is the file to install. |
| `build/ScriptHookRDR2.lib` | Import library made by the build. |

## Known limits

- Checked against the game's decompiled scripts and data files for build 1.0.1491.50, not confirmed in the game. In particular, that "visible" alone is enough to buy every listed weapon follows from the gunsmith script and has not been tried at a gunsmith.
- Four fence entries also ask for "unlocked" and stay as in the unmodded game: the Ornate Dagger, and the hatchet, tomahawk and cleaver sold as throwables.
- One enemy rule reads "visible": in eight ambush scripts the gang's revolver (Cattleman, Schofield or Double-Action) is chosen by which of them are visible. Those ambushers can carry the better revolvers from the start.
- The flags are most likely stored in the save, since the game has a save structure for unlocks. Removing the plugin is then not expected to hide the weapons again. Only a new game or a mission replay resets them.
- A weapon can now be owned before the mission that normally hands it over. The one case known to matter is the Double-Barreled Shotgun in "Paying a Social Call", which [NoWeaponDegradation](../NoWeaponDegradation/) handles. The other hand-over missions were not examined.
- Not known: whether setting "visible" every second brings back the "new" mark on a weapon in shop menus.

## Credits

- Original mod: AllWeaponsUnlocked 1.1.0, distributed as `AllWeaponsUnlocked.asi`.
- Author: not recorded in the mod's files. A third-party mirror of the mod's description ("All Weapons Unlocked V1.1") credits Josefig. This was not confirmed on the original page.
- Original download page: not recorded.
