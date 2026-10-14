// Checks ProbeLogic's bookkeeping without the game. Build and run with `make test`.
#include "../src/ProbeLogic.h"

#include <cassert>
#include <cstdio>
#include <cstring>

// The game as the audit describes it: each frame, 90 off the beat the index points at.
struct FakeGame {
    int cooldown[BEAT_COUNT] = {};
    int index = 0;
    void frame() {
        if (cooldown[index] > 0) cooldown[index] -= COOLDOWN_STEP;
        index = (index + 1) % BEAT_COUNT;
    }
};

static void watch(CooldownWatch& watch, const FakeGame& game) {
    watch.tick(game.index, [&](int i) { return game.cooldown[i]; });
}

static void testGameAsAudited() {
    FakeGame game;
    game.cooldown[3] = 1000;
    game.cooldown[70] = 45;  // ends below zero, still a drop of exactly 90
    CooldownWatch probe;
    watch(probe, game);
    for (int frame = 0; frame < 240; ++frame) {
        game.frame();
        watch(probe, game);
    }
    assert(probe.ticks == 240);
    assert(probe.visits == 3);  // beat 3 twice, beat 70 once
    assert(probe.exactDrops == 3);
    assert(probe.otherVisited == 0 && probe.unvisitedDrops == 0 && probe.otherChanges == 0 && probe.resyncs == 0);
    assert(probe.beatsOnCooldown() == 1);
}

static void testOtherWritersAreSortedApart() {
    FakeGame game;
    CooldownWatch probe;
    watch(probe, game);
    game.frame();
    game.cooldown[50] = 86400;  // the game starts a cooldown on a beat it did not visit
    watch(probe, game);
    assert(probe.otherChanges == 1 && probe.unvisitedDrops == 0);
    game.frame();
    game.cooldown[50] -= COOLDOWN_STEP;  // a drop of 90 on a beat that was not visited: the audit says never
    watch(probe, game);
    assert(probe.unvisitedDrops == 1);
}

static void testTwoFramesBetweenReads() {
    FakeGame game;
    game.cooldown[0] = 500;
    game.cooldown[1] = 500;
    CooldownWatch probe;
    watch(probe, game);
    game.frame();
    game.frame();
    watch(probe, game);
    assert(probe.visits == 2 && probe.exactDrops == 2);
}

static void testIndexJumpRetakesTheSnapshot() {
    FakeGame game;
    game.cooldown[10] = 500;
    CooldownWatch probe;
    watch(probe, game);
    for (int frame = 0; frame < 30; ++frame) game.frame();
    watch(probe, game);
    assert(probe.resyncs == 1 && probe.ticks == 0 && probe.visits == 0 && probe.otherChanges == 0);
    game.index = -1;  // an index outside the table is ignored
    watch(probe, game);
    assert(probe.resyncs == 1 && probe.ticks == 0);
}

static void testLine() {
    Line line;
    line << "value " << -2147483647 - 1 << " " << 0 << " " << Hex{0xA11BDA18};
    assert(std::strcmp(line.text, "value -2147483648 0 0xA11BDA18") == 0);
}

int main() {
    testGameAsAudited();
    testOtherWritersAreSortedApart();
    testTwoFramesBetweenReads();
    testIndexJumpRetakesTheSnapshot();
    testLine();
    std::puts("ProbeLogic: all checks passed");
}
