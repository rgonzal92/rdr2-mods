// RemoveSpeedLimits: a ScriptHookRDR2 plugin that lifts the game's speed limits on the player
// and on the horse the player is riding, and makes the cover key switch a fast walk on and off.
//
// Recreated from the disassembly of RemoveSpeedLimits.asi (built 2023-11-12). The on-foot speeds
// and the keys are that version's. Changes from it:
//   - applies the on-foot rules only on foot; the original also held the rider at a sprint
//     while the sprint key was down, and let the cover key switch fast walk from the saddle
//   - sets no minimum pace on the horse below a gallop; the original held it at a canter or
//     faster while forward was held, so it could not slow down or keep pace with a companion
//   - keeps the gallop only while the sprint key is held at a gallop; the original also forced
//     a gallop after three seconds of that key at a canter, where the game reads the hold as
//     "keep this pace"
//   - exempts the horse from the speed zones around camps and towns the way the game's own
//     scripts do, with reset flag 154 on the horse; the original fought the zones with the
//     forced pace
//   - holds a sprint or a fast walk on foot only while a direction key is held; the original
//     held a sprint with none
//   - switches fast walk only on a cover-key press made while moving with the player in
//     control; the original also switched on the press that takes cover from a standstill, and
//     on presses made in shops and scripted scenes
//   - starts with fast walk off and nothing held; the original read all four of its states
//     before setting them
//   - a missing or zero WALKING_SPEED means 150, and the value is kept between 100 and 300; the
//     original took a missing one as 0, which stopped the player from walking
//   - keeps the player out of cover while moving by disabling the cover key for the frame, as
//     the game's own scripts do; the original switched the player's ability to take cover off
//     and back on, which undid missions that had switched it off
//   - keeps lifting the limits right after the cover key is pressed; the original slept for a
//     tenth of a second
//   - works out the control hashes when built; the original asked the game for them every frame
//   - each of the four features can be switched off in RemoveSpeedLimits.ini
//   - new: a short tap of the sprint key while running brings the player back to a walk without
//     stopping. The original kept the run until every direction key was let go, and a keyboard
//     has no stick to ease off

#include "SpeedLogic.h"

// Functions imported from ScriptHookRDR2.dll, with the signatures encoded in the original's
// import table (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// The function imported from KERNEL32.dll (see Kernel32.def).
extern "C" __declspec(dllimport) unsigned int GetPrivateProfileIntA(const char* section, const char* key, int fallback, const char* file);

// The compiler refers to this marker in any code that uses floats and expects the C runtime to
// define it. The plugin is built without that runtime, so it is defined here.
extern "C" int _fltused = 0;

using Hash = unsigned int;

// Game natives, by hash.
const unsigned long long GET_GAME_TIMER = 0x4F67E8ECA7D3F667;                    // MISC::GET_GAME_TIMER() -> int
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;                         // PLAYER::PLAYER_ID() -> Player
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;                     // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long IS_PLAYER_CONTROL_ON = 0x7964097FCE4C244B;              // PLAYER::IS_PLAYER_CONTROL_ON(Player player) -> BOOL
const unsigned long long GET_MOUNT = 0xE7E11B8DCBED1058;                         // PED::GET_MOUNT(Ped ped) -> Ped
const unsigned long long IS_PED_SPRINTING = 0x57E457CD2C0FC168;                  // TASK::IS_PED_SPRINTING(Ped ped) -> BOOL
const unsigned long long IS_PED_IN_ANY_VEHICLE = 0x997ABD671D25CA0B;             // PED::IS_PED_IN_ANY_VEHICLE(Ped ped, BOOL atGetIn) -> BOOL
const unsigned long long IS_PED_IN_COVER = 0x60DFD0691A170B88;                   // PED::IS_PED_IN_COVER(Ped ped, BOOL p1, BOOL p2) -> BOOL
const unsigned long long IS_PED_GOING_INTO_COVER = 0x9F65DBC537E59AD5;           // PED::IS_PED_GOING_INTO_COVER(Ped ped) -> BOOL
const unsigned long long SET_PED_MIN_MOVE_BLEND_RATIO = 0x01A898D26E2333DD;      // PED::SET_PED_MIN_MOVE_BLEND_RATIO(Ped ped, float value)
const unsigned long long SET_PED_MAX_MOVE_BLEND_RATIO = 0x433083750C5E064A;      // PED::SET_PED_MAX_MOVE_BLEND_RATIO(Ped ped, float value)
const unsigned long long SET_PED_RESET_FLAG = 0xC1E8A365BF3B29F2;                // PED::SET_PED_RESET_FLAG(Ped ped, int flagId, BOOL doReset)
const unsigned long long IS_CONTROL_PRESSED = 0xF3A21BCD95725A4A;                // PAD::IS_CONTROL_PRESSED(int control, Hash action) -> BOOL
const unsigned long long IS_CONTROL_JUST_PRESSED = 0x580417101DDB492F;           // PAD::IS_CONTROL_JUST_PRESSED(int control, Hash action) -> BOOL
const unsigned long long IS_DISABLED_CONTROL_JUST_PRESSED = 0x91AEF906BCA88877;  // PAD::IS_DISABLED_CONTROL_JUST_PRESSED(int control, Hash action) -> BOOL
const unsigned long long DISABLE_CONTROL_ACTION = 0xFE99B66D079CF6BC;            // PAD::DISABLE_CONTROL_ACTION(int control, Hash action, BOOL disableRelatedActions)

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
static_assert(joaat("INPUT_COVER") == 0xDE794E3E, "joaat must reproduce the game's control hashes");

// The controls the original read, by the names it passed to the game's GET_HASH_KEY.
constexpr Hash INPUT_MOVE_UP_ONLY = joaat("INPUT_MOVE_UP_ONLY");
constexpr Hash INPUT_MOVE_DOWN_ONLY = joaat("INPUT_MOVE_DOWN_ONLY");
constexpr Hash INPUT_MOVE_LEFT_ONLY = joaat("INPUT_MOVE_LEFT_ONLY");
constexpr Hash INPUT_MOVE_RIGHT_ONLY = joaat("INPUT_MOVE_RIGHT_ONLY");
constexpr Hash INPUT_SPRINT = joaat("INPUT_SPRINT");
constexpr Hash INPUT_COVER = joaat("INPUT_COVER");
constexpr Hash INPUT_HORSE_MOVE_UP_ONLY = joaat("INPUT_HORSE_MOVE_UP_ONLY");
constexpr Hash INPUT_HORSE_SPRINT = joaat("INPUT_HORSE_SPRINT");

// Reset flags last one frame, and their names are not known (PRF_0x8A63AAF1 and PRF_0xE3667800).
// In the game's own scripts 154 exempts a ped from the speed zones around camps, towns and
// bridges, and is set on the player and on the horse the player rides; 174 is set on the player
// where the indoor limit should not apply.
const int PED_RESET_FLAG_154 = 154;
const int PED_RESET_FLAG_174 = 174;

// Settings from RemoveSpeedLimits.ini, read when the plugin loads. A switch that is missing is on.
static int walkingSpeed, tapToWalkMs;
static bool horseBoost, sprintBoost, coverWalkToggle, removeZoneSlowdown;

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

// A float argument travels as its bit pattern.
static unsigned int bits(float value) {
    unsigned int result;
    __builtin_memcpy(&result, &value, sizeof result);
    return result;
}

static bool pressed(Hash control) {
    return call(IS_CONTROL_PRESSED, 0, control);
}

// A pace limit lasts one frame; the game forgets it by the next.
static void hold(int ped, Pace pace) {
    if (pace.atLeast > 0) call(SET_PED_MIN_MOVE_BLEND_RATIO, ped, bits(pace.atLeast));
    if (pace.atMost > 0) call(SET_PED_MAX_MOVE_BLEND_RATIO, ped, bits(pace.atMost));
}

static void scriptMain() {
    FootPace foot(walkRatio(walkingSpeed), tapToWalkMs);
    for (;;) {
        const int player = call(PLAYER_ID);
        const int playerPed = call(PLAYER_PED_ID);
        if (!playerPed) {  // no game is running yet
            scriptWait(0);
            continue;
        }
        const int mount = call(GET_MOUNT, playerPed);
        // On a horse or in a wagon the same keys move that instead, so the on-foot rules wait.
        const bool onFoot = !mount && !call(IS_PED_IN_ANY_VEHICLE, playerPed, false);

        if (removeZoneSlowdown) {
            call(SET_PED_RESET_FLAG, playerPed, PED_RESET_FLAG_154, true);
            call(SET_PED_RESET_FLAG, playerPed, PED_RESET_FLAG_174, true);
            if (mount) call(SET_PED_RESET_FLAG, mount, PED_RESET_FLAG_154, true);
        }

        if (mount && horseBoost) {
            const bool galloping = call(IS_PED_SPRINTING, mount);
            hold(mount, horsePace(galloping, pressed(INPUT_HORSE_SPRINT), pressed(INPUT_HORSE_MOVE_UP_ONLY)));
        }

        const bool moving = onFoot && (pressed(INPUT_MOVE_UP_ONLY) || pressed(INPUT_MOVE_DOWN_ONLY) ||
                                       pressed(INPUT_MOVE_LEFT_ONLY) || pressed(INPUT_MOVE_RIGHT_ONLY));

        if (coverWalkToggle && onFoot) {
            const bool inCover = call(IS_PED_IN_COVER, playerPed, false, false);
            // The game reports the cover key as a disabled control while this plugin or one of
            // the game's scripts has it disabled, so both kinds of press are read. In a shop or
            // a scripted scene every control is disabled, so the player must be in control too.
            if ((call(IS_CONTROL_JUST_PRESSED, 0, INPUT_COVER) || call(IS_DISABLED_CONTROL_JUST_PRESSED, 0, INPUT_COVER)) &&
                !inCover && !call(IS_PED_GOING_INTO_COVER, playerPed) && call(IS_PLAYER_CONTROL_ON, player)) {
                foot.coverKeyPressed(moving, call(GET_GAME_TIMER));
            }
            // While moving, the cover key only switches fast walk. To take cover, stand still.
            if (moving && !inCover) call(DISABLE_CONTROL_ACTION, 0, INPUT_COVER, false);
        }

        hold(playerPed, foot.update(onFoot, moving, sprintBoost && pressed(INPUT_SPRINT), call(GET_GAME_TIMER)));
        scriptWait(0);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) {
        // The same lookup as the original: the file sits beside the plugin, in the game folder,
        // which is the current folder while plugins load. 0 stands for a missing WALKING_SPEED.
        const char* file = ".\\RemoveSpeedLimits.ini";
        walkingSpeed = GetPrivateProfileIntA("WALKING", "WALKING_SPEED", 0, file);
        tapToWalkMs = GetPrivateProfileIntA("WALKING", "TAP_TO_WALK_MS", 250, file);
        horseBoost = GetPrivateProfileIntA("FEATURES", "HORSE_BOOST", 1, file) != 0;
        sprintBoost = GetPrivateProfileIntA("FEATURES", "SPRINT_BOOST", 1, file) != 0;
        coverWalkToggle = GetPrivateProfileIntA("FEATURES", "COVER_WALK_TOGGLE", 1, file) != 0;
        removeZoneSlowdown = GetPrivateProfileIntA("FEATURES", "REMOVE_ZONE_SLOWDOWN", 1, file) != 0;
        scriptRegister(module, scriptMain);
    } else if (reason == DLL_PROCESS_DETACH) {
        scriptUnregister(module);
    }
    return 1;
}
