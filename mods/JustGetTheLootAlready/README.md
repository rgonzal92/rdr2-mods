# JustGetTheLootAlready

Looting and skinning go faster. The game's own animations for looting bodies, picking plants, searching saddlebags and skinning play at double speed, and 17 animal species are skinned with the game's short knife "swipe" instead of the full scene. By the game's data, what is received stays the same.

- **Type:** Lenny's Mod Loader data mod (it replaces game data files; it has no code)
- **Origin:** Rebuilt from the game's own files after Just Get The Loot Already! 1.9 and 1.9.1 by fuhq2mofo. None of the original's files are used.
- **Status:** In use (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

- **Double speed** for looting a body (dead, knocked out or hogtied), picking a plant, taking alligator eggs, and searching the saddlebags of a dead horse.
- **Double speed** for searching the saddlebags of a standing horse.
- **Short swipe skin, at double speed,** for 17 species whose carcass can be carried: deer, buck, pronghorn, bighorn ram, boar, pig, javelina, wolf (all three sizes), cougar, panther, coyote, fox, beaver, goat and the small alligator. The swipe is a short knife stroke the game itself plays for these species when the full skinning scene does not fit where the carcass lies. In the game's data it gives the same pelt, carried the same way, in the same colour.
- **Normal skinning scene, at double speed,** for everything else: bear, bison, cattle, the large alligator, elk, moose, sheep, small animals and birds.
- **Deliberately unchanged:**
  - The Chapter 1 deer that is skinned with Charles keeps the game's animation and speed. A mission script waits on that scene.
  - Animals that a script creates (mission animals, legendary animals) are not sent to the swipe. They keep their full scene, at double speed.
  - The legendary alligator and the veteran's boar, two one-off animals, keep the game's speed.
  - Prompts and hold times, loot contents, picking up weapons, moving a saddle between horses, reviving.

## Changes from the original

The original (1.9, and the 1.9.1 hotfix that replaces its loot file) is kept unchanged in `original/`. This build takes over its two ideas, a higher animation rate and the swipe for skinning, but builds them again on the game's files.

1. **Each species keeps its own pelt.** The original removes the game's per-species skinning rules and sends every animal that is not small to one swipe set, the coyote's. The pelt and the way it is carried come from that set, so a large animal yields a medium pelt. The original's page is reported to list pelts vanishing from the horse after a reload, and stacked pelts all showing the coyote texture, as known problems (taken from search summaries and a mirror of the page, not from the page itself). This build keeps every rule of the game. For 17 species only, it adds a rule that picks the swipe set the game already names as that species' own fallback.
2. **Animals too big to carry stay on the full scene.** Bear, bison, cattle and the large alligator are not on the swipe list. With the swipe as their only skinning set, a carcass it refuses could not be skinned, and it cannot be dragged to a better spot. Elk, moose, sheep and snapping turtle are left out because their swipe set does not give the same result as the full scene.
3. **The Chapter 1 deer hunt is left alone.** Version 1.9 deletes the rule that gives the deer skinned with Charles its own scene, so that mission can stall. The 1.9.1 hotfix restores the rule but plays the scene at double speed. This build changes neither.
4. **Birds and small animals keep their animation.** The original removes the per-species bird rules, replaces plucking in the generic bird set with the knife gesture, and sends small animals to that set too. This build only raises the speed.
5. **No hold-prompt files.** Version 1.9 also ships the files of the same author's Just Do The Thing Already: 11 scenario files, a `prompts.meta` (byte for byte `../JustDoTheThingAlready/optional/prompts.meta`) and a `carryconfigdata.meta`. They have the faults described in `../JustDoTheThingAlready`, and installing them would replace that mod's repaired files. Its `install.xml` also lists a `discoverables.meta` that is not in the download. This build ships the two loot files only.
6. **The loot prompt keeps the game's hold.** The original moves the loot prompt and the saddle-transfer prompt from the short hold class (0.3 s) to the medium one, which only its own `prompts.meta` makes instant. With the game's `prompts.meta` that is 0.6 s, and with this repo's 0.4 s: slower than the game. This build does not touch prompts.
7. **Standing-horse saddlebags are included.** That animation is not in the loot file but in `humaninteraction.meta`, which the original does not ship.
8. **Three speed values instead of one.** The original has one rate for everything. Here body looting, swipe skinning and scene skinning each have their own (see Settings).

Not taken over either: the original's removal of the `ShowBlipWhenFullyLooted` flag from three loot configurations, and its changed blend and slope values.

Quick Loot 1.3 (`../QuickLoot`) was the other candidate. Its method, a loot flag that forces the fallback animation, is not used: the flag also matched horses, large birds and the legendary alligator, and when the game evaluates it could not be established.

## Settings

There is no settings file. The speeds are four values in the two data files. 1.0 is the game's speed; the game's own selectors use 1.0, and up to 1.075 in combat stance.

| Name | File | Default | Meaning |
|---|---|---|---|
| `FAST_LOOT_ANIM_RATE_SELECTOR` | `lootconfigdata.meta` | 2.0 | Body looting, plant and egg picking, dead-horse saddlebags |
| `FAST_SWIPE_SKINNING_ANIM_RATE_SELECTOR` | `lootconfigdata.meta` | 2.0 | The swipe skin and the generic skinning sets by animal size |
| `FAST_SCENE_SKINNING_ANIM_RATE_SELECTOR` | `lootconfigdata.meta` | 2.0 | Skinning scenes per species, small animals, birds |
| `AnimRate` of `Interaction_LootSaddleBags` | `humaninteraction.meta` | 2.0 | Saddlebags of a standing horse |

To tune a group, change the one `<Default value="2.000000" />` of its selector (or the `AnimRate` value) and copy the file to the game folder again.

## Requirements

- Lenny's Mod Loader. The mod is an `install.xml` with whole-file replacements.

## Install

1. Copy the inner folder `JustGetTheLootAlready/` (the one that holds `install.xml`) into `lml/` in the game folder, so that `lml/JustGetTheLootAlready/install.xml` exists.
2. List it in `lml/mods.xml` the way the other mods there are listed: a `<Mod folder="JustGetTheLootAlready">` block with `<Enabled>true</Enabled>`, and a `<Mod>JustGetTheLootAlready</Mod>` line under `<LoadOrder>`.
3. Do not copy anything from `original/`.

To uninstall, delete `lml/JustGetTheLootAlready/` and remove those two entries from `lml/mods.xml`.

It replaces two whole game files, so it cannot be combined with another mod that replaces `lootconfigdata.meta` or `humaninteraction.meta`.

## Files

| Path | What it is |
|---|---|
| `JustGetTheLootAlready/` | The folder that goes into `lml/`. Every file in it is this repo's build. |
| `JustGetTheLootAlready/install.xml` | Loader instructions: two file replacements, with the change list as a comment. |
| `JustGetTheLootAlready/lootconfigdata.meta` | The game's loot settings file with three rate selectors added, 123 animation sets pointed at them, and 17 swipe rules inserted. |
| `JustGetTheLootAlready/humaninteraction.meta` | The game's file with one inserted line (the saddlebag rate). |
| `original/v1.9/JustGetTheLootAlready/` | The original author's download, version 1.9, unchanged: `install.xml` and 14 data files (loot, carry, prompts, 11 scenario files). Not installed. |
| `original/v1.9.1-hotfix/1-Vanilla Mod/lootconfigdata.meta` | The original author's hotfix loot file, unchanged. Not installed. |
| `original/v1.9.1-hotfix/2-Compatibility Patches/` | The original author's loot files for use with two other mods (1899 Firearms, Realistic Loot Rebalance), unchanged. Not installed. |

The two built files were generated by a script from the game's files; the script is not part of this repo. It checks its own output: every difference from the game's file is a changed rate line (123) or an inserted line (284 in `lootconfigdata.meta`, 1 in `humaninteraction.meta`), nothing is removed, each of the 17 species resolves to the swipe set the game names for it, and every other species resolves as before.

## Known limits

- Checked against the game's data files and decompiled scripts for build 1.0.1491.50. Not confirmed in the game.
- **Scene speed:** whether the game applies the rate to skinning scenes is not known. If it does not, large animals, small animals and birds skin at normal speed.
- **Mission animals:** the swipe rule excludes "mission" animals with a test whose answer for a dead animal is not known. If it never matches, mission and legendary animals of the 17 species also get the swipe. If it always matches, nothing gets the swipe.
- **Pelt names:** boar, cougar, wolf, pig, javelina and panther share the deer's swipe set, ram and pronghorn the buck's, and beaver, fox, goat and the small alligator the coyote's. Pelt type and colour match in the data; the pelt's name in the game has not been checked.
- **Camp cooking:** pressing Cook while carrying a carcass skins it during a 3-second fade. At double speed the skin can finish early, with unknown effect.
- **Side effects:** gang members and enemies loot at double speed too. Herb picking, alligator eggs and the washed-ashore stranger event are driven by events inside the animation, which now arrive sooner.
- 2.0 is well above any rate the game itself uses in these files.

## Credits

- Just Get The Loot Already! 1.9 by fuhq2mofo, as named in its `install.xml`, and its 1.9.1 hotfix (loot files only, no `install.xml`). Page given there: <https://www.nexusmods.com/reddeadredemption2/mods/1958>
- Quick Loot 1.3 by CoffeeKFCoffee was examined as an alternative; see `../QuickLoot`.
