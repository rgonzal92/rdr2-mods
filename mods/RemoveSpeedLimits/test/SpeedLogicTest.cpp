// Checks SpeedLogic's decisions without the game. Build and run with `make test`.

#include "../src/SpeedLogic.h"

#include <cassert>
#include <cstdio>
#include <initializer_list>

const bool ON_FOOT = true, RIDING = false;
const bool MOVING = true, STILL = false;
const bool SPRINT_KEY = true, NO_SPRINT_KEY = false;
const int TAP_TO_WALK = 250, NO_TAP_TO_WALK = 0;  // the longest tap, in milliseconds
const float FAST_WALK = 1.5f;

static bool leftToTheGame(Pace pace) { return pace.atLeast == 0 && pace.atMost == 0; }
static bool heldAt(Pace pace, float ratio) { return pace.atLeast == ratio && pace.atMost == ratio; }
static bool cappedAtAWalk(Pace pace) { return pace.atLeast == 0 && pace.atMost == 1.0f; }

// Sprints from a walk for a second and lets go, which leaves the player running. Ends at 1000 ms.
static FootPace running(int tapMs) {
    FootPace foot(FAST_WALK, tapMs);
    foot.update(ON_FOOT, MOVING, SPRINT_KEY, 0);
    foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1000);
    return foot;
}

static void walkingSpeedIsAPercentageFrom100To300() {
    assert(walkRatio(150) == 1.5f);
    assert(walkRatio(100) == 1.0f);
    assert(walkRatio(300) == 3.0f);
    assert(walkRatio(50) == 1.0f);
    assert(walkRatio(1000) == 3.0f);
    assert(walkRatio(0) == 1.5f);  // missing; the original took this as 0, which stopped the player
    assert(walkRatio(-5) == 1.5f);
}

static void thePlayerIsLeftAloneUntilAKeyAsksOtherwise() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 0)));
    assert(leftToTheGame(foot.update(ON_FOOT, STILL, NO_SPRINT_KEY, 16)));
}

static void theSprintKeyHoldsASprintAtOnceFromAWalk() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 0), 10.6f));
}

static void theSprintKeyAloneDoesNotMoveThePlayer() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    // The original held a sprint here too, with no direction pressed.
    assert(leftToTheGame(foot.update(ON_FOOT, STILL, SPRINT_KEY, 0)));
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 16)));
}

static void afterASprintThePlayerKeepsRunningUntilTheyStop() {
    FootPace foot = running(TAP_TO_WALK);
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1016), 2.0f));
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 9000), 2.0f));
    assert(leftToTheGame(foot.update(ON_FOOT, STILL, NO_SPRINT_KEY, 9016)));
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 9032)));
}

static void aTapWhileRunningDropsToAWalk() {
    FootPace foot = running(TAP_TO_WALK);
    // Until the key comes up it could still become a sprint, so the run goes on.
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2000), 2.0f));
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2100), 2.0f));
    assert(cappedAtAWalk(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2116)));
    assert(cappedAtAWalk(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 60000)));
}

static void holdingTheKeyWhileRunningSprintsAfterAQuarterSecond() {
    FootPace foot = running(TAP_TO_WALK);
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2000), 2.0f));
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2249), 2.0f));
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2250), 10.6f));
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 3000), 2.0f));  // a hold is not a tap
}

static void aTapWhileWalkingRunsAgain() {
    FootPace foot = running(TAP_TO_WALK);
    foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2000);
    foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2100);
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 3000), 10.6f));  // from a walk there is no wait
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 3100), 2.0f));
}

static void stoppingEndsTheWalk() {
    FootPace foot = running(TAP_TO_WALK);
    foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2000);
    foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2100);
    assert(leftToTheGame(foot.update(ON_FOOT, STILL, NO_SPRINT_KEY, 2200)));
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2300)));
}

static void withTapToWalkOffATapWhileRunningSprintsAndRunsOn() {
    FootPace foot = running(NO_TAP_TO_WALK);
    assert(heldAt(foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2000), 10.6f));
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2100), 2.0f));
}

static void theCoverKeySwitchesFastWalkOnAndOffWhileMoving() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    foot.coverKeyPressed(MOVING, 1000);
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1016), FAST_WALK));
    assert(leftToTheGame(foot.update(ON_FOOT, STILL, NO_SPRINT_KEY, 1032)));
    foot.coverKeyPressed(MOVING, 2000);
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2016)));
}

static void aCoverPressFromAStandstillOnlyTakesCover() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    // That press goes to the game. The original switched fast walk on it as well.
    foot.coverKeyPressed(STILL, 1000);
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1016)));
}

static void fastWalkWaitsWhileThePlayerRunsAndIsTheWalkATapReturnsTo() {
    FootPace foot = running(TAP_TO_WALK);
    foot.coverKeyPressed(MOVING, 1500);
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1516), 2.0f));
    foot.update(ON_FOOT, MOVING, SPRINT_KEY, 2000);
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 2100), FAST_WALK));
}

static void aSecondPressWithinATenthOfASecondIsIgnored() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    foot.coverKeyPressed(MOVING, 1000);
    foot.coverKeyPressed(MOVING, 1099);
    assert(heldAt(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1100), FAST_WALK));
    foot.coverKeyPressed(MOVING, 1100);
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 1116)));
}

static void ridingLeavesThePlayersOwnPaceToTheGame() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    foot.coverKeyPressed(MOVING, 1000);
    // The same keys then drive the horse. The original held the rider at a sprint as well.
    assert(leftToTheGame(foot.update(RIDING, MOVING, SPRINT_KEY, 1016)));
    assert(leftToTheGame(foot.update(RIDING, MOVING, NO_SPRINT_KEY, 1032)));
}

static void aGallopIsNotRememberedAsASprintOnFoot() {
    FootPace foot(FAST_WALK, TAP_TO_WALK);
    foot.update(RIDING, MOVING, SPRINT_KEY, 0);
    assert(leftToTheGame(foot.update(ON_FOOT, MOVING, NO_SPRINT_KEY, 16)));
}

const bool GALLOPING = true, FORWARD_KEY = true, BACK_KEY = true;

static void aHorseIsLeftAloneUntilAKeyAsksOtherwise() {
    for (bool galloping : {false, true}) assert(leftToTheGame(HorsePace().update(galloping, NO_SPRINT_KEY, !FORWARD_KEY, !BACK_KEY)));
}

static void forwardOrSprintLiftsTheCapWithoutForcingAPace() {
    // The original set a minimum of 1.6 with forward held, so a cantering horse could not slow
    // to a trot or keep pace with a slower companion.
    for (const Pace pace : {HorsePace().update(!GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY), HorsePace().update(!GALLOPING, SPRINT_KEY, FORWARD_KEY, !BACK_KEY),
                            HorsePace().update(GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY)}) {
        assert(pace.atLeast == 0);
        assert(pace.atMost == 10.0f);
    }
}

static void holdingSprintBelowAGallopDoesNotForceOne() {
    // The game reads that hold as "keep this pace" and "match my companion's speed".
    const Pace pace = HorsePace().update(!GALLOPING, SPRINT_KEY, !FORWARD_KEY, !BACK_KEY);
    assert(pace.atLeast == 0);
    assert(pace.atMost == 10.0f);
}

static void holdingSprintAtAGallopKeepsTheGallop() {
    for (bool forward : {false, true}) assert(heldAt(HorsePace().update(GALLOPING, SPRINT_KEY, forward, !BACK_KEY), 10.0f));
}

static void aGallopFromTheSprintKeyLastsWhileForwardIsHeld() {
    HorsePace horse;
    assert(heldAt(horse.update(GALLOPING, SPRINT_KEY, FORWARD_KEY, !BACK_KEY), 10.0f));
    // Sprint let go: forward alone keeps the gallop, frame after frame.
    for (int frame = 0; frame < 600; ++frame) assert(heldAt(horse.update(GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY), 10.0f));
}

static void lettingGoOfForwardEndsTheGallop() {
    HorsePace horse;
    horse.update(GALLOPING, SPRINT_KEY, FORWARD_KEY, !BACK_KEY);
    assert(leftToTheGame(horse.update(GALLOPING, NO_SPRINT_KEY, !FORWARD_KEY, !BACK_KEY)));
    // Forward again does not bring the gallop back without the sprint key.
    const Pace pace = horse.update(GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY);
    assert(pace.atLeast == 0 && pace.atMost == 10.0f);
}

static void theBackKeyEndsTheGallop() {
    HorsePace horse;
    horse.update(GALLOPING, SPRINT_KEY, FORWARD_KEY, !BACK_KEY);
    assert(horse.update(GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, BACK_KEY).atLeast == 0);
    assert(horse.update(GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY).atLeast == 0);
}

static void theBackKeyEndsTheGallopWhileSprintIsHeld() {
    // A gallop held at its top pace would fight the brake.
    for (bool forward : {false, true}) {
        HorsePace horse;
        horse.update(GALLOPING, SPRINT_KEY, FORWARD_KEY, !BACK_KEY);
        assert(horse.update(GALLOPING, SPRINT_KEY, forward, BACK_KEY).atLeast == 0);
    }
}

static void aHorseThatStopsGallopingIsNotForcedBack() {
    // A fall, an obstacle, a tired horse or a scene: the gallop is not pushed back on.
    HorsePace horse;
    horse.update(GALLOPING, SPRINT_KEY, FORWARD_KEY, !BACK_KEY);
    assert(horse.update(!GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY).atLeast == 0);
    assert(horse.update(GALLOPING, NO_SPRINT_KEY, FORWARD_KEY, !BACK_KEY).atLeast == 0);
}

int main() {
    walkingSpeedIsAPercentageFrom100To300();
    thePlayerIsLeftAloneUntilAKeyAsksOtherwise();
    theSprintKeyHoldsASprintAtOnceFromAWalk();
    theSprintKeyAloneDoesNotMoveThePlayer();
    afterASprintThePlayerKeepsRunningUntilTheyStop();
    aTapWhileRunningDropsToAWalk();
    holdingTheKeyWhileRunningSprintsAfterAQuarterSecond();
    aTapWhileWalkingRunsAgain();
    stoppingEndsTheWalk();
    withTapToWalkOffATapWhileRunningSprintsAndRunsOn();
    theCoverKeySwitchesFastWalkOnAndOffWhileMoving();
    aCoverPressFromAStandstillOnlyTakesCover();
    fastWalkWaitsWhileThePlayerRunsAndIsTheWalkATapReturnsTo();
    aSecondPressWithinATenthOfASecondIsIgnored();
    ridingLeavesThePlayersOwnPaceToTheGame();
    aGallopIsNotRememberedAsASprintOnFoot();
    aHorseIsLeftAloneUntilAKeyAsksOtherwise();
    forwardOrSprintLiftsTheCapWithoutForcingAPace();
    holdingSprintBelowAGallopDoesNotForceOne();
    holdingSprintAtAGallopKeepsTheGallop();
    aGallopFromTheSprintKeyLastsWhileForwardIsHeld();
    lettingGoOfForwardEndsTheGallop();
    theBackKeyEndsTheGallop();
    theBackKeyEndsTheGallopWhileSprintIsHeld();
    aHorseThatStopsGallopingIsNotForcedBack();
    std::puts("SpeedLogic: all checks passed");
}
