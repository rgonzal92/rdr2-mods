# GoldMedals

Gives a gold medal for every story mission that is passed, whatever happened during the mission. The medal requirements (finish in time, headshots, accuracy and so on) no longer have to be met.

- **Type:** Lenny's Mod Loader data mod (a replacement game file that the loader serves in place of the game's own)
- **Origin:** Gold Medals 2.0 by Maverick, unchanged
- **Status:** In use (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

- Replaces the game's `goals_sp_missions.meta`, the file that holds the condition of every mission medal requirement (a "goal"). All 261 goals get the same condition: playing time below -1 milliseconds, checked once, when the mission is passed.
- The game's scripts give gold when every goal of a mission is complete, silver from half, and bronze below that. All goals now behave alike, so a mission ends with gold, or with bronze if the game does not accept the condition. Silver cannot occur.
- Pause > Progress > Story shows the original requirement texts, all ticked.
- Retrying from a checkpoint after dying or failing does not cost the medal.
- During a mission no requirement is ticked or failed any more. Everything is decided at the end.
- Mission replays are scored the same way.
- It does not override two rules in the game's scripts: after a cheat code was used in the session a mission gets no medal, and after a skipped checkpoint it gets bronze.

## Changes from the original

Nothing is changed. The repo's files are the author's 2.0 files, and the installed copy is identical to them.

What was checked, against the game's own data files and decompiled scripts for build 1.0.1491.50:

- The file has the same 261 goal names in the same order as the game's current file, with the same labels and rewards. Only the conditions differ. Four goals also lose an empty `<levels />` element, which nothing in this build reads.
- Every goal name that the game's mission list and mission scripts refer to exists in the file.
- No other installed mod replaces this file.
- Apart from the gold count for one achievement (see Known limits), the game reads a medal only as "passed or not". Gold in place of bronze does not change the story flow or 100 % completion.

What was decided: the file is used as it is. A safer wording of the condition is known, for the case that missions end with bronze and nothing ticked: replace every `<desiredGoal value="-1" />` with `<desiredGoal value="360000000" />` (100 hours, the largest value the game itself stores in such a field). It is not applied.

## Requirements

- Lenny's Mod Loader. It reads `install.xml` in the mod's folder, which names the game file to replace.

## Install

1. With the game closed, copy the `Gold Medals` folder into the game's `lml` folder.
2. Check that the loader's list, `lml/mods.xml`, shows the mod as enabled. On this setup it is listed there under the folder name `Gold Medals`.

To uninstall, delete `lml/Gold Medals` and remove its entries from `lml/mods.xml`. Medals already given stay in the save.

## Files

| Path | What it is |
|---|---|
| `Gold Medals/goals_sp_missions.meta` | The replacement goals file. The author's file, unchanged. |
| `Gold Medals/install.xml` | The loader's install file: replaces `update:/common/data/stats_and_challenges/goals_sp_missions.meta`. The author's file, unchanged. |
| `Readme.txt` | The author's one-line install note. |

## Known limits

- Whether the game accepts "below -1" cannot be proven from the files. The field is declared as an unsigned number in this build, so -1 most likely reads as about 49.7 days, which every mission beats. User reports on the mod's page are mixed: several say it gives gold, several say it does not, mostly without detail. Checked against the game's scripts and data, not confirmed in the game.
- The "Gold Rush" achievement (70 gold medals) unlocks by itself at the 70th medal mission and cannot be taken back.
- Medals and ticked requirements are saved for good. Removing the mod does not revert them, and earned and unearned medals look the same.
- In four missions ("Urban Pleasures", "Country Pursuits", "The Tool Box" and "The Wheel") the script now repeats a progress call every frame for one stage, instead of making it once. No visible effect is expected.
- "A Quiet Time" and "The Aftermath of Genesis" have no requirement that the unmodded game checks at mission end, so there is less evidence for those two.
- Add or remove the mod only between missions. What happens to goal progress when the file changes in the middle of a mission is unknown.
- The file replaces the whole of `goals_sp_missions.meta` and matches build 1.0.1491.50 only.

## Credits

Gold Medals 2.0 by Maverick, as its `install.xml` names it. The link recorded there leads to the mod's page on Nexus Mods, which is titled "Auto Gold Medals": <https://www.nexusmods.com/reddeadredemption2/mods/1044/>
