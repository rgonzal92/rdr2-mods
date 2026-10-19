# AutomaticGreetReplies

Makes Arthur answer people who greet him, without the Greet button being pressed. Each new person is answered straight away. He does not keep talking to someone he has just answered.

- **Type:** ScriptHookRDR2 plugin (.asi)
- **Origin:** Rebuilt from source after Automatic Greet Replies 1.1, file `AutoGreet1.1.asi` (author not recorded in the mod's files, see Credits)
- **Status:** In use. Taken out of the game folder on 2026-10-14 for a clean camp-dialogue test (its replies may use up one-shot camp greets) and put back on 2026-10-16, identical to the build of that day; the ASI loader's log of 2026-10-18 lists it as loaded. The reply rule was moved into a tested header on 2026-10-18, with no change in behaviour.
- **Game build:** 1.0.1491.50, story mode

An `.asi` plugin is a small Windows library that an ASI loader loads into the game. ScriptHookRDR2 lets such a plugin call the game's own script commands.

## What it does

- The game has a greet auto-reply of its own. While it is on, Arthur answers a greeting that was addressed to him as if Greet had been pressed. The game turns it on only at its table games (poker, blackjack, dominoes, five finger fillet). The plugin turns it on in normal play.
- Every frame it turns the auto-reply on when all of this is true:
  - Arthur is not speaking,
  - the player is not locked on to anyone,
  - a person (human, not the player) is looking at Arthur and speaking,
  - none of the people doing so has been spoken to or greeted by Arthur in the last ten seconds. The plugin asks the game's own memory of whom the player has spoken to.
- The game decides the rest: whom Arthur answers and with which line. It replies only to a greeting addressed to him. Going by the game's data files, enemies shouting in a fight, lawmen giving orders, witnesses and scripted mission lines are not greetings and get no reply.
- It does nothing while the player is locked on, because the Greet and Antagonize choices are then on screen for the player.
- It does nothing while Arthur speaks.
- It does nothing while someone Arthur has just spoken to is talking to him. That person's answer counts as a greeting too, and answering it could start the game's follow-up exchange.
- Nothing is tied to Arthur: the player character is read again every frame, so it also applies to John in the epilogue.
- It has no settings and stores nothing.

## Changes from the original

The original came as a binary without source. The source here was written from its disassembly and then changed.

1. **A different person is answered straight away.** The original paused all replies for ten seconds after anything Arthur said.
2. **The pause is now per person and comes from the game.** Arthur stays quiet while someone he has spoken to in the last ten seconds is talking to him. Without this, one hello can turn into a second and third exchange with the same person.
3. **No hold-back after bad behaviour.** The original stopped replying when the player antagonized someone or lost honor, until the next in-game hour began or the player greeted someone by hand. That was a feature of the original. It is removed on purpose, so replies are always on.
4. **No timer that is read before it is set.** The original's ten-second pause read its start time before ever writing it. This build has no timer.
5. **Checks every frame.** The original slept a second at a time while Arthur spoke.
6. **Scans the people around only when Arthur is free to reply.** The original scanned every frame.
7. **Reads no script global.** The original read the honor value from a fixed position in the game's script memory, which ties a plugin to particular game builds.
8. **No C runtime.** The built file is 3 KB instead of 91 KB.

## Requirements

- An ASI loader (`dinput8.dll` in the game folder).
- ScriptHookRDR2 (`ScriptHookRDR2.dll`). Loaded here with ScriptHookRDR2 V2, version 1.9.

## Install

1. Run `make` in this folder. `build/` is not in git, so a fresh clone has no built file.
2. Copy `build/AutomaticGreetReplies.asi` into the game's main folder, next to `RDR2.exe`.
3. Do not install the original `AutoGreet1.1.asi` as well. The two would both switch the auto-reply on.

To uninstall, delete `AutomaticGreetReplies.asi` from the game folder. The plugin stores nothing. Any honor gained through its replies stays in the save.

## Build and test

- Tools: `make`, `clang++` with the `x86_64-pc-windows-msvc` target, `lld` and `llvm-dlltool`. No Windows SDK is needed.
- `make` builds `build/ScriptHookRDR2.lib` from `src/ScriptHookRDR2.def`, then `build/AutomaticGreetReplies.asi`.
- `make test` builds `build/ReplyRuleTest` for the local machine and runs it. Its 5 checks cover the reply rule in `src/ReplyRule.h`: nobody speaking, new speakers, a person just spoken to, and one such person holding the reply back for everyone. Which people are speaking and what they remember are questions to the game and have no automated test.

## Files

| File | What it is |
| --- | --- |
| `src/AutomaticGreetReplies.cpp` | Source of this build: the game calls and the per-frame loop. |
| `src/ReplyRule.h` | The reply rule, with no game calls. |
| `test/ReplyRuleTest.cpp` | The logic checks run by `make test`. |
| `src/ScriptHookRDR2.def` | The seven functions imported from `ScriptHookRDR2.dll`. |
| `Makefile` | Cross-builds the plugin on Linux. |
| `build/AutomaticGreetReplies.asi` | Built from this repo's source (3,072 bytes), not in git. This is the file to install. |
| `build/ScriptHookRDR2.lib`, `build/ReplyRuleTest` | Import library and test program made by the build, not in git. |

The original author's binary, `AutoGreet1.1.asi` (version 1.1, 91,136 bytes, linked 2021-03-24), is not in this repo. A reference copy is kept outside it, in `rdr2-tools/originals/AutomaticGreetReplies/`.

## Known limits

- Checked against the game's decompiled scripts and data files for build 1.0.1491.50, not confirmed in the game.
- Honor is expected to rise by itself. The game's scripts give one honor point when three people who were not greeted before are greeted within 60 seconds, outside camp and not counting enemy groups. An automatic reply is the same kind of speech as a pressed Greet. Each greeting also resets the game's count of people antagonized in a row.
- Arthur answers more than hellos. The game sends some remarks as greetings: about a body he carries, a drawn weapon or his looks, a lawman's passing warning, a child's insult, a prostitute's offer. He answers each with the friendly line before the player can choose, and the answer to an offer counts as turning it down.
- The auto-reply belongs to the player, not to one listener. The plugin decides only whether it is on in a frame; the game picks whom Arthur answers.
- "Locked on" is read with `IS_PLAYER_TARGETTING_ANYTHING`, as the original did. The game's own scripts use it only next to weapon-aim checks, and read the Greet and Antagonize lock-on through `GET_PLAYER_INTERACTION_TARGET_ENTITY`, so an unarmed lock-on may not count and Arthur may still auto-reply while those choices are on screen. Not tried in the game.
- Flag 54 (greeted by the player) is asked by none of the game's scripts, and both flag names come from a community list, not from the game.
- The ten-second rule relies on the game recording an automatic reply in the person's memory the way it records a spoken greeting. If it does not, Arthur can answer the same person's next line.
- While a person Arthur has just answered is still talking to him, a greeting from someone else in that moment can go unanswered.
- In poker and blackjack inside story missions the game leaves the auto-reply off on purpose. The plugin can turn it on there as well.

## Credits

- Original mod: Automatic Greet Replies 1.1, distributed as `AutoGreet1.1.asi`.
- Author: not recorded in the mod's files. A third-party mirror of the mod's description ("Automatic Greet Replies V1.1") credits Juliet G. This was not confirmed on the original page.
- Original download page: not recorded.
