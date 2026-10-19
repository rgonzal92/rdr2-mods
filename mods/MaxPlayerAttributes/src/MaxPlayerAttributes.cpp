// MaxPlayerAttributes: a ScriptHookRDR2 plugin that keeps the player's Health, Stamina and
// Dead Eye attributes at their highest level.
//
// The game keeps each attribute's XP as a running total of points, and the rank (level) follows
// from it. No game script writes these points for the player (the engine awards them), so
// setting the total to the cap once is enough; the loop only repeats it for a new player ped.

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// Game natives, by hash.
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;             // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long GET_ATTRIBUTE_POINTS = 0x219DA04BAA9CB065;      // ATTRIBUTE::GET_ATTRIBUTE_POINTS(Ped ped, int attributeIndex) -> int
const unsigned long long GET_MAX_ATTRIBUTE_POINTS = 0x223BF310F854871C;  // ATTRIBUTE::GET_MAX_ATTRIBUTE_POINTS(Ped ped, int attributeIndex) -> int
const unsigned long long SET_ATTRIBUTE_POINTS = 0x09A59688C26D88DF;      // ATTRIBUTE::SET_ATTRIBUTE_POINTS(Ped ped, int attributeIndex, int points)

// The game's ePedAttribute indexes for the player's three core attributes.
enum { PA_HEALTH = 0, PA_STAMINA = 1, PA_SPECIALABILITY = 2 };
const int ATTRIBUTES[] = {PA_HEALTH, PA_STAMINA, PA_SPECIALABILITY};

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

// Re-applies on a timer, so John gets the same once the epilogue makes him the player.
const unsigned long REAPPLY_INTERVAL_MS = 1000;

static void scriptMain() {
    for (;;) {
        const int ped = call(PLAYER_PED_ID);
        for (const int attribute : ATTRIBUTES) {
            const int max = call(GET_MAX_ATTRIBUTE_POINTS, ped, attribute);
            if (call(GET_ATTRIBUTE_POINTS, ped, attribute) < max) {
                call(SET_ATTRIBUTE_POINTS, ped, attribute, max);
            }
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
