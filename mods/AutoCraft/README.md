# AutoCraft

Another author's plugin that lets cooking and crafting at fires run by themselves and triples the amounts. It is kept here only as the original file that was examined. It is not installed; [BatchCrafting](../BatchCrafting/README.md) was written instead.

- **Type:** ScriptHookRDR2 plugin (.asi), the original author's binary only: a Windows library that an ASI loader loads into the game
- **Origin:** AutoCraft "no menu" 1.0.6 by JerryYOJ, unchanged
- **Status:** Not in use: rejected for gameplay bugs and for patching game code, as of 2026-10-12 (no copy is in the game folder)
- **Game build:** 1.0.1491.50, story mode

## What it does

This is what the original does, read from the binary and from the author's published source. The file has never been run here.

- **Where:** only inside the two game scripts that run fires: the player's own camp, and world campfires, fireplaces and the gang camp fire. Crafting from Rest > Craft or from the satchel is untouched.
- **Auto-press:** the prompts "Stow" and "Cook Another" / "Craft Another" fill by themselves after 3.8 seconds, so one item after another is made unattended.
- **Triple amounts:** every item or ammunition amount those two scripts add or remove is multiplied by 3.
- **No controls:** this "no menu" build has no key, menu, setting or off switch. Both behaviours are always on.
- **How:** it patches the game's code in memory. It intercepts nine of the game's script functions ("natives"): four that add or remove items and ammunition, and five that belong to button prompts and the HUD. It finds its way into the game through two byte patterns.

## Changes from the original

Nothing was changed. `original/AutoCraft.asi` is the downloaded file.

What was found, checked against the game's decompiled scripts for build 1.0.1491.50, worst first:

1. **Triple cost for one portion.** Ingredients are removed three sets at a time when a cook starts. Eating or using the result, or brewing coffee, then costs three sets for one portion.
2. **Free items.** The game checks for one ingredient set, then the plugin adds three products. With one or two sets left the player still gets three, and a carried carcass gives triple meat.
3. **Runaway loop.** "Cook Another" fires by itself and removes three more sets at once. The only way out is pressing Back inside each 3.8-second window, and the ammunition loop has no Back at all.
4. **Crash risk at start.** The two byte patterns are never checked for a miss. A miss would crash the game about 50 seconds after start. The plugin was built on 2024-02-01; the game executable of this build is dated 2024-03-11.
5. **Wrong bookkeeping.** Statistics and challenges count one craft while three items move, so crafting challenges cost triple ingredients. Near a satchel limit, items may be lost or exceed the limit.
6. **Two smaller ones.** The cooking lesson with Hosea burns all three granted pieces of meat on the one forced meal. The first item started from the recipe menu is not sped up.

Safety: no file or network access and no access to other processes was found in the binary. It only patches the nine functions in memory.

What was decided:

- **Not installed.** The bugs above cost or hand out items, and patching game code is a heavier method than any plugin installed from this repo uses.
- **No repair from its source.** A repaired build would still patch game code.
- **BatchCrafting instead.** It makes batches with the same natives the game's scripts use, pays for exactly what was made, and patches nothing.

## Requirements

Only if it were used: an ASI loader and ScriptHookRDR2. Its import table names `ScriptHookRDR2.dll`, `KERNEL32.dll` and `USER32.dll`.

## Files

| Path | What it is |
|---|---|
| `original/AutoCraft.asi` | The original author's binary, unchanged and never run. 397,312 bytes, built 2024-02-01, sha1 `30b6dd0302193514554df2ca530b9553139ab693`. |

There is no build from this repo's source in this folder.

## Known limits

- **Everything above comes from reading, not from running.** The machine code was read twice, independently, and compared with the author's published source of version 1.0.5. The binary matches that source hook for hook, with the menu and its two switches removed.
- **Not determined: greyed-out prompts.** If the game lets a disabled prompt fill by itself, the plugin would craft without ingredients, without end.
- **Not determined: the byte patterns.** Whether the two patterns match build 1.0.1491.50 could not be told without running the game.

## Credits

AutoCraft by JerryYOJ.

- Source, MIT licence: <https://github.com/JerryYOJ/RDR2-AutoCrafting>
- Nexus Mods page, as linked from the author's source repository: <https://www.nexusmods.com/reddeadredemption2/mods/3302>
- The author's readme credits Alexander Blade for the ScriptHook SDK and Halen84 for the menu base.
