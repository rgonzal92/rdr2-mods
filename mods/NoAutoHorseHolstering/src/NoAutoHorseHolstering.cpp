// NoAutoHorseHolstering: a ScriptHookRDR2 plugin that lets the player keep a long arm in hand
// on a horse, at the mount and for the rest of the ride, instead of the game putting it away, and
// keeps an unarmed rider unarmed.
// Pistols and revolvers are left to the game.
//
// Recreated from the disassembly of NoHolstering.asi (No Auto Horse Holstering 1.1.1, built
// 2023-01-23). Changes from it:
//   - gives a long arm back only when the hand goes empty, at the mount or later in the ride, and
//     stops when the game takes it again at once. An unarmed rider is corrected at most once, in
//     the four seconds after the mount. The original put an unarmed rider's empty hand back every
//     frame of the ride, which could cancel a draw made with the mouse wheel or the weapon select
//     keys
//   - any weapon control the player holds, on the horse or on foot in the last second before
//     getting on, hands the ride back to the player, including weapon cycling and selecting; the
//     original paused for five seconds only after the wheel, aim, holster and horse-attack
//     controls, and only on the horse
//   - does nothing while player control is off (scripted scenes) or a mission is running, so it
//     cannot undo a holster a mission or scene asks for
//   - the long arm is the one in hand on the last frame on foot. The original re-drew the first
//     two long arms it saw in hand during each ride, so it also cut short the first draw of each
//     on horseback, and it never looked at a third
//   - a bow counts as a long arm; the game's own scripts test bows apart from two-handed weapons
//   - no _SET_FORCE_AUTO_EQUIP(false) every frame, which no game script calls
//   - no leftover template code: the hidden "Sell Current Weapon" prompt, the toast that never
//     showed, and the coordinates and saddle-horse lookups whose results were thrown away
//   - no C runtime; the original linked it statically and seeded its random numbers for nothing

#include "MountLogic.h"

// Functions imported from ScriptHookRDR2.dll, with the signatures encoded in the original's
// import table (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// Game natives, by hash.
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;               // PLAYER::PLAYER_ID() -> Player
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;           // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long IS_PLAYER_CONTROL_ON = 0x7964097FCE4C244B;    // PLAYER::IS_PLAYER_CONTROL_ON(Player player) -> BOOL
const unsigned long long GET_MISSION_FLAG = 0xB15CD1CF58771DE1;        // MISC::GET_MISSION_FLAG() -> BOOL
const unsigned long long GET_GAME_TIMER = 0x4F67E8ECA7D3F667;          // MISC::GET_GAME_TIMER() -> int
const unsigned long long IS_PED_ON_MOUNT = 0x460BC76A0E10655E;         // PED::IS_PED_ON_MOUNT(Ped ped) -> BOOL
const unsigned long long GET_CURRENT_PED_WEAPON = 0x3A87E44BB9A01D54;  // WEAPON::GET_CURRENT_PED_WEAPON(Ped ped, Hash* weaponHash, BOOL p2, int attachPoint, BOOL p4) -> BOOL
const unsigned long long SET_CURRENT_PED_WEAPON = 0xADF692B254977C0C;  // WEAPON::SET_CURRENT_PED_WEAPON(Ped ped, Hash weaponHash, BOOL equipNow, int attachPoint, BOOL p4, BOOL p5)
const unsigned long long IS_WEAPON_BOW = 0xC4DEC3CA8C365A5D;           // WEAPON::IS_WEAPON_BOW(Hash weaponHash) -> BOOL
const unsigned long long IS_WEAPON_A_GUN = 0x705BE297EEBDB95D;         // WEAPON::IS_WEAPON_A_GUN(Hash weaponHash) -> BOOL
const unsigned long long IS_WEAPON_TWO_HANDED = 0x0556E9D2ECF39D01;    // WEAPON::_IS_WEAPON_TWO_HANDED(Hash weaponHash) -> BOOL
const unsigned long long IS_CONTROL_PRESSED = 0xF3A21BCD95725A4A;      // PAD::IS_CONTROL_PRESSED(int control, Hash action) -> BOOL

const int ATTACH_POINT_HAND_PRIMARY = 0;

// The controls with which a rider picks, draws, holsters or uses a weapon. The original's four
// (wheel, horse aim, holster, horse attack) and the ones it missed: cycling, the select keys and
// the other attacks. Aiming or attacking on foot draws a weapon too, so those count just before
// a mount. The satchel, item and emote controls are here because using an item or an emote on the
// horse puts the weapon away too, and that holster is the player's.
constexpr Hash WEAPON_CONTROLS[] = {
    joaat("INPUT_AIM"),
    joaat("INPUT_ATTACK"),
    joaat("INPUT_MELEE_ATTACK"),
    joaat("INPUT_OPEN_WHEEL_MENU"),
    joaat("INPUT_TOGGLE_HOLSTER"),
    joaat("INPUT_HORSE_AIM"),
    joaat("INPUT_HORSE_ATTACK"),
    joaat("INPUT_HORSE_ATTACK2"),
    joaat("INPUT_HORSE_MELEE"),
    joaat("INPUT_NEXT_WEAPON"),
    joaat("INPUT_PREV_WEAPON"),
    joaat("INPUT_SELECT_NEXT_WEAPON"),
    joaat("INPUT_SELECT_PREV_WEAPON"),
    joaat("INPUT_SELECT_WEAPON_UNARMED"),
    joaat("INPUT_SELECT_WEAPON_MELEE"),
    joaat("INPUT_SELECT_WEAPON_HANDGUN"),
    joaat("INPUT_SELECT_WEAPON_SHOTGUN"),
    joaat("INPUT_SELECT_WEAPON_SNIPER"),
    joaat("INPUT_SELECT_WEAPON_AUTO_RIFLE"),
    joaat("INPUT_SELECT_WEAPON_SMG"),
    joaat("INPUT_SELECT_WEAPON_HEAVY"),
    joaat("INPUT_SELECT_WEAPON_SPECIAL"),
    joaat("INPUT_QUICK_EQUIP_ITEM"),
    joaat("INPUT_DROP_WEAPON"),
    joaat("INPUT_SELECT_ITEM_WHEEL"),
    joaat("INPUT_QUICK_USE_ITEM"),
    joaat("INPUT_OPEN_SATCHEL_MENU"),
    joaat("INPUT_OPEN_SATCHEL_HORSE_MENU"),
    joaat("INPUT_OPEN_EMOTE_WHEEL"),
};

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

// The weapon in the player's primary hand, WEAPON_UNARMED for none. The game writes the answer
// through a pointer. The arguments are the original's.
static Hash heldWeapon(int ped) {
    unsigned long long weapon = 0;
    nativeInit(GET_CURRENT_PED_WEAPON);
    nativePush64(static_cast<unsigned int>(ped));
    nativePush64(reinterpret_cast<unsigned long long>(&weapon));
    nativePush64(false);
    nativePush64(ATTACH_POINT_HAND_PRIMARY);
    nativePush64(false);
    nativeCall();
    return weapon ? static_cast<Hash>(weapon) : WEAPON_UNARMED;
}

// A rifle, repeater, shotgun or bow. The game's own scripts test bows separately next to
// _IS_WEAPON_TWO_HANDED when they look for long arms, and the gun test keeps two-handed melee
// weapons and tools such as the fishing rod out.
static bool isLongArm(Hash weapon) {
    if (weapon == WEAPON_UNARMED) return false;
    return call(IS_WEAPON_BOW, weapon) || (call(IS_WEAPON_A_GUN, weapon) && call(IS_WEAPON_TWO_HANDED, weapon));
}

static bool playerChoseWeapon() {
    for (Hash control : WEAPON_CONTROLS)
        if (call(IS_CONTROL_PRESSED, 0, control)) return true;
    return false;
}

static void scriptMain() {
    MountKeeper keeper;
    for (;;) {
        const int ped = call(PLAYER_PED_ID);
        if (ped) {
            Frame f{};
            f.mounted = call(IS_PED_ON_MOUNT, ped);
            f.held = heldWeapon(ped);
            f.heldLongArm = isLongArm(f.held);
            const bool deciding = keeper.deciding(f.mounted);
            if (!f.mounted || deciding) {
                f.playerChose = playerChoseWeapon();
                f.nowMs = call(GET_GAME_TIMER);
            }
            if (deciding)
                f.blocked = !call(IS_PLAYER_CONTROL_ON, call(PLAYER_ID)) || call(GET_MISSION_FLAG);
            if (const Hash weapon = keeper.frame(f))
                call(SET_CURRENT_PED_WEAPON, ped, weapon, true, ATTACH_POINT_HAND_PRIMARY, false, false);
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
