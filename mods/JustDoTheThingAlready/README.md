# JustDoTheThingAlready

Prompts on objects in the world react to one key press instead of a hold: searching drawers and chests, sitting down, chores, inspecting things, robbing a till. Some longer holds elsewhere in the game (skip a cutscene, sleep, story choices) are shortened, but stay holds.

- **Type:** Lenny's Mod Loader data mod (it replaces game data files; it has no code)
- **Origin:** Just Do The Thing Already! 1.3 "Home Economics" by fuhq2mofo, with changes
- **Status:** In use (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

A prompt is the key hint the game shows next to something that can be used. The prompts of world objects are defined in the game's scenario files: data files that describe what can be done at a chair, a drawer, a campfire and so on. In the game most of them need the key held for 0.25 to 0.7 seconds.

- **One press instead of a hold** for every story-mode prompt in those files:
  - each step of searching furniture and containers: open, search, loot, close (drawers, cabinets, cupboards, chests, wardrobes)
  - sit, rest, take (stew, bottles), eat, drink, pour coffee
  - chores: pick up, put down, load and unload a wagon, chop wood, fill and pour a bucket, feed chickens, cut and peg a carcass
  - inspect and examine objects, peek through a window or door, knock
  - rob a cash register or safe, use a lock breaker, take a pile of money
  - the prompt that ends an activity ("Leave", "Stop", put away a note) at ten spots (campfire seats, a chair, peeking spots, a note)
- **Shorter holds** for prompts that take their length from one of the game's hold classes. A hold class is a named length that many prompts share.

  | Hold class | Game | This build | Used by, for example |
  |---|---|---|---|
  | Medium | 0.6 s | 0.4 s | sleep, fast travel, leave a minigame, story choices, hold Esc to close the pause menu or map |
  | Long | 1.5 s | 0.5 s | skip a cutscene, Buy Max in a shop, tear down camp |
  | Focus | 2 s | 0.4 s | inspect a point of interest |
  | Default (prompt names no class) | 1 s | 0.4 s | meant for menu holds such as satchel discard; see Known limits |

- **Deliberately unchanged:**
  - Looting a body, skinning, hitching, picking up and stowing keep the game's hold. The short hold class (0.3 s) is not changed. These actions share a key with a tap action (E is also mount and dismount, R is also reload). Without the hold, dismount wins over hitch.
  - Crafting and cooking bars and weapon cleaning. Their times are set in scripts and in another data file.
  - The 3 s and 5 s hold classes. No story-mode script was found that uses them.
  - Yes/no alerts such as New Game or Exit Game. The game's data sets no hold on them.
  - No animation is shortened and nothing is skipped. Only the key press that starts an action changes.

## Changes from the original

The original replaces 13 whole game files: 11 scenario files in which the hold lines are commented out, and two game-wide files. This build replaces 12.

1. **The two game-wide files are not installed.** The original's `prompts.meta` sets most hold classes to 0 or 1 ms (short 300 to 1, medium 600 to 0, focus 2000 to 0; long goes from 1500 to 1000) and changes the fill curve of every prompt ring. Its `carryconfigdata.meta` moves pick up, stow and hogtie onto those classes. This reaches far beyond chores. Story decisions use the medium class on R and F, the reload and melee keys, so one stray press would decide them; a camp ledger purchase would fire on a tap. The game tells a tap from a hold on the same key at 250 ms, so with classes below that the two collide: the original's page lists as a known issue that pressing hitch in the saddle dismounts instead. Both files are kept, unchanged, in `optional/`.
2. **Its own `prompts.meta` with shorter holds.** It is the game's file with the three class values in the table above changed and the default hold added (the game's file sets none). Every hold stays a hold and stays above 250 ms. Medium is 0.4 s, not lower, to leave a margin over the 0.25 s gate the shop catalogue uses for its Index hold. Long stays above Medium because Select and Buy Max share a key. The ring fill curves are the game's.
3. **`amb_camp.meta` and `amb_rest.meta` are rebuilt on the current game files.** The original's copies were edited from the launch version of the game. Installing them removed what later patches added: 21 scenario types (one is the dinner-table seat of the Beecher's Hope family meals in the epilogue), the links to smoke and steam effects on smoking, coffee and stew scenarios, and several flags and ped capsule sizes. In this build every scenario file is the file of build 1.0.1491.50 with hold lines turned into XML comments and no other difference (2,008 lines in 11 files).
4. **Getting up is not slowed down.** The original also commented out `TimeToHoldLSForExit` (0.25 s) on 33 scenarios: seats, beds, campfire spots and a few others. A missing value falls back to the engine default, which the game's data definitions give as 0.5 s, so getting up by pushing a direction would take twice as long. This build keeps the game's 0.25 s.
5. **Two paths corrected.** `randomEvent.meta` and `rsc.meta` exist only in the game's base archive. The original addresses them with an `update:` path. The loader replaces a file only when the game opens exactly the given path, so those two replacements would most likely never match. This build uses the `common:` path.
6. **`discoverables.meta` added.** It holds the last two story-mode scenario prompts with a hold: examine a placed artifact, and take the pirate skeleton's clothing. The only others, eleven prompts in a Red Dead Online file (`mp_amb_player.meta`), are left alone.
7. **`scriptambvigs.meta` dropped.** Its only change was one of the 33 exit times.

Unchanged from the original: which scenario prompts lose their hold. All of them do, including Rob, Use Lock Breaker, Take Money, Knock and Drink (the witch's brew).

## Requirements

- Lenny's Mod Loader. The mod is an `install.xml` with whole-file replacements.

## Install

1. Copy the inner folder `JustDoTheThingAlready/` (the one that holds `install.xml`) into `lml/` in the game folder, so that `lml/JustDoTheThingAlready/install.xml` exists.
2. List it in `lml/mods.xml` the way the other mods there are listed: a `<Mod folder="JustDoTheThingAlready">` block with `<Enabled>true</Enabled>`, and a `<Mod>JustDoTheThingAlready</Mod>` line under `<LoadOrder>`.
3. Do not copy `optional/`.

To uninstall, delete `lml/JustDoTheThingAlready/` and remove those two entries from `lml/mods.xml`. The mod writes nothing to the save.

It replaces 12 whole game files, so it cannot be combined with another mod that replaces one of them. The original Just Get The Loot Already 1.9 does: it ships its own copies of most of these files (see `../JustGetTheLootAlready`).

## Files

| Path | What it is |
|---|---|
| `JustDoTheThingAlready/` | The folder that goes into `lml/`. Every file in it is this repo's build. |
| `JustDoTheThingAlready/install.xml` | Loader instructions: 12 file replacements, with the change list as a comment. Name, author, version and link are the original's. |
| `JustDoTheThingAlready/amb_camp.meta`, `amb_rest.meta`, `mech.meta`, `mech_ransack.meta`, `mech_ransack_parent.meta`, `mech_ransack_parent_named_props.meta`, `proc.meta`, `randomEvent.meta`, `rsc.meta`, `sequencescenarios.meta` | The ten scenario files the original also replaces, rebuilt: the game's file with the hold lines commented out. |
| `JustDoTheThingAlready/discoverables.meta` | Scenario file added by this build, made the same way. |
| `JustDoTheThingAlready/prompts.meta` | The game's hold-class file with shorter holds. |
| `optional/prompts.meta`, `optional/carryconfigdata.meta` | The original author's two game-wide files, unchanged. Not installed; kept for reference. Installing `optional/prompts.meta` would replace this build's `prompts.meta` and bring back the problems of change 1. |

The rest of the original download is not kept here. The same author's Just Get The Loot Already 1.9, kept in `../JustGetTheLootAlready/original/v1.9/`, contains scenario files of the same byte sizes as the original's, with the faults listed above, and a `prompts.meta` that is byte for byte `optional/prompts.meta`.

## Known limits

- Checked against the game's data files and decompiled scripts for build 1.0.1491.50. Not confirmed in the game.
- **Rob and reload share R on the keyboard.** The Rob prompt is offered only once the clerk has been attacked or held up, so a calm shop is safe. During a robbery a reload tap next to the register may take the money instead.
- **Easier to trigger by accident:** holding Space for 0.5 s skips a cutscene, holding Select for 0.5 s buys the maximum, and a story choice needs 0.4 s instead of 0.6 s. The game's carry data puts the mercy-kill prompt for a horse on the Long class and cutting a hogtied person free on the Medium class, so those drop to 0.5 s (from 1.5 s) and 0.4 s (from 0.6 s) too.
- **Default hold:** whether the 0.4 s default reaches the menu holds that name no class (satchel discard, shop side-menu buy, Restore Defaults) is a guess from the field's name.
- **`common:` paths:** whether the loader applies `discoverables.meta`, `randomEvent.meta` and `rsc.meta` has not been confirmed. The loader's `vfs.log` has a "Global full replace" line for each file it applies.
- **One unidentified prompt:** a compiled tuning file also names the Long class. Which prompt it drives is not known; it gets 0.5 s too.
- **Two prompts on one key:** the random-event scenario `PROP_PLAYER_RE_FROZEN_TO_DEATH` has its inspect prompt and its "put away note" prompt on the same key. Both are taps now, as in the original.
- Save, load and autosave prompts are built by the game's code. No data file reaches them.

## Credits

- Just Do The Thing Already! 1.3 "Home Economics" by fuhq2mofo, as named in its `install.xml`. Page given there: <https://www.nexusmods.com/reddeadredemption2/mods/1960>
- The idea and the method (commenting out the hold lines) are the original author's.
