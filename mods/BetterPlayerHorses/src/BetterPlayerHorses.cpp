// BetterPlayerHorses: a ScriptHookRDR2 plugin that gives the player's own horses top stats.
//
// Replaces the "Better All Horses - Elite" data mod (Nexus mod 842), which wrote the same
// ranks into the game's pedattributes.ymt. That file sets ranks per horse breed, so the data
// mod also boosted the horses of lawmen, enemies and gang members. This plugin sets the ranks
// on the player's horses only and leaves the game's file alone.
//
// It also keeps every horse the player owns at the top bonding level, in the horse's saved
// record as well as on the horse in the world.

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();
__declspec(dllimport) unsigned long long* getGlobalPtr(int index);

// The Microsoft linker expects this symbol in any program that uses floating point; the C
// runtime normally provides it, and this plugin has none.
extern "C" int _fltused = 0;

// Game natives, by hash.
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;                    // PLAYER::PLAYER_ID() -> Player
const unsigned long long GET_ACTIVE_HORSE_FOR_PLAYER = 0x46FA0AE18F4C7FA9;  // PLAYER::_GET_ACTIVE_HORSE_FOR_PLAYER(Player player) -> Ped
const unsigned long long GET_SADDLE_HORSE_FOR_PLAYER = 0xB48050D326E9A2F3;  // PLAYER::_GET_SADDLE_HORSE_FOR_PLAYER(Player player) -> Ped
const unsigned long long GET_ATTRIBUTE_BASE_RANK = 0x147149F2E909323C;      // ATTRIBUTE::GET_ATTRIBUTE_BASE_RANK(Ped ped, int attributeIndex) -> int
const unsigned long long SET_ATTRIBUTE_BASE_RANK = 0x5DA12E025D47D4E5;      // ATTRIBUTE::SET_ATTRIBUTE_BASE_RANK(Ped ped, int attributeIndex, int newValue)
const unsigned long long GET_ATTRIBUTE_RANK = 0xA4C8E23E29040DE0;           // ATTRIBUTE::GET_ATTRIBUTE_RANK(Ped ped, int attributeIndex) -> int
const unsigned long long GET_MAX_ATTRIBUTE_RANK = 0x704674A0535A471D;       // ATTRIBUTE::GET_MAX_ATTRIBUTE_RANK(Ped ped, int attributeIndex) -> int
const unsigned long long GET_MAX_ATTRIBUTE_POINTS = 0x223BF310F854871C;     // ATTRIBUTE::GET_MAX_ATTRIBUTE_POINTS(Ped ped, int attributeIndex) -> int
const unsigned long long SET_ATTRIBUTE_POINTS = 0x09A59688C26D88DF;         // ATTRIBUTE::SET_ATTRIBUTE_POINTS(Ped ped, int attributeIndex, int points)
const unsigned long long GET_DEFAULT_MAX_ATTRIBUTE_RANK = 0x7C059C55AD940CB4;               // ATTRIBUTE::GET_DEFAULT_MAX_ATTRIBUTE_RANK(Hash modelHash, int attributeIndex) -> int
const unsigned long long GET_DEFAULT_ATTRIBUTE_POINTS_NEEDED_FOR_RANK = 0x94A7F191DB49A44D;  // ATTRIBUTE::GET_DEFAULT_ATTRIBUTE_POINTS_NEEDED_FOR_RANK(Hash modelHash, int attributeIndex, int rank) -> int
const unsigned long long SET_MOUNT_BONDING_LEVEL = 0xA69899995997A63B;      // PED::_SET_MOUNT_BONDING_LEVEL(Ped ped, int bondingLevel)
const unsigned long long IS_MODEL_VALID = 0x392C8D8E07B70EFC;               // STREAMING::IS_MODEL_VALID(Hash model) -> BOOL
const unsigned long long GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(Hash scriptHash) -> int

// The game's ePedAttribute indexes for the five horse stats and bonding.
enum { PA_HEALTH = 0, PA_STAMINA = 1, PA_AGILITY = 4, PA_SPEED = 5, PA_ACCELERATION = 6, PA_BONDING = 7 };

// The game script that owns the player's horses: joaat("player_horse").
const unsigned int PLAYER_HORSE_SCRIPT = 0x07C73408;

// The saved records of the player's horses: Global_40.f_1095.f_1 in the decompiled scripts of
// game build 1.0.1491.50, 7 stable slots of 436 values behind a length. In a record, f_9 is the
// horse's model (0 for an empty slot), f_372 its bonding rank and f_372.f_1 its bonding points,
// a float. The game copies these onto the horse when it appears and after each bonding award
// (short_update.c func_3160), so bonding set on the horse alone does not last. A game update
// can move script globals: the length check keeps the plugin from writing elsewhere then.
const int HORSE_RECORDS = 40 + 1095 + 1;
const int HORSE_SLOTS = 7, RECORD_SIZE = 436;
const int RECORD_MODEL = 9, RECORD_BONDING_RANK = 372, RECORD_BONDING_POINTS = 373;

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
    // Bonding on the horse in the world, for the time until the game copies the saved record
    // onto it again. The game's scripts set the bonding level after the points, as here.
    if (call(GET_ATTRIBUTE_RANK, horse, PA_BONDING) < call(GET_MAX_ATTRIBUTE_RANK, horse, PA_BONDING)) {
        call(SET_ATTRIBUTE_POINTS, horse, PA_BONDING, call(GET_MAX_ATTRIBUTE_POINTS, horse, PA_BONDING));
        call(SET_MOUNT_BONDING_LEVEL, horse, call(GET_ATTRIBUTE_RANK, horse, PA_BONDING));
    }
}

static unsigned long long* recordField(int slot, int field) {
    return getGlobalPtr(HORSE_RECORDS + 1 + RECORD_SIZE * slot + field);
}

// Gives every horse the player owns, stabled ones too, the top bonding rank in its saved record,
// the way the game's own scripts set a rank (short_update.c func_1521): the rank, and the points
// that rank needs. With the record at the top, the game's bonding awards stop changing it.
static void maxSavedBonding() {
    // getGlobalPtr does not check that the game has given the global any memory, and a plugin
    // can be running before the game's scripts are. While player_horse runs, it has.
    if (call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, PLAYER_HORSE_SCRIPT) <= 0) return;
    const unsigned long long* const length = getGlobalPtr(HORSE_RECORDS);
    if (!length || static_cast<int>(*length) != HORSE_SLOTS) return;  // another game build
    for (int slot = 0; slot < HORSE_SLOTS; slot++) {
        const int model = static_cast<int>(*recordField(slot, RECORD_MODEL));
        if (!model || !call(IS_MODEL_VALID, model)) continue;  // an empty slot
        const int rank = call(GET_DEFAULT_MAX_ATTRIBUTE_RANK, model, PA_BONDING);
        if (static_cast<int>(*recordField(slot, RECORD_BONDING_RANK)) >= rank) continue;
        const union {
            float number;
            unsigned int bits;
        } points = {static_cast<float>(call(GET_DEFAULT_ATTRIBUTE_POINTS_NEEDED_FOR_RANK, model, PA_BONDING, rank))};
        // A script value fills the low half of its 8-byte slot.
        *recordField(slot, RECORD_BONDING_RANK) = static_cast<unsigned int>(rank);
        *recordField(slot, RECORD_BONDING_POINTS) = points.bits;
    }
}

// ponytail: re-applies on a timer, because the game creates the horse again after a save load,
// a stable visit or a fast travel; detect those instead if a delay of up to a second matters.
const unsigned long REAPPLY_INTERVAL_MS = 1000;

static void scriptMain() {
    for (;;) {
        maxSavedBonding();
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
