# MoneyHotkeys

Changes the player's cash by $500 per key press: `]` adds $500, `[` takes $500 away, never below $0.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Written from scratch for this repo
- **Status:** Built 2026-10-16, not yet tried in game
- **Game build:** 1.0.1491.50, story mode

## What it does

- Each fresh press of `]` adds $500 with the game's own money call (`MONEY::_MONEY_INCREMENT_CASH_BALANCE`, reason `ADD_REASON_DEFAULT`, as the game's scripts use it).
- Each fresh press of `[` takes $500 away (`MONEY::_MONEY_DECREMENT_CASH_BALANCE`). With less than $500, it takes what is there, so cash stops at $0. The game's own scripts clamp the same way.
- Holding a key does not repeat it: one press, one change.
- Presses made outside play (the main menu, a loading screen) are dropped.
- The keys come from ScriptHookRDR2's keyboard handler, which only sees keys the game window receives. Typing `[` or `]` in another window does nothing.

Money is counted in cents inside the game; the game's cheat menu adds $500 as 50000 (`cheat_ui.c`), and so does this plugin.

## Requirements

- An ASI loader: the `dinput8.dll` that loads `.asi` plugins from the game folder.
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). The plugin imports eight of its functions, listed in `src/ScriptHookRDR2.def`.

## Install

1. Run `make` to produce `build/MoneyHotkeys.asi`.
2. Copy `build/MoneyHotkeys.asi` into the game's root folder.

To uninstall, delete the file from the game folder. Money already added or taken stays in the save.

## Build and test

- Tools: `make`, `clang++` with the `lld` linker, and `llvm-dlltool`. The plugin is cross-built on Linux and needs no Windows SDK or C runtime.
- `make` builds `build/MoneyHotkeys.asi`, after making the import library from `src/ScriptHookRDR2.def`.
- There is no `make test`. The plugin is one loop of game calls, with nothing that can run outside the game.
- There is no settings file. To change the amount or the keys, edit `CENTS_PER_PRESS`, `VK_OEM_4` or `VK_OEM_6` in `src/MoneyHotkeys.cpp` and rebuild.

## Files

| Path | What it is |
|---|---|
| `src/MoneyHotkeys.cpp` | The plugin. |
| `src/ScriptHookRDR2.def` | The functions the plugin imports; the Makefile turns it into an import library. |
| `Makefile` | Build rules. |
| `build/` | Build output from this repo's source: `MoneyHotkeys.asi` and the import library. |

## Known limits

- Not confirmed in the game.
- The keys are fixed by Windows virtual-key code (`VK_OEM_4`, `VK_OEM_6`), which sit on `[` and `]` on a US layout; on other layouts they may be other keys.
- If the game has an action bound to `[` or `]`, that action fires too. The default bindings were not checked.
- The game's money message does not show; the balance just changes.
- The game's career cash stat is not changed, so hotkey money does not count as earned.
