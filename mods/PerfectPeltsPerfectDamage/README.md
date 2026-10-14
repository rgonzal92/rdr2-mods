# PerfectPeltsPerfectDamage

Makes every kill with a listed weapon or ammo type count as a clean kill, so the weapon used and the number of hits never lower what an animal gives. It does not change the animal's own star rating.

- **Type:** Lenny's Mod Loader data mod (a replacement game file that the loader serves in place of the game's own)
- **Origin:** Perfect Pelts - Perfect Damage 1.0 by MaVeRicK, with changes
- **Status:** In use (as of 2026-10-12)
- **Game build:** 1.0.1491.50, story mode

## What it does

- Replaces the game's `damagecleanlinessdata.meta`. That table rates each kill as Poor, Good or Perfect from four things: the animal's size class, the body region hit, the weapon or ammo, and the number of hits.
- Every entry in this build's table is rated Perfect: 5 size classes, 29 size-and-region blocks, the same 60 weapon and ammo names in each block, 1,740 entries. The names cover arrows, pistol, revolver, repeater, rifle and shotgun ammo, throwing knives, tomahawks, knives, hatchets, the cleaver, the broken sword, the machete, the lasso, the Varmint Rifle and bare hands.
- When an animal is skinned, the game looks up the kill rating together with the animal's own quality (its one to three stars). With these weapons the kill rating is always Perfect, so the pelt depends on the stars alone.
- The kill rating also sets how much meat, how many feathers and how many teeth an animal gives. A deer gives 4 Mature Venison at Perfect and 2 at Poor.
- It does not raise a low-star animal's pelt. On this setup that is done by the same author's separate Perfect Pelts mod, which replaces the loot table and is installed next to this one. It is not part of this repo. The two replace different game files and do not clash.
- It has no entry for kills by dynamite, fire bottles, burning, blasts, trampling, falls, drowning or predators. Those keep whatever rating the game gives.

## Changes from the original

1. **Hit limit of 100 in place of -1.** The original gave every entry a hit limit of `-1` and relied on the game reading that as "no limit". The game's own table uses `-1` only for tracking arrows, and only together with the `IgnoreCleanliness` flag; every other entry in it uses 1. Nothing in the game's data shows what `-1` does without the flag. If the game did not read it as "no limit", every kill would be rated Poor: the lowest meat and feather counts, and with this mod alone every pelt would be poor. This build sets 100 on the 1,711 entries without the flag. A positive count is what the game's own entries use, and 100 hits acts as no limit in play. The 29 tracking-arrow entries keep `-1` with the flag, the pair the game's own tracking-arrow entries use.
2. **Neck-and-lungs region added.** The original had no block for the `NeckAndLungs` hit region. The game's own table has that region for four of the five size classes. This build adds the four blocks (240 entries). Whether the original's `AnyRegion` block would have covered a neck or lung shot depends on the game's lookup order, which cannot be read from the data; with the blocks present, either order finds a Perfect entry. The smallest size class has no such region in the game, so it gets none here.
3. **Eight names added to every block.** The Civil War knife, fire arrows, dynamite arrows, the explosive pistol, revolver, repeater and rifle rounds, and explosive slugs. The original did not list them, and neither does the game's own table. This build lists them so that the mod covers them like the other weapons. They are harmless, but their effect is unknown. For the fire and explosive types the burn or blast that follows the hit is a separate cause of damage with no entry, so those kills are probably still not rated clean.

In numbers: the original had 25 blocks of 52 names (1,300 entries); this build has 29 blocks of 60 names (1,740 entries). No original entry was removed, and the hit limit is the only value changed in them.

## Requirements

- Lenny's Mod Loader. It reads `install.xml` in the mod's folder, which names the game file to replace.

## Install

1. With the game closed, copy the `Perfect Pelts - Perfect Damage` folder into the game's `lml` folder.
2. Check that the loader's list, `lml/mods.xml`, shows the mod as enabled. On this setup it is listed there under the folder name `Perfect Pelts - Perfect Damage`.

To uninstall, delete `lml/Perfect Pelts - Perfect Damage` and remove its entries from `lml/mods.xml`. Pelts and items already collected are ordinary game items and stay as they are.

## Files

| Path | What it is |
|---|---|
| `Perfect Pelts - Perfect Damage/damagecleanlinessdata.meta` | The replacement table, with the three changes above. Not the author's file as downloaded; that one is not kept in the repo. |
| `Perfect Pelts - Perfect Damage/install.xml` | The loader's install file: replaces `update:/common/data/damage/damagecleanlinessdata.meta`. The author's file, unchanged. |
| `Readme.txt` | The author's one-line install note. |

## Known limits

- Checked against the game's own data files and decompiled scripts for build 1.0.1491.50, not confirmed in the game. A quick check: kill a rabbit with one revolver shot and skin it. 2 Game Meat means the kill was rated Perfect; 1 means it was not.
- On its own the mod does not give a perfect pelt from a one-star or two-star animal.
- With Perfect Pelts installed as well, pelts are already the best item whatever the kill rating. This mod then decides the meat, feather and tooth counts and the clean-kill XP.
- Side effect on XP: a clean kill pays +3 Health XP and the game then skips its other kill awards. Predators killed with a bow or in melee would otherwise pay +10, and with a thrown weapon +20.
- Hits outside every hit region the game defines for an animal (fish have none, snakes only the head, small birds only head and heart) are rated by game code that could not be read.
- The file replaces the whole of `damagecleanlinessdata.meta` and matches build 1.0.1491.50 only.

## Credits

Perfect Pelts - Perfect Damage by MaVeRicK, one of the files on the "Perfect Pelts" page on Nexus Mods: <https://www.nexusmods.com/reddeadredemption2/mods/984/> (the link recorded in its `install.xml`). The `install.xml` gives version 1.0; the downloaded archive was labelled 1.1.
