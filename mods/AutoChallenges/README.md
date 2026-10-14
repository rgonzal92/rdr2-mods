# AutoChallenges

Completes the story-mode challenges (Bandit, Explorer, Gambler and the other six sets) by itself and pays their normal rewards. The player does not have to do what a challenge asks. Only the last Master Hunter challenge is left to the player.

- **Type:** Lenny's Mod Loader data mod (a replacement game file that the loader serves in place of the game's own)
- **Origin:** Auto Challenges 1.0 by Maverick, unchanged
- **Status:** Not in use: held back for one use in Chapter 2, then to be removed (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

- Replaces the game's `goals_sp.meta`, the file that holds the condition of every challenge goal. 137 of its 138 goals get one fixed condition: the play-time counter `TOTAL_PLAYING_GAME_TIME` reaching 10.
- In a challenge set that the story has unlocked, each rank then completes by itself shortly after the one before it. The nine sets have 90 ranks in all.
- The game pays each rank's normal reward as it completes. Over all nine sets that is 1,425 XP each for Health, Stamina and Dead Eye (an attribute reaches its top level at 1,100), $675, and the 36 reinforced equipment items at the Trapper.
- It does nothing for a set the story has not unlocked yet. Eight sets unlock when the second mission, "Enter, Pursued by a Memory", is passed. Survivalist unlocks when "A Fisher of Men" (Chapter 2) is passed.
- Master Hunter 10 is not changed: the Legendary Panther still has to be killed by hand.
- Outside a mission, each completion asks the game for an autosave at once. Completed ranks and their rewards stay after the mod is removed.

## Changes from the original

Nothing is changed. The three files are the same as in the author's download (same sizes and checksums).

What was checked, against the game's own data files and decompiled scripts for build 1.0.1491.50:

- The file is the game's current `goals_sp.meta` with only the goal conditions replaced; the three timed-ride goals also lose their route data (see Known limits). Rebuilding it from the game's file with those edits gives a byte-identical result: 138 goals with the same names and order, 137 changed, 1 unchanged, none missing or added.
- No other installed mod replaces `goals_sp.meta` or the game's list of challenge ranks. Gold Medals replaces a different file in the same folder.
- No mission or tutorial script waits for a challenge rank, so ranks that complete early do not block the story.

What was decided:

- **Not enabled at the start of a new game.** The story unlocks eight sets after the second mission, but it switches Dead Eye XP on only after "Old Friends". The game's reward script drops XP for an attribute that is not switched on yet. Enabled from the start, the mod would complete 79 ranks in the snow at Colter, and all 18 Dead Eye XP rewards (1,425 XP) would be lost for good. The author's mod page gives the same warning; the bundled `Readme.txt` does not.
- **Used once, then removed.** It is added after "A Fisher of Men", so that every set it can complete runs in one sitting, and taken out when they are done. Completed ranks and rewards stay in the save. Left in place, it would act again in any later new game.

## Requirements

- Lenny's Mod Loader. It reads `install.xml` in the mod's folder, which names the game file to replace.

## Install

1. Play until "A Fisher of Men" (Chapter 2) is passed, then make a manual save. Only an older save can undo what the mod does.
2. With the game closed, copy the `Challenges` folder into the game's `lml` folder.
3. Start the game, load the save and stand in free roam, outside a mission. If LegalAndMainMenuSkip is installed, the game loads the newest save by itself, so make sure that is the save the mod should act on.
4. Wait until the pop-ups have stopped for about two minutes. Check Pause > Progress > Challenges: eight sets at 10/10 and Master Hunter at 9/10. Save.
5. Close the game and remove the mod again.

To uninstall, delete `lml/Challenges`. If the loader's list, `lml/mods.xml`, has entries for it, remove those too.

## Files

| Path | What it is |
|---|---|
| `Challenges/goals_sp.meta` | The replacement goals file. The author's file, unchanged. |
| `Challenges/install.xml` | The loader's install file: replaces `update:/common/data/stats_and_challenges/goals_sp.meta`. The author's file, unchanged. |
| `Readme.txt` | The author's one-line install note. |

## Known limits

- Not run in the game on this setup. Everything above is checked against the game's scripts and data, not confirmed in the game.
- Whether the loader picks up a folder that has no entry in `lml/mods.xml` was not checked. After the first start, the loader's log `vfs.log` in the game folder should have a line that names `challenges/goals_sp.meta`.
- How the game counts the condition, and so the pace, cannot be read from the data. The author's page says the whole run takes a minute or two.
- The check marks cannot be had without the rewards: the attribute bars, the money and the Trapper items all arrive within minutes.
- XP rewards are also dropped for ranks that complete during a mission replay or during the first mission on Guarma.
- The Legend of the East outfit, the reward for finishing all nine sets, is granted by a single event when a set finishes and is never checked again. Remove the mod only when every set it can complete shows 10/10, kill the Legendary Panther last, and wait for the outfit message before saving.
- Horseman 3, 6 and 9 are timed rides. The mod deletes their route and gives them the fixed condition, a form the game itself never uses. If the game did not accept it, Horseman would stop at 2/10 for as long as the mod is installed.
- Progress texts count to 10 under titles that ask for something else, for example "3 of 10 townsfolk held up" for "Hold up 5 townsfolk". No statistic is faked.
- What happens to a half-counted rank when the mod is added or removed is unknown. Add and remove it in free roam, when no pop-ups are running.
- The file replaces the whole of `goals_sp.meta` and matches build 1.0.1491.50 only.

## Credits

Auto Challenges by Maverick (named "Challenges" in its `install.xml`): <https://www.nexusmods.com/reddeadredemption2/mods/1067/>
