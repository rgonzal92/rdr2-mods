# ShowWaypointDistance

Shows how far away you are from where you are going, as a small number in metres below the mini-map. For your own map waypoint the number is the straight-line distance on the map. With no waypoint set, it shows the length of the yellow GPS route the game draws to a mission or encounter objective, measured along the route.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after Show Waypoint Distance by srgamerio (version not recorded)
- **Status:** In use, as of 2026-10-14 (the file in the game folder is identical to `build/ShowWaypointDistance.asi`; not yet tried in game)
- **Game build:** 1.0.1491.50, story mode

An `.asi` plugin is a small Windows library that an ASI loader loads into the game. ScriptHookRDR2 lets such a plugin call the game's own script commands.

## What it does

- While a waypoint is set on the map and the game shows the radar (the mini-map), it draws the distance from the player to the waypoint, for example `87 m` or `1235 m`.
- The distance is measured on the map plane, in whole metres, halves rounded up.
- The text is white with a black copy behind it as a shadow. It sits at a fixed spot near the bottom left of the screen, the same spot and size the original used.
- With no waypoint set, but a GPS route drawn by the game (the yellow route to an objective, for example in story missions, strangers, random encounters, bounties and coach robberies), it draws the length of that route, for example `route 1235 m`. This is the distance along the route, not a straight line, so the label differs. Your own waypoint wins when both are set.
- If the game briefly reports a length of 0 while it works out a route again (expected, not confirmed), the last good length stays on screen for up to half a second of game time. After that, or when the route is gone, the number is cleared, so a length that belonged to an earlier route or that really reached 0 does not stay up.
- It draws nothing when neither a waypoint nor a game route is set.
- It draws nothing while the game reports the radar as hidden. The game's own scripts hide the radar in shop catalogues, in baths, on stagecoach rides and in many mission scenes.
- A distance that cannot be real (negative, not a number, or 100 km and more) is shown as `? m`.

## Changes from the original

The original came as a binary without source. The source here was written from its disassembly and then changed.

1. **Measures on the map plane.** The original measured in three dimensions against a waypoint whose height the game reports as 0. Its number could never fall below the player's own altitude: about 308 at Colter and 118 in Valentine, going by the heights in the game's scripts. Near the waypoint the reading was mostly altitude.
2. **Draws only while the radar is shown.** The original drew whenever a waypoint was set, also over cutscenes, the cinematic camera and shop catalogues.
3. **Whole metres with a unit.** The original showed a bare number with one decimal, which changes every frame or two at a gallop.
4. **Calls the game's current text commands.** The original called text commands that the game emptied or removed in build 1436. On this game build it can show text only because the script hook redirects those old commands.
5. **Draws its own shadow.** The original asked for a drop shadow through one of the removed commands. This build draws a black copy of the text a little down and to the right.
6. **Formats the number itself, with limits.** The original printed into a 16-character buffer with no length limit. Real distances could not overflow it, but nothing enforced that.
7. **Measures the game's own GPS routes.** The original showed a number only for the player's waypoint. Yellow routes set by the game are never the player's waypoint, so they gave no number. This build reads the route length the game reports for the route it is drawing and shows it as `route N m`.
8. **No keyboard hook, no C runtime, no unused tables.** The original carried a key handler nothing read and tables of ped, vehicle, weapon and weather names nothing used. The built file is under 5 KB instead of 291 KB.

## Requirements

- An ASI loader (`dinput8.dll` in the game folder).
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). Loaded here with ScriptHookRDR2 V2, version 1.9. The original's readme names Alexander Blade's ScriptHookRDR2; this build has not been tried with it.

## Install

1. Run `make` in this folder, or use the built file that is already in `build/`.
2. Copy `build/ShowWaypointDistance.asi` into the game's main folder, next to `RDR2.exe`.
3. Do not copy the `ShowWaypointDistance.asi` at the top of this folder. It has the same name but is the original.

To uninstall, delete `ShowWaypointDistance.asi` from the game folder. The plugin stores nothing, so nothing is left behind.

## Build and test

- Tools: `make`, `clang++` with the `x86_64-pc-windows-msvc` target, `lld` and `llvm-dlltool`. No Windows SDK is needed.
- `make` builds `build/ScriptHookRDR2.lib` from `src/ScriptHookRDR2.def`, then `build/ShowWaypointDistance.asi`.
- `make test` builds `build/DistanceTextTest` as a normal Linux program and runs it. It checks the text made by `src/DistanceText.h` without the game: rounding (`0.49` gives `0 m`, `0.5` gives `1 m`), ordinary values, the largest value (`99999 m`), `? m` for values that are too large, negative, infinite or not a number, and that nothing is written past the end of the text buffer. It also checks the route text (`route 0 m`, `route 1234 m`, `route 99999 m`, `route ? m` for negative and not-a-number values, nothing written past its buffer) and the rule that shows any positive route length at once (whether or not the game reports the route as found), keeps the last one for at most 500 ms of game time while a route is found but reports 0 or less, and clears it after that or when there is neither a route nor a length.

## Files

| File | What it is |
| --- | --- |
| `ShowWaypointDistance.asi` | The original author's binary (290,816 bytes, linked 2020-12-08). Kept for reference, not installed. |
| `READ ME!!.txt` | The original author's readme. |
| `src/ShowWaypointDistance.cpp` | Source of this build: the loop, the game calls and the drawing. |
| `src/DistanceText.h` | Turns a distance into the text that is drawn, and holds the rule for keeping the last route length. No game calls. |
| `src/ScriptHookRDR2.def` | The six functions imported from `ScriptHookRDR2.dll`. |
| `test/DistanceTextTest.cpp` | The checks run by `make test`. |
| `Makefile` | Cross-builds the plugin on Linux and runs the test. |
| `build/ShowWaypointDistance.asi` | Built from this repo's source (4,608 bytes). This is the file to install. |
| `build/ScriptHookRDR2.lib`, `build/DistanceTextTest` | Made by the build. |

## Known limits

- Checked against the game's decompiled scripts and data files for build 1.0.1491.50, not confirmed in the game. How the text looks (font, size, the shadow copy) has not been seen on screen.
- For your own waypoint it is the straight line, not the length of the route the radar draws.
- Routes the game builds in other ways are not measured: custom routes from waypoint recordings (for example in `mudtown1`, `susan1`, `braithwaites1` and `braithwaites2`) and multi-point routes (`bounty1`, `marston1`, the horse race encounter). No native reads them, so they show no number unless the game happens to report them as a GPS blip route.
- Not confirmed in game: that the route length is in metres (it is in GTA V, and Rockstar's own scripts use it as a distance), which route the length belongs to when a waypoint and a mission route are both set, whether the game reports 0 for a moment while it works a route out again, and how the kept last length behaves. Right after your own waypoint is cleared or reached, the game might still report that waypoint's route for a few frames, which would show as `route N m`; this has not been checked.
- The route number goes away whenever the game drops the route: within about 15 m of the objective, when a mission only forces the route while you are far away (the bank robbery does so beyond 40 m), and on foot for bounties whose route is shown only on horseback.
- The unit is always metres, whatever the game's measurement setting.
- The position is a fixed fraction of the screen, chosen by the original author. It takes no account of the aspect ratio, the safe-zone setting or the radar mode. With the expanded radar, the compass or a wider screen the number sits differently against the radar.
- Whether the radar also counts as hidden in every cutscene, under the cinematic camera, and when it is switched off in the settings, is expected but not confirmed.
- That the game reports the waypoint's height as 0 comes from the public native database and two open-source trainers. The map-plane measure is right either way.
- The game's radar has a distance text element of its own in its interface data. Whether story mode ever shows it could not be established.

## Credits

- Original mod: Show Waypoint Distance by srgamerio, as named in the bundled `READ ME!!.txt`. That readme also gives the address gitloft.com.
- Version: not recorded.
- Page: <https://www.nexusmods.com/reddeadredemption2/mods/366>. Matched by mod name, author and description through a web search; the page itself could not be opened.
