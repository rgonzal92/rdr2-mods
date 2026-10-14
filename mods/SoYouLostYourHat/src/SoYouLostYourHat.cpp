// SoYouLostYourHat: a ScriptHookRDR2 plugin that puts a blip on the map and minimap on the
// player's lost hat, once the first story mission is complete.
//
// Recreated from the disassembly of HatBlip2.0.asi (So You Lost Your Hat 2.0, linked
// 2021-03-06). Changes from that version:
//   - checks that the hat exists and is not being carried; the original blipped handle 0 every
//     frame while no hat was lost
//   - adds only a blip and never takes the hat over as its own mission entity, so the game's own
//     scripts can still clean it up (long_update deletes a hat left over 250 m away)
//   - removes only the blip it added itself, never another script's blip on the hat, and only
//     while the old hat still exists and still carries that blip
//   - blips a hat that is nearly at rest, or that is still moving after three seconds; the
//     original needed a speed of exactly 0
//   - keeps the blip while a spare hat is worn, until the lost hat is picked up or gone
//   - uses the game's own hat blip style (the one winter1 puts on the lost hat), with its
//     localized name, white colour, map legend entry and height marker
//   - reads the mission result as a 32-bit BOOL, drops two unused GET_ENTITY_COORDS calls, and
//     shows no blip while the player is not playing (no ped, dead, injured or arrested)
//   - stays out of winter1 while it runs, including a replay, as that mission blips the hat itself

#include "HatLogic.h"

// Functions imported from ScriptHookRDR2.dll, with the signatures encoded in the original's
// import table (see ScriptHookRDR2.def).
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

// Game natives, by hash.
const unsigned long long MISSIONDATA_WAS_COMPLETED = 0xE54DC27571D5EDC4;   // MISSIONDATA::MISSIONDATA_WAS_COMPLETED(Hash missionId) -> BOOL
const unsigned long long GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(Hash scriptHash) -> int
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;                   // PLAYER::PLAYER_ID() -> Player
const unsigned long long IS_PLAYER_PLAYING = 0xBFFB35986CAAE58C;           // PLAYER::IS_PLAYER_PLAYING(Player player) -> BOOL
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;               // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long GET_PED_LAST_DROPPED_HAT = 0x1F714E7A9DADFC42;    // PED::_GET_PED_LAST_DROPPED_HAT(Ped ped) -> Object
const unsigned long long DOES_ENTITY_EXIST = 0xD42BD6EB2E0F1677;           // ENTITY::DOES_ENTITY_EXIST(Entity entity) -> BOOL
const unsigned long long IS_ENTITY_ATTACHED = 0xEE6AD63ABF59C0B7;          // ENTITY::IS_ENTITY_ATTACHED(Entity entity) -> BOOL
const unsigned long long GET_ENTITY_SPEED = 0xFB6BA510A533DF81;            // ENTITY::GET_ENTITY_SPEED(Entity entity) -> float
const unsigned long long GET_GAME_TIMER = 0x4F67E8ECA7D3F667;              // MISC::GET_GAME_TIMER() -> int
const unsigned long long DOES_ENTITY_HAVE_BLIP = 0x9FA00E2FC134A9D0;       // MAP::_DOES_ENTITY_HAVE_BLIP(Entity entity) -> BOOL
const unsigned long long DOES_BLIP_EXIST = 0xCD82FA174080B3B1;             // MAP::DOES_BLIP_EXIST(Blip blip) -> BOOL
const unsigned long long GET_BLIP_FROM_ENTITY = 0x6D2C41A8BD6D6FD0;        // MAP::GET_BLIP_FROM_ENTITY(Entity entity) -> Blip
const unsigned long long BLIP_ADD_FOR_ENTITY = 0x23F74C2FDA6E7C61;         // MAP::BLIP_ADD_FOR_ENTITY(Hash blipHash, Entity entity) -> Blip
const unsigned long long REMOVE_BLIP = 0xF2C3C9DA47AAA54A;                 // MAP::REMOVE_BLIP(Blip* blip)

// joaat("WNT1"): the first story mission, Outlaws from the West (winter1). That mission blips
// the lost hat itself.
const unsigned int MISSION_WNT1 = 0x01CEFB5C;
// joaat("winter1"): that mission's script. While it runs, a replay included, the plugin stays out,
// because the mission adds its own hat blip without checking for one already there.
const unsigned int WINTER1_SCRIPT = 0x0E0B5E9D;
// joaat("BLIP_STYLE_PLAYER_HAT_BLIP"): the style winter1 uses for the lost hat. It brings the
// hat sprite, the localized label BLIP_HAT, white, the PICKUPS legend category and a height marker.
const unsigned int BLIP_STYLE_PLAYER_HAT_BLIP = 0x1D053A85;

// Calls a native whose arguments and result are all 32-bit values. A BOOL is read as an int.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

static float entitySpeed(int entity) {
    nativeInit(GET_ENTITY_SPEED);
    nativePush64(static_cast<unsigned int>(entity));
    return *reinterpret_cast<float*>(nativeCall());
}

static void removeBlip(int* blip) {
    nativeInit(REMOVE_BLIP);
    nativePush64(reinterpret_cast<unsigned long long>(blip));
    nativeCall();
}

// The player's last dropped hat while it lies in the world, or 0.
static int lostHat() {
    const int hat = call(GET_PED_LAST_DROPPED_HAT, call(PLAYER_PED_ID));
    return hat && call(DOES_ENTITY_EXIST, hat) && !call(IS_ENTITY_ATTACHED, hat) ? hat : 0;
}

static void scriptMain() {
    HatTracker tracker;
    int blip = 0;     // the only blip this plugin ever adds or removes
    int blipHat = 0;  // the hat that blip was added to
    for (;;) {
        const bool active = call(MISSIONDATA_WAS_COMPLETED, MISSION_WNT1) &&
                            call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, WINTER1_SCRIPT) <= 0 &&
                            call(IS_PLAYER_PLAYING, call(PLAYER_ID));
        const int hat = active ? lostHat() : 0;
        const bool ourBlipExists = blip && call(DOES_BLIP_EXIST, blip);
        const bool hatHasBlip = hat && call(DOES_ENTITY_HAVE_BLIP, hat);
        const float speed = hat ? entitySpeed(hat) : 0.0f;

        switch (tracker.update(active, hat, ourBlipExists, hatHasBlip, speed, call(GET_GAME_TIMER))) {
        case HatAction::ADD_BLIP:
            blip = call(BLIP_ADD_FOR_ENTITY, BLIP_STYLE_PLAYER_HAT_BLIP, hat);
            blipHat = hat;
            tracker.blipAdded(blip && call(DOES_BLIP_EXIST, blip));
            break;
        case HatAction::REMOVE_BLIP:
            // A deleted hat took its blip with it, and the freed handle may already belong to
            // another script's blip, so remove only while the old hat still carries ours.
            if (ourBlipExists && call(DOES_ENTITY_EXIST, blipHat) && call(GET_BLIP_FROM_ENTITY, blipHat) == blip)
                removeBlip(&blip);
            blip = 0;
            blipHat = 0;
            break;
        case HatAction::NONE:
            break;
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
