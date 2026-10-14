# BatchCrafting

Makes a whole batch of items from one crafting or cooking animation instead of one item per animation. A batch is never larger than the ingredients in the satchel pay for, or than the player can carry. It also holds the speed-up key of the cooking progress bar, so the bar fills in about 4 to 5 seconds instead of 10.

- **Type:** ScriptHookRDR2 plugin (.asi): a small Windows library that an ASI loader loads into the game and that calls the game's script functions ("natives") through ScriptHookRDR2
- **Origin:** Written from scratch for this repo
- **Status:** In use, as of 2026-10-12 (the file in the game folder is identical to `BatchCrafting.asi` here)
- **Game build:** 1.0.1491.50, story mode

## What it does

- **Batches.** The game still plays its one animation and makes its one item. In the moment that item is put away, the plugin adds the rest of the batch, removes the ingredients for exactly as many items as arrived, and writes the statistics the game's scripts write for a craft (items crafted, made per item and per recipe type, herbs used, seasoned food).
- **Where.** At camp fires, in the player's own camp and in the satchel's crafting menu (Rest > Craft), for every recipe in the crafting menu: ammunition, arrows, thrown weapons, tonics, horse items, baits and cooked meat.
- **Batch size.** The smallest of three numbers: `MAX_PER_ANIMATION` (the game's own item included), the ingredient sets in the satchel, and the room left under the game's own carry limits. The carry limit is often the real cap: in the confirmed run the game reported 5 for a cooked meat and 3 for a tonic, while split point cartridges came 50 from one animation.
- **Recipes come from the game.** Product and ingredient set are read from the row of the crafting menu, ingredients and amounts from the game's item catalog. The plugin holds no recipe table.
- **Keep one.** The game decides whether "Cook Another" or "Craft Another" is offered in the moment it finishes a craft, and does not check again. A batch that used the last ingredients would let the game hand out a free item (or, with no room left, take ingredients and add nothing). So the plugin leaves one ingredient set and one free place for the game, unless it sees that the game will decide after the batch. With 30 sets, 29 items come from the first animation and the 30th from a second one.
- **Item message.** The "item added" message shows the batch's count ("30x ..."). The plugin holds the game's message queue while the crafting menu is open, raises the count on the message the game queued for its own item, and lets the game show it.
- **Progress bar.** While the progress bar of cooking or fire crafting is on, the plugin holds its speed-up key for the player (Space with the keyboard layout used here). Measured in the game: 3.8 to 4.9 seconds per bar. The game's scripts set 10 seconds for a bar nobody touches.
- **Safe order.** Products are added first and counted again; ingredients are removed only when all products arrived. If a count is off or an ingredient cannot be removed, everything is put back, the log says what was put back, and the plugin makes no more batches until the game is restarted.
- **Log.** Every batch, every craft that got none, and the time of every progress bar are appended to `BatchCrafting.log` in the game folder.

It deliberately does nothing when:

- the result is eaten, used or poured instead of stowed
- the mission that teaches cooking and crafting runs ("Exit Pursued by a Bruised Ego")
- a carcass or a piece of meat is cooked directly with the Cook prompt, or in the recipe menu reached with Back after such a cook
- a craft is interrupted (the player is shot at, say)
- only one ingredient set is left, or there is no room for more
- a product or ingredient is a weapon or unknown to the item catalog
- the game build lays out its recipe list differently

Three kinds of lines from the log. The first one says: after the game's own item 186 ingredient sets were left, the carry limits left room for 99, one set was kept for the game, and 49 items were added.

```
item 0x44750C88 cost 0x23D1A6A9 sets 186 limit 100 weapon limit 200 room 99 keep one batch 49 added 49
message: the game's own now shows 50
progress bar took 4531 ms with the key held by the plugin
```

## Why it exists

The game makes one item per animation, so 30 arrows or 30 pieces of meat mean 30 animations. Three other ways were rejected:

- **AutoCraft, another author's plugin** (see [`../AutoCraft`](../AutoCraft/README.md)). It patches game code in memory and triples every amount behind the scripts' back, which gives triple cost for one portion, free items, lost items and wrong statistics. This plugin patches no game code and searches nothing in the game's memory. It adds and removes items with the same natives the game's scripts use.
- **A data-only mod.** A recipe file can say "use 50, make 50", but not "make as many as the ingredients allow". The bar's times are fixed numbers inside the compiled scripts, so no data file shortens the bar either.
- **A shorter progress bar.** An earlier build gave the bar a 2-second time of its own. Every bar then ended at once, and while cooking several items in a row the player sat frozen at a switched-off "Cook" prompt. By the game's scripts no key leaves that state, so a save has to be loaded. That build was withdrawn. Holding the key has the timing of a player who holds it, which the game is built for.

## Settings

`BatchCrafting.ini`, section `[BATCH]`, read once when the game loads the plugin. Only the number after `=` is read.

| Name | Default | Meaning |
|---|---|---|
| `MAX_PER_ANIMATION` | 50 | How many items one animation makes at most, the game's own item included. 1 or 0 switches the whole plugin off, the key hold too. |
| `HOLD_SPEED_UP_KEY` | 1 | 1 holds the speed-up key of the progress bar. 0 leaves the key to the player. |

A missing file or line means the default.

## Requirements

- An ASI loader and ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports seven of its functions, `getGlobalPtr` among them. It runs here under ScriptHookRDR2 V2 1.9.
- Lenny's Mod Loader is not needed.

## Install

1. Close the game.
2. Copy `BatchCrafting.asi` and `BatchCrafting.ini` from this folder into the game folder, next to `RDR2.exe`.
3. Start the game and load a save. `BatchCrafting.log` in the game folder then gets the line `BatchCrafting started, at most 50 per animation`.

Uninstall: delete `BatchCrafting.asi`, `BatchCrafting.ini` and `BatchCrafting.log` from the game folder.

## Build and test

- **Tools:** `make`, `clang++` (cross-compiling to `x86_64-pc-windows-msvc`), `lld` and `llvm-dlltool`. No Windows SDK and no C runtime are needed; the import libraries are made from the two `.def` files.
- **`make`** builds `build/BatchCrafting.asi`. There is no install target: copy it and `src/BatchCrafting.ini` to this folder's root, or to the game folder, by hand.
- **`make test`** builds and runs `build/BatchLogicTest` on Linux. It checks the rules in `src/BatchLogic.h` without the game: which row of the recipe list was crafted, ingredient sets, batch size and "keep one", the recipe types and statistic names, the table of 39 herbs, the message queue, the progress bar's prompt row, the positions of the script values, and the log line builder. It prints `BatchLogic: all checks passed`.

## Files

| Path | What it is |
|---|---|
| `BatchCrafting.asi` | The plugin to install, built from this repo's source (a copy of `build/BatchCrafting.asi`). A rebuild from the current source differs only in the build time stamp. |
| `BatchCrafting.ini` | The settings file to install (a copy of `src/BatchCrafting.ini`) |
| `Makefile` | Build and test recipes |
| `src/BatchCrafting.cpp` | The part that talks to the game: reading the recipe list, adding and removing items, statistics, messages, the key hold, the log |
| `src/BatchLogic.h` | The rules, free of game calls, so they can be tested |
| `src/BatchCrafting.ini` | Source of the settings file |
| `src/ScriptHookRDR2.def`, `src/Kernel32.def` | The functions imported from `ScriptHookRDR2.dll` (7) and `KERNEL32.dll` (4) |
| `test/BatchLogicTest.cpp` | The checks behind `make test` |
| `build/` | Build output: the plugin, the two import libraries and the test program |

## Known limits

- **Confirmed in the game** (2026-10-12): batches of plain cooked meat, tonics, split point cartridges and improved arrows; the message showing the batch count, up to 50; seven progress bars of 3.8 to 4.9 seconds with no freeze; no batch when the item was eaten.
- **Not confirmed in the game:** the "use everything" case (every batch so far kept one set, cooking included), the undo after a wrong count, and recipes other than those above. Also not measured: how long a bar takes when the player holds the key.
- **The last item takes a second animation.** That is the "keep one" rule, on purpose.
- **Extra items differ from hand-made ones:** they give no Dead Eye experience, do not count toward the camp's "craft three things" task, and count as crafted, made and seasoned but not as cooked.
- **Not counted as ingredients:** meat still inside a carcass, and meat or pelts on the horse. The game turns those into ingredients one craft at a time.
- **No speed-up for:** coffee (it has no progress bar), ammunition-type crafting (the game has no speed-up key there), and the Cook prompt on a carcass.
- **Two messages** ("Item", then "29x Item") appear when the game showed its own message before the batch was made.
- **Freeze cause.** Why the shortened bar froze the player is a reading of the game's scripts, not proven in the game. The key hold sets no time of its own, and no freeze occurred in the seven bars above.
- **Interrupted craft while the key is held:** releasing the key might count as a press of "Pour" or "Fast Travel", as it might for a player holding the key. Seen only in a simulation under a pessimistic assumption.
- **Tied to the game build.** The positions of four script values (recipe list, item message queue, prompt table, camp state) are those of build 1.0.1491.50. The plugin checks the lengths of the three lists and leaves a list alone when its length differs, but after a game update the positions in `src/BatchLogic.h` and `src/BatchCrafting.cpp` must be checked against the new scripts.
