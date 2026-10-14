# LegalAndMainMenuSkip

Starts the game straight into story mode: no logo, no legal screens and no main menu. The most recent save loads by itself.

- **Type:** Lenny's Mod Loader data mod: a folder with replacement game files and an `install.xml` that tells Lenny's Mod Loader (LML) which game files they replace
- **Origin:** Legal and Main Menu Skip 1.1.0 by WesternGamer, unchanged
- **Status:** In use, as of 2026-10-12 (enabled in `lml/mods.xml`; the installed files are identical to the ones here)
- **Game build:** 1.0.1491.50, story mode

## What it does

It replaces one file of the game's user interface, the "landing page" flow. A flow is a list of screens and checks and the links between them. The game's own file has 45 steps and leads through the boot sequence (logo, legal screen, sign-in checks) to the main menu. The replacement has 7 steps and leads to neither.

- **At start:** it waits for the player profile to load, asks the game once whether story mode may be played, and then sends the same "load story" event as the main menu's Story button. The most recent save loads. With no save at all, a new game starts (traced in the scripts only).
- **Starting a new game:** Pause > Story > New Game. The game's own main menu has no New Game entry either. Until the new game has made its first autosave, every start of the game still loads the newest save of the old playthrough.
- **Pause > Story > Exit Game** does not show a menu. It reloads the most recent save. To leave the game, use Pause > Quit Game.
- **Red Dead Online entries** end in the alert "Red Dead Online is unavailable at this time."
- **If the game answers that story mode may not be played,** the alert "Your Social Club account is not entitled to Red Dead Redemption 2 ..." appears, and OK quits the game. There is no retry.
- **Never shown or run with this mod:** the logo and legal screens, the main menu and its news feed, the game's sign-in and ownership screens, the patch-download check, the step that applies HDR settings at start, the first-run setup screens (language, HDR, brightness, subtitles, audio; only when no profile exists), and the "controls unbound or conflict" warning.
- It writes nothing to saves or to the profile. Removing it brings the legal screens and the main menu back.

## Changes from the original

Nothing was changed. Both files are the original author's, byte for byte.

What was checked against the game's own data files and decompiled scripts for build 1.0.1491.50:

- **The file is valid for this build.** Every element, step type, property and link name it uses occurs in the game's own flow files (158 checks against 171 flows). Every link leads to an existing step, and every step can be reached.
- **The texts exist.** Both alert texts are named by hash, and both hashes belong to labels in the game's current English text.
- **The story event is the game's own.** Same channel, type and name as the Story button. The script that receives it was traced: it never picks a save itself; the game loads the newest one.
- **No clash.** No other installed data mod replaces a file in the same folder (`startup_data`).

What was decided:

- **Use it as it is.** It took the place of "Startup Screen Skip", a mod that replaces the boot flow file (`0x5A9FE249.ymt`) instead, removes the logo and legal screens, and keeps the main menu. Only one of the two is needed.
- **Not built:** a mix that goes straight into the story at start but keeps the real menu for Exit Game.
- **Not applied:** making the "not entitled" alert try again instead of quitting. It would be a one-line change of a link, but it needs a test in the game first.

## Requirements

- Lenny's Mod Loader: `vfs.asi` and the `lml` folder in the game folder. LML is itself loaded by an ASI loader.
- ScriptHookRDR2 is not needed for this mod.

## Install

1. Close the game.
2. Copy the inner folder `LegalAndMainMenuSkip` (the one that holds `install.xml`) into the game's `lml` folder, so that `lml/LegalAndMainMenuSkip/install.xml` exists.
3. Make sure the mod is enabled in `lml/mods.xml`, the file that holds LML's mod list and load order. In the install in use it has an entry `<Mod folder="LegalAndMainMenuSkip">` with `<Enabled>true</Enabled>`, and a line `<Mod>LegalAndMainMenuSkip</Mod>` in the `<LoadOrder>` list.
4. Start the game. `vfs.log` in the game folder should then contain `Replaced file from archive addonDLC:/legalandmainmenuskip/0xe279d1a8.ymt`.

Uninstall: delete `lml/LegalAndMainMenuSkip`, or set its entry in `lml/mods.xml` to `<Enabled>false</Enabled>`.

## Files

| Path | What it is |
|---|---|
| `LegalAndMainMenuSkip/install.xml` | The original author's install list: mod name, author, version, link, and the one game file to replace, `update:/x64/data/ui/startup_data/0xE279D1A8.ymt` |
| `LegalAndMainMenuSkip/0xE279D1A8.ymt` | The original author's replacement flow, plain XML, 3,211 bytes. The file name is the game's hash of `landing_page/flow/landing_launcher_flow`. |

## Known limits

- **Not confirmed in the game.** The behaviour above was checked against the game's scripts and data. The loader's log shows the file being replaced at start; what happens on screen has not been confirmed.
- **Sign-in trouble has no screen.** The game's own sign-in and ownership screens are gone, so a late or failed sign-in can show as a black screen after the loading bar, or as the "not entitled" alert that only quits. What happens on an offline start is not known. It is also untested whether OK reacts on an alert shown this early.
- **HDR.** The game applies HDR settings in a step at every start; this mod skips that step. Whether the picture differs is not known.
- **Exit Game.** That the reload after Exit Game completes without a long wait is not verified.
- **New profile or new game install.** The first-run setup screens never appear. The settings can still be changed in the pause menu.
- **Version numbers.** The download is version 1.1.0; its `install.xml` still says 1.0.0.
- **Game updates.** The mod replaces a whole flow file. After a game update, compare it with the game's new file before using it.

## Credits

Legal and Main Menu Skip by WesternGamer: <https://www.nexusmods.com/reddeadredemption2/mods/1469> (name, author and link as given in the mod's `install.xml`).
