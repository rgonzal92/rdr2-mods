// BetterPlayerHorses: a ScriptHookRDR2 plugin that gives the player's own horses top stats.
//
// Replaces the "Better All Horses - Elite" data mod (Nexus mod 842), which wrote the same
// ranks into the game's pedattributes.ymt. That file sets ranks per horse breed, so the data
// mod also boosted the horses of lawmen, enemies and gang members. This plugin sets the ranks
// on the player's horses only and leaves the game's file alone.

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// Game natives, by hash.
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;                    // PLAYER::PLAYER_ID() -> Player
const unsigned long long GET_ACTIVE_HORSE_FOR_PLAYER = 0x46FA0AE18F4C7FA9;  // PLAYER::_GET_ACTIVE_HORSE_FOR_PLAYER(Player player) -> Ped
const unsigned long long GET_SADDLE_HORSE_FOR_PLAYER = 0xB48050D326E9A2F3;  // PLAYER::_GET_SADDLE_HORSE_FOR_PLAYER(Player player) -> Ped
const unsigned long long GET_ATTRIBUTE_BASE_RANK = 0x147149F2E909323C;      // ATTRIBUTE::GET_ATTRIBUTE_BASE_RANK(Ped ped, int attributeIndex) -> int
const unsigned long long SET_ATTRIBUTE_BASE_RANK = 0x5DA12E025D47D4E5;      // ATTRIBUTE::SET_ATTRIBUTE_BASE_RANK(Ped ped, int attributeIndex, int newValue)

// The game's ePedAttribute indexes for the five horse stats.
enum { PA_HEALTH = 0, PA_STAMINA = 1, PA_AGILITY = 4, PA_SPEED = 5, PA_ACCELERATION = 6 };

// The ranks the Elite data mod gave every horse. 9 is the highest rank the game's file allows;
// its own best horses have health 6, stamina 6, agility 7, speed 6 and acceleration 5.
struct Rank {
    int attribute;
    int rank;
};
const Rank RANKS[] = {
    {PA_HEALTH, 6},
    {PA_STAMINA, 6},
    {PA_AGILITY, 9},
    {PA_SPEED, 9},
    {PA_ACCELERATION, 9},
};

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

static void raiseRanks(int horse) {
    if (!horse) return;  // the player has no such horse right now
    for (const Rank& r : RANKS) {
        // Raise only, so a horse that is already better keeps what it has.
        if (call(GET_ATTRIBUTE_BASE_RANK, horse, r.attribute) < r.rank) {
            call(SET_ATTRIBUTE_BASE_RANK, horse, r.attribute, r.rank);
        }
    }
}

// ponytail: re-applies on a timer, because the game creates the horse again after a save load,
// a stable visit or a fast travel; detect those instead if a delay of up to a second matters.
const unsigned long REAPPLY_INTERVAL_MS = 1000;

static void scriptMain() {
    for (;;) {
        const int player = call(PLAYER_ID);
        // The main horse, and the horse carrying the player's saddle when that is another one.
        // A temporary horse without the saddle is not the player's, so it is left alone.
        raiseRanks(call(GET_ACTIVE_HORSE_FOR_PLAYER, player));
        raiseRanks(call(GET_SADDLE_HORSE_FOR_PLAYER, player));
        scriptWait(REAPPLY_INTERVAL_MS);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    else if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
