# RemoveSpeedLimits

Lifts the slow pace the game forces on the player and on the horse being ridden: around camps, towns and bridges, indoors, and where a mission caps the horse's pace. On foot it also gives a flat-out sprint, a fast walk switched with the cover key, and a tap of the sprint key to drop from a run back to a walk.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after Remove Speed Limits 1.2 by alfabravozapa
- **Status:** In use, as of 2026-10-12
- **Game build:** 1.0.1491.50, story mode

## What it does

Pace in this game is a "move blend ratio": 1 is a walk, 2 a run, 3 a sprint. A script can set a lowest and a highest ratio for a person or a horse, and each setting lasts one frame. The plugin reads the game's controls, not fixed keys. On a keyboard the defaults are Left Shift for sprint, Q for cover and W for forward.

- **Slow zones.** Every frame the player is exempted from the speed zones around camps, towns and bridges and from the indoor limit, and the horse being ridden is exempted from the zones. This uses the two per-frame flags the game's own scripts set for the same purpose.
- **Sprint on foot.** Sprint plus a direction holds a flat-out sprint (ratio 10.6). After sprint is let go, the player keeps running (ratio 2.0) until they stop or tap sprint.
- **Tap to walk.** While running, a sprint press shorter than 250 ms drops the player to a walk without stopping. The pace stays capped at a walk until the next sprint press or a stop. A press from a walk sprints at once.
- **Fast walk.** Pressing cover while moving switches a fast walk (ratio 1.5) on or off. That press does not take cover. While fast walk is on, the walk after a tap is the fast walk. Fast walk is off after every game start and every loaded save.
- **On horseback.** While forward or sprint is held, the horse's pace cap is lifted and no lowest pace is set, so the gait stays the rider's choice. While sprint is held on a horse that already gallops, the gallop is kept.

Where it deliberately does nothing:

- On a horse or in a wagon, boat or other vehicle, the on-foot rules are off. The rider's own pace is left to the game, and cover does not switch fast walk.
- On foot with no direction held, no pace is held. Sprint alone does not move the player.
- Standing still, the cover key belongs to the game: it takes cover and fast walk does not change. In cover the key is left alone too.
- Cover presses made while the game has taken control (shops, scripted scenes) are ignored.
- Holding sprint on a horse below a gallop forces nothing. The game reads that hold as "keep this pace" and "match my companion's speed".
- With neither forward nor sprint held on horseback, the horse's pace is left to the game.
- Before a game is loaded there is no player yet, and the plugin waits.

## Changes from the original

The original was read from its binary (`RemoveSpeedLimits.asi`, built 2023-11-12). Its on-foot speeds, its keys and its `WALKING_SPEED` setting are kept.

1. **On-foot rules while riding.** The original ran every rule in every state. With sprint held it also held the rider at a sprint, and cover switched fast walk from the saddle. The expected effect is the rider losing stamina while riding. This build applies the on-foot rules only on foot.
2. **Forced canter.** On a running horse with forward held, the original set a lowest pace of 1.6 (a canter) that kept itself going. The horse could not slow to a trot or keep pace with a slower companion, and the game's first ride asks keyboard players to hold forward to match speed with Dutch. This build sets no lowest pace below a gallop.
3. **Forced gallop.** The original forced a full gallop after about three seconds of sprint held at a canter with forward let go, where the game reads the hold as "keep this pace". This build keeps a gallop only while sprint is held on a horse that already gallops.
4. **Speed zones on horseback.** The original set the zone flags on the player only and fought the zones with the forced pace. This build sets the flag on the horse too, as the game's own scripts do.
5. **Sprint with no direction.** The original held a sprint whenever sprint was down. A lowest pace is expected to make a person move with no input. This build holds a pace only while a direction is held.
6. **Stray fast-walk switches.** The original switched fast walk on every cover press it read out of cover. It did not check that the player was moving or in control, and it read the key even while the game had it disabled. The press that takes cover from a standstill, and presses in shops and scripted scenes, are expected to have switched it too. This build switches only on a press made while moving, with the player in control.
7. **Undefined start.** The original read its four state values before setting them, and the fast-walk switch worked only when the leftover value happened to be 0 or 1. This build starts with fast walk off and nothing held.
8. **Missing setting.** The original took a missing `WALKING_SPEED` as 0, which stopped the player from walking once fast walk was on. This build uses 150 and keeps the value between 100 and 300.
9. **Cover permission.** To keep the player out of cover while moving, the original switched the player's ability to take cover off and back on, which undid missions that had switched it off. This build disables the cover key for the frame, as the game's own scripts do.
10. **Pause after a switch.** The original slept a tenth of a second after each cover press, so the limits came back for those frames. This build does not sleep. It ignores a second press within a tenth of a second.
11. **Feature switches.** Each of the four features can be switched off in the settings file. The original had one setting.
12. **New: tap to walk.** The original kept the run until every direction was let go, and a keyboard has no stick to ease off.
13. **Housekeeping.** Control hashes are worked out when the plugin is built (the original asked the game every frame), and the original's unused keyboard hook is gone.

## Settings

`RemoveSpeedLimits.ini` sits beside the plugin in the game folder and is read once, when the game starts. Only the number after `=` is read, so the text after it is a note. A missing file or key gives the default.

| Section | Name | Default | Meaning |
|---|---|---|---|
| `[WALKING]` | `WALKING_SPEED` | 150 | Fast-walk pace times 100 (150 is ratio 1.5). Kept between 100 and 300. Missing, 0 or not a number means 150. |
| `[WALKING]` | `TAP_TO_WALK_MS` | 250 | Longest sprint press, in milliseconds, that counts as a tap while running. 0 switches tap to walk off. Needs `SPRINT_BOOST=1`. |
| `[FEATURES]` | `HORSE_BOOST` | 1 | The horseback rules. 0 leaves the horse's pace to the game. |
| `[FEATURES]` | `SPRINT_BOOST` | 1 | The on-foot sprint and the run that follows it. |
| `[FEATURES]` | `COVER_WALK_TOGGLE` | 1 | Fast walk on the cover key. 0 leaves the cover key alone; fast walk is then off. |
| `[FEATURES]` | `REMOVE_ZONE_SLOWDOWN` | 1 | The exemption from speed zones and the indoor limit. |

## Requirements

- An ASI loader: the `dinput8.dll` that loads `.asi` plugins from the game folder.
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports six of its functions, listed in `src/ScriptHookRDR2.def`, and one `KERNEL32.dll` function to read the settings file.

## Install

1. Run `make` to produce `build/RemoveSpeedLimits.asi`.
2. Copy `build/RemoveSpeedLimits.asi` into the game's root folder.
3. Copy `src/RemoveSpeedLimits.ini` into the same folder.

Do not install the `.asi` and `.ini` in this folder's root: those are the original author's files. To uninstall, delete the two files from the game folder. The plugin stores nothing in a save.

## Build and test

- Tools: `make`, `clang++` with the `lld` linker, and `llvm-dlltool`. The plugin is cross-built on Linux and needs no Windows SDK or C runtime.
- `make` builds `build/RemoveSpeedLimits.asi`, after making the two import libraries from the `.def` files.
- `make test` builds `build/SpeedLogicTest` for the local machine and runs it. Its 20 checks cover the pace decisions in `src/SpeedLogic.h` without the game: the walking-speed range, sprint, the run after a sprint, tap to walk, the fast-walk switch, riding, and the horse rules.

## Files

| Path | What it is |
|---|---|
| `src/RemoveSpeedLimits.cpp` | The plugin: settings, game calls and the per-frame loop. |
| `src/SpeedLogic.h` | The pace decisions, with no game calls. |
| `src/RemoveSpeedLimits.ini` | Settings file for this build. Install this one. |
| `src/ScriptHookRDR2.def`, `src/Kernel32.def` | The functions the plugin imports; the Makefile turns them into import libraries. |
| `test/SpeedLogicTest.cpp` | The logic checks run by `make test`. |
| `Makefile` | Build and test rules. |
| `build/` | Build output from this repo's source: `RemoveSpeedLimits.asi` (the installed plugin), the import libraries and the test program. |
| `RemoveSpeedLimits.asi` | The original author's binary, version 1.2. Kept for reference, not installed. |
| `RemoveSpeedLimits.ini` | The original author's settings file (`WALKING_SPEED` only). |

## Known limits

- Checked against the game's decompiled scripts and control data for build 1.0.1491.50 and by the logic tests, not confirmed in the game.
- The ratios 10.6 and 10.0 are the original's. The game's own scripts never pass more than 3, and whether the game treats a larger value as faster than its own sprint is not known.
- The plugin and a mission script can both set a pace for the same frame. Which one counts is not known, so sprint may or may not override a scripted slow section.
- Whether the flag on the horse lifts every speed zone by itself has not been tried in the game.
- Holding sprint on foot is always the flat-out sprint. A sprint from a run starts `TAP_TO_WALK_MS` (250 ms) late, because the press could still be a tap.
- Holding sprint on a galloping horse keeps the full gallop. The unmodded game uses that hold to keep pace with a companion or a train, so matching speed at a gallop is expected not to work while sprint is held.
- With `COVER_WALK_TOGGLE=1`, cover can only be taken from a standstill. Nothing on screen shows whether fast walk is on, and fast walk holds the pace at exactly `WALKING_SPEED`.
- On the keyboard the cover key is also the key for tagging Dead Eye targets, studying an animal and turning a prompt page. Such a press made while moving on foot is expected to switch fast walk as well.
- The holds look only at the keys, so the run after a sprint and the fast walk also apply while aiming, crouching, carrying or swimming.
- Tap to walk suits keyboard play. The game's default gamepad scheme uses a tap of the same button to sprint.
- The zone flags are set every frame, also in cutscenes.
- The game calls in `src/RemoveSpeedLimits.cpp` have no automated test.

## Credits

- Original mod: Remove Speed Limits 1.2, published on Nexus Mods by alfabravozapa: <https://www.nexusmods.com/reddeadredemption2/mods/975>. The downloaded files do not name an author; the name is taken from the mod's Nexus Mods listing.
