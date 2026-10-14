# BetterPlayerHorses

Gives the player's own horses top stats: health and stamina as good as the game's best breeds, and the highest agility, speed and acceleration the game allows. The horses of lawmen, enemies and gang members keep their normal stats.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Written from scratch for this repo
- **Status:** In use, as of 2026-10-12
- **Game build:** 1.0.1491.50, story mode

## What it does

Each horse stat has a "base rank" from 0 to 9, which the game shows as level 1 to 10. Once a second the plugin looks up two horses, the player's main horse and the horse carrying the player's saddle, and raises five base ranks on each:

| Stat | Rank set | The game's own best |
|---|---|---|
| Health | 6 | 6 |
| Stamina | 6 | 6 |
| Agility | 9 | 7 |
| Speed | 9 | 6 |
| Acceleration | 9 | 5 |

- It only raises. A rank that is already higher is left alone.
- The ranks are not stored in the save. The game creates the horse again after a loaded save, a stable visit or a fast travel, with its breed's ranks, and the plugin raises them again within a second.
- A stabled horse gets the ranks when it is taken out as the main horse.

Where it deliberately does nothing:

- A temporary horse without the player's saddle is not the player's, so it is left alone.
- Stabled horses, horses lent by a mission, and the horses of lawmen, enemies and gang members are never touched.
- The game's horse attribute file (`pedattributes.ymt`) is not replaced.
- With no horse, and before a save is loaded, nothing is written.

## Why it exists

The same ranks can be had from a data mod, "Better All Horses - Elite", which replaces `pedattributes.ymt` through Lenny's Mod Loader. That mod was rejected for these reasons:

- The file sets ranks per stat set, and every horse model that uses a set gets its ranks, whoever rides it. The Elite file gives 75 stat sets these ranks, and 143 of the 147 horse, donkey and mule models in the game's ped table use one of them. Lawmen, enemies and gang members then ride horses as fast as the player's, and breed stops mattering.
- Only one mod can replace that file, so it cannot be combined with another horse stat mod.

A plugin can tell the player's horses from all others, which a data file cannot. The rank values are the Elite mod's.

What was checked against the game's scripts: none of them writes these five base ranks on any horse (they write base ranks only for bonding, hunger, weight and dirtiness), so nothing undoes the plugin's values between its passes.

Not built: a fix for the pause menu showing breed values (see Known limits). It would need a data file that raises the breed table for the stat set of one horse, which also boosts every other horse sharing that set.

## Requirements

- An ASI loader: the `dinput8.dll` that loads `.asi` plugins from the game folder.
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports six of its functions, listed in `src/ScriptHookRDR2.def`.
- No data mod is needed. The plugin works on the game's own horse attribute file.

## Install

1. Run `make` to produce `build/BetterPlayerHorses.asi`.
2. Copy `build/BetterPlayerHorses.asi` into the game's root folder.

To uninstall, delete the file from the game folder. Nothing stays in the save: a horse has its breed's ranks again the next time the game creates it.

## Build and test

- Tools: `make`, `clang++` with the `lld` linker, and `llvm-dlltool`. The plugin is cross-built on Linux and needs no Windows SDK or C runtime.
- `make` builds `build/BetterPlayerHorses.asi`, after making the import library from `src/ScriptHookRDR2.def`.
- There is no `make test`. The plugin is one loop of game calls, with nothing that can run outside the game.
- There is no settings file. To change the ranks, edit `RANKS` in `src/BetterPlayerHorses.cpp` and rebuild.

## Files

| Path | What it is |
|---|---|
| `src/BetterPlayerHorses.cpp` | The plugin. |
| `src/ScriptHookRDR2.def` | The functions the plugin imports; the Makefile turns it into an import library. |
| `Makefile` | Build rules. |
| `build/` | Build output from this repo's source: `BetterPlayerHorses.asi` (the installed plugin) and the import library. |

No file of another author is kept in this folder. The Elite data mod is not in the repo.

## Known limits

- Checked against the game's decompiled scripts and data for build 1.0.1491.50, not confirmed in the game. That a raised base rank changes how the horse performs is expected, not measured.
- The screens disagree. The pause menu reads speed, acceleration and handling from the breed table, so it shows the breed's values for a boosted horse. It reads health and stamina from the horse itself while the horse exists in the world. Stables and tack shops read the horse itself and show the raised ranks.
- Tack adds bonus ranks to speed and acceleration, and a horse-race reward adds one to speed, on top of the base rank. 9 plus a bonus is above the top rank. The plugin relies on the game stopping at 9, which is not confirmed. If it does, those bonuses add nothing on a boosted horse.
- Bonding adds health and stamina at bonding levels 2, 3 and 4. Rank 6 plus those three steps is exactly 9. Whether the game keeps that bonus apart from the base rank is not known.
- For up to a second after the game creates a horse, it has its breed's ranks. The larger health and stamina tanks may then start partly empty and fill up by themselves.
- A horse that stops being the main horse, for example after the saddle is moved to another one, keeps its raised ranks until the game creates it again. Nothing lowers a rank.
- Nothing in the game's attribute file takes agility as an input, so what agility 9 changes in play is not known.

## Credits

- The rank values come from "Better All Horses - Elite", a download of the Better Horses mod on Nexus Mods (mod 842) by MaVeRicK: <https://www.nexusmods.com/reddeadredemption2/mods/842>. Its version is not recorded.
