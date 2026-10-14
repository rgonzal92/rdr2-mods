// AutomaticGreetReplies: a ScriptHookRDR2 plugin that makes Arthur reply to people who greet him.
//
// Recreated from the disassembly of AutoGreet1.1.asi (built 2021-03-24), which turned on the
// game's own greet auto-reply whenever a person looking at the player was speaking. With the
// auto-reply on, the game itself decides what happens: Arthur answers a greeting that was
// addressed to him, as if the Greet button had been pressed (playerresponses.meta, rule
// AutoReplyToGreet). Changes from that version:
//   - answers a different person straight away; the original waited ten seconds after any reply
//   - stays quiet while someone he has spoken to in the last ten seconds is talking to him,
//     going by the game's own record of whom the player has spoken to. A person's answer to
//     Arthur counts as a greeting too, and answering it would start a second and third exchange
//   - no longer holds back after the player antagonizes someone or loses honor
//   - checks every frame; the original slept a second at a time while Arthur spoke
//   - scans the nearby people only when Arthur is free to reply

// Functions imported from ScriptHookRDR2.dll, with the signatures encoded in the original's
// import table (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();
__declspec(dllimport) int worldGetAllPeds(int* peds, int size);

// Game natives, by hash.
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;                           // PLAYER::PLAYER_ID() -> Player
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;                       // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long IS_PLAYER_TARGETTING_ANYTHING = 0x4605C66E0F935F83;       // PLAYER::IS_PLAYER_TARGETTING_ANYTHING(Player player) -> BOOL
const unsigned long long SET_PLAYER_RESET_FLAG = 0x9F9A829C6751F3C7;               // PLAYER::_SET_PLAYER_RESET_FLAG(Player player, int flag, BOOL toggle)
const unsigned long long IS_PED_HEADTRACKING_PED = 0x5CD3CB88A7F8850D;             // PED::IS_PED_HEADTRACKING_PED(Ped ped1, Ped ped2) -> BOOL
const unsigned long long IS_PED_A_PLAYER = 0x12534C348C6CB68B;                     // PED::IS_PED_A_PLAYER(Ped ped) -> BOOL
const unsigned long long IS_PED_HUMAN = 0xB980061DA992779D;                        // PED::IS_PED_HUMAN(Ped ped) -> BOOL
const unsigned long long GET_PED_HAS_INTERACTED_WITH_PLAYER = 0x947E43F544B6AB34;  // PED::_GET_PED_HAS_INTERACTED_WITH_PLAYER(Ped targetPed, Player player, int flag, int durationMs) -> BOOL
const unsigned long long IS_ANY_SPEECH_PLAYING = 0x729072355FA39EC9;               // AUDIO::IS_ANY_SPEECH_PLAYING(Ped ped) -> BOOL

const int PLAYER_RESET_FLAG_ENABLE_GREET_AUTO_REPLY = 41;

// What a person remembers about the player, as asked through _GET_PED_HAS_INTERACTED_WITH_PLAYER.
const int SPOKEN_TO_BY_PLAYER = 7;
const int GREETED_BY_PLAYER = 54;

// How long a person Arthur has spoken to is left alone. The game's own scripts ask the same
// question with the same ten seconds (medium_update).
const int SAME_PERSON_MS = 10000;

const int MAX_PEDS = 1024;

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

// True when a person other than the player is looking at the player and speaking, and nobody
// who is doing so has been spoken to by Arthur in the last ten seconds. The auto-reply belongs
// to the player, not to one listener, so it has to stay off for as long as such a person talks.
static bool someoneNewIsSpeakingToPlayer(int player, int playerPed) {
    static int peds[MAX_PEDS];
    const int count = worldGetAllPeds(peds, MAX_PEDS);
    bool found = false;
    for (int i = 0; i < count; ++i) {
        const int ped = peds[i];
        if (!call(IS_PED_HEADTRACKING_PED, ped, playerPed) || !call(IS_ANY_SPEECH_PLAYING, ped) ||
            call(IS_PED_A_PLAYER, ped) || !call(IS_PED_HUMAN, ped)) {
            continue;
        }
        if (call(GET_PED_HAS_INTERACTED_WITH_PLAYER, ped, player, SPOKEN_TO_BY_PLAYER, SAME_PERSON_MS) ||
            call(GET_PED_HAS_INTERACTED_WITH_PLAYER, ped, player, GREETED_BY_PLAYER, SAME_PERSON_MS)) {
            return false;
        }
        found = true;
    }
    return found;
}

static void scriptMain() {
    for (;;) {
        const int player = call(PLAYER_ID);
        const int playerPed = call(PLAYER_PED_ID);

        // Locked on, the player picks greet or antagonize by hand. Otherwise request a reply;
        // the flag lasts one frame, so it is set again every frame until Arthur starts speaking.
        if (playerPed && !call(IS_ANY_SPEECH_PLAYING, playerPed) && !call(IS_PLAYER_TARGETTING_ANYTHING, player) &&
            someoneNewIsSpeakingToPlayer(player, playerPed)) {
            call(SET_PLAYER_RESET_FLAG, player, PLAYER_RESET_FLAG_ENABLE_GREET_AUTO_REPLY, true);
        }
        scriptWait(0);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    else if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
