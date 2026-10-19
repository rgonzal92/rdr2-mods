// Decides which speeds to hold from what the player is pressing. Holds no game calls, so it can
// be tested without the game (see test/SpeedLogicTest.cpp).
//
// Speeds are move blend ratios, the game's scale for how fast a person or a horse moves: 1 is a
// walk, 2 a run, 3 a sprint. The ratios above 3 are the original's and are kept as found.
#pragma once

// The fast-walk ratio for a WALKING_SPEED setting, a percentage from 100 to 300: 150 means 1.5.
// Values outside that range are pulled to its nearest end. A setting that is missing or not a
// number arrives as 0 and means the value the mod ships with.
constexpr float walkRatio(int walkingSpeed) {
    if (walkingSpeed <= 0) return 1.5f;
    return (walkingSpeed < 100 ? 100 : walkingSpeed > 300 ? 300 : walkingSpeed) / 100.0f;
}

// A pace range for one frame. atMost 0 means leave the pace to the game; atLeast 0 means set
// no minimum.
struct Pace {
    float atLeast;
    float atMost;
};

// The player on foot.
class FootPace {
public:
    static constexpr float WALK = 1.0f;
    static constexpr float RUN = 2.0f;
    static constexpr float SPRINT = 10.6f;

    // tapMs is the longest press of the sprint key that counts as a tap. 0 switches tap to walk
    // off.
    FootPace(float fastWalk, int tapMs) : fastWalk(fastWalk), tapMs(tapMs) {}

    // Call when the cover key goes down. While the player is moving the plugin keeps that press
    // from the game, and it switches fast walk on or off instead. A press from a standstill
    // belongs to the game (it takes cover) and changes nothing here. A press within
    // TOGGLE_GAP_MS of the last switch is ignored, as the original ignored it.
    void coverKeyPressed(bool moving, int nowMs) {
        if (!moving || nowMs - switchedAtMs < TOGGLE_GAP_MS) return;
        fastWalkOn = !fastWalkOn;
        switchedAtMs = nowMs;
    }

    // Call every frame. Returns the pace range for this frame. Riding or driving, the same keys
    // move the horse or the wagon, so the player's own pace is left alone and an earlier sprint
    // is forgotten.
    Pace update(bool onFoot, bool moving, bool sprintHeld, int nowMs) {
        // Every hold needs a direction key: a held pace with none would walk the player off.
        if (!onFoot || !moving) {
            gait = Gait::THE_GAMES;
            sprintWasHeld = mayBeTap = false;
            return {0, 0};
        }
        if (sprintHeld && !sprintWasHeld) {
            // A tap means "walk again" only to a player who is running.
            mayBeTap = gait == Gait::RUN;
            pressedAtMs = nowMs;
        }
        sprintWasHeld = sprintHeld;
        if (sprintHeld) {
            // Until the press has outlasted a tap it could still be one, so the run goes on.
            if (mayBeTap && nowMs - pressedAtMs < tapMs) return {RUN, RUN};
            mayBeTap = false;
            gait = Gait::RUN;
            return {SPRINT, SPRINT};
        }
        if (mayBeTap) {
            mayBeTap = false;
            gait = Gait::WALK;
        }
        // The game's limits come back the moment the sprint key is let go, so a player who has
        // sprinted is kept at a run until they tap the key or stop.
        if (gait == Gait::RUN) return {RUN, RUN};
        if (fastWalkOn) return {fastWalk, fastWalk};
        // After a tap the game may still count the player as running, so the pace is capped. No
        // minimum is set: slower than a walk stays the game's choice.
        if (gait == Gait::WALK) return {0, WALK};
        return {0, 0};
    }

private:
    static constexpr int TOGGLE_GAP_MS = 100;

    // What holds the player's pace while the sprint key is up: the game, the run that follows
    // a sprint, or the walk that follows a tap. Stopping gives it back to the game.
    enum class Gait { THE_GAMES, RUN, WALK };

    float fastWalk;
    int tapMs;
    bool fastWalkOn = false;
    int switchedAtMs = -TOGGLE_GAP_MS;  // the game's clock starts at 0, so the first press counts
    Gait gait = Gait::THE_GAMES;
    bool sprintWasHeld = false;
    bool mayBeTap = false;  // the sprint key went down during a run and has not yet outlasted a tap
    int pressedAtMs = 0;
};

// The horse the player is riding.
constexpr float HORSE_TOP = 10.0f;

class HorsePace {
public:
    // Call every frame on the horse. A new HorsePace is needed for each horse ridden.
    Pace update(bool galloping, bool sprintHeld, bool forwardHeld, bool backHeld) {
        // The sprint key at a gallop starts a full gallop, and it lasts while sprint or forward is
        // held, so the rider can let go of sprint. Letting go of both, the back key (even with
        // sprint held), or the horse leaving the gallop (a fall, an obstacle, tiredness, a scene)
        // ends it. Below a gallop a sprint hold is the game's "keep this pace" and "match my
        // companion's speed", so nothing is forced there.
        fullGallop = galloping && !backHeld && (sprintHeld || (fullGallop && forwardHeld));
        if (fullGallop) return {HORSE_TOP, HORSE_TOP};
        // Asking the horse to move lifts whatever cap a mission has put on it, and sets no
        // minimum: the gait stays the rider's choice.
        if (sprintHeld || forwardHeld) return {0, HORSE_TOP};
        return {0, 0};
    }

private:
    bool fullGallop = false;
};
