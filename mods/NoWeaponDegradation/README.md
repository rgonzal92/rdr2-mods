# NoWeaponDegradation

Keeps the weapons the player carries clean, so they do not wear out and need no gun oil. It steps aside for the one story mission that teaches weapon cleaning, so that mission can still be finished.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after No Weapon Degradation V2 (author not recorded in the mod's files, see Credits)
- **Status:** In use, as of 2026-10-12 (the file in the game folder is identical to `build/NoWeaponDegradation.asi`, and the ASI loader's log lists it as loaded)
- **Game build:** 1.0.1491.50, story mode

An `.asi` plugin is a small Windows library that an ASI loader loads into the game. ScriptHookRDR2 lets such a plugin call the game's own script commands.

## What it does

- Once a second it looks at the weapons at six places on the player character: both hands, both hip holsters, and the two long-gun places on the back and shoulder.
- Each weapon is left as the game's own cleaning with gun oil would leave it. Wear (the game calls it degradation, from 0 for good to 1 for poor) and visible damage go down to the weapon's permanent wear. Dirt and soot go to 0.
- Permanent wear is wear that no cleaning removes. Guns picked up from enemies carry some. It stays as it is.
- While the Chapter 2 mission "Paying a Social Call" runs, nothing is cleaned. That mission teaches cleaning and cannot end until the player has cleaned a dirty Double-Barreled Shotgun.
- During that mission a Double-Barreled Shotgun that is already spotless is given a little wear (0.1 above its permanent wear). The game offers "Clean" only on a weapon that has something to clean.
- It does nothing while there is no player character, for example before a save is loaded.
- Weapons that are not on the player's body, such as those stored on the horse, are not touched.

## Changes from the original

The original came as a binary without source. The source here was written from its disassembly and then changed.

1. **Holstered weapons are kept clean too.** The original cleaned the weapons in the two hands and asked for two more places on the body that the game's own scripts never use for the player's weapons. It never asked for the four holster places they do use, so a holstered gun was most likely not cleaned until it was drawn.
2. **Every weapon is left alone during "Paying a Social Call".** The original spared only the Double-Barreled Shotgun, and only while it was in a hand.
3. **A spotless shotgun gets something to clean in that mission.** A player who already owns a Double-Barreled Shotgun has a clean one, because the plugin keeps it clean. If that is the shotgun in hand at the cleaning step, the game offers no "Clean" and the mission cannot end. The shotgun can be owned early when it was bought before the mission (possible with [AllWeaponsUnlocked](../AllWeaponsUnlocked/)) or taken from the rack at Six Point Cabin before the mission.
4. **Cleans exactly as far as the game's gun oil does.** The original wrote zero wear to every weapon, also to picked-up worn guns, and passed a flag value that none of the game's own calls use. This build stops at the permanent wear and uses the game's own commands and arguments.
5. **Checks that a player character exists** before asking for weapons. The original did not.
6. **Re-applies once a second** instead of every frame.
7. **No keyboard hook, no C runtime.** The original registered a key handler that only recorded key presses nothing read. The built file is 3 KB instead of 253 KB.

## Requirements

- An ASI loader (`dinput8.dll` in the game folder).
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). Loaded here with ScriptHookRDR2 V2, version 1.9. The original's readme asks for ScriptHookRDR2 V2: <https://www.nexusmods.com/reddeadredemption2/mods/1472>

## Install

1. Run `make` in this folder, or use the built file that is already in `build/`.
2. Copy `build/NoWeaponDegradation.asi` into the game's main folder, next to `RDR2.exe`.
3. Do not copy the `NoWeaponDegradation.asi` at the top of this folder. It has the same name but is the original.

To uninstall, delete `NoWeaponDegradation.asi` from the game folder. The plugin writes nothing but the condition of the weapons, so there is nothing else to undo.

## Build and test

- Tools: `make`, `clang++` with the `x86_64-pc-windows-msvc` target, `lld` and `llvm-dlltool`. No Windows SDK is needed.
- `make` builds `build/ScriptHookRDR2.lib` from `src/ScriptHookRDR2.def`, then `build/NoWeaponDegradation.asi`.
- `make test` builds `build/WeaponCareTest` as a normal Linux program and runs it. It checks the decision in `src/WeaponCare.h` without the game:
  - a worn weapon is cleaned down to its permanent wear and no further,
  - a clean weapon is left alone,
  - nothing is cleaned during the cleaning mission, including the dirty shotgun it hands over,
  - a spotless shotgun gets its small wear in that mission, and only once.

## Files

| File | What it is |
| --- | --- |
| `NoWeaponDegradation.asi` | The original author's binary (252,928 bytes, linked 2025-04-02). Kept for reference, not installed. |
| `!README.txt` | The original author's install note. |
| `src/NoWeaponDegradation.cpp` | Source of this build: the loop and the game calls. |
| `src/WeaponCare.h` | The rule that decides what to write to one weapon. No game calls. |
| `src/ScriptHookRDR2.def` | The six functions imported from `ScriptHookRDR2.dll`. |
| `test/WeaponCareTest.cpp` | The checks run by `make test`. |
| `Makefile` | Cross-builds the plugin on Linux and runs the test. |
| `build/NoWeaponDegradation.asi` | Built from this repo's source (3,072 bytes). This is the file to install. |
| `build/ScriptHookRDR2.lib`, `build/WeaponCareTest` | Made by the build. |

## Known limits

- Checked against the game's decompiled scripts and data files for build 1.0.1491.50, not confirmed in the game. The important check is still open: in "Paying a Social Call", take the shotgun, choose Maintain, and see that "Clean" is offered.
- Not known: what the game hands over at the cabin rack when the player already owns a Double-Barreled Shotgun. Change 3 is there for that case.
- While that mission runs, a shotgun that has just been cleaned is spotless again and so gets the small wear back within a second. It is cleaned like every other weapon once the mission is over.
- A weapon can show up to one second of fresh dirt.
- Only the six places above are looked at. A bow on the back, the knife and other holstered gear are covered only while they are in a hand.
- The plugin writes to whatever is in a hand, including a lantern or the lasso. No effect is expected there; this is not confirmed.
- Expected side effect: gun oil, the gunsmith's cleaning service and gear that slows wear have nothing left to do, apart from the shotgun in the cleaning mission.

## Credits

- Original mod: No Weapon Degradation V2, distributed as `NoWeaponDegradation.asi`.
- Author: not recorded in the mod's files. A third-party mirror of the mod's description ("No Weapon Degradation V2.0", change log up to V1.3) credits Rspect. This was not confirmed on the original page.
- Original download page: not recorded.
