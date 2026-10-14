// Checks WeaponCare's decisions without the game. Build and run with `make test`.

#include "../src/WeaponCare.h"

#include <cassert>
#include <cstdio>

const bool LESSON = true, NO_LESSON = false;
const bool SHOTGUN = true, OTHER_GUN = false;

static void aWornWeaponIsCleanedAsFarAsTheGameCleansIt() {
    assert(wearToWrite(NO_LESSON, OTHER_GUN, 0.4f, 0.0f) == 0.0f);
    assert(wearToWrite(NO_LESSON, SHOTGUN, 1.0f, 0.0f) == 0.0f);
    // A picked-up gun keeps the permanent wear that no cleaning removes; the original wrote 0.
    assert(wearToWrite(NO_LESSON, OTHER_GUN, 0.9f, 0.25f) == 0.25f);
}

static void aCleanWeaponIsLeftAlone() {
    assert(wearToWrite(NO_LESSON, OTHER_GUN, 0.0f, 0.0f) == LEAVE_WEAR);
    assert(wearToWrite(NO_LESSON, OTHER_GUN, 0.25f, 0.25f) == LEAVE_WEAR);
    assert(wearToWrite(NO_LESSON, OTHER_GUN, 0.0f, 0.25f) == LEAVE_WEAR);
}

static void nothingIsCleanedDuringTheCleaningLesson() {
    assert(wearToWrite(LESSON, OTHER_GUN, 0.8f, 0.0f) == LEAVE_WEAR);
    assert(wearToWrite(LESSON, OTHER_GUN, 0.0f, 0.0f) == LEAVE_WEAR);
    assert(wearToWrite(LESSON, SHOTGUN, 1.0f, 0.0f) == LEAVE_WEAR);  // the dirty one the lesson hands over
}

static void aSpotlessShotgunGetsSomethingToCleanForTheLesson() {
    // The lesson cannot be finished with a shotgun the game refuses to clean.
    assert(wearToWrite(LESSON, SHOTGUN, 0.0f, 0.0f) == 0.1f);
    assert(wearToWrite(LESSON, SHOTGUN, 0.25f, 0.25f) == 0.35f);
    assert(wearToWrite(LESSON, SHOTGUN, 0.1f, 0.0f) == LEAVE_WEAR);  // and only once
}

int main() {
    aWornWeaponIsCleanedAsFarAsTheGameCleansIt();
    aCleanWeaponIsLeftAlone();
    nothingIsCleanedDuringTheCleaningLesson();
    aSpotlessShotgunGetsSomethingToCleanForTheLesson();
    std::puts("WeaponCare: all checks passed");
}
