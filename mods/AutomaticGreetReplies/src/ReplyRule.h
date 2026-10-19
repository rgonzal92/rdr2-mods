// Decides whether Arthur may reply, from the people looking at the player and speaking. Holds no
// game calls, so it can be tested without the game (see test/ReplyRuleTest.cpp).
#pragma once

// The auto-reply belongs to the player, not to one listener, so a single person Arthur has spoken
// to in the last ten seconds holds it back for everyone: answering that person's answer would
// start a second and third exchange.
class ReplyRule {
public:
    // Call for each person looking at the player and speaking. Returns false once the answer is
    // no, so the caller can stop asking the game about the rest.
    bool speaker(bool recentlySpokenTo) {
        if (recentlySpokenTo) heldBack = true;
        else someoneNew = true;
        return !heldBack;
    }

    bool reply() const { return someoneNew && !heldBack; }

private:
    bool someoneNew = false;
    bool heldBack = false;
};
