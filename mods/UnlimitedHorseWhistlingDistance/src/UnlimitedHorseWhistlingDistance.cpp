// UnlimitedHorseWhistlingDistance: a ScriptHookRDR2 plugin that lets the player's horse answer
// the whistle from any distance.
//
// Replaces UnlimitedHorseWhistlingDistance.asi 1.3 (built 2021-06-01), kept in this folder. That
// version watched the whistle button and, when the saddled horse was more than 204.77 m away,
// moved it to a point 50 m from the player and told it to run over. The game can do this by
// itself: its "Increase Whistle Range For Horse" cheat sets one flag, and its horse script then
// brings a horse that is too far away to hear. This plugin sets that flag and nothing else, so
// none of the cheat's bookkeeping runs (the cheat turns achievements off). Changes from 1.3:
//   - starts where the horse stops hearing the whistle, a range that grows with bonding; 1.3
//     used a fixed 204.77 m whatever the bond
//   - the game picks where the horse appears: 40 to 80 m away, out of view, on ground with a
//     path to the player, or it says the horse could not find you. 1.3 used a blind 50 m offset
//     and the ground below a point 100 m up, which can be a roof or a clifftop, and left the
//     horse at that height when no ground was found
//   - a dead horse stays where it is; 1.3 never checked that the horse existed or was alive
//   - the game's whistle rules still apply (indoors, caves, missions that hold the horse back);
//     1.3 moved the horse regardless
//   - the horse is unhitched and comes on the game's own whistle task, not a 70 second errand
//   - a bonded temporary horse is brought as well, as with the cheat; 1.3 moved only the horse
//     carrying the saddle
//   - no work every frame, no leftover keyboard hook

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();
__declspec(dllimport) unsigned long long* getGlobalPtr(int index);

// Game natives, by hash.
const unsigned long long GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(Hash scriptHash) -> int

// The game script that owns the player's horses and answers the whistle: joaat("player_horse").
const unsigned int PLAYER_HORSE_SCRIPT = 0x07C73408;

// That script's flags: Global_1900383.f_316 in the decompiled scripts of game build 1.0.1491.50
// (and of 1355 and 1436). A game update can move script globals, and a stale index would write
// into some other variable: check it against the new scripts before using this on another build.
const int HORSE_FLAGS = 1900383 + 316;

// The flag the cheat sets (CHEAT_SET_HORSE_TO_SPAWN_NEAR_PLAYER_ON_WHISTLE). In the 1491.50
// scripts only the whistle handling in player_horse acts on it.
const unsigned long long SPAWN_NEAR_PLAYER_ON_WHISTLE = 8;

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

// ponytail: re-applies on a timer instead of detecting when the flag is lost (a loaded save
// restarts the game's scripts); set it every frame if a whistle in that first second matters.
const unsigned long REAPPLY_INTERVAL_MS = 1000;

static void scriptMain() {
    for (;;) {
        // getGlobalPtr does not check that the game has given the global any memory, and a
        // plugin can be running before the game's scripts are. While player_horse runs, it has.
        if (call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, PLAYER_HORSE_SCRIPT) > 0) {
            *getGlobalPtr(HORSE_FLAGS) |= SPAWN_NEAR_PLAYER_ON_WHISTLE;
        }
        scriptWait(REAPPLY_INTERVAL_MS);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    else if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
