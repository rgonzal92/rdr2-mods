# NoAutoHorseHolstering

Lets the player keep a rifle, repeater, shotgun or bow in hand when getting on a horse, instead of the game putting it away. A player who mounts with empty hands stays unarmed. Pistols and revolvers are left to the game.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after No Auto Horse Holstering 1.1.1, file `NoHolstering.asi` (author not recorded in the mod's files, see Credits)
- **Status:** In use, as of 2026-10-14 (the file in the game folder is identical to `build/NoAutoHorseHolstering.asi`; not yet tried in game)
- **Game build:** 1.0.1491.50, story mode

An `.asi` plugin is a small Windows library that an ASI loader loads into the game. ScriptHookRDR2 lets such a plugin call the game's own script commands.

## What it does

- On foot it notes the weapon in the player's hand every frame.
- On the first frame on a horse, if that weapon was a long arm, it puts it in the hand again, which is the call the original made at that moment. If the hand was empty, it asks for an empty hand.
- For the next four seconds it watches the hand. If the long arm leaves it (the hand goes empty), it puts it back once. If a weapon shows up in an unarmed rider's hand, it puts it away once. After that one correction the ride is the player's.
- It stops watching at once, without acting, when the player holds any weapon control: the weapon wheel, aim, attack, holster, next and previous weapon, the weapon select keys, quick equip or drop weapon. It does not act at the mount at all when the player held one on foot in the last second before getting on, since a holster or draw started then may not show in the hand yet.
- It does nothing while player control is off or while a mission is running (the game's mission flag). The game's scripts turn player control off for many of their scripted scenes, and set the mission flag for story and stranger missions, so a holster asked for by a mission or such a scene is not undone.
- Long arm means a bow, or a gun the game calls two-handed. The game's own scripts check bows separately from `_IS_WEAPON_TWO_HANDED` when they look for long arms, so the two-handed test alone is taken not to cover bows. Requiring a gun keeps any two-handed melee weapon and tools such as the fishing rod out.
- Pistols, revolvers, melee weapons, the lasso and tools in hand at the mount are left alone for the whole ride.
- Getting off the horse is left to the game, including its setting that stows long arms on the horse.
- A ride already under way when the plugin starts is left alone. Every mount after the next dismount counts.
- It has no settings and stores nothing.

## Changes from the original

The original came as a binary without source (95,744 bytes, linked 2023-01-23). The source here was written from its disassembly and then changed.

1. **No every-frame unarmed call.** The original put an unarmed rider's empty hand back with `SET_CURRENT_PED_WEAPON(..., equipNow)` on every frame of the ride. It paused for five seconds only after the wheel, horse aim, holster and horse attack controls, not after weapon cycling or the select keys, so it could cancel a draw made with those. This build asks once at the mount and at most once more in the four seconds after.
2. **Weapon controls end its say.** The original's pause list missed cycling and selecting. This build stops for the rest of the ride as soon as any weapon control is held, and also leaves the ride alone when one (including aiming or attacking on foot) was held in the second before the mount, so a holster or draw started just before getting on is not undone.
3. **Scenes and missions are left alone.** The original could undo a holster asked for by a mission or a scene on horseback. This build does nothing while player control is off or the mission flag is set. In the decompiled 1.0.1491.50 scripts, 243 scripts set the mission flag, among them the story and stranger missions. The game has no cutscene test of its own in those scripts: its cutscenes are anim scenes, and its scripts turn player control off for many of them.
4. **Tied to the mount.** The original's gate was reset only by the weapon wheel, so it also cut short the first draw of each of the first two long arms in every ride, and never looked at a third. This build looks only at the weapon in hand on the last frame on foot.
5. **Bows count.** The original used `_IS_WEAPON_TWO_HANDED` alone, which by the game's own scripts appears not to include bows.
6. **No `_SET_FORCE_AUTO_EQUIP(player, false)`.** The original called it every frame, on foot too. None of the 1,639 decompiled game scripts calls it, by name or by hash, so nothing shows what it changes or that the plugin needs it. It is left out.
7. **No leftover template code.** The original registered a hidden, disabled "Sell Current Weapon" prompt on `INPUT_INTERACT_OPTION1` and never deleted it, carried a toast routine behind a flag nothing set, and asked every frame for the player's coordinates and saddle horse without using either.
8. **No C runtime.** The original linked the C runtime statically and seeded its random numbers, which it never used. The built file is 3 KB.

## Requirements

- An ASI loader (`dinput8.dll` in the game folder).
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports six of its functions, listed in `src/ScriptHookRDR2.def`.

## Install

1. Run `make` in this folder.
2. Copy `build/NoAutoHorseHolstering.asi` into the game's main folder, next to `RDR2.exe`.
3. Do not install the original `NoHolstering.asi` as well. The two would both change the weapon at the mount.

To uninstall, delete `NoAutoHorseHolstering.asi` from the game folder. The plugin stores nothing.

## Build and test

- Tools: `make`, `clang++` with the `x86_64-pc-windows-msvc` target, `lld` and `llvm-dlltool`. No Windows SDK is needed.
- `make` builds `build/ScriptHookRDR2.lib` from `src/ScriptHookRDR2.def`, then `build/NoAutoHorseHolstering.asi`.
- `make test` builds `build/MountLogicTest` for the local machine and runs it. Its 13 checks cover the mount decisions in `src/MountLogic.h` without the game: the mount call, the one correction, unarmed riders, pistols, weapon controls on the horse and just before the mount, scenes and missions, the end of the window, and a ride under way at start.

## Files

| Path | What it is |
| --- | --- |
| `src/NoAutoHorseHolstering.cpp` | The plugin: game calls and the per-frame loop. |
| `src/MountLogic.h` | The mount decisions, with no game calls. |
| `src/ScriptHookRDR2.def` | The six functions imported from `ScriptHookRDR2.dll`. |
| `test/MountLogicTest.cpp` | The logic checks run by `make test`. |
| `Makefile` | Build and test rules. |
| `build/` | Build output from this repo's source: `NoAutoHorseHolstering.asi`, the import library and the test program. |
| `NoHolstering.asi` | The original author's binary, version 1.1.1 (sha256 `507ee362...99c4af`). Kept for reference, not installed. |

## Known limits

- Checked against the disassembly and the game's decompiled scripts for build 1.0.1491.50 and by the logic tests, not confirmed in the game.
- The game holsters the long arm on mount in its own code; no game script does it. When that happens is not known. Like the original, this build relies on the long arm still being in hand on the last frame on foot or the first frame on the horse. If the game puts it away earlier, while the player is still walking up to the horse, the plugin sees an empty hand and keeps the rider unarmed.
- What the game does to an unarmed rider's hand at the mount is not known either. The original's every-frame call suggests the game can draw a weapon then. This build undoes such a draw once, after it shows in the hand, so a short draw animation may play.
- Four seconds is a guess at the length of a mount. A holster after that is not undone.
- One second is a guess at how long a holster or draw takes to show in the hand. A weapon control used on foot in the second before getting on, even one that did not change the weapon, leaves that ride to the game.
- A weapon control held as the player gets on, such as aim, hands that ride to the player.
- During missions the plugin is off, so the game holsters as usual.
- A scripted scene outside a mission that leaves player control on (some random encounters) is not detected. Only the four seconds after a mount are exposed to it.
- `UnofficialPatch.asi` also sets the player's weapon, but only on foot (its knife guard in "We Loved Once and True") or by switching inventories. This plugin sets it only while mounted and never during a mission, so the two do not meet.
- The game calls in `src/NoAutoHorseHolstering.cpp` have no automated test.

## Credits

- Original mod: No Auto Horse Holstering 1.1.1, Nexus Mods file 1954 for Red Dead Redemption 2, distributed as `NoHolstering.asi`: <https://www.nexusmods.com/reddeadredemption2/mods/1954>.
- Author: not recorded in the mod's files (the archive holds only the `.asi`). Not confirmed from the Nexus Mods page.
