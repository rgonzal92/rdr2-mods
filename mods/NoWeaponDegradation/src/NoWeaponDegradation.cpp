// NoWeaponDegradation: a ScriptHookRDR2 plugin that keeps the player's weapons clean.
//
// Recreated from the disassembly of NoWeaponDegradation.asi V2 (built 2025-04-02), which every
// frame wrote zero wear, dirt and soot to the weapons in the player's hands. Changes from it:
//   - also keeps the holstered weapons clean. The original asked for two places on the body
//     that the game does not use for the player's weapons, and never for the four it does use
//   - leaves every weapon alone while the mission "Paying a Social Call" runs. That mission
//     teaches cleaning and cannot be finished until its dirty Double-Barreled Shotgun has been
//     cleaned by the player. The original spared only that shotgun, and only in a hand
//   - during that mission gives a spotless Double-Barreled Shotgun a little wear. A player who
//     already owns one would otherwise have nothing to clean, and the mission would not end
//   - cleans exactly as far as the game's own cleaning does: down to a weapon's permanent wear,
//     with the same commands and arguments. The original wrote zero to picked-up worn guns too
//   - re-applies once a second instead of every frame
//   - no keyboard hook, no C runtime

#include "WeaponCare.h"

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// The compiler refers to this marker in any code that uses floats and expects the C runtime to
// define it. The plugin is built without that runtime, so it is defined here.
extern "C" int _fltused = 0;

using Hash = unsigned int;

// Game natives, by hash.
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;                                            // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(Hash scriptHash) -> int
const unsigned long long GET_CURRENT_PED_WEAPON_ENTITY_INDEX = 0x3B390A939AF0B5FC;                      // WEAPON::GET_CURRENT_PED_WEAPON_ENTITY_INDEX(Ped ped, int attachPoint) -> Entity
const unsigned long long GET_CURRENT_PED_WEAPON = 0x3A87E44BB9A01D54;                                   // WEAPON::GET_CURRENT_PED_WEAPON(Ped ped, Hash* weaponHash, BOOL p2, int attachPoint, BOOL p4) -> BOOL
const unsigned long long GET_WEAPON_DEGRADATION = 0x0D78E1097F89E637;                                   // WEAPON::GET_WEAPON_DEGRADATION(Object weaponObject) -> float
const unsigned long long GET_WEAPON_PERMANENT_DEGRADATION = 0xD56E5F336C675EFA;                         // WEAPON::GET_WEAPON_PERMANENT_DEGRADATION(Object weaponObject) -> float
const unsigned long long SET_WEAPON_DEGRADATION = 0xA7A57E89E965D839;                                   // WEAPON::_SET_WEAPON_DEGRADATION(Object weaponObject, float level)
const unsigned long long SET_WEAPON_DAMAGE = 0xE22060121602493B;                                        // WEAPON::_SET_WEAPON_DAMAGE(Object weaponObject, float level, BOOL p2)
const unsigned long long SET_WEAPON_DIRT = 0x812CE61DEBCAB948;                                          // WEAPON::_SET_WEAPON_DIRT(Object weaponObject, float level, BOOL p2)
const unsigned long long SET_WEAPON_SOOT = 0xA9EF4AD10BDDDB57;                                          // WEAPON::_SET_WEAPON_SOOT(Object weaponObject, float level, BOOL p2)

// The game's string hash: Jenkins one-at-a-time over the lowercased name.
constexpr Hash joaat(const char* name) {
    Hash hash = 0;
    for (; *name; ++name) {
        char c = *name;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        hash += static_cast<unsigned char>(c);
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

// The script of the mission "Paying a Social Call", and the weapon it teaches cleaning with.
constexpr Hash CLEANING_LESSON_SCRIPT = joaat("rcm_dusters21");
constexpr Hash WEAPON_SHOTGUN_DOUBLEBARREL = joaat("WEAPON_SHOTGUN_DOUBLEBARREL");
static_assert(CLEANING_LESSON_SCRIPT == 0x2ABCB7D6 && WEAPON_SHOTGUN_DOUBLEBARREL == 0x6DFA071B,
              "joaat must reproduce the hashes found in the original");

// Where the player carries weapons, as the game's own scripts look them up: the two hands, the
// two hip holsters, and the two long-gun places on the back and shoulder.
const int ATTACH_POINTS[] = {0, 1, 2, 3, 9, 10};

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

// A float travels to and from a native as its bit pattern.
static unsigned int bits(float value) {
    unsigned int result;
    __builtin_memcpy(&result, &value, sizeof result);
    return result;
}
static float asFloat(int pattern) {
    float result;
    __builtin_memcpy(&result, &pattern, sizeof result);
    return result;
}

// The weapon carried at one attach point, or 0. The game writes the answer through a pointer.
static Hash weaponAt(int ped, int attachPoint) {
    unsigned long long weapon = 0;
    nativeInit(GET_CURRENT_PED_WEAPON);
    nativePush64(static_cast<unsigned int>(ped));
    nativePush64(reinterpret_cast<unsigned long long>(&weapon));
    nativePush64(true);
    nativePush64(static_cast<unsigned int>(attachPoint));
    nativePush64(true);
    return static_cast<int>(*nativeCall()) ? static_cast<Hash>(weapon) : 0;
}

// ponytail: re-applies on a timer instead of every frame; a weapon can show up to a second of
// fresh dirt. Shorten it if that is ever visible.
const unsigned long REAPPLY_INTERVAL_MS = 1000;

static void scriptMain() {
    for (;;) {
        const int playerPed = call(PLAYER_PED_ID);
        if (playerPed) {
            const bool lessonRunning = call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, CLEANING_LESSON_SCRIPT) > 0;
            for (int point : ATTACH_POINTS) {
                const int weapon = call(GET_CURRENT_PED_WEAPON_ENTITY_INDEX, playerPed, point);
                if (!weapon) continue;
                const bool isLessonGun = lessonRunning && weaponAt(playerPed, point) == WEAPON_SHOTGUN_DOUBLEBARREL;
                const float permanentWear = asFloat(call(GET_WEAPON_PERMANENT_DEGRADATION, weapon));
                const float wear = wearToWrite(lessonRunning, isLessonGun, asFloat(call(GET_WEAPON_DEGRADATION, weapon)), permanentWear);
                if (wear != LEAVE_WEAR) call(SET_WEAPON_DEGRADATION, weapon, bits(wear));
                if (!lessonRunning) {
                    // The rest of what the game's own cleaning does, with its arguments.
                    call(SET_WEAPON_DAMAGE, weapon, bits(permanentWear), false);
                    call(SET_WEAPON_DIRT, weapon, bits(0.0f), false);
                    call(SET_WEAPON_SOOT, weapon, bits(0.0f), false);
                }
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
