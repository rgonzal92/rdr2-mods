# MaxPlayerAttributes

Keeps the player's Health, Stamina and Dead Eye attributes at their highest level.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Written from scratch for this repo
- **Status:** Built 2026-10-16, not yet tried in game
- **Game build:** 1.0.1491.50, story mode

## What it does

The game keeps each attribute's XP as a running total of points; the level shown in the player menu follows from it. Once a second the plugin reads the player's points for Health, Stamina and Dead Eye and, where they are below the game's cap for that attribute (`GET_MAX_ATTRIBUTE_POINTS`), sets them to the cap.

- It only raises. Points already at the cap are left alone, so after the first pass the plugin writes nothing.
- The points are stored in the save, so the levels stay maxed after the plugin is removed.
- John gets the same in the epilogue: the plugin reads the player each pass.

Where it deliberately does nothing:

- The cores (the inner circles that drain with hunger and tiredness) are not touched.
- The Dead Eye ability levels 1 to 5 are unlocked by the story and are not touched; only the Dead Eye attribute (the bar) is maxed.
- Horses are handled by BetterPlayerHorses, which keeps their bonding at the top.

## Why it exists

To have these three attributes maxed without the grind. What was checked against the game's decompiled scripts: none of them writes the player's points for these three attributes (they only read the levels), so nothing in the scripts lowers the values again. The engine awards the XP itself.

## Requirements

- An ASI loader: the `dinput8.dll` that loads `.asi` plugins from the game folder.
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports six of its functions, listed in `src/ScriptHookRDR2.def`.

## Install

1. Run `make` to produce `build/MaxPlayerAttributes.asi`.
2. Copy `build/MaxPlayerAttributes.asi` into the game's root folder.

To uninstall, delete the file from the game folder. The maxed levels stay in the save; only a save from before the install brings back the old levels.

## Build and test

- Tools: `make`, `clang++` with the `lld` linker, and `llvm-dlltool`. The plugin is cross-built on Linux and needs no Windows SDK or C runtime.
- `make` builds `build/MaxPlayerAttributes.asi`, after making the import library from `src/ScriptHookRDR2.def`.
- There is no `make test`. The plugin is one loop of game calls, with nothing that can run outside the game.
- There is no settings file. To leave an attribute alone, remove it from `ATTRIBUTES` in `src/MaxPlayerAttributes.cpp` and rebuild.

## Files

| Path | What it is |
|---|---|
| `src/MaxPlayerAttributes.cpp` | The plugin. |
| `src/ScriptHookRDR2.def` | The functions the plugin imports; the Makefile turns it into an import library. |
| `Makefile` | Build rules. |
| `build/` | Build output from this repo's source: `MaxPlayerAttributes.asi` and the import library. |

## Known limits

- Checked against the game's decompiled scripts for build 1.0.1491.50, not confirmed in the game. That the points cap is the level-10 total is expected from the native's name, not measured.
- The points are set directly, so the game's level-up messages do not show.
