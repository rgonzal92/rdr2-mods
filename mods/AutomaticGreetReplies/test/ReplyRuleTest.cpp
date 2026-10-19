// Checks ReplyRule's decision without the game. Build and run with `make test`.

#include "../src/ReplyRule.h"

#include <cassert>
#include <cstdio>
#include <initializer_list>

const bool RECENTLY_SPOKEN_TO = true, NEW = false;

// Feeds the speakers in order, stopping where the plugin would stop asking the game.
static bool replies(std::initializer_list<bool> speakers) {
    ReplyRule rule;
    for (bool recentlySpokenTo : speakers) {
        if (!rule.speaker(recentlySpokenTo)) break;
    }
    return rule.reply();
}

static void nobodySpeakingGetsNoReply() { assert(!replies({})); }

static void aNewSpeakerGetsAReply() {
    assert(replies({NEW}));
    assert(replies({NEW, NEW}));
}

static void someoneJustSpokenToIsLeftAlone() { assert(!replies({RECENTLY_SPOKEN_TO})); }

static void oneRecentListenerHoldsBackTheReplyForEveryone() {
    // The auto-reply belongs to the player, not to one listener.
    assert(!replies({NEW, RECENTLY_SPOKEN_TO}));
    assert(!replies({RECENTLY_SPOKEN_TO, NEW}));
}

static void theAnswerIsKnownAtTheFirstRecentListener() {
    ReplyRule rule;
    assert(rule.speaker(NEW));
    assert(!rule.speaker(RECENTLY_SPOKEN_TO));
}

int main() {
    nobodySpeakingGetsNoReply();
    aNewSpeakerGetsAReply();
    someoneJustSpokenToIsLeftAlone();
    oneRecentListenerHoldsBackTheReplyForEveryone();
    theAnswerIsKnownAtTheFirstRecentListener();
    std::puts("ReplyRule: all checks passed");
}
