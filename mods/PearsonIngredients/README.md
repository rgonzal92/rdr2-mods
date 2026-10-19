# PearsonIngredients

Press F9 and the player's inventory is topped up with every ingredient Pearson needs to craft all his satchels and camp upgrades: 28 kinds, 49 items. Then craft them at Pearson's wagon as usual.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Written from scratch for this repo
- **Status:** Built 2026-10-16, not yet tried in game
- **Game build:** 1.0.1491.50, story mode

## What it does

- A fresh press of F9 (holding it does not repeat) brings each ingredient up to the total all 24 recipes need together. What the player already holds counts, so a second press adds nothing.
- Items are added the way the game's scripts add a satchel item (`_INVENTORY_ADD_ITEM_WITH_GUID` under `SLOTID_SATCHEL`), and counted with `_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID`, the same count Pearson's menu uses (`shop_pearson.c` func_1248 → func_947).
- Each press writes one line per ingredient to `PearsonIngredients.log` in the game folder: needed, held before, held after, and "(add refused)" when the game turned the add down.
- A press outside play (main menu, loading screen) is dropped.

It gives ingredients only. Pearson's menu still decides what can be crafted: story and camp requirements, and the Legend of the East satchel coming after the others, are the game's.

## The recipes

From the game's item catalog (`update_4` … `CF8DA776.ymt`, parsed in `rdr2-tools/batchcraft-2026-10-12/catalog`), every acquire cost with the key `COST_CRAFTING_PEARSON`:

| Product | Ingredients |
|---|---|
| Tonics Satchel (`SATCHEL_002`) | perfect deer, buck, elk pelt |
| Ingredients Satchel (`003`) | perfect deer, badger, squirrel pelt |
| Kit Satchel (`004`) | perfect deer, elk, panther pelt |
| Provisions Satchel (`005`) | perfect deer, buffalo, raccoon pelt |
| Materials Satchel (`006`) | perfect deer, boar, iguana pelt |
| Valuables Satchel (`007`) | perfect deer, beaver, rabbit pelt |
| Legend of the East Satchel (`008`) | perfect deer, cougar, wolf pelt |
| Alligator Skull | perfect alligator skin |
| Snake Skin Banjo Head | 2 perfect snake skins |
| Boar Skin Rug | 4 perfect boar pelts |
| Pronghorn Leather Table Tops | 2 perfect pronghorn pelts |
| Cougar Pelt Covered Chest | 2 perfect cougar pelts |
| Ox Hide Rug | 2 perfect ox hides |
| Boar Skin Table Cover | perfect boar pelt |
| Campfire Seat Cover | perfect beaver, muskrat, fox pelt |
| Lean-to Cow Hide Ground Cover | perfect cow hide |
| Hanging Bones | buck antlers |
| Moose Antlers | moose antler |
| Pronghorn Skull | perfect pronghorn carcass |
| Big Horn Ram Skull | perfect ram carcass |
| Elk Antlers | elk antlers |
| Scout Fire Ground Cover | 2 perfect goat hides |
| Campfire Log Seat Cover | 2 perfect wolf pelts |
| Wolf Skull | perfect wolf carcass |

The query that sums them (the result is the table in `src/PearsonIngredients.cpp`, checked at compile time for 28 rows and 49 items):

```sh
cd ~/Projects/rdr2-tools/batchcraft-2026-10-12/catalog && python3 -I -c "
import sys, os; sys.path.insert(0, os.getcwd())
import collections; from lib import load, nm, joaat
items = load()['0622eda3']['c53812ff']; need = collections.Counter()
for it in items.values():
    for ac in it['4f728f50']:
        if ac['e9806307'] == joaat('COST_CRAFTING_PEARSON'):
            for c in ac['c53812ff']: need[nm(c['d3b360c4'])] += c['9ea8b8f4']
print(len(need), sum(need.values()), sorted(need.items()))"
```

## Requirements

- An ASI loader: the `dinput8.dll` that loads `.asi` plugins from the game folder.
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports eight of its functions, listed in `src/ScriptHookRDR2.def`, including the keyboard handler.

## Install

1. Run `make` to produce `build/PearsonIngredients.asi`.
2. Copy `build/PearsonIngredients.asi` into the game's root folder.

To uninstall, delete the file (and `PearsonIngredients.log`) from the game folder. Items already given stay in the save.

## Build and test

- Tools: `make`, `clang++` with the `lld` linker, and `llvm-dlltool`. The plugin is cross-built on Linux and needs no Windows SDK or C runtime.
- `make` builds `build/PearsonIngredients.asi`, after making the import libraries from `src/ScriptHookRDR2.def` and `src/Kernel32.def`.
- There is no `make test`. The ingredient table is checked by `static_assert` at compile time; the rest is game calls.

## Files

| Path | What it is |
|---|---|
| `src/PearsonIngredients.cpp` | The plugin and the ingredient table. |
| `src/ScriptHookRDR2.def`, `src/Kernel32.def` | The functions the plugin imports; the Makefile turns them into import libraries. |
| `Makefile` | Build rules. |
| `build/` | Build output: `PearsonIngredients.asi` and the import libraries. |

## Known limits

- Not confirmed in the game. Small pelts and antlers have a satchel carry limit in the catalog and should go in like any satchel item. Large pelts and the three carcasses have none: in normal play they exist only as pelts on the horse or as carried carcasses. Whether the inventory takes them as items is what the log of the first press shows.
- F9 is assumed to be free; if the game has an action on it, that fires too.
- The table comes from the 1.0.1491.50 catalog.
