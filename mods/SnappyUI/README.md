# SnappyUI

Makes the pause menu open and change page almost at once and close faster, and makes side menus close at once, by shortening the game's menu animation timings.

- **Type:** Lenny's Mod Loader data mod (a replacement game file that the loader serves in place of the game's own)
- **Origin:** SnappyUI 1.0 by WickedHorseMan, unchanged
- **Status:** In use (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

It replaces the game's `durations.xml`, the table of named timings that the UI animations use. The file has the same 126 timings and 60 aliases as the game's own; 18 values differ:

| Timing | Game (s) | Mod (s) | Drives |
|---|---|---|---|
| `PAUSEMENU_ENTER_CUBIC`, `PAUSEMENU_ENTER_DELAY` | 0.16, 0.06 | 0.00001 | Opening the pause menu |
| `PAUSEMENU_EXIT_CUBIC` | 0.55 | 0.00001 | Closing the pause menu |
| `PAUSEMENU_SUBMENU_ENTER`, `PAUSEMENU_SUBMENU_EXIT` | 0.3 | 0.00001 | Changing page in the pause menu |
| `PAUSEMENU_CONTENT_ENTER`, `PAUSEMENU_CONTENT_EXIT` | 0.5 | 0.00001 | Nothing found in the game's data |
| `SIDEMENU_EXIT` | 0.35 | 0.00001 | Closing a side menu or the map menu |
| `ANALOG_SCROLL` | 0.2 | 0.1 | Scrolling a list |
| `HONOR_DISPLAY_WAIT`, `HONOR_DISPLAY_FLASH`, `HONOR_DISPLAY_CONTRACT` | 0.25, 0.5, 0.5 | 0 (`IMMEDIATE`) | The marker on the honor bar |
| `LOCATION_BACKGROUND_ENTER`, `LOCATION_ALL_CONTENT_ENTER`, `LOCATION_EXIT` | 0.15 | 0.00001 | The bounty-hunter banner |
| `LEGAL_SCREENS_DURATION` | 4 | 0.00001 | The legal screens at start-up |
| `DEATH_SCREEN_FX_ROTATION`, `DEATH_SCREEN_TEXT_FADE_IN` | 30, 0.5 | 0, 0.001 | The Red Dead Online death screen |

What the player sees in story mode, going by the game's UI data:

- The pause menu opens at once.
- The pause menu's closing animation takes 0.25 s instead of 0.55 s: its 0.55 s mask movement is cut to nothing, and an unchanged 0.25 s fade of the list remains.
- On a page change the new page appears after 0.05 s instead of 0.65 s, and input is locked for 0.05 s instead of 0.95 s.
- Side menus (the Log, shops, crafting, fast travel and others) close on the next frame instead of after 0.35 s. The map opened from gameplay closes in 0.05 s.
- The list scroll animation takes 0.1 s instead of 0.2 s.
- When honor changes, the marker is already on its new notch when the bar appears. It no longer rests on the old notch, jumps and flashes. The bar still shows for 4 seconds.
- The bounty-hunter banner pops in and out instead of wiping. It still stays for 4 seconds.

Where it does nothing on this setup:

- **Legal screens.** LegalAndMainMenuSkip is also installed and skips them altogether. Without it they would take about 1.2 s instead of 9.2 s.
- **Death screen.** Both `DEATH_SCREEN_` timings belong to the Online death screen. The story-mode DEAD and mission-failed screens use other timings, which are not changed.
- **Location banner.** In this build it uses other timings, so it keeps the game's wipes.
- **Everything else.** Alerts (yes/no warnings), toasts, the satchel, the weapon wheel and the wanted display use only unchanged timings.

Nothing is written to the save. The game reads the file once at start-up.

## Changes from the original

Nothing is changed. The repo's files are the author's, and the installed copy is identical to them.

What was checked, against the game's own data files and decompiled scripts for build 1.0.1491.50:

- The file has exactly the timing names of the game's current `durations.xml`, in the same order, with every type and easing untouched. Apart from the 18 values the differences are whitespace only.
- Every animation that uses a changed timing was traced. The same events fire in the same order as in the unmodded game, and the menus end in the same state.
- Starting a new game and replaying a mission from the pause menu do not pass through a changed animation. Saving and loading only get shorter fades; they still wait for the real save operation. No script reads these timings.
- No other installed mod replaces this file.

What was decided:

- The ten `ALERT_*` timings (the fades of yes/no warning screens, shared with the mission-failed and DEAD screens) stay at the game's values. Shorter values for eight of them were installed for a time and taken out again. The delay that mattered was how long a button must be held, which JustDoTheThingAlready in this repo shortens, and it was never shown that the game waits for these fades before it takes an answer.

## Requirements

- Lenny's Mod Loader (stated in the author's `Readme.txt`). It reads `install.xml` in the mod's folder, which names the game file to replace.

## Install

1. With the game closed, place the `WickedHorseMan` folder inside the game's `lml` folder. The mod then sits in `lml/WickedHorseMan/UI/Durations`.
2. Check that the loader's list, `lml/mods.xml`, shows the mod as enabled. On this setup it is listed there under the folder name `WickedHorseMan/UI/Durations`.

To uninstall, delete `lml/WickedHorseMan` and remove the mod's entries from `lml/mods.xml`. The game's own timings are back at the next start.

## Files

| Path | What it is |
|---|---|
| `WickedHorseMan/UI/Durations/durations.xml` | The replacement timing table. The author's file, unchanged. |
| `WickedHorseMan/UI/Durations/install.xml` | The loader's install file: replaces `update:/common/data/ui/durations.xml`. The author's file, unchanged. |
| `Readme.txt` | The author's install note. |

## Known limits

- The effects listed above are read from the game's UI data for build 1.0.1491.50, not confirmed in the game.
- **Possible flicker.** The game's own comment on the two page-change timings says the time "is required on console to hide pause menu transition flicker". With both near zero, a page change may show a one-frame flash.
- **Possible stray input.** The Log now closes within a frame or two and the map within 0.05 s, usually before the button that closed them is released. If that button also has a gameplay action (the right mouse button aims), it may act on the character. The unmodded 0.35 s close outlasts a normal press. Whether this happens depends on game code that could not be read. If it shows, set `SIDEMENU_EXIT` back to 0.35 and `PAUSEMENU_SUBMENU_EXIT` to about 0.15.
- **Replaying a mission from the Log.** The Log sends the replay request and closes at once, and the game's script handles the request only while the Log is still open. That window shrinks from 0.35 s to a few frames. The script runs every frame, so it most likely still works; not confirmed in the game.
- The mod uses timings the unmodded game never does: waits of zero length in the honor animation, and waits and input locks of 0.00001 s. How the game handles them could not be read.
- The file replaces the whole of `durations.xml` and matches build 1.0.1491.50 only. A game update that adds or retunes timings would be hidden by it.

## Credits

SnappyUI 1.0 by WickedHorseMan. Its `install.xml` records no link; the mod's page on Nexus Mods is "Snappy UI (QoL)": <https://www.nexusmods.com/reddeadredemption2/mods/971>
