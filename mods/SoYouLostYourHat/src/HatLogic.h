// Decides when the plugin adds or removes its blip on the player's lost hat. Holds no game calls,
// so it can be tested without the game (see test/HatLogicTest.cpp).
#pragma once

enum class HatAction { NONE, ADD_BLIP, REMOVE_BLIP };

class HatTracker {
public:
    // A hat moving slower than this, in metres per second, counts as at rest. The original
    // needed exactly 0, which a hat jittering on a slope or bobbing in water never reaches.
    static constexpr float AT_REST_SPEED = 0.5f;
    // A hat that has not come to rest this long after it was first seen is blipped anyway.
    static constexpr int SETTLE_MS = 3000;

    // Call every frame.
    //   active:        the first mission is complete, winter1 is not running and the player is
    //                  playing.
    //   lostHat:       the player's last dropped hat, or 0 when there is none, it no longer
    //                  exists or it is being carried.
    //   ourBlipExists: the blip this plugin added is still there.
    //   hatHasBlip:    the hat has a blip of any kind (another script's, or ours).
    //   speed:         the hat's speed. Only read while lostHat is not 0.
    // Whatever the player is wearing plays no part: the blip stays on the lost hat while a spare
    // is worn, and goes only when that hat is picked up or gone.
    HatAction update(bool active, int lostHat, bool ourBlipExists, bool hatHasBlip, float speed, int nowMs) {
        if (!active) lostHat = 0;
        if (lostHat != hat) {
            const bool hadBlip = blipped;
            hat = lostHat;
            seenAtMs = nowMs;
            blipped = false;
            if (hadBlip) return HatAction::REMOVE_BLIP;  // a new hat is looked at from the next frame
        }
        if (hat == 0) return HatAction::NONE;
        if (blipped) {
            if (ourBlipExists) return HatAction::NONE;
            blipped = false;  // removed from outside; add it again unless someone else blipped the hat
        }
        if (hatHasBlip) return HatAction::NONE;
        // Unsigned, so the difference stays right when the game timer wraps.
        const bool waitedLongEnough = static_cast<unsigned>(nowMs) - static_cast<unsigned>(seenAtMs) >= SETTLE_MS;
        return speed < AT_REST_SPEED || waitedLongEnough ? HatAction::ADD_BLIP : HatAction::NONE;
    }

    // Call after ADD_BLIP with whether the blip now exists.
    void blipAdded(bool added) { blipped = added; }

private:
    int hat = 0;
    int seenAtMs = 0;
    bool blipped = false;
};
