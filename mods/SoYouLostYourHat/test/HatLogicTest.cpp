// Checks HatLogic's decisions without the game. Build and run with `make test`.

#include "../src/HatLogic.h"

#include <cassert>
#include <cstdio>

const bool ACTIVE = true, INACTIVE = false;
const bool OUR_BLIP = true, NO_OUR_BLIP = false;
const bool OTHER_BLIP = true, NO_OTHER_BLIP = false;
const int HAT = 0x1234, OTHER_HAT = 0x5678, NO_HAT = 0;
const float RESTING = 0.0f, ROLLING = 3.0f, DRIFTING = 0.6f, JITTER = 0.2f;

// A hat that came to rest at 0 ms and was blipped on the first frame.
static HatTracker blipped() {
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 0) == HatAction::ADD_BLIP);
    t.blipAdded(true);
    return t;
}

static void nothingHappensWithoutAHat() {
    HatTracker t;
    assert(t.update(ACTIVE, NO_HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 0) == HatAction::NONE);
    assert(t.update(ACTIVE, NO_HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 99999) == HatAction::NONE);
}

static void nothingHappensBeforeTheFirstMissionOrWhileThePlayerIsNotPlaying() {
    HatTracker t;
    assert(t.update(INACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 0) == HatAction::NONE);
    assert(t.update(INACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 99999) == HatAction::NONE);
}

static void aHatAtRestIsBlippedAtOnce() {
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 0) == HatAction::ADD_BLIP);
}

static void aHatThatStillJittersCountsAsAtRest() {
    // The original needed a speed of exactly 0.
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, JITTER, 0) == HatAction::ADD_BLIP);
}

static void aRollingHatWaitsUntilItSettles() {
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, ROLLING, 0) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, ROLLING, 500) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 516) == HatAction::ADD_BLIP);
}

static void aHatThatNeverSettlesIsBlippedAfterTheSettleTime() {
    // A hat drifting in a river. The original never blipped it.
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, DRIFTING, 0) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, DRIFTING, HatTracker::SETTLE_MS - 1) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, DRIFTING, HatTracker::SETTLE_MS) == HatAction::ADD_BLIP);
}

static void theSettleTimeSurvivesTheGameTimerWrapping() {
    HatTracker t;
    const int start = 0x7FFFFF00;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, DRIFTING, start) == HatAction::NONE);
    const int later = static_cast<int>(static_cast<unsigned>(start) + HatTracker::SETTLE_MS);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, DRIFTING, later) == HatAction::ADD_BLIP);
}

static void aHatAnotherScriptHasBlippedIsLeftToIt() {
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, OTHER_BLIP, RESTING, 0) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, OTHER_BLIP, RESTING, 99999) == HatAction::NONE);
    // Once that script removes its blip, the hat is still lost and gets ours.
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 100000) == HatAction::ADD_BLIP);
}

static void theBlipIsAddedOnlyOnce() {
    HatTracker t = blipped();
    assert(t.update(ACTIVE, HAT, OUR_BLIP, NO_OTHER_BLIP, RESTING, 16) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, OUR_BLIP, NO_OTHER_BLIP, ROLLING, 99999) == HatAction::NONE);
}

static void aFailedAddIsTriedAgain() {
    HatTracker t;
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 0) == HatAction::ADD_BLIP);
    t.blipAdded(false);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 16) == HatAction::ADD_BLIP);
}

static void theBlipGoesWhenTheHatIsPickedUpOrGone() {
    // The plugin is told NO_HAT once the hat no longer exists or is carried.
    HatTracker t = blipped();
    assert(t.update(ACTIVE, NO_HAT, OUR_BLIP, NO_OTHER_BLIP, RESTING, 16) == HatAction::REMOVE_BLIP);
    assert(t.update(ACTIVE, NO_HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 32) == HatAction::NONE);
}

static void wearingASpareHatKeepsTheBlip() {
    // Nothing about worn hats reaches the tracker: only the lost hat itself counts.
    HatTracker t = blipped();
    for (int now = 16; now < 60000; now += 16) {
        assert(t.update(ACTIVE, HAT, OUR_BLIP, NO_OTHER_BLIP, RESTING, now) == HatAction::NONE);
    }
}

static void aNewlyLostHatMovesTheBlip() {
    HatTracker t = blipped();
    assert(t.update(ACTIVE, OTHER_HAT, OUR_BLIP, NO_OTHER_BLIP, ROLLING, 16) == HatAction::REMOVE_BLIP);
    // The new hat waits to settle from the moment it was seen.
    assert(t.update(ACTIVE, OTHER_HAT, NO_OUR_BLIP, NO_OTHER_BLIP, ROLLING, 32) == HatAction::NONE);
    assert(t.update(ACTIVE, OTHER_HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 48) == HatAction::ADD_BLIP);
}

static void theBlipGoesWhileThePlayerIsNotPlayingAndComesBackAfter() {
    HatTracker t = blipped();
    assert(t.update(INACTIVE, HAT, OUR_BLIP, NO_OTHER_BLIP, RESTING, 16) == HatAction::REMOVE_BLIP);
    assert(t.update(INACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 32) == HatAction::NONE);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 48) == HatAction::ADD_BLIP);
}

static void aBlipRemovedFromOutsideIsAddedAgainUnlessAnotherScriptBlippedTheHat() {
    HatTracker t = blipped();
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, NO_OTHER_BLIP, RESTING, 16) == HatAction::ADD_BLIP);
    t.blipAdded(true);
    assert(t.update(ACTIVE, HAT, NO_OUR_BLIP, OTHER_BLIP, RESTING, 32) == HatAction::NONE);
}

int main() {
    nothingHappensWithoutAHat();
    nothingHappensBeforeTheFirstMissionOrWhileThePlayerIsNotPlaying();
    aHatAtRestIsBlippedAtOnce();
    aHatThatStillJittersCountsAsAtRest();
    aRollingHatWaitsUntilItSettles();
    aHatThatNeverSettlesIsBlippedAfterTheSettleTime();
    theSettleTimeSurvivesTheGameTimerWrapping();
    aHatAnotherScriptHasBlippedIsLeftToIt();
    theBlipIsAddedOnlyOnce();
    aFailedAddIsTriedAgain();
    theBlipGoesWhenTheHatIsPickedUpOrGone();
    wearingASpareHatKeepsTheBlip();
    aNewlyLostHatMovesTheBlip();
    theBlipGoesWhileThePlayerIsNotPlayingAndComesBackAfter();
    aBlipRemovedFromOutsideIsAddedAgainUnlessAnotherScriptBlippedTheHat();
    std::puts("HatLogic: all checks passed");
}
