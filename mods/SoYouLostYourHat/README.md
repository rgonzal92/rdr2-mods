# SoYouLostYourHat

Puts a hat blip on the map and minimap on the player's lost hat, so it can be found again. It starts once the first story mission is complete.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after So You Lost Your Hat 2.0 (Nexus Mods, mod 442), file `HatBlip2.0.asi` (author not recorded in the mod's files, see Credits)
- **Status:** In use, as of 2026-10-14 (the file in the game folder is identical to `build/SoYouLostYourHat.asi`; not yet tried in game)
- **Game build:** 1.0.1491.50, story mode

An `.asi` plugin is a small Windows library that an ASI loader loads into the game. ScriptHookRDR2 lets such a plugin call the game's own script commands.

## What it does

- Every frame it asks the game for the player's last dropped hat. That is the hat that was knocked or shot off, or lost in a fall.
- The hat gets a blip when all of this is true:
  - the first story mission, Outlaws from the West (`WNT1`), is complete, and its script (`winter1`) is not running, as it is during a replay. That mission puts its own blip on the lost hat,
  - the player is playing: the player has a character that is not dead, injured or arrested,
  - the hat exists and is not being carried,
  - no other script has a blip on the hat,
  - the hat moves slower than 0.5 m/s, or 3 seconds have passed since the plugin first saw it lost while the player was playing. A hat drifting down a river is blipped after the 3 seconds.
- The blip uses the game's own lost-hat style, `BLIP_STYLE_PLAYER_HAT_BLIP`, the one the first mission uses. It brings the hat icon, the game's own localized label `BLIP_HAT`, white colour, the Pickups entry in the map legend, and the height marker.
- The blip stays on the lost hat while the player wears a spare hat. It goes when the hat is picked up, when it no longer exists, when another hat is lost (the blip moves to that one), or while the player is not playing.
- The plugin removes only the blip it added itself, and only while the old hat still exists and still carries that blip. A hat the game deletes takes its blip with it, so the plugin then just forgets the handle.
- It never takes the hat over. The game's own scripts handle it as they would without the plugin. For example, a hat left more than 250 m behind is still removed by the game (`long_update`).
- Nothing is tied to Arthur: the player character is read again every frame, so it also applies to John in the epilogue.
- It has no settings and stores nothing.

## Changes from the original

The original came as a binary without source. The source here was written from its disassembly and then changed.

1. **Checks that the hat exists.** The original never checked. While the player had no hat on and no lost hat existed, it ran the blip calls and the take-over call on handle 0 every frame.
2. **Never takes the hat over.** The original made the hat its own mission entity, taking it from any other script. That blocked the game's own cleanup: `long_update` deletes a hat left over 250 m away, and the Home Robbery stranger mission (`rcm_homerob00`) deletes it only when that script owns it. This build only adds a blip.
3. **Removes only its own blip.** The original removed whatever blip the last dropped hat had, and released the hat, every frame while any hat was worn. That would also have removed a mission's blip on the hat. This build keeps the handle of the blip it added and removes that one, once, when it is no longer needed, after checking that the old hat still exists and still carries it. A deleted hat's blip is already gone, and its handle could by then belong to another script's blip.
4. **Blips a hat that is nearly at rest.** The original needed a speed of exactly 0, so a hat floating in water or jittering on a slope never got a blip. This build blips below 0.5 m/s, or after 3 seconds in any case.
5. **Keeps the blip while a spare hat is worn.** In the original, putting on any hat removed the blip, and taking it off again blipped and took over the hat again. The point of the mod is to find the lost hat, so the blip now stays until that hat is picked up or gone.
6. **Looks like the game's own hat blip.** The original used a plain style (`BLIP_MODIFIER_RADAR_EDGE_NEVER`, default colour, no map-legend category, no height marker) with the hat sprite, scale 0.5 and an English name chosen by player model ("Arthur's Hat", "John's Hat", "Player's Hat"). This build uses the style the first mission uses for the lost hat, with the game's localized label.
7. **Reads the mission result as a BOOL.** The original tested all 64 bits of the result, so stray upper bits could have passed the first-mission check. This build reads 32 bits, as the game returns.
8. **No unused calls, and nothing while the player is not playing.** The original read the hat's and the player's positions every frame and never used them. It also ran while the player was dead or arrested.
9. **Stays out of the first mission while it runs.** The original's only check was that the first mission was complete, which stays true when the mission is replayed. During a replay it would blip the hat as soon as it fell, before the mission makes the hat an objective, and the mission would then add a second hat blip, as it does not check for one. This build does nothing while `winter1` runs.
10. **No C runtime.** The built file is 3 KB instead of 91 KB.

## Requirements

- An ASI loader (`dinput8.dll` in the game folder).
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). Checked against ScriptHookRDR2 V2, version 1.9, which exports the six functions the plugin uses.

## Install

1. Run `make` in this folder, or use the built file that is already in `build/`.
2. Copy `build/SoYouLostYourHat.asi` into the game's main folder, next to `RDR2.exe`.
3. Do not install the original `HatBlip2.0.asi` as well. It would remove this build's blip whenever a hat is worn.

To uninstall, delete `SoYouLostYourHat.asi` from the game folder. The plugin stores nothing, and its blip is not saved.

## Build and test

- Tools: `make`, `clang++` with the `x86_64-pc-windows-msvc` target, `lld` and `llvm-dlltool`. No Windows SDK is needed.
- `make` builds `build/ScriptHookRDR2.lib` from `src/ScriptHookRDR2.def`, then `build/SoYouLostYourHat.asi`.
- `make test` builds `build/HatLogicTest` for the local machine and runs it. Its 15 checks cover the blip decisions in `src/HatLogic.h` without the game: when a hat is blipped, the settle time, other scripts' blips, the blip staying while a spare is worn, and when it is removed or moved.

## Files

| Path | What it is |
| --- | --- |
| `HatBlip2.0.asi` | The original author's binary, version 2.0 (91,136 bytes, linked 2021-03-06, sha256 `b117620f…b931b7`). Kept for reference, not installed. |
| `src/SoYouLostYourHat.cpp` | The plugin: game calls and the per-frame loop. |
| `src/HatLogic.h` | The blip decisions, with no game calls. |
| `src/ScriptHookRDR2.def` | The six functions imported from `ScriptHookRDR2.dll`. |
| `test/HatLogicTest.cpp` | The logic checks run by `make test`. |
| `Makefile` | Build and test rules. |
| `build/` | Build output from this repo's source: `SoYouLostYourHat.asi` (3,072 bytes, the file to install), the import library and the test program. |

## Known limits

- Checked against the game's decompiled scripts and blip style data for build 1.0.1491.50, not confirmed in the game.
- The text of the label `BLIP_HAT` was not looked up. It is the game's own label for the lost-hat blip.
- A hat being carried is recognised by the game reporting it as attached. That a carried or picked-up hat is attached or deleted was not confirmed in the game. Vanilla missions delete the hat object once it is equipped.
- Without the take-over, the game may remove a lost hat the original kept in the world, for example one left more than 250 m behind. The blip then goes with it. That is the game's own behaviour.
- The blip stays while a spare hat is worn. If you no longer want the lost hat, the blip remains until you ride more than 250 m away or lose another hat.
- In a mission that puts its own blip on the hat, this plugin adds none.
- The `winter1` check counts that script's threads with `GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH`. One reading of the engine code says it returns the script's reference count minus one rather than a thread count. Either way it is 0 or less while the script is not loaded, which is not confirmed in the game.
- A replay of the first mission was not tried in the game. The overlap it guards against was read from the `winter1` script.
- The blip is not shown during cutscenes only because the game hides the map and minimap there. The plugin does not check for cutscenes.
- If ScriptHookRDR2 unloads the plugin while a blip is shown, the plugin does not remove that blip, as with the original.

## Credits

- Original mod: So You Lost Your Hat 2.0, Nexus Mods mod 442, distributed as `HatBlip2.0.asi`.
- Author: not recorded in the mod's files.
- The blip style and the first-mission check follow the game's own `winter1` script.
