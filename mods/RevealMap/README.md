# RevealMap

Shows the whole map from the first minute: the fog that covers unexplored areas is not drawn while the plugin runs. Nothing is marked as discovered, and the fog is back once the plugin is removed.

- **Type:** ScriptHookRDR2 plugin (.asi): a small Windows library that an ASI loader loads into the game and that calls the game's script functions ("natives") through ScriptHookRDR2
- **Origin:** RevealMap (author not recorded), unchanged
- **Status:** In use, as of 2026-10-12 (the file in the game folder is identical to `RevealMap.asi` here)
- **Game build:** 1.0.1491.50, story mode

## What it does

- Every frame it calls one native, `MAP::SET_MINIMAP_HIDE_FOW(true)`. That tells the game not to draw the "fog of war", the dark cover over map areas the player has not visited. The map is then drawn without it.
- It repeats the call every frame because the game switches the fog back on by itself: at every story start-up (each loaded save), whenever a world-state flag changes, and when a new game is set up. One call would last only until then.
- It reads nothing: no player, horse, vehicle, key or file. So it behaves the same on foot, mounted, in missions, in cutscenes and in the epilogue.
- It does not mark locations as discovered, and it does not change statistics, the compendium, map sketches such as hunting areas, or fast travel destinations. The game's scripts drive those from their own state, not from the fog.
- It is not the game's "Fog of War" cheat. The cheat also calls a second native that marks everything explored for good, and it switches achievements off. The plugin does neither.
- On Guarma the game hides the fog itself, so the plugin changes nothing there.
- In Chapter 1 the whole map is visible, but the limit that keeps the player near Colter is a separate check and still applies.
- After the plugin is removed, the fog is back from the next loaded save. Areas explored before it was installed stay clear.

## Changes from the original

Nothing was changed. `RevealMap.asi` is the original author's file, byte for byte.

What was checked, by reading the binary without running it:

- **Its own code is 146 bytes:** register a script function when loaded, unregister when unloaded, then loop forever over the one native call and a one-frame wait. It also seeds a random number generator once, which nothing uses.
- **All other code is the compiler's runtime:** 400 of its 401 code blocks are identical to code found in other binaries. The one remaining block is the loop.
- **Imports:** `KERNEL32.dll` and six functions of `ScriptHookRDR2.dll`. No network, registry, shell or process-creation functions. No packed or hidden data, no path, no URL.
- **The native in the game's scripts:** all ten places where the game calls it were read (Guarma on, everywhere else off, the cheat, new-game set-up), and every player situation was walked through against them.
- **Other mods:** no other plugin source in this repo calls a fog native, and no data mod installed at the time of the check replaced a script, a minimap file or a fog file.

What was decided: no bug was found, so the original binary is used as it is, not a rebuild from source. A rebuild would need to keep two things: the call every frame (with a timer the fog would come back for a moment after each world-state change), and only this native (adding the cheat's reveal call would make the reveal permanent; in the scripts only a new game undoes it, by wiping the map).

## Requirements

- An ASI loader and ScriptHookRDR2 (`ScriptHookRDR2.dll`). The six functions the plugin imports are all exported by the ScriptHookRDR2 in use here (V2 1.9).
- Lenny's Mod Loader is not needed.

## Install

1. Close the game.
2. Copy `RevealMap.asi` into the game folder, next to `RDR2.exe`.

Uninstall: delete `RevealMap.asi` from the game folder, or rename it to `RevealMap.asi.disabled`. Nothing else is left behind: the plugin writes no file, and the game's scripts switch the fog on again at every loaded save.

## Files

| Path | What it is |
|---|---|
| `RevealMap.asi` | The original author's binary, unchanged. 91,136 bytes, built 2022-01-03, sha1 `9c949eba73c31ca59521c82a9fd83925d461c64e`. No source is included. |

## Known limits

- **Not confirmed in the game.** Everything above was checked against the game's scripts and data for build 1.0.1491.50 and against the binary. The loaders' logs show the plugin being loaded and its script thread started; that the map shows no fog has not been confirmed.
- **Exploration while the fog is hidden.** The game uses separate switches for "hide the fog" and "the player clears fog", so the areas visited with the plugin on are very likely still recorded. This is not proven. Test: ride somewhere new, save, disable the plugin, load the save and look at the map.
- **A one-frame return of the fog** is possible in the frame in which the game resets the flag. It is not expected to be visible.
- **"The Fine Joys of Tobacco"** marks its fields with map areas whose style name contains "FOW". The game's data defines them as ordinary area markers, so the plugin should not hide them. Not confirmed.
- **What the native does inside the game's code** could not be read. The conclusions rest on how the game's own scripts use it.
- **Spoils discovery.** The whole map is visible from the start of a new game. That is the purpose of the plugin, and there is no setting to limit it.

## Credits

RevealMap. The author and the download page are not recorded in the file or in this repo.

The download archive was named `RevealMap.asi-1245-1-0-0-1641246140.rar`. By the naming of the other mods' download archives (name, Nexus Mods mod id, version, time stamp) that would be mod 1245, version 1.0.0. The page itself could not be opened, so no link is given.
