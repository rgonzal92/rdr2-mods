# PatchProbe

Watches the running game and writes what it sees to a log. It changes nothing. It exists to check, in the game, three things that the fixes planned after the audit of 2026-10-13 depend on and that could only be derived from files until now.

- **Type:** ScriptHookRDR2 plugin (.asi): a small Windows library that an ASI loader loads into the game and that calls the game's script functions ("natives") through ScriptHookRDR2
- **Origin:** Written from scratch for this repo
- **Status:** Built 2026-10-13, not yet run in the game
- **Game build:** 1.0.1491.50, story mode

## What it does

It appends lines to `PatchProbe.log` in the game folder. There are three kinds:

- **`threads`**: for twelve game scripts, the number the native `GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH` returns, next to a count made with the game's thread iterator. A line is written whenever one of the numbers changes, with `DIFFER` when the two disagree. One reading of the engine code says the native returns a reference count minus one, not a thread count. Every planned fix uses it as its "is this script running" test, so this decides whether they may.
- **`layout`**: once per loaded save, nine script globals whose position was derived offline, with the value the audit expects (for example the length 120 in front of the random-encounter table). `DIFFERS` means an index is wrong and nothing may be built on it.
- **`cooldowns`**: once a minute, how the 120 random-encounter cooldowns changed. The audit says the game takes exactly 90 off one cooldown per frame and nothing else lowers one. `-90 when not visited` must stay 0, and `exact -90` must be nearly all of `visits`.

It writes no script global, calls no native that changes the game, patches no code and searches no memory.

## What to do in the game

Play as usual for ten minutes or more. The log says more when the session includes a mission start, a visit to camp, a shop and a few minutes of riding in open country.

## Install and remove

Copy `PatchProbe.asi` into the game folder. To remove it, delete `PatchProbe.asi` and `PatchProbe.log` there.

## Build

`make` builds `build/PatchProbe.asi` on Linux with clang and lld. `make test` runs the checks of the bookkeeping in `src/ProbeLogic.h` without the game.

## Sources

The indexes and hashes come from `~/Projects/rdr2-tools/audit-2026-10-13-unofficial-patch` (`SUMMARY.md`, `p2-encounter-cooldowns.md`, `p2-critic.md`).
