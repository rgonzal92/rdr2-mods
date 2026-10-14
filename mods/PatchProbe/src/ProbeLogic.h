// The parts of PatchProbe that need no game: hashes, global indexes, the log line and the
// bookkeeping that watches the random-encounter cooldown table. Checked by `make test`.
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
static_assert(joaat("short_update") == 0xA11BDA18 && joaat("long_update") == 0x64CD15ED && joaat("main") == 0x27EB33D7,
              "joaat must reproduce the script hashes");

// Global indexes of game build 1.0.1491.50, as derived in the audit of 2026-10-13
// (rdr2-tools/audit-2026-10-13-unofficial-patch). An array's own index holds its length and
// element i of size S starts at index + 1 + S * i.
const int BEAT_TABLE = 17504 + 42;  // Global_17504.f_42: 120 random encounters ("beats"), 8 values each
const int BEAT_COUNT = 120;
const int BEAT_SIZE = 8;
const int BEAT_COOLDOWN_FIELD = 4;         // .f_4: the cooldown, in nominal game seconds
const int BEAT_TICK_INDEX = 1326862 + 1;   // Global_1326862.f_1: the beat short_update visits this frame
const int COOLDOWN_STEP = 90;              // what short_update takes off the visited beat
constexpr int beatCooldownGlobal(int beat) { return BEAT_TABLE + 1 + BEAT_SIZE * beat + BEAT_COOLDOWN_FIELD; }
static_assert(beatCooldownGlobal(0) == 17551 && beatCooldownGlobal(119) == 18503, "the cooldown cells must be the audited ones");

// Cells whose values the audit derived offline. The probe prints what the game really holds.
struct LayoutCell {
    int global;
    int expected;
    const char* name;
};
constexpr LayoutCell LAYOUT[] = {
    {17546, 120, "beat table length"},
    {18507, 180, "length after the beat table"},
    {19588, 24, "used-location table length"},
    {19709, 500, "length after the used-location table"},
    {8903, 143, "dreamcatcher array length"},
    {11069, 13, "train table length"},
    {1347400, 45, "inventory layout cell A"},
    {1347446, 29, "inventory layout cell B"},
    {1572899, -1, "story-mode marker"},
};

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

// Watches the cooldown table frame by frame and sorts every change it sees. The audit says:
// each frame short_update takes exactly 90 off the one beat its index points at, if that beat
// is above 0, and nothing else ever lowers a cooldown. The counters show whether that holds.
struct CooldownWatch {
    bool primed = false;
    int previousIndex = 0;
    int snapshot[BEAT_COUNT];

    int ticks = 0;              // frames watched
    int visits = 0;             // visited beats that were above 0
    int exactDrops = 0;         // of those, how many dropped by exactly 90
    int otherVisited = 0;       // visited beats that changed in any other way
    int unvisitedDrops = 0;     // beats not visited that dropped by exactly 90: must stay 0
    int otherChanges = 0;       // any other change (the game setting or clearing a cooldown)
    int resyncs = 0;            // frames where the index jumped and the snapshot was retaken

    void clearCounters() { ticks = visits = exactDrops = otherVisited = unvisitedDrops = otherChanges = resyncs = 0; }
    void restart() {
        primed = false;
        clearCounters();
    }

    // `index` is the game's tick index as read this frame (the beat it will visit next), and
    // `cooldown(i)` reads beat i. An index outside the table is not counted.
    template <typename Read>
    void tick(int index, Read cooldown) {
        if (index < 0 || index >= BEAT_COUNT) return;
        if (!primed) {
            for (int i = 0; i < BEAT_COUNT; ++i) snapshot[i] = cooldown(i);
            previousIndex = index;
            primed = true;
            return;
        }
        const int visited = (index - previousIndex + BEAT_COUNT) % BEAT_COUNT;
        if (visited > 8) {  // a loading screen or a long hitch: nothing can be attributed
            ++resyncs;
            for (int i = 0; i < BEAT_COUNT; ++i) snapshot[i] = cooldown(i);
            previousIndex = index;
            return;
        }
        ++ticks;
        for (int i = 0; i < BEAT_COUNT; ++i) {
            const int now = cooldown(i);
            const int before = snapshot[i];
            const bool wasVisited = (i - previousIndex + BEAT_COUNT) % BEAT_COUNT < visited;
            if (wasVisited && before > 0) ++visits;
            if (now != before) {
                const bool exact = before - now == COOLDOWN_STEP;
                if (wasVisited && before > 0 && exact) ++exactDrops;
                else if (wasVisited) ++otherVisited;
                else if (exact) ++unvisitedDrops;
                else ++otherChanges;
            }
            snapshot[i] = now;
        }
        previousIndex = index;
    }

    int beatsOnCooldown() const {
        int count = 0;
        for (int i = 0; i < BEAT_COUNT; ++i) count += snapshot[i] > 0;
        return primed ? count : 0;
    }
};
