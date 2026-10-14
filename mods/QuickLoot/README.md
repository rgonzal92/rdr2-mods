# QuickLoot

The unmodified download of Quick Loot 1.3, kept for reference. It is meant to make looting bodies and skinning animals quick by forcing the game's short fallback animation (a knife "swipe" for animals). It is not installed: fast looting and skinning come from `../JustGetTheLootAlready` instead.

- **Type:** Lenny's Mod Loader data mod (it replaces one game data file; it has no code)
- **Origin:** Quick Loot 1.3 by CoffeeKFCoffee, unchanged
- **Status:** Not in use: method rejected, kept as a reference copy (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

As its one data file shows. It has not been run in the game.

- It replaces `defaultlootdata.meta`, the file that gives every body and carcass its default loot flags.
- The file is the game's own with one added rule (30 inserted lines, nothing else changed).
- The rule sets the flag `ForceGesticularLootFallbacks`, which asks the game to play the short fallback animation instead of the full loot or skinning animation.
- The rule applies to:
  - every human body, at all times
  - every animal that is not of the "Small" skinning size, while the player is not on a mission (the game's earlier rule in the same file still excludes fish and other animals that cannot be skinned, special characters that cannot be looted, and the extra-small size)
- The rule's flag list is the game's list for human bodies plus that flag. A matched animal therefore also gets flags the game's animal rules do not set (`CanBeQuickLooted`, `CanQuickLootTargets` and, except for dangerous species, `CanLongLootTargets`). What they do on an animal was not examined.
- The file holds no loot contents, no prompt settings and no animation speeds.

## Changes from the original

Nothing was changed. Both files are the download, byte for byte.

What was checked, against the game's data files and decompiled scripts for build 1.0.1491.50:

- **File structure.** The added rule writes its nested conditions as `<Condition>` elements where the game's own files write `<Item>`. It was judged to load as the author intended.
- **Missions.** The "not on a mission" test sits in the animal branch only. Human bodies get the flag on missions too.
- **Animals it catches by mistake.** Dead horses match the rule. Large birds match it and would get a knife skin instead of plucking. The legendary alligator matches it and has no animation data for the fallback.
- **Legendary hunts.** They do not count as missions, so legendary animals get the swipe. Whether the legendary pelt is still registered was not established.
- **When the flag is read.** When the game evaluates the conditions in this file could not be established, so the "not on a mission" test may not protect anything. For human bodies the game's data holds no fallback animation at all.

Decision: not installed and not repaired. The flag is not a safe lever. `../JustGetTheLootAlready` reaches the same goal another way: it raises the speed of the normal animations and picks the swipe per species, inside the loot settings file.

## Requirements

- Lenny's Mod Loader. The mod is an `install.xml` with one whole-file replacement.

## Install

Not installed, and installing it is not recommended (see above). For reference:

1. The folder `original/lml/Quick Loot/` would go into `lml/` in the game folder as `lml/Quick Loot/`.
2. It would be listed in `lml/mods.xml` the way the other mods there are listed.

To uninstall, delete `lml/Quick Loot/` and remove its entries from `lml/mods.xml`.

## Files

| Path | What it is |
|---|---|
| `original/lml/Quick Loot/install.xml` | The original author's loader instructions: one replacement, for `common:/data/ai/looting/defaultlootdata.meta`. Unchanged. |
| `original/lml/Quick Loot/defaultlootdata.meta` | The original author's data file. Unchanged. |

There is no build of this repo in this folder.

## Known limits

- Nothing here has been run in the game. What the flag does to a human body, and whether a legendary pelt survives the swipe, are open.
- The rule and `../JustGetTheLootAlready` change different files, so the loader would accept both. Their effects on skinning would overlap, and that combination was not examined.

## Credits

- Quick Loot 1.3 by CoffeeKFCoffee, as named in its `install.xml`. Page given there: <https://www.nexusmods.com/games/reddeadredemption2/mods/8439>
