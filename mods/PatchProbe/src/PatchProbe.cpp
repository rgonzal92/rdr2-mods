// PatchProbe: an observe-only ScriptHookRDR2 plugin for game build 1.0.1491.50, story mode.
//
// It answers, from the running game, three questions the fixes planned after the audit of
// 2026-10-13 depend on, and writes the answers to PatchProbe.log in the game folder:
//
//   1. Does GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH count threads? One reading
//      of the engine code says it returns the script program's reference count minus one. The
//      probe prints it next to a count made with the thread iterator, for a list of scripts,
//      whenever either number changes.
//   2. Do the global indexes derived offline hold the values the audit expects?
//   3. Does the random-encounter cooldown table lose exactly 90 on the one beat short_update
//      visits each frame, and nothing else?
//
// It writes no global, calls no native that changes game state, patches no code and searches no
// memory.

#include "ProbeLogic.h"

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
}

// Game natives, by hash. All of them only read.
const Slot GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::...(Hash scriptHash) -> int
const Slot SCRIPT_THREAD_ITERATOR_RESET = 0x39382EB8DCD8684D;                             // SCRIPT::SCRIPT_THREAD_ITERATOR_RESET()
const Slot SCRIPT_THREAD_ITERATOR_GET_NEXT_THREAD_ID = 0x3CE3FB167E837D7C;                // SCRIPT::SCRIPT_THREAD_ITERATOR_GET_NEXT_THREAD_ID() -> int
const Slot GET_HASH_OF_THREAD = 0x724CB89D35B283D0;                                       // SCRIPT::_GET_HASH_OF_THREAD(int threadId) -> Hash
const Slot IS_THREAD_ACTIVE = 0x46E9AE36D8FA6417;                                         // SCRIPT::IS_THREAD_ACTIVE(int threadId, BOOL ignoreKilledState) -> BOOL
const Slot GET_GAME_TIMER = 0x4F67E8ECA7D3F667;                                           // MISC::GET_GAME_TIMER() -> int

// The scripts whose two counts are compared: three that always run in story mode, the two the
// cooldown table belongs to, and scripts that start and stop during play (a mission, fishing,
// a camp, the crafting scripts, a shop).
struct Watched {
    const char* name;
    Hash hash;
    int nativeCount, iterated, active;
};
static Watched watched[] = {
    {"main", joaat("main"), -2, -2, -2},
    {"short_update", joaat("short_update"), -2, -2, -2},
    {"long_update", joaat("long_update"), -2, -2, -2},
    {"medium_update", joaat("medium_update"), -2, -2, -2},
    {"flow_controller", joaat("flow_controller"), -2, -2, -2},
    {"mary1", joaat("mary1"), -2, -2, -2},
    {"fishing_core", joaat("fishing_core"), -2, -2, -2},
    {"camp_horseshoeoverlook", joaat("camp_horseshoeoverlook"), -2, -2, -2},
    {"player_camp", joaat("player_camp"), -2, -2, -2},
    {"interactive_campfire", joaat("interactive_campfire"), -2, -2, -2},
    {"shop_general", joaat("shop_general"), -2, -2, -2},
    {"player_horse", joaat("player_horse"), -2, -2, -2},
};
const int WATCHED = sizeof(watched) / sizeof(watched[0]);

// An argument travels as 64 bits.
static Slot arg(int value) { return static_cast<unsigned int>(value); }
static Slot arg(Hash value) { return value; }

// Calls a native whose result is a 32-bit value.
template <typename... Args>
static int call(Slot native, Args... args) {
    nativeInit(native);
    (nativePush64(arg(args)), ...);
    return static_cast<int>(*nativeCall());
}

// Appends one line to PatchProbe.log in the game folder, the current folder of the game.
static void log(const Line& line) {
    const unsigned long FILE_APPEND_DATA = 4, FILE_SHARE_READ = 1, OPEN_ALWAYS = 4, FILE_ATTRIBUTE_NORMAL = 0x80;
    void* const file = CreateFileA(".\\PatchProbe.log", FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == reinterpret_cast<void*>(-1LL)) return;
    unsigned long written;
    WriteFile(file, line.text, static_cast<unsigned long>(line.length), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

// Counts threads with the iterator, as the game's own scripts do. The iterator is the game's
// one shared cursor; a game script never keeps it across a frame, and the plugin runs between
// the game scripts' turns.
static void countThreads() {
    for (Watched& script : watched) script.iterated = script.active = 0;
    call(SCRIPT_THREAD_ITERATOR_RESET);
    for (int guard = 0; guard < 2048; ++guard) {
        const int thread = call(SCRIPT_THREAD_ITERATOR_GET_NEXT_THREAD_ID);
        if (thread <= 0) break;
        const Hash hash = static_cast<Hash>(call(GET_HASH_OF_THREAD, thread));
        for (Watched& script : watched) {
            if (script.hash != hash) continue;
            ++script.iterated;
            if (call(IS_THREAD_ACTIVE, thread, 0)) ++script.active;
        }
    }
}

// Question 1. Logs a script's line whenever one of its three numbers changed.
static void compareThreadCounts(int now) {
    int before[WATCHED][2];
    for (int i = 0; i < WATCHED; ++i) before[i][0] = watched[i].iterated, before[i][1] = watched[i].active;
    countThreads();
    for (int i = 0; i < WATCHED; ++i) {
        Watched& script = watched[i];
        const int nativeCount = call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, script.hash);
        if (nativeCount == script.nativeCount && before[i][0] == script.iterated && before[i][1] == script.active) continue;
        script.nativeCount = nativeCount;
        log(Line() << "threads " << now << " ms " << script.name << ": native " << nativeCount << " iterated " << script.iterated
                   << " active " << script.active << (nativeCount == script.iterated ? "" : "  DIFFER"));
    }
}

// The low 32 bits of a script global as a signed number, or `missing` when ScriptHook has no
// address for it.
static int global(int index, int missing = -999999) {
    const Slot* const cell = getGlobalPtr(index);
    return cell ? static_cast<int>(*cell) : missing;
}

// Globals exist once the story scripts run. The iterator's count decides, because the other
// count is the one in question.
static bool storyScriptsRun() {
    for (int i = 0; i < 3; ++i)  // main, short_update, long_update
        if (watched[i].active <= 0) return false;
    return true;
}

// Question 2.
static void logLayout() {
    for (const LayoutCell& cell : LAYOUT) {
        const int value = global(cell.global);
        log(Line() << "layout " << cell.name << ": global " << cell.global << " holds " << value << ", expected " << cell.expected
                   << (value == cell.expected ? "" : "  DIFFERS"));
    }
}

static void scriptMain() {
    log(Line() << "PatchProbe started (observe only, build 1.0.1491.50 expected)");
    for (Watched& script : watched) script.nativeCount = script.iterated = script.active = -2;
    CooldownWatch cooldowns;
    bool layoutLogged = false, layoutAsExpected = false;
    int lastThreadCheck = 0, lastReport = 0;

    for (;;) {
        scriptWait(0);
        const int now = call(GET_GAME_TIMER);

        if (lastThreadCheck == 0 || now - lastThreadCheck >= 2000) {
            lastThreadCheck = now;
            compareThreadCounts(now);
        }
        if (!storyScriptsRun()) {
            cooldowns.restart();
            lastReport = now;
            continue;
        }
        if (!layoutLogged) {
            layoutLogged = true;
            logLayout();
            layoutAsExpected = global(BEAT_TABLE) == BEAT_COUNT;
            if (!layoutAsExpected) log(Line() << "cooldown watch off: the beat table is not where the audit put it");
            lastReport = now;
        }
        if (!layoutAsExpected) continue;

        // Question 3.
        cooldowns.tick(global(BEAT_TICK_INDEX, -1), [](int beat) { return global(beatCooldownGlobal(beat), 0); });
        if (now - lastReport >= 60000) {
            log(Line() << "cooldowns " << now << " ms: " << cooldowns.ticks << " frames in " << (now - lastReport) << " ms, visits "
                       << cooldowns.visits << ", exact -90 " << cooldowns.exactDrops << ", other change when visited "
                       << cooldowns.otherVisited << ", -90 when not visited " << cooldowns.unvisitedDrops << ", other changes "
                       << cooldowns.otherChanges << ", resyncs " << cooldowns.resyncs << ", beats on cooldown "
                       << cooldowns.beatsOnCooldown());
            cooldowns.clearCounters();
            lastReport = now;
        }
    }
}

extern "C" int __stdcall DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
