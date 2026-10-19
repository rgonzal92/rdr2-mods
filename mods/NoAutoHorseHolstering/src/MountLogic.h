// Decides when the plugin gives the player back the weapon they mounted with. Holds no game
// calls, so it can be tested without the game (see test/MountLogicTest.cpp).
#pragma once

using Hash = unsigned int;

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

constexpr Hash WEAPON_UNARMED = joaat("WEAPON_UNARMED");

// What the plugin knows about the player in one frame.
struct Frame {
    bool mounted;      // IS_PED_ON_MOUNT
    Hash held;         // the weapon in the primary hand, WEAPON_UNARMED for none
    bool heldLongArm;  // held is a rifle, repeater, shotgun or bow
    // The next three are read only on foot (playerChose, nowMs) or while deciding() is true (all
    // three); pass false/0 otherwise.
    bool playerChose;  // a weapon control is held: wheel, aim, attack, holster, cycle or select
    bool blocked;      // player control is off (scripted scenes) or a mission is running
    int nowMs;         // the game timer
};

// One ride, from the last frame on foot until the player gets off.
//
// On foot it remembers the weapon in hand. On the first frame on the horse, if that weapon was a
// long arm or no weapon at all, it asks for it once (the original's mount-moment call).
//
// A long arm, the one mounted with or one drawn on the horse, is then kept for the whole ride:
// when the hand goes empty it is asked for again. Not when the player touched a weapon control in
// the last RECENT_MS (their own holster), a scripted scene or mission is running, or the game
// takes it again within FIGHT_MS of the last time it was given back (the game insists; the mount
// itself may take it once). Then it is let go until the player draws a long arm again. After
// asking, it waits up to RECENT_MS for the weapon to show in the hand.
//
// An unarmed rider is watched for WINDOW_MS after the mount: a weapon that shows up in the hand
// is put away once, unless the player touched a weapon control.
//
// Nothing is asked at the mount when the player touched a weapon control on foot in the last
// RECENT_MS, since a holster or draw begun just before getting on may not show in the hand yet.
class MountKeeper {
public:
    static constexpr int WINDOW_MS = 4000;
    static constexpr int RECENT_MS = 1000;
    static constexpr int FIGHT_MS = 5000;

    // True on the frames where frame() may ask for a weapon, so the caller reads the gates, the
    // controls and the timer only then and on foot.
    bool deciding(bool mounted) const {
        return mounted && (state == State::ON_FOOT || (state == State::RIDING && (watchingUnarmed || kept)));
    }

    // Call every frame. Returns the weapon to put in the player's hand this frame, or 0.
    Hash frame(const Frame& f) {
        if (f.playerChose) {
            chose = true;
            choseMs = f.nowMs;
        }
        if (!f.mounted) {
            state = State::ON_FOOT;
            lastHeld = f.held;
            heldOnFootLongArm = f.heldLongArm;
            return 0;
        }
        if (state == State::ON_FOOT) return mount(f);
        if (state == State::RIDING) return ride(f);
        return 0;
    }

private:
    enum class State { ON_FOOT, RIDING, UNTRACKED };

    bool choseRecently(int nowMs) const { return chose && nowMs - choseMs < RECENT_MS; }

    Hash mount(const Frame& f) {
        state = State::RIDING;
        const Hash heldOnFoot = lastHeld;
        lastHeld = f.held;
        kept = 0;
        watchingUnarmed = false;
        if (f.blocked || choseRecently(f.nowMs)) return 0;
        if (heldOnFoot == WEAPON_UNARMED) {
            watchingUnarmed = true;
            mountedAtMs = f.nowMs;
            return WEAPON_UNARMED;
        }
        if (!heldOnFootLongArm) return 0;  // pistols and the rest
        kept = heldOnFoot;
        return giveBack(f.nowMs, true);
    }

    Hash ride(const Frame& f) {
        const bool changed = f.held != lastHeld;
        lastHeld = f.held;
        if (watchingUnarmed) {
            if (f.blocked || f.playerChose || f.nowMs - mountedAtMs >= WINDOW_MS) {
                watchingUnarmed = false;
            } else if (f.held != WEAPON_UNARMED) {
                watchingUnarmed = false;
                return WEAPON_UNARMED;
            }
            return 0;
        }
        if (f.held != WEAPON_UNARMED) {
            // Only a change counts as a draw, so a long arm still in hand from a holster begun
            // just before the mount is not taken up. Anything else in hand ends the keeping.
            if (changed) kept = f.heldLongArm ? f.held : 0;
            waiting = false;
            return 0;
        }
        if (!kept) return 0;
        if (waiting) {
            if (f.nowMs - givenBackMs < RECENT_MS) return 0;  // not in the hand yet
            waiting = false;
        } else if (!changed) {
            return 0;
        }
        if (f.blocked || choseRecently(f.nowMs) || (f.nowMs - givenBackMs < FIGHT_MS && !givenBackAtMount)) {
            kept = 0;
            return 0;
        }
        return giveBack(f.nowMs, false);
    }

    Hash giveBack(int nowMs, bool atMount) {
        givenBackMs = nowMs;
        givenBackAtMount = atMount;
        waiting = true;
        return kept;
    }

    // Starts untracked: a plugin loaded while the player is already riding leaves that ride alone.
    State state = State::UNTRACKED;
    Hash lastHeld = WEAPON_UNARMED;  // the weapon in hand on the previous frame
    bool heldOnFootLongArm = false;
    Hash kept = 0;                   // the long arm kept in hand on this ride, 0 for none
    int givenBackMs = 0;
    bool givenBackAtMount = false;
    bool waiting = false;            // asked for kept, not in the hand yet
    bool watchingUnarmed = false;
    int mountedAtMs = 0;
    bool chose = false;              // a weapon control has been held
    int choseMs = 0;                 // the last such frame
};
