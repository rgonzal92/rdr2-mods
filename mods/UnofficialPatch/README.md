# UnofficialPatch

Fixes for bugs in the game's own story scripts and data. One plugin with one settings file, one log and one switch per fix, plus three data files for Lenny's Mod Loader.

- **Type:** ScriptHookRDR2 plugin (.asi): a small Windows library that an ASI loader loads into the game and that calls the game's script functions ("natives") through ScriptHookRDR2. The data file is a Lenny's Mod Loader file replacement.
- **Origin:** Written from scratch for this repo. The causes were traced in the audit of 2026-10-13 (`~/Projects/rdr2-tools/audit-2026-10-13-unofficial-patch`, start at `SUMMARY.md`).
- **Status:** In use since 2026-10-13. In the game so far: the night-clothes reading matched the screen once (Mary-Beth); the cooldown fix gave back the expected share on 2026-10-14 (about 73 % at 157 fps with the setting at 40); the knife guard, the night-clothes request and Sadie's name have not been seen acting yet. The late-game parts (sections 5 to 10, added 2026-10-14) have not run in the game yet.
- **Game build:** 1.0.1491.50, story mode

## What it does

### 1. Knife guard: "We Loved Once and True" (plugin, `KNIFE_GUARD`)

- **Bug.** When Arthur confronts the Chelonian leader, the mission puts the fists in his hand and blocks the weapon wheel, but not the mouse wheel and the number keys. The game's choke exists for bare hands only, and the script waits for the choke without a time limit. With a weapon in hand the scene never ends (reported for the knife).
- **Fix.** While the mission's script (`mary1`) runs and Arthur is on foot within 110 m of the Chelonian camp, the plugin disables the mouse-wheel weapon inputs and, when the right hand holds anything but the fists, puts the fists back with the script's own call.
- **Limits it sets itself.** Nothing during a hold or while the holster state changes. Half a second between attempts. After ten attempts on one drawn weapon it stops until Arthur has left the area and says so in the log.
- **Side effect.** In that area, during that mission, Arthur cannot hold a weapon or a tool (binoculars, camera) on foot. The mission needs none there; the next place that needs a gun is about 160 m away.

### 2. Night clothes: camp members awake in their night outfit (plugin, `NIGHT_CLOTHES`)

- **Bug.** When a member wakes, the camp script picks a day outfit at random from a list. Twelve lists hold an entry the member cannot wear (at Horseshoe Overlook: Dutch, Javier, Charles, Abigail). Such a pick keeps the night outfit until the member's next schedule entry, where the same roll happens again.
- **Fix.** Every five seconds the plugin looks for a member who is awake while the wanted outfit is a night outfit, with no decision pending and no script holding the outfit. For that member it sets the script's own "decide again" value. The script then rolls again and changes the outfit its usual way, out of the player's sight.
- **Modes.** `1` (the default) only writes to the log who it found and changes nothing. `2` also sets the value. Mode 1 exists so that the reading of the game's values can be compared with what is on screen before anything is written.
- **Limits it sets itself.** It stays off unless both tables it reads have the expected length of 27. Six requests five seconds apart per member, then one every five minutes. Nothing at Colter and on Guarma, where the script takes no outfit decision. At most 60 put-away lines of the knife guard are logged per game start.
- **Not touched.** A sleeping member, and a member whose schedule says "sleep" in the daytime (Sadie early in Chapter 2: until 09:00 and from 14:10 to 15:00).

### 3. Sadie's name in mission replays (data file)

- **Bug.** Sadie's speaker name in subtitles is the only gang name tied to a "name discovered" value. Only the first mission sets it, and a mission replay starts with all discovered names cleared. In a replay of any later mission her subtitles read "Stranger".
- **Fix.** `lml/Sadie Name On Replay Fix/dialoguecharacters.meta` is this build's file with four lines removed from her entry, which puts the entry back in the form the game launched with.
- **Side effect.** In the first minutes of the first mission she is labelled "Sadie" before she gives her name.

### 4. Encounter cooldowns (plugin, `COOLDOWN_REFERENCE_FPS`, off by default)

- **Bug.** Every random encounter has a cooldown in game seconds. Each frame the game takes a fixed 90 off one of the 120 cooldowns, so the clock runs with the frame rate: a "12 day" cooldown lasts 12.8 hours of play at 30 fps and about 2.5 hours at 157 fps. Confirmed in the game by PatchProbe on 2026-10-13 (30 minutes, every step exactly 90, nothing else lowering a cooldown).
- **Fix.** With the setting at 30, 40 or 60 the plugin gives part of each 90 back, so the clock runs as it would at that frame rate. It writes one cooldown, on the frame after the game's own step on it, a value between what the game left and what was there before. It never touches an encounter the game has brought to 0, and does nothing when the game runs at or below the chosen rate.
- **This changes how the game feels.** Every hold in the table gets longer by the same factor, the short ones too (minutes after an encounter starts or is passed by). Encounters you have not met are not affected.

| | today at 157 fps | 60 | 40 (the game clock) | 30 (consoles) |
|---|---|---|---|---|
| an encounter you only saw | about 40 to 75 min | 1.6 to 3.2 h | 2.4 to 4.8 h | 3.2 to 6.4 h |
| an encounter you finished | about 3 to 3.7 h | 8 to 9.6 h | 12 to 14.4 h | 16 to 19 h |

- **Saved values.** The cooldowns are in the save. Switching the fix off later leaves them where they are and they run down at the game's rate. The only way back to today's values is a copy of the save from before.

### Watch only: trains (`TRAIN_WATCH`)

The game switches an ambient train off while the player is in a town that is short of memory, and only switches it on again inside a town. The plugin logs when one of trains 0 to 5 is switched off while the player is outside every town, and for how long. It writes nothing. The log decides whether a fix is worth building.

### 5. Bill's eyebrows on Guarma (data file)

- **Bug.** Bill's Guarma arrival outfit has no eyebrows.
- **Fix.** `lml/Bill Eyebrows Guarma Fix/stream/cs_billwilliamson.ymt` is this build's ped file for Bill (from `update_4.rpf`, no patch pack overrides it) with the eyebrows added to that outfit. It is loaded as a streaming file, like the installed Arthur's Eyebrows Fix.
- **Side effect.** The file defines Bill for the whole game. Look at Bill in camp once after installing: face, hair, beard, hat and clothes should be unchanged.

### 6. The Cowhand outfit on the horse (data file)

- **Bug.** In the epilogue John's outfit The Cowhand can be stored on the horse but is missing from the horse's wheel, and The Bronco Buster entry has a key typo.
- **Fix.** `lml/Cowhand Horse Outfit Fix/quickselectitems.ymt` is this build's wheel file with both entries corrected, served as a full replace of `update:/x64/packs/base/data/ai/quickselectitems.ymt`. That game path was worked out by comparison, not proven: if `vfs.log` names no replacement for it, the file is not served.
- **Side effect.** The whole wheel file is replaced; after a game update it must be rebuilt from the new one.

### 7. Dreamcatchers entry in the Log (plugin, `DREAMCATCHERS`)

- **Bug.** Every load rebuilds the Log, and the Dreamcatchers entry is added without a test for a finished collection (`main.c` 2661, `func_209`).
- **Fix.** Once all 20 are found, the arrowhead is taken and the entry is present for 60 unpaused readings two seconds apart, the plugin removes the entry. Nothing is saved, so the next load adds it back and the plugin removes it again.
- **Modes.** 1 (default) logs the readings and "would remove"; 2 removes. The table length must read 143.

### 8. Legendary fish map marks (plugin, `FISH_MARKS`)

- **Bug.** short_update sets the last-held legendary fish's map mark back to "not caught" when a pelt or body is put down with empty hands (`short_update.c` 25538). After the fish is mailed nothing sets it back.
- **Fix.** Every ten seconds, for each of 13 fish: if it is mailed, its spot is found and the marks are wrong on two readings in a row, it hides "not caught" and shows "caught", in the game's own order. The Northern Pike has no mark the scripts hold and is not covered.
- **Modes.** 1 (default) logs; 2 sets the marks. Off unless the game lists exactly the expected 14 legendary fish.

### 9. "A New Jerusalem": stuck on the empty backup inventory (plugin, `JERUSALEM`)

- **Bug.** The mission puts John on a second, empty inventory while he builds, and switches back only when a variable of its own script says so (`beechers2_2.c` 17937-17954). A restarted copy of the script can end without the switch, and the autosave keeps John without weapons, satchel or outfits.
- **Fix.** When John is on the backup inventory after the epilogue hand-over (`SP_ENDLESS_SUMMER_WEAPONS` is set only after it), outside any mission and replay, with no bath or other inventory-switching script running, for ten seconds in a row, the plugin makes the mission's own call `_INVENTORY_USE_BACKUP_INVENTORY(false)`. At most three tries per load.
- **Guards from the harm review.** It never switches when John's own inventory holds no weapons. It acts by itself only when the stuck state arose in this session (an inventory-switching script ran since the load); a save that was already stuck when loaded is only logged, because things gathered while stuck sit on the backup inventory and stay behind. `JERUSALEM=3` switches such a save too.
- **Not tested.** How the stuck state arises is not established, so a normal playthrough only shows that the fix stays quiet (a hotel bath is the easy check: the log shows the backup inventory on and off, and no switch).

### 10. Ranch chores after Emmet Granger (plugin, `RANCH_CHORES`)

- **Bug.** The clean-up of Granger's second part always clears the stall-cleaning unlock (bit 5 of `Global_40.f_7854`, saved as `minigameUnlocked`; `rcm_gunslinger1_2.c` 15198). Done after "The Best of Women", it locks stall cleaning and with it cow milking for good.
- **Fix.** At Beecher's Hope (camp 8, game initialised, its camp script running, no ranch mission script and no mission running), when the value reads as "after The Best of Women, stall locked" (bits 0, 1, 3 and 4 set, 5 clear, nothing above bit 6) on three readings a second apart, the plugin sets bit 5, reading and writing in the same tick. The chore comes back at the next camp start or the next day.
- **Saved.** The bit is in the save. It is the value every unaffected save has; switching the fix off leaves it set.
- **Expected values** (for checking the log): 0x0B in Chapter 2 before poker, 0x0F after it and in the epilogue before "Uncle's Bad Day", 0x2F after that, 0x3F after "The Best of Women", 0x1F when affected. Bit 6 may be added in the epilogue.

### Watch only: late game (`LATE_GAME_WATCH`)

Logs, on every change, what two further fixes would decide on. It writes nothing.

- **Cleaver.** Cleaver ammo, whether the player has the cleaver, and its unlock. A fix waits until this shows whether the cleaver can be given back at all.
- **"Advertising, the New American Art".** Each kill-camera episode while `braithwaites1` runs, with its length in unpaused system time, and "WOULD REPAIR" after 3 s. A normal one lasts about 1.5 s.

## Settings

`UnofficialPatch.ini`, section `[FIXES]`, read once when the game loads the plugin. Only the number after `=` is read.

| Name | Default | Meaning |
|---|---|---|
| `KNIFE_GUARD` | 1 | 1 on, 0 off |
| `NIGHT_CLOTHES` | 1 | 0 off, 1 log only, 2 on |
| `COOLDOWN_REFERENCE_FPS` | 0 | 0 off; 20 to 1000: the frame rate the cooldown clock runs as. Anything else is off |
| `TRAIN_WATCH` | 1 | 1 on (log only), anything else off |
| `DREAMCATCHERS` | 1 | 0 off, 1 log only, 2 on |
| `FISH_MARKS` | 1 | 0 off, 1 log only, 2 on |
| `JERUSALEM` | 2 | 0 off, 1 log only, 2 on, 3 on and also for a save stuck when loaded |
| `RANCH_CHORES` | 2 | 0 off, 1 log only, 2 on |
| `LATE_GAME_WATCH` | 1 | 1 on (log only), anything else off |

A missing file or line means the default.

## The log

`UnofficialPatch.log` in the game folder. One line when the plugin starts, with the state of both fixes. Then:

```
knife guard 812345 ms: active, "We Loved Once and True" runs and Arthur is on foot near the camp
knife guard 815000 ms: put weapon 0xDB21AC8C away, attempt 1
night clothes 30000 ms: member table holds 27 and outfit table holds 27, expected 27 each
night clothes 420000 ms: Abigail (13) is awake in night outfit 0xE5769C59, schedule entry type 0x12345678: would ask the script to decide again, sighting 1
night clothes 435000 ms: Abigail (13) is no longer awake in a night outfit after 3 sightings, wanted outfit now 0x9B4F2C11
```

## Requirements

- An ASI loader and ScriptHookRDR2 (`ScriptHookRDR2.dll`). It runs here under ScriptHookRDR2 V2 1.9.
- Lenny's Mod Loader for the data file only.

## Install

1. Close the game.
2. Copy `UnofficialPatch.asi` and `UnofficialPatch.ini` from this folder into the game folder, next to `RDR2.exe`.
3. Copy the three folders under `lml/` into the game's `lml/UI Fixes/` and add each to `lml/mods.xml` (one `<Mod folder="UI Fixes/<name>">` entry and one line in `<LoadOrder>`).
4. Start the game and load a save. `UnofficialPatch.log` gets its start line, and `vfs.log` gets a line naming `dialoguecharacters.meta`.

Switch off: set a fix to 0 in the settings file; for a data file set `<Enabled>false</Enabled>` in `mods.xml` (deleting the entry is not proven to switch it off).

Uninstall: delete `UnofficialPatch.asi`, `UnofficialPatch.ini` and `UnofficialPatch.log` from the game folder, delete the three mod folders and take their entries out of `mods.xml`.

## Build and test

- **Tools:** `make`, `clang++` (cross-compiling to `x86_64-pc-windows-msvc`), `lld` and `llvm-dlltool`. No Windows SDK and no C runtime are needed.
- **`make`** builds `build/UnofficialPatch.asi`. There is no install target.
- **`make test`** builds and runs `build/PatchLogicTest` on Linux. It checks the rules in `src/PatchLogic.h` without the game: the positions of the script values, the scene radius, when the weapon is put away and when the plugin gives up, who counts as stuck in night clothes (per member, because one night outfit is another member's day outfit), the cooldown clock against a model of the game's tick at several frame rates, the train flag masks, the pacing of the requests, the Dreamcatchers, fish-mark, "A New Jerusalem" and ranch-chore rules and the kill-camera timing. It prints `PatchLogic: all checks passed`.

## Files

| Path | What it is |
|---|---|
| `UnofficialPatch.asi` | The plugin to install (a copy of `build/UnofficialPatch.asi`) |
| `UnofficialPatch.ini` | The settings file to install (a copy of `src/UnofficialPatch.ini`) |
| `lml/Sadie Name On Replay Fix/` | The data file and its `install.xml` |
| `lml/Bill Eyebrows Guarma Fix/` | Bill's ped file and its `install.xml` |
| `lml/Cowhand Horse Outfit Fix/` | The wheel file and its `install.xml` |
| `Makefile` | Build and test recipes |
| `src/UnofficialPatch.cpp` | The part that talks to the game |
| `src/PatchLogic.h` | The rules, free of game calls, so they can be tested |
| `src/ScriptHookRDR2.def`, `src/Kernel32.def` | The functions imported from `ScriptHookRDR2.dll` (7) and `KERNEL32.dll` (4) |
| `test/PatchLogicTest.cpp` | The checks behind `make test` |

## Known limits

- **Most of this is not confirmed in the game yet.** The knife guard can first act in "We Loved Once and True" (Chapter 2). Sadie's name needs a replay of a mission after the first one in which she speaks. Night clothes needs mornings in camp. Sections 5 to 10 and the late-game watch need a late save: the Guarma arrival, the epilogue.
- **Knife guard:** how the weapon gets into Arthur's hand in the reported bug is not known, and which of the script's two waits hangs is not known. The fix does not depend on either. Only the right hand is read; whether something in the left hand (a lantern) blocks the choke is not known.
- **Night clothes:** the two table positions are derived from the decompiled scripts and are checked in the game only by their length. A request can fail the same way the first pick did, so a member can stay in night clothes for a few more seconds. The outfit changes only while the member is out of sight, as in the unmodified game.
- **Sadie's name:** the name shown when she is targeted comes from elsewhere and may still read "Stranger" in a replay.
- **Tied to the game build.** After a game update the positions in `src/PatchLogic.h` must be checked against the new scripts, and the data file must be rebuilt from the new build's file.
