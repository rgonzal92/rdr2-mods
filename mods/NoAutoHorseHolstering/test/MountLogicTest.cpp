// Checks MountLogic's decisions without the game. Build and run with `make test`.

#include "../src/MountLogic.h"

#include <cassert>
#include <cstdio>

static_assert(WEAPON_UNARMED == 0xA2719263, "joaat must reproduce the game's weapon hashes");

const Hash RIFLE = joaat("WEAPON_RIFLE_SPRINGFIELD");
const Hash BOW = joaat("WEAPON_BOW");
const Hash REVOLVER = joaat("WEAPON_REVOLVER_CATTLEMAN");
const bool LONG_ARM = true, NOT_LONG_ARM = false;

static Frame onFoot(Hash held, bool longArm, int nowMs) { return {false, held, longArm, false, false, nowMs}; }
static Frame riding(Hash held, bool longArm, int nowMs) { return {true, held, longArm, false, false, nowMs}; }

// Walks up holding `held` and mounts at 1000 ms. Returns what the mount frame asked for.
static Hash mountWith(MountKeeper& keeper, Hash held, bool longArm) {
    keeper.frame(onFoot(held, longArm, 0));
    keeper.frame(onFoot(held, longArm, 984));
    return keeper.frame(riding(held, longArm, 1000));
}

static void aLongArmIsAskedForOnceOnTheMountFrame() {
    MountKeeper keeper;
    assert(mountWith(keeper, RIFLE, LONG_ARM) == RIFLE);
    // Still in hand: nothing more is asked, frame after frame.
    for (int t = 1016; t < 1000 + MountKeeper::WINDOW_MS; t += 16) assert(keeper.frame(riding(RIFLE, LONG_ARM, t)) == 0);
}

static void aBowCountsAsALongArm() {
    MountKeeper keeper;
    assert(mountWith(keeper, BOW, LONG_ARM) == BOW);
}

static void aLongArmTheGameHolstersIsDrawnBackOnce() {
    MountKeeper keeper;
    mountWith(keeper, RIFLE, LONG_ARM);
    assert(keeper.frame(riding(RIFLE, LONG_ARM, 1500)) == 0);
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1516)) == RIFLE);
    // Once only: if the game takes it again, the plugin does not fight it.
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1532)) == 0);
    assert(!keeper.deciding(true));
}

static void anUnarmedRiderIsKeptUnarmed() {
    MountKeeper keeper;
    assert(mountWith(keeper, WEAPON_UNARMED, NOT_LONG_ARM) == WEAPON_UNARMED);
    // Never re-asserted while the hand stays empty.
    for (int t = 1016; t < 1000 + MountKeeper::WINDOW_MS; t += 16) assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, t)) == 0);
}

static void aWeaponTheGameDrawsForAnUnarmedRiderIsPutAwayOnce() {
    MountKeeper keeper;
    mountWith(keeper, WEAPON_UNARMED, NOT_LONG_ARM);
    assert(keeper.frame(riding(REVOLVER, NOT_LONG_ARM, 1800)) == WEAPON_UNARMED);
    assert(keeper.frame(riding(REVOLVER, NOT_LONG_ARM, 1816)) == 0);
}

static void pistolsAreLeftAlone() {
    MountKeeper keeper;
    assert(mountWith(keeper, REVOLVER, NOT_LONG_ARM) == 0);
    assert(!keeper.deciding(true));
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1500)) == 0);
}

static void anyWeaponControlHandsTheRideToThePlayer() {
    // On the mount frame itself.
    MountKeeper first;
    first.frame(onFoot(RIFLE, LONG_ARM, 0));
    assert(first.frame({true, RIFLE, LONG_ARM, true, false, 16}) == 0);
    assert(first.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 32)) == 0);

    // Later in the window: holstering by hand, then drawing a revolver by the mouse wheel.
    MountKeeper later;
    mountWith(later, RIFLE, LONG_ARM);
    assert(later.frame({true, RIFLE, LONG_ARM, true, false, 1200}) == 0);
    assert(later.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1216)) == 0);

    MountKeeper unarmed;
    mountWith(unarmed, WEAPON_UNARMED, NOT_LONG_ARM);
    assert(unarmed.frame({true, WEAPON_UNARMED, NOT_LONG_ARM, true, false, 1200}) == 0);
    assert(unarmed.frame(riding(REVOLVER, NOT_LONG_ARM, 1216)) == 0);
}

static void aWeaponControlJustBeforeTheMountHandsTheRideToThePlayer() {
    // A holster tapped on foot half a second before getting on: the rifle may still read as in
    // hand while it is put away, and is not drawn back, neither at the mount nor in the window.
    MountKeeper holstered;
    holstered.frame(onFoot(RIFLE, LONG_ARM, 0));
    holstered.frame({false, RIFLE, LONG_ARM, true, false, 500});
    holstered.frame(onFoot(RIFLE, LONG_ARM, 516));
    assert(holstered.frame(riding(RIFLE, LONG_ARM, 1000)) == 0);
    assert(holstered.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1200)) == 0);

    // A weapon picked by an unarmed player just before getting on is not put away.
    MountKeeper drawn;
    drawn.frame({false, WEAPON_UNARMED, NOT_LONG_ARM, true, false, 500});
    assert(drawn.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1000)) == 0);
    assert(drawn.frame(riding(REVOLVER, NOT_LONG_ARM, 1300)) == 0);

    // A control used on foot long enough before the mount does not count.
    MountKeeper earlier;
    earlier.frame({false, RIFLE, LONG_ARM, true, false, 0});
    earlier.frame(onFoot(RIFLE, LONG_ARM, 16));
    assert(earlier.frame(riding(RIFLE, LONG_ARM, MountKeeper::RECENT_MS)) == RIFLE);
}

static void scriptedScenesAndMissionsAreLeftAlone() {
    MountKeeper atMount;
    atMount.frame(onFoot(RIFLE, LONG_ARM, 0));
    assert(atMount.frame({true, RIFLE, LONG_ARM, false, true, 16}) == 0);
    assert(atMount.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 32)) == 0);

    // A scene that starts during the window and holsters the rifle is not undone, also after it.
    MountKeeper during;
    mountWith(during, RIFLE, LONG_ARM);
    assert(during.frame({true, WEAPON_UNARMED, NOT_LONG_ARM, false, true, 1500}) == 0);
    assert(during.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1516)) == 0);
}

static void theWindowEndsAfterTheMount() {
    MountKeeper keeper;
    mountWith(keeper, RIFLE, LONG_ARM);
    // The first draw later in the ride is the player's business, not a holster to undo.
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1000 + MountKeeper::WINDOW_MS)) == 0);
    assert(keeper.frame(riding(RIFLE, LONG_ARM, 9000)) == 0);
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 9016)) == 0);
}

static void eachMountIsItsOwnRide() {
    MountKeeper keeper;
    mountWith(keeper, RIFLE, LONG_ARM);
    keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 1100));  // the one correction
    keeper.frame(onFoot(BOW, LONG_ARM, 20000));
    assert(keeper.frame(riding(BOW, LONG_ARM, 20016)) == BOW);
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 20100)) == BOW);
}

static void aRideUnderwayWhenThePluginStartsIsLeftAlone() {
    MountKeeper keeper;
    assert(!keeper.deciding(true));
    assert(keeper.frame(riding(RIFLE, LONG_ARM, 0)) == 0);
    assert(keeper.frame(riding(WEAPON_UNARMED, NOT_LONG_ARM, 16)) == 0);
}

static void decidingIsOnlyWhileMountedAndUnsettled() {
    MountKeeper keeper;
    keeper.frame(onFoot(RIFLE, LONG_ARM, 0));
    assert(!keeper.deciding(false));
    assert(keeper.deciding(true));
    keeper.frame(riding(RIFLE, LONG_ARM, 16));
    assert(keeper.deciding(true));
}

int main() {
    aLongArmIsAskedForOnceOnTheMountFrame();
    aBowCountsAsALongArm();
    aLongArmTheGameHolstersIsDrawnBackOnce();
    anUnarmedRiderIsKeptUnarmed();
    aWeaponTheGameDrawsForAnUnarmedRiderIsPutAwayOnce();
    pistolsAreLeftAlone();
    anyWeaponControlHandsTheRideToThePlayer();
    aWeaponControlJustBeforeTheMountHandsTheRideToThePlayer();
    scriptedScenesAndMissionsAreLeftAlone();
    theWindowEndsAfterTheMount();
    eachMountIsItsOwnRide();
    aRideUnderwayWhenThePluginStartsIsLeftAlone();
    decidingIsOnlyWhileMountedAndUnsettled();
    std::puts("MountLogic: all checks passed");
}
