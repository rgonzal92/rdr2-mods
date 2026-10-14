# UnlimitedHorseWhistlingDistance

Lets the player's horse answer the whistle from any distance. When the horse is too far away to hear, the game itself puts it at a checked spot near the player and sends it over.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after Unlimited Horse Whistling Distance 1.3 by konstantinos96b
- **Status:** In use, as of 2026-10-12
- **Game build:** 1.0.1491.50, story mode

## What it does

The game has a cheat, "Increase Whistle Range For Horse", that works by setting one flag in the data of `player_horse`, the game script that owns the player's horses and answers the whistle. The plugin sets that same flag once a second while that script is running, and does nothing else. The rest is the game's own code:

- A whistle to a horse that is out of hearing range makes the game search for a spot 40 to 80 m from the player, out of the camera's view, on ground with a path to the player. It moves the horse there, unhitches it and sends it over on the normal whistle task.
- Hearing range grows with bonding (the horse's trust level): from 185 m to 350 m in the game's tuning data.
- If no spot is found, the game says "Your horse could not find you" and the horse stays where it is.
- A bonded temporary horse is brought the same way as the main horse.
- It also works from the back of another horse.

Where it deliberately changes nothing:

- A horse within hearing range comes as in the unmodded game, running the whole way.
- The game's whistle rules still apply: no whistle indoors, in caves and tunnels, on Guarma, where a mission blocks it, for a horse with no bond, or in Chapter 1 before the hunting trip with Charles unlocks the whistle.
- A dead horse stays where it is.
- The game's camp rule still applies: a horse standing in camp is not sent to a player who is in camp.
- The cheat itself stays off, and its bookkeeping does not run. Achievements are not switched off, and the "a cheat was used" marker that mission ratings read is not set.
- Nothing is written before a save is loaded, and nothing is stored in the save.

## Changes from the original

Version 1.3 was read from its binary. It watched the on-foot whistle button every frame and, when the horse carrying the saddle was more than 204.77 m away, moved the horse itself and told it to run to the player.

1. **Where the horse appears.** 1.3 chose the spot itself: a road node inside 13 fixed areas, elsewhere a blind offset of about 50 to 70 m from the player, at the height of the ground found below a point 100 m up. That can be a roof or a clifftop, and with no ground found the horse was left at that height. This build leaves the choice to the game: 40 to 80 m away, out of view, with a path to the player, or a message that the horse could not come.
2. **Dead or missing horse.** 1.3 never checked that the horse existed or was alive, so it moved a dead horse's body too. Here a dead horse stays where it is.
3. **Game rules.** 1.3 moved the horse whatever the game's whistle rules said (indoors, caves, missions that hold the horse back). Here those rules run first.
4. **Distance.** 1.3 used a fixed 204.77 m whatever the bond. This build starts where the horse stops hearing the whistle, which depends on bonding.
5. **How the horse comes.** 1.3 gave the horse a 70-second errand to go to the player. Here the horse is unhitched and comes on the game's own whistle task.
6. **Which horse.** 1.3 moved only the horse carrying the saddle. Here a bonded temporary horse is brought as well.
7. **When it works.** 1.3 did nothing while the player sat on any mount, as its readme warns. This build works for every whistle the game handles.
8. **Cost.** 1.3 polled the whistle button every frame and registered a keyboard hook. This build does one check and one write per second.

## Requirements

- An ASI loader: the `dinput8.dll` that loads `.asi` plugins from the game folder.
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports seven of its functions, listed in `src/ScriptHookRDR2.def`, among them `getGlobalPtr` for the flag.
- Game build 1.0.1491.50. The flag's position in the game's script data is tied to the build (see Known limits).

## Install

1. Run `make` to produce `build/UnlimitedHorseWhistlingDistance.asi`.
2. Copy `build/UnlimitedHorseWhistlingDistance.asi` into the game's root folder.

Do not install the `.asi` in this folder's root: that is the original author's 1.3 binary. To uninstall, delete the file from the game folder. Nothing stays in the save, and the flag is gone the next time the game's scripts restart (a loaded save or a game start).

## Build and test

- Tools: `make`, `clang++` with the `lld` linker, and `llvm-dlltool`. The plugin is cross-built on Linux and needs no Windows SDK or C runtime.
- `make` builds `build/UnlimitedHorseWhistlingDistance.asi`, after making the import library from `src/ScriptHookRDR2.def`.
- There is no `make test`. The plugin is one loop with one game call and one write, so it has no logic that can run outside the game.

## Files

| Path | What it is |
|---|---|
| `src/UnlimitedHorseWhistlingDistance.cpp` | The plugin. |
| `src/ScriptHookRDR2.def` | The functions the plugin imports; the Makefile turns it into an import library. |
| `Makefile` | Build rules. |
| `build/` | Build output from this repo's source: `UnlimitedHorseWhistlingDistance.asi` (the installed plugin) and the import library. |
| `UnlimitedHorseWhistlingDistance.asi` | The original author's binary, version 1.3. Kept for reference, not installed. |
| `Read me.txt` | The original author's readme and changelog. |

## Known limits

- Checked against the game's decompiled scripts and data for build 1.0.1491.50, not confirmed in the game.
- The flag is a script global at a fixed index (`Global_1900383.f_316`). The index is the same in the scripts of builds 1355, 1436 and 1491.50. A game update can move it, and a stale index would write into some other variable, so check it against the new scripts before using the plugin on another build.
- Story mode only. In the online script set that index is not the horse flag.
- With a well-bonded horse, hearing range reaches 350 m. A horse between about 205 m and that range now runs the whole way, where 1.3 moved it close.
- A horse that is down but can still be revived, and is out of range, is not excluded by the game's code. It may be moved while down, or the whistle may stop answering until the horse is revived or dies. The game's own cheat has the same gap.
- The map still shows the horse's blip as out of range although the whistle works, and a horse ridden off by a thief beyond normal range still counts as stolen.
- When the main horse and a temporary horse are both out of range, the game may pick the same spot for both.
- The flag is set again once a second, so after a loaded save a whistle in the first second may still be answered the old way.

## Credits

- Original mod: Unlimited Horse Whistling Distance 1.3 by konstantinos96b: <https://www.nexusmods.com/reddeadredemption2/mods/306>. The author's readme is kept in this folder as `Read me.txt`.
