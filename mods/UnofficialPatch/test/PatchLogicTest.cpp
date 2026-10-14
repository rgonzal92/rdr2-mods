// Checks PatchLogic's rules without the game. Build and run with `make test`.
#include "../src/PatchLogic.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static void testGlobalIndexes() {
    // Global_1360165 is an array of 27 members of 1157 values behind a length.
    assert(memberGlobal(0, MEMBER_PERSCHAR) == 1360167);
    assert(memberGlobal(0, MEMBER_DECIDE_AGAIN) == 1360222);
    assert(memberGlobal(13, MEMBER_SCHEDULE_ENTRY) == 1360165 + 1 + 1157 * 13 + 47);
    // Global_40.f_4942 is an array of 60 values per member behind a length.
    assert(OUTFIT_TABLE == 4982);
    assert(outfitGlobal(0, OUTFIT_WANTED) == 4986);
    assert(outfitGlobal(26, OUTFIT_LOCK) == 4982 + 1 + 60 * 26 + 59);
}

static void testSceneRadius() {
    assert(nearScene(880.0f, 1270.0f, 234.0f));
    assert(nearScene(880.0f + 109.0f, 1270.0f, 234.0f));
    assert(!nearScene(880.0f + 111.0f, 1270.0f, 234.0f));
    assert(!nearScene(947.0f, 1148.0f, 148.0f));  // where the mission next needs a gun
}

static void testWeaponIsPutAwayOnceAndAgainAfterAWait() {
    UnarmedKeeper keeper;
    assert(keeper.frame(1000, false, false) == UnarmedKeeper::NOTHING);
    assert(keeper.frame(1010, true, false) == UnarmedKeeper::PUT_AWAY);
    assert(keeper.frame(1020, true, false) == UnarmedKeeper::NOTHING);  // give the game time to act
    assert(keeper.frame(1509, true, false) == UnarmedKeeper::NOTHING);
    assert(keeper.frame(1510, true, false) == UnarmedKeeper::PUT_AWAY);
    assert(keeper.frame(1520, false, false) == UnarmedKeeper::NOTHING);  // it worked
    assert(keeper.frame(1530, true, false) == UnarmedKeeper::PUT_AWAY);  // a new draw is a new first attempt
    assert(keeper.attempts == 1);
}

static void testNothingDuringAHoldOrAHolsterChange() {
    UnarmedKeeper keeper;
    assert(keeper.frame(0, true, true) == UnarmedKeeper::NOTHING);
    assert(keeper.attempts == 0);
    assert(keeper.frame(10, true, false) == UnarmedKeeper::PUT_AWAY);
}

static void testGivesUpAfterTenAttemptsUntilThePlayerLeaves() {
    UnarmedKeeper keeper;
    int now = 0, putAway = 0;
    for (; putAway < 10; now += UnarmedKeeper::WAIT_MS)
        if (keeper.frame(now, true, false) == UnarmedKeeper::PUT_AWAY) ++putAway;
    assert(keeper.frame(now, true, false) == UnarmedKeeper::GIVE_UP);
    assert(keeper.frame(now + 100000, true, false) == UnarmedKeeper::NOTHING);
    assert(keeper.frame(now + 100001, false, false) == UnarmedKeeper::NOTHING);
    assert(keeper.frame(now + 100002, true, false) == UnarmedKeeper::NOTHING);  // still given up
    keeper.leave();
    assert(keeper.frame(now + 100003, true, false) == UnarmedKeeper::PUT_AWAY);
}

static MemberState member(int wanted, int who = 11) {
    MemberState state = {};
    state.member = who;
    state.wanted = wanted;
    state.simulated = true;
    return state;
}

static void testWhoIsStuckInNightClothes() {
    const int NIGHTGOWN = -946772361, DAY = 1818898449;
    assert(isNightOutfit(0, -445211559) && isNightOutfit(13, -1205468859) && isNightOutfit(3, 1744281750) && isNightOutfit(11, NIGHTGOWN));
    assert(!isNightOutfit(11, 0) && !isNightOutfit(11, DAY));
    // Bill's night outfit is a day outfit of Sean, Lenny, Abigail, Swanson and Trelawny (def_comp_brain.c 6179, 6187, 6211, 6250, 6262).
    for (int other : {8, 9, 13, 21, 23}) assert(!isNightOutfit(other, 1744281750));
    assert(classify(member(1744281750, 13)) == FINE && classify(member(1744281750, 4)) == STUCK);
    assert(!isNightOutfit(5, -445211559) && !isNightOutfit(12, -445211559));  // Hosea and the dog have none
    assert(CAMP == 4323 && !campDecidesOutfits(0) && !campDecidesOutfits(4) && campDecidesOutfits(1));

    assert(classify(member(NIGHTGOWN)) == STUCK);
    assert(classify(member(DAY)) == FINE);
    MemberState asleep = member(NIGHTGOWN);
    asleep.asleep = true;
    assert(classify(asleep) == FINE);
    // The script will not or cannot decide now: neither stuck nor over.
    MemberState pending = member(NIGHTGOWN), locked = member(NIGHTGOWN), away = member(NIGHTGOWN);
    pending.pending = true;
    locked.locked = true;
    away.simulated = false;
    assert(classify(pending) == WAITING && classify(locked) == WAITING && classify(away) == WAITING);
}

static void testAsksArePaced() {
    AskPacer pacer;
    assert(pacer.check(0, FINE) == AskPacer::NOTHING);
    assert(pacer.check(5000, STUCK) == AskPacer::ASK);
    assert(pacer.check(6000, WAITING) == AskPacer::NOTHING);  // the script has the request
    assert(pacer.check(7000, STUCK) == AskPacer::NOTHING);    // the roll failed, but not yet
    assert(pacer.check(10000, STUCK) == AskPacer::ASK);
    assert(pacer.asks == 2);
    assert(pacer.check(15000, FINE) == AskPacer::CLEARED);
    assert(pacer.asks == 0);
    assert(pacer.check(16000, FINE) == AskPacer::NOTHING);
}

static void testAsksSlowDownWhenNothingHelps() {
    AskPacer pacer;
    int now = 0;
    for (int ask = 0; ask < AskPacer::QUICK_ASKS; ++ask, now += AskPacer::QUICK_MS) assert(pacer.check(now, STUCK) == AskPacer::ASK);
    assert(pacer.check(now, STUCK) == AskPacer::ASK);  // the last quick wait has passed
    assert(pacer.check(now + AskPacer::QUICK_MS, STUCK) == AskPacer::NOTHING);
    assert(pacer.check(now + AskPacer::SLOW_MS, STUCK) == AskPacer::ASK);
}

static void testLine() {
    Line line;
    line << "value " << -2147483647 - 1 << " " << 0 << " " << Hex{0xA11BDA18};
    assert(std::strcmp(line.text, "value -2147483648 0 0xA11BDA18") == 0);
}

// The game as the audit describes it and PatchProbe saw it: each frame, 90 off the beat the index points at.
struct FakeGame {
    int cooldown[BEAT_COUNT] = {};
    int index = 0;
    int now = 1000;
    int writes = 0;
    void frame(int milliseconds) {
        if (cooldown[index] > 0) cooldown[index] -= COOLDOWN_STEP;
        index = (index + 1) % BEAT_COUNT;
        now += milliseconds;
    }
};

static void run(CooldownClock& clock, FakeGame& game) {
    clock.tick(game.now, game.index, [&](int i) { return game.cooldown[i]; }, [&](int i, int value) {
        assert(value <= clock.snapshot[i] && value >= clock.snapshot[i] - COOLDOWN_STEP);  // never outside what the game produces
        assert(game.cooldown[i] == clock.snapshot[i] - COOLDOWN_STEP);                     // only right after the game's own step
        assert(value > 0);
        ++game.writes;
        return game.cooldown[i] = value;
    });
}

// Units a second that a long cooldown loses over `seconds` at a frame time alternating a and b.
static double rate(int referenceFps, int a, int b, int seconds) {
    FakeGame game;
    game.cooldown[7] = 1036800;  // "12 days"
    CooldownClock clock;
    clock.referenceFps = referenceFps;
    clock.restart();
    run(clock, game);
    const int start = game.now;
    for (int frame = 0; game.now - start < seconds * 1000; ++frame) {
        game.frame(frame % 2 ? a : b);
        run(clock, game);
    }
    return (1036800 - game.cooldown[7]) * 1000.0 / (game.now - start);
}

static void testCooldownRunsAtTheReferenceRate() {
    assert(BEAT_TABLE == 17546 && beatCooldownGlobal(0) == 17551 && beatCooldownGlobal(119) == 18503 && BEAT_TICK_INDEX == 1326863);
    assert(STORY_MARKER == 1572899);
    const double uncorrected = rate(0, 6, 7, 600);  // about 154 fps, as under the 157 cap
    assert(uncorrected > 114 && uncorrected < 117);
    const double at30 = rate(30, 6, 7, 600), at40 = rate(40, 6, 7, 600), at60 = rate(60, 6, 7, 600);
    assert(at30 > 22.4 && at30 < 22.9);
    assert(at40 > 29.9 && at40 < 30.4);
    assert(at60 > 44.9 && at60 < 45.4);
    // At or below the reference rate nothing is given back.
    const double slow = rate(60, 33, 33, 600);
    assert(slow > 22.4 && slow < 23.0);
}

static void testCooldownLeavesAnExpiringBeatAlone() {
    FakeGame game;
    game.cooldown[0] = 270;
    CooldownClock clock;
    clock.referenceFps = 30;
    clock.restart();
    run(clock, game);
    bool wasAvailable = false;
    for (int frame = 0; frame < 120 * 40; ++frame) {
        game.frame(6);
        run(clock, game);
        if (game.cooldown[0] <= 0) wasAvailable = true;
        assert(!wasAvailable || game.cooldown[0] <= 0);  // once available, it stays available
    }
    assert(wasAvailable);
}

static void testCooldownAdoptsOtherWriters() {
    FakeGame game;
    game.cooldown[5] = 9000;
    CooldownClock clock;
    clock.referenceFps = 30;
    clock.restart();
    run(clock, game);
    for (int frame = 0; frame < 300; ++frame) {
        game.frame(6);
        if (frame == 125) game.cooldown[5] = 0;       // a script clears the beat in the frame of its visit
        if (frame == 130) game.cooldown[60] = 86400;  // and sets another one
        run(clock, game);
    }
    assert(game.cooldown[5] == 0);
    assert(clock.failedWrites == 0);
    // A jump of the index (a loading screen) starts the timing again without a write.
    const int writes = game.writes;
    for (int frame = 0; frame < 30; ++frame) game.frame(6);
    run(clock, game);
    assert(game.writes == writes && clock.lastVisit[60] == 0);
}

static void testReferenceFpsSetting() {
    assert(validReferenceFps(0) == 0 && validReferenceFps(30) == 30 && validReferenceFps(1000) == 1000);
    assert(validReferenceFps(-30) == 0 && validReferenceFps(3) == 0 && validReferenceFps(1001) == 0);
}

static void testTrainMasks() {
    assert(TRAIN_TABLE == 11069 && trainFlagsGlobal(0) == 11070 && trainFlagsGlobal(5) == 11095 && TOWN_REGION == 1894901);
    assert(trainStuck(0x2, -1) && trainStuck(0x2 | 0x4 | 0x2000, -1));
    assert(!trainStuck(0x2, 3));            // in a town region the script handles it
    assert(!trainStuck(0x4, -1));           // mask 0x4 is another flag, not "bit 2"
    assert(!trainStuck(0x2 | 0x10, -1) && !trainStuck(0x2 | 0x10000, -1));
}

static void testDreamcatchers() {
    assert(DREAM_TABLE == 8903 && DREAM_ARROWHEAD == 8904 && DREAM_FOUND == 9051);
    assert(dreamcatchersDone(0x1FFFFE, 2) && dreamcatchersDone(0x1FFFFF, 3));
    assert(!dreamcatchersDone(0x1FFFFC, 2));  // one dreamcatcher missing
    assert(!dreamcatchersDone(0x1FFFFE, 0));  // arrowhead not taken
    StreakGate gate;
    gate.needed = 3;
    assert(!gate.step(true) && !gate.step(true) && gate.step(true));
    assert(!gate.step(true));  // once per streak
    assert(!gate.step(false) && !gate.step(true) && !gate.step(true) && gate.step(true));
}

static void testFishMarks() {
    assert(FISH_COUNT == 13 && FISH[0].caught == 0xD68748A9 && FISH[12].uncaught == 0xBBE7B206);
    assert(fishMarkWrong(1, 1, true, false) && fishMarkWrong(1, 1, false, false) && fishMarkWrong(1, 1, true, true));
    assert(!fishMarkWrong(1, 1, false, true));  // what the game leaves after a mailing
    assert(!fishMarkWrong(0, 1, true, false));  // not mailed
    assert(!fishMarkWrong(1, 0, true, false));  // spot not found
    Hash catalog[15];
    for (int i = 0; i < 14; ++i) catalog[i] = FISH_COLLECTABLES[13 - i];
    assert(fishCatalogMatches(catalog, 14));
    catalog[3] = 0;
    assert(!fishCatalogMatches(catalog, 14));
}

static void testKillcamWatch() {
    assert(nearKillcam(1344.794f, -1372.709f, 84.588f) && nearKillcam(1344.4f, -1372.5f, 84.5f) && !nearKillcam(1342.3f, -1370.66f, 84.5f));
    KillcamWatch watch;
    assert(watch.frame(1000, true, false) == KillcamWatch::STARTED);
    for (int t = 1050; t <= 2500; t += 50) assert(watch.frame(t, true, false) == KillcamWatch::NOTHING);
    assert(watch.frame(2700, false, false) == KillcamWatch::NOTHING);  // gone for under 250 ms
    assert(watch.frame(2800, false, false) == KillcamWatch::ENDED && watch.shownMs == 1500);  // the normal kill camera
    assert(watch.frame(10000, true, false) == KillcamWatch::STARTED);
    assert(watch.frame(10050, true, true) == KillcamWatch::NOTHING);       // paused: does not count
    for (int t = 10100; t < 30000; t += 100) assert(watch.frame(t, true, true) == KillcamWatch::NOTHING);
    assert(watch.guardedMs > 19000 && watch.shownMs < 100);
    int now = 30000;
    KillcamWatch::Event event = KillcamWatch::NOTHING;
    for (; event == KillcamWatch::NOTHING; now += 100) event = watch.frame(now, true, false);
    assert(event == KillcamWatch::STUCK && watch.shownMs >= KillcamWatch::STUCK_MS && now < 33300);
    assert(watch.frame(now, true, false) == KillcamWatch::NOTHING);  // said once
}

static void testRanchChores() {
    assert(RANCH_CHORES == 7894 && CAMP_BEECHERS_HOPE == 8);
    assert(choresNeedStall(0x1F) && choresNeedStall(0x5F));                 // after "The Best of Women", Granger cleared it
    assert(!choresNeedStall(0x3F) && !choresNeedStall(0x2F) && !choresNeedStall(0x0F) && !choresNeedStall(0x0B) && !choresNeedStall(0));
    assert(!choresNeedStall(0x10) && !choresNeedStall(0x19));               // bits 0, 1 and 3 must be there too
    assert(!choresNeedStall(0x9F) && !choresNeedStall(-1));                 // only types 0 to 6 exist
}

static JerusalemState stuck() {
    JerusalemState state = {};
    state.freeRoam = true;
    state.onBackup = true;
    state.epilogueWeapons = true;
    state.weaponsInMain = 5;
    state.ownerSeen = true;
    return state;
}

static void testJerusalem() {
    assert(WEAPON_ITEM_COUNT > 50);
    assert(jerusalemVerdict(stuck(), 2) == SWITCH);
    assert(jerusalemVerdict(stuck(), 1) == WOULD_SWITCH);
    assert(jerusalemVerdict(stuck(), 0) == LEAVE);
    JerusalemState state = stuck();
    state.ownerSeen = false;  // stuck when the save was loaded: only with mode 3
    assert(jerusalemVerdict(state, 2) == ASK_FIRST && jerusalemVerdict(state, 3) == SWITCH);
    state = stuck();
    state.weaponsInMain = 0;  // nothing to go back to
    assert(jerusalemVerdict(state, 3) == MAIN_EMPTY);
    state = stuck();
    state.epilogueWeapons = false;  // Guarma or epilogue part 1: the backup inventory is meant
    assert(jerusalemVerdict(state, 3) == LEAVE);
    state = stuck();
    state.ownerRunning = true;  // a bath or the mission itself
    assert(jerusalemVerdict(state, 3) == LEAVE);
    state = stuck();
    state.freeRoam = false;
    assert(jerusalemVerdict(state, 3) == LEAVE);
    state = stuck();
    state.onBackup = false;
    assert(jerusalemVerdict(state, 3) == LEAVE);
}

int main() {
    testGlobalIndexes();
    testSceneRadius();
    testWeaponIsPutAwayOnceAndAgainAfterAWait();
    testNothingDuringAHoldOrAHolsterChange();
    testGivesUpAfterTenAttemptsUntilThePlayerLeaves();
    testWhoIsStuckInNightClothes();
    testAsksArePaced();
    testAsksSlowDownWhenNothingHelps();
    testCooldownRunsAtTheReferenceRate();
    testCooldownLeavesAnExpiringBeatAlone();
    testCooldownAdoptsOtherWriters();
    testReferenceFpsSetting();
    testTrainMasks();
    testDreamcatchers();
    testFishMarks();
    testKillcamWatch();
    testRanchChores();
    testJerusalem();
    testLine();
    std::puts("PatchLogic: all checks passed");
}
