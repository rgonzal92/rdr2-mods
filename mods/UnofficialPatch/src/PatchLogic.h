// The parts of UnofficialPatch that need no game: hashes, global indexes, the log line and the
// rules of its two fixes. Checked by `make test`.
#pragma once

using Hash = unsigned int;
// Script values are 8 bytes each.
using Slot = unsigned long long;

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
static_assert(joaat("mary1") == 0x9445E33D && joaat("WEAPON_UNARMED") == 0xA2719263 && joaat("SLEEP") == 0xEEFA61FF,
              "joaat must reproduce the game's hashes");

// An 8-digit hexadecimal number in a log line.
struct Hex {
    Hash value;
};

// One line of the log, put together without the C runtime. Text that does not fit is cut.
struct Line {
    static const int CAPACITY = 255;
    char text[CAPACITY + 1];
    int length = 0;

    Line() { text[0] = '\0'; }

    Line& operator<<(const char* words) {
        while (*words && length < CAPACITY) text[length++] = *words++;
        text[length] = '\0';
        return *this;
    }
    Line& operator<<(int number) {
        char digits[12];
        int count = 0;
        unsigned int rest = number < 0 ? 0u - static_cast<unsigned int>(number) : static_cast<unsigned int>(number);
        do {
            digits[count++] = static_cast<char>('0' + rest % 10);
            rest /= 10;
        } while (rest);
        if (number < 0) digits[count++] = '-';
        while (count && length < CAPACITY) text[length++] = digits[--count];
        text[length] = '\0';
        return *this;
    }
    Line& operator<<(Hex hash) {
        char digits[11] = "0x";
        for (int i = 0; i < 8; ++i) digits[2 + i] = "0123456789ABCDEF"[(hash.value >> (28 - 4 * i)) & 15];
        digits[10] = '\0';
        return *this << digits;
    }
};

// ---------------------------------------------------------------------------------------------
// Fix 1, the knife guard: "We Loved Once and True" (script mary1).
//
// The mission lets the player wander on foot up to 100 m from the Chelonian camp (mary1.c line
// 48503) and reads the "Throttle" key without a distance test (37352). The radius covers that.
constexpr float SCENE_X = 880.0f, SCENE_Y = 1270.0f, SCENE_Z = 234.0f, SCENE_RADIUS = 110.0f;

constexpr bool nearScene(float x, float y, float z) {
    return (x - SCENE_X) * (x - SCENE_X) + (y - SCENE_Y) * (y - SCENE_Y) + (z - SCENE_Z) * (z - SCENE_Z) <= SCENE_RADIUS * SCENE_RADIUS;
}

// Decides, frame by frame, when to put the fists in Arthur's hand. After an attempt it waits for
// the game to act before the next one, and after ten attempts on one drawn weapon it stops until
// the player has left the scene, so that it can never send the command every frame.
struct UnarmedKeeper {
    static const int MAX_ATTEMPTS = 10;
    static const int WAIT_MS = 500;
    enum Action { NOTHING, PUT_AWAY, GIVE_UP };

    int attempts = 0;
    int nextAttemptAt = 0;
    bool gaveUp = false;

    // `armed`: the right hand holds something other than the fists. `busy`: a hold is running
    // or the holster state is changing, when the game's own script does not equip either.
    Action frame(int now, bool armed, bool busy) {
        if (!armed) {
            attempts = 0;
            return NOTHING;
        }
        if (gaveUp || busy || (attempts > 0 && now < nextAttemptAt)) return NOTHING;
        if (attempts == MAX_ATTEMPTS) {
            gaveUp = true;
            return GIVE_UP;
        }
        ++attempts;
        nextAttemptAt = now + WAIT_MS;
        return PUT_AWAY;
    }

    void leave() {
        attempts = 0;
        gaveUp = false;
    }
};

// ---------------------------------------------------------------------------------------------
// Fix 2, night clothes: a camp member who is awake in the night outfit (script def_comp_brain).
//
// Global indexes of game build 1.0.1491.50, derived from the decompiled scripts in the audit of
// 2026-10-13. An array's own index holds its length and element i of size S starts at
// index + 1 + S * i. The plugin checks both lengths in the game before it reads further.
const int MEMBERS = 27;
const int MEMBER_TABLE = 1360165;        // Global_1360165: 27 camp members, 1157 values each
const int MEMBER_SIZE = 1157;
const int MEMBER_PERSCHAR = 1;           // .f_1: the member's persistent character
const int MEMBER_SCHEDULE_ENTRY = 47;    // .f_47: index of the current schedule entry
const int MEMBER_DECIDE_AGAIN = 56;      // .f_56: "decide the outfit again", set by the script at each new schedule entry
const int OUTFIT_TABLE = 40 + 4942;      // Global_40.f_4942: 60 saved values per member
const int OUTFIT_SIZE = 60;
const int OUTFIT_WANTED = 3;             // .f_3: the outfit the member should wear
const int OUTFIT_LOCK = 59;              // .f_59: not 0 while a script holds the outfit
constexpr int memberGlobal(int member, int field) { return MEMBER_TABLE + 1 + MEMBER_SIZE * member + field; }
constexpr int outfitGlobal(int member, int field) { return OUTFIT_TABLE + 1 + OUTFIT_SIZE * member + field; }

const int CAMP = 40 + 4283;              // Global_40.f_4283: the current camp, 0 Colter, 4 Guarma
// def_comp_brain takes no outfit decision in these two camps (line 2186).
constexpr bool campDecidesOutfits(int camp) { return camp != 0 && camp != 4; }

// The outfit def_comp_brain gives this member for sleeping (func_197, line 5868). Per member,
// because the night outfit of Bill, Uncle and Strauss is a day outfit of five other members.
constexpr bool isNightOutfit(int member, int outfit) {
    switch (member) {
        case 13: case 14:  // Abigail and Jack: a second one at the epilogue camps
            return outfit == -445211559 || outfit == -1205468859;
        case 0: case 1: case 2: case 7: case 8: case 9: case 10: case 15: case 16: case 17: case 19: case 20: case 21: case 22:
            return outfit == -445211559;
        case 3: case 4: case 18:
            return outfit == 1744281750;
        case 11:
            return outfit == -946772361;
        default:  // Hosea, Micah, the dog, Trelawny and the late members have none
            return false;
    }
}

// What the game holds about one member.
struct MemberState {
    int member;      // 0 to 26
    int wanted;      // the wanted outfit
    bool locked;     // a script holds the outfit
    bool pending;    // "decide again" is already set
    bool simulated;  // the member's persistent character is valid and active
    bool asleep;     // the current schedule entry is SLEEP
};

enum Verdict {
    FINE,     // day clothes, or night clothes while asleep
    WAITING,  // night clothes, but the script has a request, holds the outfit or does not run the member
    STUCK,    // awake in night clothes with no decision coming before the next schedule entry
};

constexpr Verdict classify(const MemberState& state) {
    if (!isNightOutfit(state.member, state.wanted) || state.asleep) return FINE;
    if (state.locked || state.pending || !state.simulated) return WAITING;
    return STUCK;
}

// Paces the requests for one member. A request can fail the same way the first decision did
// (one roll in two to four), so the first ones come quickly; if the member is still stuck after
// those, something the plugin does not read blocks the decision and the requests become rare.
struct AskPacer {
    static const int QUICK_ASKS = 6;
    static const int QUICK_MS = 5000;
    static const int SLOW_MS = 300000;
    enum Action { NOTHING, ASK, CLEARED };

    int asks = 0;
    int nextAskAt = 0;

    Action check(int now, Verdict verdict) {
        if (verdict == FINE) {
            if (asks == 0) return NOTHING;
            asks = 0;
            return CLEARED;
        }
        if (verdict == WAITING || (asks > 0 && now < nextAskAt)) return NOTHING;
        nextAskAt = now + (asks < QUICK_ASKS ? QUICK_MS : SLOW_MS);
        ++asks;
        return ASK;
    }
};

// ---------------------------------------------------------------------------------------------
// Fix 3, encounter cooldowns: the cooldown clock of the 120 random encounters ("beats").
//
// Each frame short_update takes 90 off the one beat its index points at (a beat is visited
// every 120 frames), so a cooldown runs out faster the higher the frame rate. The values are
// "game seconds": 30 a second by the game clock, which the tick only matches at 40 fps.
// Indexes confirmed in the game by PatchProbe on 2026-10-13.
const int BEAT_TABLE = 17504 + 42;        // Global_17504.f_42: 120 beats, 8 values each
const int BEAT_COUNT = 120;
const int BEAT_SIZE = 8;
const int BEAT_COOLDOWN_FIELD = 4;        // .f_4: the cooldown; the beat is available at 0 or below
const int BEAT_TICK_INDEX = 1326862 + 1;  // Global_1326862.f_1: the beat short_update visits this frame
const int COOLDOWN_STEP = 90;             // what short_update takes off the visited beat
const int STORY_MARKER = 1572887 + 12;    // Global_1572887.f_12: -1 in story mode
constexpr int beatCooldownGlobal(int beat) { return BEAT_TABLE + 1 + BEAT_SIZE * beat + BEAT_COOLDOWN_FIELD; }

// Lengths that must hold before the table is touched.
struct LayoutCell {
    int global;
    int expected;
};
constexpr LayoutCell BEAT_LAYOUT[] = {{17546, 120}, {18507, 180}, {19588, 24}, {19709, 500}};

// The setting: 0 is off, 20 to 1000 is the frame rate the clock should run as. Anything else
// is off, because a negative or tiny number would make cooldowns grow or last for weeks.
constexpr int validReferenceFps(int setting) { return setting >= 20 && setting <= 1000 ? setting : 0; }

// Gives back part of the 90 each time it sees the game take it, so that the clock runs as it
// would at the reference frame rate. It writes one cell, on the frame after the game's own
// decrement of exactly that cell, a value between what the game left and what was there before.
// So it can only keep a beat on cooldown longer, and it never touches a beat the game has
// brought to 0 or below.
struct CooldownClock {
    static const int MAX_VISIT_MS = 10000;  // a longer gap between two visits is a pause or a load

    int referenceFps = 0;
    bool primed = false;
    int previousIndex = 0;
    int snapshot[BEAT_COUNT];
    int lastVisit[BEAT_COUNT];   // game timer of the beat's previous visit, 0 for none
    int carry[BEAT_COUNT];       // thousandths of a unit owed from the previous visit

    int corrections = 0;   // visits where part of the 90 was given back
    int taken = 0;         // units the game took on those visits
    int givenBack = 0;     // units given back on those visits
    int failedWrites = 0;  // a cell that did not read back as written

    void restart() {
        primed = false;
        for (int i = 0; i < BEAT_COUNT; ++i) lastVisit[i] = carry[i] = 0;
    }
    void clearCounters() { corrections = taken = givenBack = failedWrites = 0; }

    // `index` is the game's tick index as read this frame, `read(i)` reads beat i and
    // `write(i, value)` writes it and returns what the cell holds afterwards.
    template <typename Read, typename Write>
    void tick(int now, int index, Read read, Write write) {
        if (index < 0 || index >= BEAT_COUNT) return;
        const int visited = primed ? (index - previousIndex + BEAT_COUNT) % BEAT_COUNT : 0;
        if (!primed || visited > 8) {  // the first frame, or a loading screen or long hitch
            restart();
            for (int i = 0; i < BEAT_COUNT; ++i) snapshot[i] = read(i);
            previousIndex = index;
            primed = true;
            return;
        }
        for (int i = 0; i < BEAT_COUNT; ++i) {
            int value = read(i);
            if ((i - previousIndex + BEAT_COUNT) % BEAT_COUNT < visited) {
                const int before = snapshot[i];
                const int elapsed = now - lastVisit[i];
                if (referenceFps > 0 && before > COOLDOWN_STEP && value == before - COOLDOWN_STEP && lastVisit[i] != 0 && elapsed > 0 && elapsed <= MAX_VISIT_MS) {
                    // 0.75 units a frame at the reference rate, in thousandths.
                    const int owed = referenceFps * elapsed * 3 / 4 + carry[i];
                    const int step = owed / 1000;
                    if (step < COOLDOWN_STEP) {
                        carry[i] = owed % 1000;
                        value = before - step;
                        if (write(i, value) != value) ++failedWrites;
                        ++corrections;
                        taken += COOLDOWN_STEP;
                        givenBack += COOLDOWN_STEP - step;
                    } else {
                        carry[i] = 0;  // at or below the reference rate: the game's own step stands
                    }
                } else {
                    carry[i] = 0;
                }
                lastVisit[i] = now;
            }
            snapshot[i] = value;  // anything else the game wrote is adopted as it is
        }
        previousIndex = index;
    }

    int beatsOnCooldown() const {
        int count = 0;
        for (int i = 0; i < BEAT_COUNT; ++i) count += snapshot[i] > 0;
        return primed ? count : 0;
    }
};

// ---------------------------------------------------------------------------------------------
// Watch only, trains: the "suppress" flag of an ambient train, which medium_update clears only
// while the player is inside a town region (medium_update.c lines 8606-8657). Mask values.
const int TRAIN_TABLE = 40 + 11029;     // Global_40.f_11029: 13 trains, 5 saved values each, flags first
const int TRAIN_COUNT = 13;
const int AMBIENT_TRAINS = 6;           // trains 0 to 5; 6 and 7 are the trolleys, 8 to 12 special
const int TOWN_REGION = 1894899 + 2;    // Global_1894899.f_2: the player's town region, -1 for none
const int TRAIN_SUPPRESSED = 0x2, TRAIN_LOCKED = 0x10, TRAIN_HAS_VEHICLE = 0x2000, TRAIN_ROBBERY_TIMER = 0x10000;
constexpr int trainFlagsGlobal(int train) { return TRAIN_TABLE + 1 + 5 * train; }

// Suppressed where the script will not lift it: outside every town region, with neither lock.
constexpr bool trainStuck(int flags, int townRegion) {
    return townRegion == -1 && (flags & TRAIN_SUPPRESSED) && !(flags & (TRAIN_LOCKED | TRAIN_ROBBERY_TIMER));
}

// ---------------------------------------------------------------------------------------------
// Late-game fixes (batch 4). Indexes of build 1.0.1491.50, checked in the compiled scripts but
// not yet in the game; each fix starts as log only.

// Settings that switch a fix: 0 off, 1 log only, 2 on. Anything else is log only.
constexpr int validMode(int setting) { return setting == 0 || setting == 2 ? setting : 1; }

// True once, on the `needed`-th call in a row with `condition` true; false resets the count.
struct StreakGate {
    int needed = 1;
    int streak = 0;
    bool step(bool condition) {
        if (!condition) {
            streak = 0;
            return false;
        }
        return ++streak == needed;
    }
};

// Dreamcatchers: main rebuilds the Log on every load and adds the Dreamcatchers entry without
// asking whether the collection is finished (main.c 2661, func_209 at 11432).
const int DREAM_TABLE = 40 + 8863;          // Global_40.f_8863: 143 discoverable flags behind a length
const int DREAM_TABLE_LENGTH = 143;
const int DREAM_ARROWHEAD = DREAM_TABLE + 1;  // element 0, the Ancient Arrowhead; bit value 2 = taken (main.c 11463)
const int DREAM_FOUND = DREAM_TABLE + 148;  // .f_148, DiscoDreamcatchers_bitField: bits 1 to 20 (startup.c 3586)
const int DREAM_ALL = 0x1FFFFE;
const int DREAM_LOG_TYPE = 3;
const Hash DISCO_DREAM = 0x11D82498;
static_assert(joaat("DISCO_DREAM") == DISCO_DREAM, "Log entry hash");
constexpr bool dreamcatchersDone(int found, int arrowhead) { return (found & DREAM_ALL) == DREAM_ALL && (arrowhead & 2); }
constexpr int bitsSet(int value) { return value ? (value & 1) + bitsSet(static_cast<int>(static_cast<unsigned int>(value) >> 1)) : 0; }

// Legendary fish map marks: short_update turns a fish's mark back to "not caught" when a pelt or
// body is put down with empty hands (short_update.c 25538), and nothing fixes it after the fish
// is mailed. Thirteen fish; the Northern Pike has no mark the scripts hold.
struct FishMark {
    Hash fish;      // LEGENDARY_FISH_nn
    Hash spot;      // LEGENDARY_FISHING_SPOT_nn
    Hash uncaught;  // map mark "not caught"
    Hash caught;    // map mark "caught"
};
const int FISH_COUNT = 13;
constexpr FishMark FISH[FISH_COUNT] = {
    {joaat("LEGENDARY_FISH_01"), joaat("LEGENDARY_FISHING_SPOT_01"), 0x6ACB9287, 0xD68748A9},
    {joaat("LEGENDARY_FISH_02"), joaat("LEGENDARY_FISHING_SPOT_02"), 0xA36676CE, 0x63A5E624},
    {joaat("LEGENDARY_FISH_03"), joaat("LEGENDARY_FISHING_SPOT_03"), 0xE08647D0, 0x577127B6},
    {joaat("LEGENDARY_FISH_04"), joaat("LEGENDARY_FISHING_SPOT_04"), 0x485F8025, 0x46428A0E},
    {joaat("LEGENDARY_FISH_05"), joaat("LEGENDARY_FISHING_SPOT_05"), 0xAB12229D, 0xAD698EFE},
    {joaat("LEGENDARY_FISH_06"), joaat("LEGENDARY_FISHING_SPOT_06"), 0xE74533FE, 0xA6146018},
    {joaat("LEGENDARY_FISH_07"), joaat("LEGENDARY_FISHING_SPOT_07"), 0xA2FFCAA5, 0xF912DB43},
    {joaat("LEGENDARY_FISH_08"), joaat("LEGENDARY_FISHING_SPOT_08"), 0x73837F9E, 0x3E141D12},
    {joaat("LEGENDARY_FISH_09"), joaat("LEGENDARY_FISHING_SPOT_09"), 0x1E27E01A, 0x2D109D7B},
    {joaat("LEGENDARY_FISH_10"), joaat("LEGENDARY_FISHING_SPOT_10"), 0xED66162F, 0x3DBD4681},
    {joaat("LEGENDARY_FISH_11"), joaat("LEGENDARY_FISHING_SPOT_11"), 0x0E08A5E7, 0xC140D1EB},
    {joaat("LEGENDARY_FISH_12"), joaat("LEGENDARY_FISHING_SPOT_12"), 0x6AE2A8FD, 0x4B6E0F5F},
    {joaat("LEGENDARY_FISH_13"), joaat("LEGENDARY_FISHING_SPOT_13"), 0xBBE7B206, 0x50108630},
};
static_assert(FISH[0].fish == 0x90D2ADDF && FISH[0].spot == 0xD9BB75EB && FISH[12].fish == 0x225E4F40 && FISH[12].spot == 0x216D83EE,
              "fish_table.tsv hashes");
const Hash LEGENDARY_FISH = 0x76EED176;
static_assert(joaat("LEGENDARY_FISH") == LEGENDARY_FISH, "collectable category");
// The 14 collectables the game lists in that category: the 13 above and the Northern Pike.
constexpr Hash FISH_COLLECTABLES[14] = {
    FISH[0].fish, FISH[1].fish, FISH[2].fish, FISH[3].fish, FISH[4].fish, FISH[5].fish, FISH[6].fish,
    FISH[7].fish, FISH[8].fish, FISH[9].fish, FISH[10].fish, FISH[11].fish, FISH[12].fish, joaat("LEGENDARY_FISH_14"),
};

// The game's category holds exactly these 14, in any order.
inline bool fishCatalogMatches(const Hash* listed, int count) {
    if (count != 14) return false;
    for (Hash wanted : FISH_COLLECTABLES) {
        bool seen = false;
        for (int i = 0; i < count; ++i) seen = seen || listed[i] == wanted;
        if (!seen) return false;
    }
    return true;
}

// A mailed fish whose spot is found should show "caught" and not "not caught".
constexpr bool fishMarkWrong(int turnedIn, int spotFound, bool uncaughtShown, bool caughtShown) {
    return turnedIn > 0 && spotFound > 0 && (uncaughtShown || !caughtShown);
}

// Watch only, "Advertising, the New American Art" (braithwaites1): the slow-motion kill camera.
// It normally ends after 1.5 s of system time; two branches skip its clean-up (func_818).
constexpr float KILLCAM_X = 1344.794f, KILLCAM_Y = -1372.709f, KILLCAM_Z = 84.588f, KILLCAM_RADIUS = 0.5f;
constexpr bool nearKillcam(float x, float y, float z) {
    return (x - KILLCAM_X) * (x - KILLCAM_X) + (y - KILLCAM_Y) * (y - KILLCAM_Y) + (z - KILLCAM_Z) * (z - KILLCAM_Z) <= KILLCAM_RADIUS * KILLCAM_RADIUS;
}

// One episode of the kill camera in system milliseconds. `shown`: the camera renders, the player
// has no control. `guarded`: paused, loading, fading, dead or failing, which does not count.
struct KillcamWatch {
    static const int STUCK_MS = 3000;
    static const int GONE_MS = 250;
    static const int MAX_STEP_MS = 100;
    enum Event { NOTHING, STARTED, STUCK, ENDED };

    bool running = false, reported = false;
    int last = 0, lastSeen = 0, shownMs = 0, guardedMs = 0;

    Event frame(int now, bool shown, bool guarded) {
        if (!running) {
            if (!shown) return NOTHING;
            running = true;
            reported = false;
            last = lastSeen = now;
            shownMs = guardedMs = 0;
            return STARTED;
        }
        int step = now - last;
        step = step < 0 ? 0 : step > MAX_STEP_MS ? MAX_STEP_MS : step;
        last = now;
        if (!shown) {
            if (now - lastSeen < GONE_MS) return NOTHING;
            running = false;
            return ENDED;
        }
        lastSeen = now;
        (guarded ? guardedMs : shownMs) += step;
        if (!reported && shownMs >= STUCK_MS) {
            reported = true;
            return STUCK;
        }
        return NOTHING;
    }
};

// Watch only, the ranch chores after Emmet Granger's second part: rcm_gunslinger1_2 clears bit 5
// (stall cleaning) of this saved value (line 15198); after "The Best of Women" nothing sets it
// again. Expected at Beecher's Hope before that mission: 0x0F, or 0x2F after "Uncle's Bad Day".
const int RANCH_CHORES = 40 + 7854;     // Global_40.f_7854
const int CAMP_BEECHERS_HOPE = 8;       // the value of CAMP there

// Watch only, "A New Jerusalem" (beechers2_2) and the cleaver of "Jim Milton Rides, Again?".
const int REPLAY_FLAGS = 1572867;       // & 0x8000 while a mission replay runs
const Hash SP_ENDLESS_SUMMER_WEAPONS = 0xA4FD7BC2;
const Hash WEAPON_MELEE_CLEAVER = 0xEF32A25D, AMMO_HATCHET_CLEAVER = 0xB925EC32;
static_assert(joaat("SP_ENDLESS_SUMMER_WEAPONS") == SP_ENDLESS_SUMMER_WEAPONS && joaat("WEAPON_MELEE_CLEAVER") == WEAPON_MELEE_CLEAVER &&
                  joaat("AMMO_HATCHET_CLEAVER") == AMMO_HATCHET_CLEAVER && joaat("braithwaites1") == 0xEED47143 && joaat("beechers2_2") == 0x2E1683B7,
              "late-game hashes");

// "A New Jerusalem": the mission puts John on the empty backup inventory and switches back only
// when its own script variable says so (beechers2_2.c 17937-17954), so a restarted copy of the
// script can leave him there and the autosave keeps it. The fix makes the mission's own call,
// _INVENTORY_USE_BACKUP_INVENTORY(false), with the guards of the harm review (B1, B2).
//
// The player's weapons, tools and kits (weapon-items-1491.50.tsv without damage types and animals),
// to count what each inventory holds.
constexpr Hash WEAPON_ITEMS[] = {
    joaat("WEAPON_BOW"),
    joaat("WEAPON_BOW_CHARLES"),
    joaat("WEAPON_FISHINGROD"),
    joaat("WEAPON_KIT_BINOCULARS"),
    joaat("WEAPON_KIT_CAMERA"),
    joaat("WEAPON_KIT_DETECTOR"),
    joaat("WEAPON_LASSO"),
    joaat("WEAPON_MELEE_ANCIENT_HATCHET"),
    joaat("WEAPON_MELEE_BROKEN_SWORD"),
    joaat("WEAPON_MELEE_CLEAVER"),
    joaat("WEAPON_MELEE_DAVY_LANTERN"),
    joaat("WEAPON_MELEE_HATCHET"),
    joaat("WEAPON_MELEE_HATCHET_DOUBLE_BIT"),
    joaat("WEAPON_MELEE_HATCHET_DOUBLE_BIT_RUSTED"),
    joaat("WEAPON_MELEE_HATCHET_HEWING"),
    joaat("WEAPON_MELEE_HATCHET_HUNTER"),
    joaat("WEAPON_MELEE_HATCHET_HUNTER_RUSTED"),
    joaat("WEAPON_MELEE_HATCHET_MELEEONLY"),
    joaat("WEAPON_MELEE_HATCHET_VIKING"),
    joaat("WEAPON_MELEE_KNIFE"),
    joaat("WEAPON_MELEE_KNIFE_BEAR"),
    joaat("WEAPON_MELEE_KNIFE_BILL"),
    joaat("WEAPON_MELEE_KNIFE_CHARLES"),
    joaat("WEAPON_MELEE_KNIFE_CIVIL_WAR"),
    joaat("WEAPON_MELEE_KNIFE_DUTCH"),
    joaat("WEAPON_MELEE_KNIFE_HOSEA"),
    joaat("WEAPON_MELEE_KNIFE_JAVIER"),
    joaat("WEAPON_MELEE_KNIFE_JAWBONE"),
    joaat("WEAPON_MELEE_KNIFE_JOHN"),
    joaat("WEAPON_MELEE_KNIFE_KIERAN"),
    joaat("WEAPON_MELEE_KNIFE_LENNY"),
    joaat("WEAPON_MELEE_KNIFE_MICAH"),
    joaat("WEAPON_MELEE_KNIFE_MINER"),
    joaat("WEAPON_MELEE_KNIFE_SADIE"),
    joaat("WEAPON_MELEE_KNIFE_SEAN"),
    joaat("WEAPON_MELEE_KNIFE_UNCLE"),
    joaat("WEAPON_MELEE_KNIFE_VAMPIRE"),
    joaat("WEAPON_MELEE_LANTERN"),
    joaat("WEAPON_MELEE_LANTERN_ELECTRIC"),
    joaat("WEAPON_MELEE_MACHETE"),
    joaat("WEAPON_MELEE_TORCH"),
    joaat("WEAPON_MELEE_TORCH_CROWD"),
    joaat("WEAPON_PISTOL_M1899"),
    joaat("WEAPON_PISTOL_MAUSER"),
    joaat("WEAPON_PISTOL_MAUSER_DRUNK"),
    joaat("WEAPON_PISTOL_SEMIAUTO"),
    joaat("WEAPON_PISTOL_VOLCANIC"),
    joaat("WEAPON_REPEATER_CARBINE"),
    joaat("WEAPON_REPEATER_CARBINE_SADIE"),
    joaat("WEAPON_REPEATER_EVANS"),
    joaat("WEAPON_REPEATER_HENRY"),
    joaat("WEAPON_REPEATER_WINCHESTER"),
    joaat("WEAPON_REPEATER_WINCHESTER_JOHN"),
    joaat("WEAPON_REVOLVER_CATTLEMAN"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_DUALWIELD"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_HOSEA"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_HOSEA_DUALWIELD"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_JOHN"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_KIERAN"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_LENNY"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_MEXICAN"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_PIG"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_SADIE"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_SADIE_DUALWIELD"),
    joaat("WEAPON_REVOLVER_CATTLEMAN_SEAN"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION_DUALWIELD"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION_EXOTIC"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION_GAMBLER"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION_JAVIER"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION_MICAH"),
    joaat("WEAPON_REVOLVER_DOUBLEACTION_MICAH_DUALWIELD"),
    joaat("WEAPON_REVOLVER_LEMAT"),
    joaat("WEAPON_REVOLVER_SCHOFIELD"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_BILL"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_CALLOWAY"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_DUALWIELD"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_DUTCH"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_DUTCH_DUALWIELD"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_GOLDEN"),
    joaat("WEAPON_REVOLVER_SCHOFIELD_UNCLE"),
    joaat("WEAPON_RIFLE_BOLTACTION"),
    joaat("WEAPON_RIFLE_BOLTACTION_BILL"),
    joaat("WEAPON_RIFLE_SPRINGFIELD"),
    joaat("WEAPON_RIFLE_VARMINT"),
    joaat("WEAPON_SHOTGUN_DOUBLEBARREL"),
    joaat("WEAPON_SHOTGUN_DOUBLEBARREL_EXOTIC"),
    joaat("WEAPON_SHOTGUN_DOUBLEBARREL_UNCLE"),
    joaat("WEAPON_SHOTGUN_FOLDING"),
    joaat("WEAPON_SHOTGUN_PUMP"),
    joaat("WEAPON_SHOTGUN_REPEATING"),
    joaat("WEAPON_SHOTGUN_SAWEDOFF"),
    joaat("WEAPON_SHOTGUN_SAWEDOFF_CHARLES"),
    joaat("WEAPON_SHOTGUN_SEMIAUTO"),
    joaat("WEAPON_SHOTGUN_SEMIAUTO_HOSEA"),
    joaat("WEAPON_SNIPERRIFLE_CARCANO"),
    joaat("WEAPON_SNIPERRIFLE_ROLLINGBLOCK"),
    joaat("WEAPON_SNIPERRIFLE_ROLLINGBLOCK_EXOTIC"),
    joaat("WEAPON_SNIPERRIFLE_ROLLINGBLOCK_LENNY"),
    joaat("WEAPON_THROWN_DYNAMITE"),
    joaat("WEAPON_THROWN_MOLOTOV"),
    joaat("WEAPON_THROWN_THROWING_KNIVES"),
    joaat("WEAPON_THROWN_THROWING_KNIVES_JAVIER"),
    joaat("WEAPON_THROWN_TOMAHAWK"),
    joaat("WEAPON_THROWN_TOMAHAWK_ANCIENT"),
    joaat("WEAPON_THROWN_TOMAHAWK_MELEEONLY"),
};
const int WEAPON_ITEM_COUNT = sizeof(WEAPON_ITEMS) / sizeof(WEAPON_ITEMS[0]);
const int MAIN_INVENTORY = 1, BACKUP_INVENTORY = 5;

struct JerusalemState {
    bool freeRoam;         // story scripts run, no mission, no replay
    bool ownerRunning;     // beechers2_2, bathing, odriscolls3 or the replay launcher runs
    bool onBackup;         // the player uses the backup inventory
    bool epilogueWeapons;  // SP_ENDLESS_SUMMER_WEAPONS: set only after the epilogue hand-over to inventory 1
    int weaponsInMain;     // weapon names with a count above 0 in inventory 1
    bool ownerSeen;        // an owner script ran since this save was loaded
};

enum JerusalemAction { LEAVE, WOULD_SWITCH, SWITCH, ASK_FIRST, MAIN_EMPTY };

// `mode` 0 off, 1 log only, 2 on, 3 on and also for a save that was stuck when loaded.
constexpr JerusalemAction jerusalemVerdict(const JerusalemState& state, int mode) {
    if (mode == 0 || !state.freeRoam || state.ownerRunning || !state.onBackup || !state.epilogueWeapons) return LEAVE;
    if (state.weaponsInMain == 0) return MAIN_EMPTY;
    if (mode == 1) return WOULD_SWITCH;
    // Something gathered on the backup inventory while stuck stays there; the owner decides.
    if (!state.ownerSeen && mode != 3) return ASK_FIRST;
    return SWITCH;
}

// Granger's clean-up clears bit 5 (stall cleaning) after "The Best of Women" set bit 4: bits 0, 1,
// 3 and 4 set, 5 clear, nothing above bit 6 (harm review 2.3; only types 0 to 6 exist).
constexpr bool choresNeedStall(int value) { return (value & ~0x7F) == 0 && (value & 0x3B) == 0x1B; }

// The values one watch logs; `changed` is true when any differs from the last call.
template <int N>
struct Watched {
    int values[N] = {};
    bool seen = false;
    bool changed(const int (&now)[N]) {
        bool differs = !seen;
        for (int i = 0; i < N; ++i) {
            differs = differs || values[i] != now[i];
            values[i] = now[i];
        }
        seen = true;
        return differs;
    }
};
