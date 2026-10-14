// UnofficialPatch: fixes for bugs in the game's own story scripts, for game build 1.0.1491.50,
// story mode. One plugin, one settings file (UnofficialPatch.ini), one log (UnofficialPatch.log),
// one switch per fix. The causes were traced in the audit of 2026-10-13
// (rdr2-tools/audit-2026-10-13-unofficial-patch).
//
//   1. Knife guard, "We Loved Once and True". The mission puts the fists in Arthur's hand and
//      blocks the weapon wheel, but not the mouse wheel and the number keys (mary1.c lines
//      37064-37140, 54748). The game's choke exists for bare hands only, and the script waits
//      for it without a time limit (37427, 37455). The plugin keeps Arthur unarmed around the
//      Chelonian camp while that mission runs, with the script's own call.
//
//   2. Night clothes. On waking, def_comp_brain picks a day outfit at random from a list
//      (func_198, line 5908). Twelve lists hold an entry the member cannot wear; such a pick
//      keeps the night outfit until the next schedule entry (lines 6496, 2199-2224). The plugin
//      sets the script's own "decide again" value for a member who is awake in the night outfit.
//      The script then rolls again and changes the outfit its usual way.
//
//   3. Encounter cooldowns. short_update takes a fixed 90 off one random encounter's cooldown
//      per frame (confirmed in the game by PatchProbe), so cooldowns run out faster the higher
//      the frame rate. With COOLDOWN_REFERENCE_FPS set, the plugin gives part of each 90 back
//      so that the clock runs as it would at that frame rate.
//
//   4. Dreamcatchers. The Log entry is added on every load without a test for a finished
//      collection (main.c 2661). With DREAMCATCHERS=2 the plugin removes it once finished.
//
//   5. Legendary fish map marks. A mailed fish's mark can be left at "not caught"
//      (short_update.c 25538). With FISH_MARKS=2 the plugin sets it to "caught".
//
//   6. "A New Jerusalem". The mission can leave John on the empty backup inventory
//      (beechers2_2.c 17937-17954). The plugin makes the mission's own switch back.
//
//   7. Ranch chores. Emmet Granger's clean-up clears the stall-cleaning unlock even after "The
//      Best of Women" set it (rcm_gunslinger1_2.c 15198). The plugin sets the bit again.
//
//   Watch only: logs when an ambient train carries the "suppress" flag outside every town
//   region, where medium_update never clears it (lines 8606-8657); and, with LATE_GAME_WATCH,
//   what later fixes for the cleaver and the "Advertising" kill camera would decide on.
//
// It patches no code and searches no memory. The script values it writes are "decide again"
// (only with NIGHT_CLOTHES=2) and an encounter cooldown (only with COOLDOWN_REFERENCE_FPS set);
// the game state it changes through natives is the Dreamcatchers Log entry, the fish marks and
// the inventory in use (only with mode 2 or 3); and the stall-cleaning bit of the ranch chores
// (RANCH_CHORES=2).

#include "PatchLogic.h"

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();
__declspec(dllimport) unsigned long long* getGlobalPtr(int index);

// Functions imported from KERNEL32.dll (see Kernel32.def).
extern "C" {
__declspec(dllimport) void* CreateFileA(const char* name, unsigned long access, unsigned long share, void* security,
                                        unsigned long creation, unsigned long flags, void* templateFile);
__declspec(dllimport) int WriteFile(void* file, const void* buffer, unsigned long size, unsigned long* written, void* overlapped);
__declspec(dllimport) int CloseHandle(void* handle);
__declspec(dllimport) unsigned int GetPrivateProfileIntA(const char* section, const char* key, int fallback, const char* file);
}

// The Microsoft linker expects this symbol in any program that uses floating point; the C
// runtime normally provides it, and this plugin has none.
extern "C" int _fltused = 0;

// Game natives, by hash (rdr2-tools/reference/natives.json).
const Slot GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::...(Hash scriptHash) -> int
const Slot GET_GAME_TIMER = 0x4F67E8ECA7D3F667;                    // MISC::GET_GAME_TIMER() -> int
const Slot PLAYER_PED_ID = 0x096275889B8E0EE0;                     // PLAYER::PLAYER_PED_ID() -> Ped
const Slot DOES_ENTITY_EXIST = 0xD42BD6EB2E0F1677;                 // ENTITY::DOES_ENTITY_EXIST(Entity) -> BOOL
const Slot IS_ENTITY_DEAD = 0x7D5B1F88E7504BBA;                    // ENTITY::IS_ENTITY_DEAD(Entity) -> BOOL
const Slot GET_ENTITY_COORDS = 0xA86D5F069399F44D;                 // ENTITY::GET_ENTITY_COORDS(Entity, BOOL alive, BOOL realCoords) -> Vector3
const Slot IS_PED_ON_MOUNT = 0x460BC76A0E10655E;                   // PED::IS_PED_ON_MOUNT(Ped) -> BOOL
const Slot IS_PED_IN_ANY_VEHICLE = 0x997ABD671D25CA0B;             // PED::IS_PED_IN_ANY_VEHICLE(Ped, BOOL atGetIn) -> BOOL
const Slot GET_PED_IS_GRAPPLING = 0x0E99E3BF11BB6367;              // PED::GET_PED_IS_GRAPPLING(Ped) -> BOOL
const Slot DISABLE_CONTROL_ACTION = 0xFE99B66D079CF6BC;            // PAD::DISABLE_CONTROL_ACTION(int control, Hash action, BOOL disableRelated)
const Slot GET_CURRENT_PED_WEAPON = 0x3A87E44BB9A01D54;            // WEAPON::GET_CURRENT_PED_WEAPON(Ped, Hash* weapon, BOOL p2, int attachPoint, BOOL p4) -> BOOL
const Slot SET_CURRENT_PED_WEAPON = 0xADF692B254977C0C;            // WEAPON::SET_CURRENT_PED_WEAPON(Ped, Hash weapon, BOOL equipNow, int attachPoint, BOOL p4, BOOL p5)
const Slot IS_WEAPON_HOLSTER_STATE_CHANGING = 0x2387D6E9C6B478AA;  // WEAPON::_IS_WEAPON_HOLSTER_STATE_CHANGING(Ped) -> BOOL
const Slot IS_PERSISTENT_CHARACTER_VALID = 0x800DF3FC913355F3;     // PERSCHAR::_IS_PERSISTENT_CHARACTER_VALID(PersChar) -> BOOL
const Slot PERSCHAR_IS_ACTIVE = 0xA8120EBEAF290C7A;                // PERSCHAR::_0xA8120EBEAF290C7A(PersChar) -> BOOL, def_comp_brain's test before it reads a schedule
const Slot PERSCHAR_SCHEDULE_ENTRY_TYPE = 0xEC254C2C9B0F08F1;      // PERSCHAR::_0xEC254C2C9B0F08F1(PersChar, int entry) -> Hash, joaat("SLEEP") for a sleep entry
const Slot PLAYER_ID = 0x217E9DC48139933D;                         // PLAYER::PLAYER_ID() -> Player
const Slot GET_SYSTEM_TIME = 0xBE7F225417E35A7C;                   // MISC::GET_SYSTEM_TIME() -> int, not slowed by the time scale
const Slot GET_MISSION_FLAG = 0xB15CD1CF58771DE1;                  // MISC::GET_MISSION_FLAG() -> BOOL
const Slot IS_PAUSE_MENU_ACTIVE = 0x535384D6067BA42E;              // HUD::IS_PAUSE_MENU_ACTIVE() -> BOOL
const Slot IS_LOADING_SCREEN_VISIBLE = 0xB54ADBE65D528FCB;         // SCRIPT::IS_LOADING_SCREEN_VISIBLE() -> BOOL
const Slot IS_SCREEN_FADING_OUT = 0x02F39BEFE7B88D00;              // CAM::IS_SCREEN_FADING_OUT() -> BOOL
const Slot IS_SCREEN_FADED_OUT = 0xF5472C80DF2FF847;               // CAM::IS_SCREEN_FADED_OUT() -> BOOL
const Slot IS_DEATH_FAIL_CAMERA_RUNNING = 0x139EFB0A71DD9011;      // CAM::IS_DEATH_FAIL_CAMERA_RUNNING() -> BOOL
const Slot IS_PLAYER_DEAD = 0x2E9C3FCB6798F397;                    // PLAYER::IS_PLAYER_DEAD(Player) -> BOOL
const Slot IS_PLAYER_CONTROL_ON = 0x7964097FCE4C244B;              // PLAYER::IS_PLAYER_CONTROL_ON(Player) -> BOOL
const Slot GET_RENDERING_CAM = 0x03A8931ECC8015D6;                 // CAM::GET_RENDERING_CAM() -> Cam, -1 for the game camera
const Slot DOES_CAM_EXIST = 0x153AD457764FD704;                    // CAM::DOES_CAM_EXIST(Cam) -> BOOL
const Slot GET_CAM_COORD = 0x6B12F11C2A9F0344;                     // CAM::GET_CAM_COORD(Cam) -> Vector3
const Slot UILOG_IS_ENTRY_REGISTERED = 0xB8188CCF52202475;         // UILOG::_UILOG_IS_ENTRY_REGISTERED(int type, Hash entry) -> BOOL
const Slot UILOG_REMOVE_ENTRY = 0xD594A19BE09A75C6;                // UILOG::_UILOG_REMOVE_ENTRY(int type, Hash entry)
const Slot COLLECTABLE_GET_ITEM_HASH = 0x126CBEBBA46693CF;         // COLLECTABLE::_COLLECTABLE_GET_COLLECTABLE_ITEM_HASH(int index, Hash category, Hash subcategory) -> Hash
const Slot COLLECTABLE_GET_NUM_TURNED_IN = 0x9A03F22AD446EEAC;     // COLLECTABLE::_COLLECTABLE_GET_NUM_TURNED_IN(Hash) -> int
const Slot COLLECTABLE_GET_NUM_FOUND = 0xF83D3DDA4D3C8169;         // COLLECTABLE::_COLLECTABLE_GET_NUM_FOUND(Hash) -> int
const Slot MAP_IS_DISCOVERY_ACTIVE = 0x3F81EA4275D39D6F;           // MAP::_MAP_IS_DISCOVERY_ACTIVE(Hash) -> BOOL
const Slot MAP_DISCOVERY_SET_ENABLED = 0xDA98246C7A3C2189;         // MAP::_MAP_DISCOVERY_SET_ENABLED(Hash): despite the name, hides the mark
const Slot MAP_DISCOVER_REGION = 0xD8C7162AB2E2AF45;               // MAP::_MAP_DISCOVER_REGION(Hash): shows the mark
const Slot INVENTORY_IS_USING_BACKUP_INVENTORY = 0x7C7E4AB748EA3B07;  // INVENTORY::_INVENTORY_IS_USING_BACKUP_INVENTORY() -> BOOL
const Slot INVENTORY_USE_BACKUP_INVENTORY = 0xE36D4A38D28D9CFB;    // INVENTORY::_INVENTORY_USE_BACKUP_INVENTORY(BOOL)
const Slot INVENTORY_GET_ITEM_COUNT = 0xE787F05DFC977BDE;          // INVENTORY::_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID(int inventory, Hash item, BOOL) -> int
const Slot UNLOCK_IS_UNLOCKED = 0xC4B660C7B6040E75;                // UNLOCK::UNLOCK_IS_UNLOCKED(Hash) -> BOOL
const Slot GET_PED_AMMO_BY_TYPE = 0x39D22031557946C1;              // WEAPON::GET_PED_AMMO_BY_TYPE(Ped, Hash ammo) -> int
const Slot HAS_PED_GOT_WEAPON = 0x8DECB02F88F428BC;                // WEAPON::HAS_PED_GOT_WEAPON(Ped, Hash, int, BOOL) -> BOOL
const Slot GET_WEAPON_UNLOCK = 0x865F36299079FB75;                 // WEAPON::_GET_WEAPON_UNLOCK(Hash weapon) -> Hash

const Hash INPUT_NEXT_WEAPON = joaat("INPUT_NEXT_WEAPON"), INPUT_PREV_WEAPON = joaat("INPUT_PREV_WEAPON");
const Hash WEAPON_UNARMED = joaat("WEAPON_UNARMED");

// An argument travels as 64 bits: a 32-bit value, or the address of a slot the native writes to.
static Slot arg(int value) { return static_cast<unsigned int>(value); }
static Slot arg(Hash value) { return value; }
static Slot arg(Slot* out) { return reinterpret_cast<Slot>(out); }

// Calls a native whose result is a 32-bit value, or none.
template <typename... Args>
static int call(Slot native, Args... args) {
    nativeInit(native);
    (nativePush64(arg(args)), ...);
    return static_cast<int>(*nativeCall());
}

// Appends one line to UnofficialPatch.log in the game folder, the current folder of the game.
static void log(const Line& line) {
    const unsigned long FILE_APPEND_DATA = 4, FILE_SHARE_READ = 1, OPEN_ALWAYS = 4, FILE_ATTRIBUTE_NORMAL = 0x80;
    void* const file = CreateFileA(".\\UnofficialPatch.log", FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == reinterpret_cast<void*>(-1LL)) return;
    unsigned long written;
    WriteFile(file, line.text, static_cast<unsigned long>(line.length), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

static int threads(const char* script) { return call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, joaat(script)); }

// ---------------------------------------------------------------------------------------------
// Fix 1, the knife guard.

// A Vector3 result is three 8-byte slots with the float in the low half of each.
static float coordinate(const Slot* vector, int axis) { return *reinterpret_cast<const float*>(vector + axis); }

// True while "We Loved Once and True" runs and Arthur is alive and on foot around the camp.
static bool inScene(int ped) {
    if (threads("mary1") <= 0) return false;
    if (!call(DOES_ENTITY_EXIST, ped) || call(IS_ENTITY_DEAD, ped)) return false;
    if (call(IS_PED_ON_MOUNT, ped) || call(IS_PED_IN_ANY_VEHICLE, ped, 0)) return false;
    nativeInit(GET_ENTITY_COORDS);
    nativePush64(arg(ped));
    nativePush64(1);
    nativePush64(0);
    const Slot* const position = nativeCall();
    return nearScene(coordinate(position, 0), coordinate(position, 1), coordinate(position, 2));
}

// Runs every frame. Outside the scene it looks four times a second and does nothing else.
static void knifeGuard(int now) {
    static UnarmedKeeper keeper;
    static bool active = false;
    static int linesLeft = 60;  // put-away lines per game start; a player can draw without end

    static int lastLook = 0;

    if (!active && now >= lastLook && now - lastLook < 250) return;
    lastLook = now;
    const int ped = call(PLAYER_PED_ID);
    if (!inScene(ped)) {
        if (active) log(Line() << "knife guard " << now << " ms: left the scene");
        active = false;
        keeper.leave();
        return;
    }
    if (!active) log(Line() << "knife guard " << now << " ms: active, \"We Loved Once and True\" runs and Arthur is on foot near the camp");
    active = true;

    // The mouse wheel. 36 of the game's scripts disable these two next to the weapon wheel;
    // mary1 does not.
    call(DISABLE_CONTROL_ACTION, 0, INPUT_NEXT_WEAPON, 0);
    call(DISABLE_CONTROL_ACTION, 0, INPUT_PREV_WEAPON, 0);

    Slot weapon = 0;
    const bool armed = call(GET_CURRENT_PED_WEAPON, ped, &weapon, 1, 0, 0) != 0 && static_cast<Hash>(weapon) != 0 &&
                       static_cast<Hash>(weapon) != WEAPON_UNARMED;
    const bool busy = armed && (call(GET_PED_IS_GRAPPLING, ped) || call(IS_WEAPON_HOLSTER_STATE_CHANGING, ped));
    switch (keeper.frame(now, armed, busy)) {
        case UnarmedKeeper::PUT_AWAY:
            // The script's own call from mary1.c line 54748.
            call(SET_CURRENT_PED_WEAPON, ped, WEAPON_UNARMED, 1, 0, 0, 0);
            if (linesLeft > 0 && --linesLeft >= 0)
                log(Line() << "knife guard " << now << " ms: put weapon " << Hex{static_cast<Hash>(weapon)} << " away, attempt " << keeper.attempts
                           << (linesLeft == 0 ? ". No more of these lines until the game restarts" : ""));
            break;
        case UnarmedKeeper::GIVE_UP:
            log(Line() << "knife guard " << now << " ms: weapon " << Hex{static_cast<Hash>(weapon)} << " is still in hand after "
                       << UnarmedKeeper::MAX_ATTEMPTS << " attempts, stopping until Arthur leaves the scene. Put it away yourself before pressing Throttle");
            break;
        case UnarmedKeeper::NOTHING:
            break;
    }
}

// ---------------------------------------------------------------------------------------------
// Fix 2, night clothes.

const char* const MEMBER_NAMES[MEMBERS] = {
    "Dutch", "John", "Javier", "Bill", "Uncle", "Hosea", "Micah", "Charles", "Sean", "Lenny", "Kieran", "Sadie", "the dog", "Abigail",
    "Jack", "Mary-Beth", "Molly", "Pearson", "Strauss", "Grimshaw", "Karen", "Swanson", "Tilly", "Trelawny", "Micah's man 1",
    "Micah's man 2", "Eagle Flies",
};

// The low 32 bits of a script global as a signed number, or `missing` when ScriptHook has no
// address for it.
static int global(int index, int missing = -999999) {
    const Slot* const cell = getGlobalPtr(index);
    return cell ? static_cast<int>(*cell) : missing;
}

// Runs every five seconds. `mode` 1 only writes to the log what it would do, 2 also does it.
static void nightClothes(int now, int mode) {
    static AskPacer pacers[MEMBERS];
    static bool layoutChecked = false, layoutAsExpected = false;

    // The globals exist once the story scripts run.
    if (threads("main") <= 0 || threads("def_comp_brain") <= 0) return;
    if (!layoutChecked) {
        layoutChecked = true;
        const int members = global(MEMBER_TABLE), outfits = global(OUTFIT_TABLE);
        layoutAsExpected = members == MEMBERS && outfits == MEMBERS;
        log(Line() << "night clothes " << now << " ms: member table holds " << members << " and outfit table holds " << outfits
                   << ", expected " << MEMBERS << " each" << (layoutAsExpected ? "" : ": DIFFERS, this fix stays off"));
    }
    if (!layoutAsExpected || !campDecidesOutfits(global(CAMP))) return;

    for (int member = 0; member < MEMBERS; ++member) {
        MemberState state = {};
        state.member = member;
        state.wanted = global(outfitGlobal(member, OUTFIT_WANTED));
        int entryType = 0;
        if (isNightOutfit(member, state.wanted)) {
            // The same reads as def_comp_brain's sleep test (func_194, line 5807).
            const int persChar = global(memberGlobal(member, MEMBER_PERSCHAR));
            state.locked = global(outfitGlobal(member, OUTFIT_LOCK)) != 0;
            state.pending = global(memberGlobal(member, MEMBER_DECIDE_AGAIN)) != 0;
            state.simulated = call(IS_PERSISTENT_CHARACTER_VALID, persChar) && call(PERSCHAR_IS_ACTIVE, persChar);
            if (state.simulated) {
                entryType = call(PERSCHAR_SCHEDULE_ENTRY_TYPE, persChar, global(memberGlobal(member, MEMBER_SCHEDULE_ENTRY)));
                state.asleep = static_cast<Hash>(entryType) == joaat("SLEEP");
            }
        }
        AskPacer& pacer = pacers[member];
        const int asksBefore = pacer.asks;
        switch (pacer.check(now, classify(state))) {
            case AskPacer::ASK:
                if (mode == 2) {
                    if (Slot* const decideAgain = getGlobalPtr(memberGlobal(member, MEMBER_DECIDE_AGAIN))) *decideAgain = 1;
                }
                log(Line() << "night clothes " << now << " ms: " << MEMBER_NAMES[member] << " (" << member << ") is awake in night outfit "
                           << Hex{static_cast<Hash>(state.wanted)} << ", schedule entry type " << Hex{static_cast<Hash>(entryType)}
                           << (mode == 2 ? ": asked the script to decide again, request " : ": would ask the script to decide again, sighting ")
                           << pacer.asks);
                break;
            case AskPacer::CLEARED:
                log(Line() << "night clothes " << now << " ms: " << MEMBER_NAMES[member] << " (" << member << ") is no longer awake in a night outfit after "
                           << asksBefore << (mode == 2 ? " requests" : " sightings") << ", wanted outfit now " << Hex{static_cast<Hash>(state.wanted)});
                break;
            case AskPacer::NOTHING:
                break;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Fix 3, encounter cooldowns. Runs every frame.
static void encounterCooldowns(int now, CooldownClock& clock) {
    static bool layoutChecked = false, layoutAsExpected = false;
    static int lastReport = 0;

    if (threads("short_update") <= 0 || threads("long_update") <= 0) {
        clock.primed = false;
        return;
    }
    if (!layoutChecked) {
        layoutChecked = layoutAsExpected = true;
        for (const LayoutCell& cell : BEAT_LAYOUT) layoutAsExpected = layoutAsExpected && global(cell.global) == cell.expected;
        log(Line() << "cooldowns " << now << " ms: running as at " << clock.referenceFps << " fps"
                   << (layoutAsExpected ? "" : ". The encounter table is not where expected: this fix stays off"));
        lastReport = now;
    }
    if (!layoutAsExpected) return;
    if (global(STORY_MARKER) != -1) {
        clock.primed = false;
        return;
    }
    clock.tick(
        now, global(BEAT_TICK_INDEX, -1), [](int beat) { return global(beatCooldownGlobal(beat), 0); },
        [](int beat, int value) {
            // A script integer sits zero-extended in its 8-byte cell.
            Slot* const cell = getGlobalPtr(beatCooldownGlobal(beat));
            if (!cell) return value - 1;
            *cell = static_cast<unsigned int>(value);
            return static_cast<int>(*cell);
        });
    if (clock.failedWrites > 0) {
        log(Line() << "cooldowns " << now << " ms: a value did not read back as written. This fix is off until the game restarts");
        layoutAsExpected = false;
        return;
    }
    if (now < lastReport || now - lastReport >= 600000) {
        if (clock.corrections > 0)
            log(Line() << "cooldowns " << now << " ms: " << clock.beatsOnCooldown() << " encounters on cooldown; in the last ten minutes the game took "
                       << clock.taken << " units in " << clock.corrections << " steps and the plugin gave " << clock.givenBack << " back");
        clock.clearCounters();
        lastReport = now;
    }
}

// ---------------------------------------------------------------------------------------------
// Watch only, trains. Runs every five seconds and writes nothing but log lines.
static void trainWatch(int now) {
    static int stuckSince[AMBIENT_TRAINS];  // game timer, 0 for "not stuck"

    if (threads("medium_update") <= 0 || global(TRAIN_TABLE) != TRAIN_COUNT) return;
    const int town = global(TOWN_REGION);
    for (int train = 0; train < AMBIENT_TRAINS; ++train) {
        const int flags = global(trainFlagsGlobal(train));
        if (trainStuck(flags, town)) {
            if (stuckSince[train] == 0) {
                stuckSince[train] = now ? now : 1;
                log(Line() << "trains " << now << " ms: train " << train << " is suppressed outside every town region, flags " << Hex{static_cast<Hash>(flags)});
            }
        } else if (stuckSince[train] != 0) {
            log(Line() << "trains " << now << " ms: train " << train << " is no longer suppressed outside a town after " << (now - stuckSince[train]) / 1000
                       << " s, flags " << Hex{static_cast<Hash>(flags)} << ", town region " << town);
            stuckSince[train] = 0;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Fix 4, the Dreamcatchers entry in the Log. Runs every two seconds. `mode` 1 only writes to the
// log what it would do, 2 also removes the entry. Nothing is saved: the next load adds it again.
static void dreamcatchers(int now, int mode) {
    static StreakGate held{60, 0};  // two minutes of unpaused play, so "hold for details" still works
    static Watched<4> seen;

    if (threads("main") <= 0) return;
    const int length = global(DREAM_TABLE), found = global(DREAM_FOUND), arrowhead = global(DREAM_ARROWHEAD);
    const int registered = call(UILOG_IS_ENTRY_REGISTERED, DREAM_LOG_TYPE, DISCO_DREAM) != 0;
    const int values[4] = {length, found, arrowhead, registered};
    if (seen.changed(values))
        log(Line() << "dreamcatchers " << now << " ms: table length " << length << " (expected " << DREAM_TABLE_LENGTH << "), found "
                   << bitsSet(found & DREAM_ALL) << " of 20 (" << Hex{static_cast<Hash>(found)} << "), arrowhead " << ((arrowhead & 2) ? "taken" : "not taken")
                   << ", Log entry " << (registered ? "present" : "absent"));
    if (call(IS_PAUSE_MENU_ACTIVE)) return;
    if (!held.step(length == DREAM_TABLE_LENGTH && registered && dreamcatchersDone(found, arrowhead))) return;
    if (mode == 2) call(UILOG_REMOVE_ENTRY, DREAM_LOG_TYPE, DISCO_DREAM);
    log(Line() << "dreamcatchers " << now << " ms: collection finished, " << (mode == 2 ? "removed" : "would remove") << " the Log entry");
}

// ---------------------------------------------------------------------------------------------
// Fix 5, legendary fish map marks. Runs every ten seconds; a fish is repaired when two readings
// in a row agree. `mode` 1 only writes to the log, 2 also sets the marks.
static void fishMarks(int now, int mode) {
    static int catalog = 0;  // 0 not checked, 1 as expected, -1 differs: stay off
    static StreakGate wrong[FISH_COUNT];
    static Watched<4> seen[FISH_COUNT];

    if (threads("main") <= 0 || catalog < 0) return;
    if (catalog == 0) {
        Hash listed[14];
        for (int i = 0; i < 14; ++i) listed[i] = static_cast<Hash>(call(COLLECTABLE_GET_ITEM_HASH, i, LEGENDARY_FISH, 0));
        catalog = fishCatalogMatches(listed, 14) ? 1 : -1;
        const Hash pike = FISH_COLLECTABLES[13];
        log(Line() << "fish marks " << now << " ms: the game lists " << (catalog > 0 ? "the 14 expected legendary fish" : "OTHER legendary fish, this fix stays off")
                   << "; Northern Pike (not covered) mailed " << call(COLLECTABLE_GET_NUM_TURNED_IN, pike) << ", found " << call(COLLECTABLE_GET_NUM_FOUND, pike));
        if (catalog < 0) return;
    }
    for (int f = 0; f < FISH_COUNT; ++f) {
        const FishMark& fish = FISH[f];
        const int turnedIn = call(COLLECTABLE_GET_NUM_TURNED_IN, fish.fish), spotFound = call(COLLECTABLE_GET_NUM_FOUND, fish.spot);
        const int uncaught = call(MAP_IS_DISCOVERY_ACTIVE, fish.uncaught) != 0, caught = call(MAP_IS_DISCOVERY_ACTIVE, fish.caught) != 0;
        const int values[4] = {turnedIn, spotFound, uncaught, caught};
        if (seen[f].changed(values))
            log(Line() << "fish marks " << now << " ms: fish " << f + 1 << " mailed " << turnedIn << ", spot found " << spotFound << ", mark \"not caught\" "
                       << (uncaught ? "on" : "off") << ", mark \"caught\" " << (caught ? "on" : "off"));
        if (wrong[f].needed != 2) wrong[f].needed = 2;
        if (!wrong[f].step(fishMarkWrong(turnedIn, spotFound, uncaught, caught))) continue;
        if (mode == 2) {
            // The game's own order after a mailing.
            call(MAP_DISCOVERY_SET_ENABLED, fish.uncaught);
            call(MAP_DISCOVER_REGION, fish.caught);
        }
        log(Line() << "fish marks " << now << " ms: fish " << f + 1 << " is mailed but its map mark is wrong: "
                   << (mode == 2 ? "set it to caught" : "would set it to caught"));
    }
}

// ---------------------------------------------------------------------------------------------
// Fix 6, "A New Jerusalem", and fix 7, the ranch chores. Run every second; log lines only on a
// change. `late` holds what must start again with each load: ScriptHook restarts scriptMain on a
// load, and its locals with it.
struct LateGame {
    bool ownerSeen = false;  // a script that switches the backup inventory ran since the load
    StreakGate jerusalemHeld{10, 0};
    int jerusalemTries = 0;
    StreakGate choresHeld{3, 0};
};

static int weaponsIn(int inventory) {
    int count = 0;
    for (Hash item : WEAPON_ITEMS) count += call(INVENTORY_GET_ITEM_COUNT, inventory, item, 0) > 0;
    return count;
}

static void jerusalem(int now, int mode, LateGame& late, int mission) {
    static Watched<8> seen;
    static int lastAction = -1;

    const int owners[4] = {threads("beechers2_2"), threads("bathing"), threads("odriscolls3"), threads("pause_menu_replay_launcher")};
    JerusalemState state = {};
    state.ownerRunning = owners[0] > 0 || owners[1] > 0 || owners[2] > 0 || owners[3] > 0;
    if (owners[0] > 0 || owners[1] > 0 || owners[2] > 0) late.ownerSeen = true;
    state.ownerSeen = late.ownerSeen;
    const int replay = (global(REPLAY_FLAGS) & 0x8000) != 0;
    state.freeRoam = !mission && !replay;
    state.onBackup = call(INVENTORY_IS_USING_BACKUP_INVENTORY) != 0;
    state.epilogueWeapons = call(UNLOCK_IS_UNLOCKED, SP_ENDLESS_SUMMER_WEAPONS) != 0;
    // Counting 106 items in two inventories is only needed when the rest already says "stuck".
    const bool suspect = state.freeRoam && !state.ownerRunning && state.onBackup && state.epilogueWeapons;
    state.weaponsInMain = suspect ? weaponsIn(MAIN_INVENTORY) : -1;
    const int values[8] = {state.onBackup, state.epilogueWeapons, mission, replay, owners[0], owners[1], owners[2], owners[3]};
    if (seen.changed(values))
        log(Line() << "jerusalem " << now << " ms: backup inventory " << state.onBackup << ", epilogue weapons unlocked " << state.epilogueWeapons << ", mission "
                   << mission << ", replay " << replay << "; scripts beechers2_2 " << owners[0] << ", bathing " << owners[1] << ", odriscolls3 " << owners[2]
                   << ", replay launcher " << owners[3]);

    const JerusalemAction action = jerusalemVerdict(state, mode);
    if (!late.jerusalemHeld.step(action != LEAVE)) {
        if (action == LEAVE) lastAction = -1;
        return;
    }
    late.jerusalemHeld.streak = 0;  // ten more seconds before the next step
    const int weaponsInBackup = weaponsIn(BACKUP_INVENTORY);
    Line line;
    line << "jerusalem " << now << " ms: on the backup inventory after the epilogue hand-over, outside any mission; weapons in inventory 1: "
         << state.weaponsInMain << ", in the backup inventory: " << weaponsInBackup << ". ";
    if (action == SWITCH && late.jerusalemTries < 3) {
        ++late.jerusalemTries;
        call(INVENTORY_USE_BACKUP_INVENTORY, 0);  // the mission's own clean-up, beechers2_2.c 17951
        log(line << "Switched back to inventory 1 (attempt " << late.jerusalemTries << "), now on the backup inventory: "
                 << (call(INVENTORY_IS_USING_BACKUP_INVENTORY) != 0) << (weaponsInBackup ? ". What was gathered while stuck stays behind" : ""));
        return;
    }
    if (action == lastAction) return;  // say the rest once
    lastAction = action;
    if (action == SWITCH) log(line << "Three switches did not hold; stopping until the next load");
    if (action == WOULD_SWITCH) log(line << "Would switch back (JERUSALEM=1 only logs)");
    if (action == MAIN_EMPTY) log(line << "Inventory 1 holds no weapons, so switching would not help; leaving it");
    if (action == ASK_FIRST)
        log(line << "It was like this when the save was loaded, so things gathered since are on the backup inventory and would stay behind. Set JERUSALEM=3 and restart to switch anyway");
}

static void ranchChores(int now, int mode, LateGame& late, int mission) {
    static Watched<5> seen;

    const int camp = global(CAMP), initialised = global(40), beechers = threads("camp_beechershope");
    const bool missionScripts = threads("rcm_gunslinger1_2") > 0 || threads("rcm_abigail22") > 0 || threads("marston5_1") > 0 || threads("marston5_2") > 0 ||
                                threads("abigail2_1") > 0;
    Slot* const cell = getGlobalPtr(RANCH_CHORES);
    const int value = cell ? static_cast<int>(*cell) : -1;
    const int values[5] = {value, camp, initialised, beechers, missionScripts};
    if (seen.changed(values))
        log(Line() << "ranch chores " << now << " ms: value " << Hex{static_cast<Hash>(value)} << ", camp " << camp << " (Beecher's Hope is " << CAMP_BEECHERS_HOPE
                   << "), initialised " << initialised << ", camp_beechershope " << beechers << ", a ranch mission script running " << missionScripts);
    const bool guarded = cell && beechers > 0 && camp == CAMP_BEECHERS_HOPE && initialised == 1 && !missionScripts && !mission;
    if (!late.choresHeld.step(guarded && choresNeedStall(value))) return;
    late.choresHeld.streak = 0;
    if (mode != 2) {
        log(Line() << "ranch chores " << now << " ms: stall cleaning is locked after \"The Best of Women\" (value " << Hex{static_cast<Hash>(value)}
                   << "): would unlock it (RANCH_CHORES=1 only logs)");
        return;
    }
    // Read and write in the same tick, so a bit the story set meanwhile is kept.
    const int before = static_cast<int>(*cell);
    if (!choresNeedStall(before)) return;
    *cell = static_cast<unsigned int>(before | 0x20);
    log(Line() << "ranch chores " << now << " ms: stall cleaning was locked after \"The Best of Women\": unlocked it, value " << Hex{static_cast<Hash>(before)}
               << " -> " << Hex{static_cast<Hash>(static_cast<int>(*cell))} << ". The chore comes back at the next camp start or the next day");
}

// Watch only: the cleaver lost after "Jim Milton Rides, Again?".
static void cleaverWatch(int now) {
    static Watched<5> seen;
    const int ped = call(PLAYER_PED_ID);
    const Hash unlock = static_cast<Hash>(call(GET_WEAPON_UNLOCK, WEAPON_MELEE_CLEAVER));
    const int c[5] = {call(GET_PED_AMMO_BY_TYPE, ped, AMMO_HATCHET_CLEAVER), call(HAS_PED_GOT_WEAPON, ped, WEAPON_MELEE_CLEAVER, 0, 0) != 0,
                      static_cast<int>(unlock), unlock ? call(UNLOCK_IS_UNLOCKED, unlock) != 0 : -1, call(INVENTORY_IS_USING_BACKUP_INVENTORY) != 0};
    if (seen.changed(c))
        log(Line() << "cleaver " << now << " ms: cleaver ammo " << c[0] << ", has cleaver " << c[1] << ", its unlock " << Hex{unlock} << " is "
                   << (c[3] < 0 ? "none" : c[3] ? "set" : "not set") << ", backup inventory " << c[4]);
}

static void lateGame(int now, int jerusalemMode, int choresMode, bool watchOn, LateGame& late) {
    if (threads("main") <= 0) return;
    const int mission = call(GET_MISSION_FLAG) != 0;
    if (jerusalemMode) jerusalem(now, jerusalemMode, late, mission);
    if (choresMode) ranchChores(now, choresMode, late, mission);
    if (watchOn) cleaverWatch(now);
}

// Watch only, "Advertising, the New American Art". Every frame while braithwaites1 runs, else
// once a second. Logs every kill-camera episode and "would repair" after 3 s.
static void killcamWatch(int now) {
    static KillcamWatch watch;
    static int running = 0, lastLook = 0, farthestCm = 0;

    if (running <= 0 && now >= lastLook && now - lastLook < 1000) return;
    lastLook = now;
    const int count = threads("braithwaites1");
    if ((count > 0) != (running > 0)) log(Line() << "killcam " << now << " ms: braithwaites1 " << (count > 0 ? "started" : "ended") << ", thread value " << count);
    running = count;
    if (count <= 0) {
        watch = KillcamWatch();
        return;
    }
    const int system = call(GET_SYSTEM_TIME);
    const int player = call(PLAYER_ID);
    const int cam = call(GET_RENDERING_CAM);
    bool shown = false;
    int distanceCm = -1;
    if (cam != -1 && call(DOES_CAM_EXIST, cam) && !call(IS_PLAYER_CONTROL_ON, player)) {
        nativeInit(GET_CAM_COORD);
        nativePush64(arg(cam));
        const Slot* const position = nativeCall();
        const float x = coordinate(position, 0), y = coordinate(position, 1), z = coordinate(position, 2);
        shown = nearKillcam(x, y, z);
        const float dx = x - KILLCAM_X, dy = y - KILLCAM_Y, dz = z - KILLCAM_Z;
        float d = dx * dx + dy * dy + dz * dz, root = d > 1 ? d : 1;  // a few Newton steps are enough for a log line
        for (int i = 0; i < 20; ++i) root = (root + d / root) / 2;
        distanceCm = static_cast<int>(root * 100);
    }
    const bool guarded = call(IS_PAUSE_MENU_ACTIVE) || call(IS_LOADING_SCREEN_VISIBLE) || call(IS_PLAYER_DEAD, player) || call(IS_SCREEN_FADING_OUT) ||
                         call(IS_SCREEN_FADED_OUT) || call(IS_DEATH_FAIL_CAMERA_RUNNING);
    switch (watch.frame(system, shown, guarded)) {
        case KillcamWatch::STARTED:
            farthestCm = distanceCm;
            log(Line() << "killcam " << now << " ms: kill camera on, camera " << cam << " at " << distanceCm << " cm from the expected spot, player control off");
            break;
        case KillcamWatch::STUCK:
            log(Line() << "killcam " << now << " ms: WOULD REPAIR, the kill camera has held " << watch.shownMs << " ms of unpaused time (paused " << watch.guardedMs << " ms)");
            break;
        case KillcamWatch::ENDED:
            log(Line() << "killcam " << now << " ms: kill camera off after " << watch.shownMs << " ms (paused " << watch.guardedMs << " ms), farthest " << farthestCm << " cm");
            break;
        case KillcamWatch::NOTHING:
            if (shown && distanceCm > farthestCm) farthestCm = distanceCm;
            break;
    }
}

static void scriptMain() {
    const char* const SETTINGS = ".\\UnofficialPatch.ini";
    const bool knifeGuardOn = GetPrivateProfileIntA("FIXES", "KNIFE_GUARD", 1, SETTINGS) != 0;
    const int nightClothesMode = static_cast<int>(GetPrivateProfileIntA("FIXES", "NIGHT_CLOTHES", 1, SETTINGS));
    const int referenceSetting = static_cast<int>(GetPrivateProfileIntA("FIXES", "COOLDOWN_REFERENCE_FPS", 0, SETTINGS));
    const bool trainWatchOn = GetPrivateProfileIntA("FIXES", "TRAIN_WATCH", 1, SETTINGS) == 1;
    const int dreamcatchersMode = validMode(static_cast<int>(GetPrivateProfileIntA("FIXES", "DREAMCATCHERS", 1, SETTINGS)));
    const int fishMarksMode = validMode(static_cast<int>(GetPrivateProfileIntA("FIXES", "FISH_MARKS", 1, SETTINGS)));
    const bool lateWatchOn = GetPrivateProfileIntA("FIXES", "LATE_GAME_WATCH", 1, SETTINGS) == 1;
    const unsigned int jerusalemSetting = GetPrivateProfileIntA("FIXES", "JERUSALEM", 2, SETTINGS);
    const int jerusalemMode = jerusalemSetting <= 3 ? static_cast<int>(jerusalemSetting) : 1;
    const int choresMode = validMode(static_cast<int>(GetPrivateProfileIntA("FIXES", "RANCH_CHORES", 2, SETTINGS)));
    LateGame late;
    CooldownClock cooldowns;
    cooldowns.referenceFps = validReferenceFps(referenceSetting);
    cooldowns.restart();
    cooldowns.clearCounters();
    log(Line() << "UnofficialPatch started (build 1.0.1491.50 expected): knife guard " << (knifeGuardOn ? "on" : "off") << ", night clothes "
               << (nightClothesMode == 2 ? "on" : nightClothesMode == 1 ? "log only" : "off") << ", encounter cooldowns as at "
               << cooldowns.referenceFps << " fps (0 is off)" << (referenceSetting != cooldowns.referenceFps ? ", the setting was not 0 or 20 to 1000" : "")
               << ", train watch " << (trainWatchOn ? "on" : "off"));
    const char* const MODES[3] = {"off", "log only", "on"};
    log(Line() << "UnofficialPatch late game: dreamcatchers " << MODES[dreamcatchersMode] << ", fish marks " << MODES[fishMarksMode] << ", A New Jerusalem "
               << (jerusalemMode == 3 ? "on, also for a save stuck when loaded" : MODES[jerusalemMode]) << ", ranch chores " << MODES[choresMode]
               << ", late-game watch " << (lateWatchOn ? "on" : "off"));

    int lastNightClothes = 0, lastTrainWatch = 0, lastDreamcatchers = 0, lastFishMarks = 0, lastLateWatch = 0;
    for (;;) {
        scriptWait(0);
        const int now = call(GET_GAME_TIMER);
        if (knifeGuardOn) knifeGuard(now);
        if ((nightClothesMode == 1 || nightClothesMode == 2) && (now < lastNightClothes || now - lastNightClothes >= AskPacer::QUICK_MS)) {
            lastNightClothes = now;
            nightClothes(now, nightClothesMode);
        }
        if (cooldowns.referenceFps > 0) encounterCooldowns(now, cooldowns);
        if (trainWatchOn && (now < lastTrainWatch || now - lastTrainWatch >= 5000)) {
            lastTrainWatch = now;
            trainWatch(now);
        }
        if (dreamcatchersMode && (now < lastDreamcatchers || now - lastDreamcatchers >= 2000)) {
            lastDreamcatchers = now;
            dreamcatchers(now, dreamcatchersMode);
        }
        if (fishMarksMode && (now < lastFishMarks || now - lastFishMarks >= 10000)) {
            lastFishMarks = now;
            fishMarks(now, fishMarksMode);
        }
        if (lateWatchOn) killcamWatch(now);
        if ((lateWatchOn || jerusalemMode || choresMode) && (now < lastLateWatch || now - lastLateWatch >= 1000)) {
            lastLateWatch = now;
            lateGame(now, jerusalemMode, choresMode, lateWatchOn, late);
        }
    }
}

extern "C" int __stdcall DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
