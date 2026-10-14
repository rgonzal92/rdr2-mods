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

// One mount, from the last frame on foot until the weapon question is settled.
//
// On foot it remembers the weapon in hand. On the first frame on the horse, if that weapon was a
// long arm or no weapon at all, it asks for it once (the original's mount-moment call). For the
// next WINDOW_MS it watches: if the game takes the long arm away (the hand goes unarmed), or
// puts a weapon in an unarmed rider's hand, it asks once more and is done. It is also done the
// moment the player touches a weapon control, a scripted scene or mission is running, or the
// window ends, and it does not start when the player touched one on foot in the last RECENT_MS,
// since a holster or draw begun just before getting on may not show in the hand yet. Nothing is asked again until the player has been on foot and mounts again.
class MountKeeper {
public:
    static constexpr int WINDOW_MS = 4000;
    static constexpr int RECENT_MS = 1000;

    // True on the frames where frame() may ask for a weapon, so the caller reads the gates only
    // then, and the controls and the timer only then and on foot.
    bool deciding(bool mounted) const { return mounted && state != State::SETTLED; }

    // Call every frame. Returns the weapon to put in the player's hand this frame, or 0.
    Hash frame(const Frame& f) {
        if (!f.mounted) {
            state = State::ON_FOOT;
            heldOnFoot = f.held;
            heldOnFootLongArm = f.heldLongArm;
            if (f.playerChose) {
                choseOnFoot = true;
                choseOnFootMs = f.nowMs;
            }
            return 0;
        }
        switch (state) {
            case State::ON_FOOT: {
                state = State::SETTLED;
                const bool choseJustBefore = choseOnFoot && f.nowMs - choseOnFootMs < RECENT_MS;
                choseOnFoot = false;  // never carried to the next mount
                if (f.blocked || f.playerChose || choseJustBefore) return 0;
                if (!heldOnFootLongArm && heldOnFoot != WEAPON_UNARMED) return 0;  // pistols and the rest
                kept = heldOnFoot;
                mountedAtMs = f.nowMs;
                state = State::WATCHING;
                return kept;
            }
            case State::WATCHING:
                if (f.blocked || f.playerChose || f.nowMs - mountedAtMs >= WINDOW_MS) {
                    state = State::SETTLED;
                    return 0;
                }
                // A long arm counts as taken when the hand is empty; a different long arm or a
                // pistol in hand is not the game's holster. An unarmed rider counts as armed by
                // anything.
                if (kept == WEAPON_UNARMED ? f.held != WEAPON_UNARMED : f.held == WEAPON_UNARMED) {
                    state = State::SETTLED;
                    return kept;
                }
                return 0;
            case State::SETTLED:
                return 0;
        }
        return 0;
    }

private:
    enum class State { ON_FOOT, WATCHING, SETTLED };

    // Starts settled: a plugin loaded while the player is already riding leaves that ride alone.
    State state = State::SETTLED;
    Hash heldOnFoot = WEAPON_UNARMED;
    bool heldOnFootLongArm = false;
    Hash kept = 0;
    int mountedAtMs = 0;
    bool choseOnFoot = false;  // a weapon control was held on foot since the last mount
    int choseOnFootMs = 0;     // the last such frame
};
