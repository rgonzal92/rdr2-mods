// Checks DistanceText's output without the game. Build and run with `make test`.
#include "../src/DistanceText.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

static const char* text(float metres) {
    static char buffer[DISTANCE_TEXT_SIZE + 4];
    std::memset(buffer, '#', sizeof buffer);  // anything written past the size would show
    distanceText(metres, buffer);
    assert(std::strlen(buffer) < static_cast<std::size_t>(DISTANCE_TEXT_SIZE));
    assert(buffer[DISTANCE_TEXT_SIZE] == '#');
    return buffer;
}

static const char* routeText(float metres) {
    static char buffer[ROUTE_TEXT_SIZE + 4];
    std::memset(buffer, '#', sizeof buffer);  // anything written past the size would show
    routeDistanceText(metres, buffer);
    assert(std::strlen(buffer) < static_cast<std::size_t>(ROUTE_TEXT_SIZE));
    assert(buffer[ROUTE_TEXT_SIZE] == '#');
    return buffer;
}

int main() {
    assert(!std::strcmp(text(0.0f), "0 m"));
    assert(!std::strcmp(text(0.49f), "0 m"));
    assert(!std::strcmp(text(0.5f), "1 m"));
    assert(!std::strcmp(text(87.4f), "87 m"));
    assert(!std::strcmp(text(1234.5f), "1235 m"));
    assert(!std::strcmp(text(99999.4f), "99999 m"));
    assert(!std::strcmp(text(99999.5f), "? m"));
    assert(!std::strcmp(text(-1.0f), "? m"));
    assert(!std::strcmp(text(std::numeric_limits<float>::infinity()), "? m"));
    assert(!std::strcmp(text(std::numeric_limits<float>::quiet_NaN()), "? m"));
    assert(!std::strcmp(text(3.4e38f), "? m"));
    assert(!std::strcmp(routeText(0.0f), "route 0 m"));
    assert(!std::strcmp(routeText(1234.0f), "route 1234 m"));
    assert(!std::strcmp(routeText(99999.0f), "route 99999 m"));
    assert(!std::strcmp(routeText(-1.0f), "route ? m"));
    assert(!std::strcmp(routeText(std::numeric_limits<float>::quiet_NaN()), "route ? m"));

    // nextHeldRoute(found, length, nowMs, held). A positive length shows at once, found or not:
    // Rockstar's scripts read the length without asking whether a route was found.
    HeldRoute held = nextHeldRoute(false, 500, 1000, HeldRoute{});
    assert(held.length == 500);
    held = nextHeldRoute(true, 300, 1016, held);
    assert(held.length == 300);
    // While a route is found, a length of 0 or less keeps the last good one for ROUTE_HOLD_MS.
    held = nextHeldRoute(true, 0, 1016 + ROUTE_HOLD_MS, held);
    assert(held.length == 300);
    held = nextHeldRoute(true, -5, 1016 + ROUTE_HOLD_MS, held);
    assert(held.length == 300);
    held = nextHeldRoute(true, 0, 1017 + ROUTE_HOLD_MS, held);  // hold over: cleared
    assert(held.length == 0);
    held = nextHeldRoute(true, 0, 5000, held);  // stays cleared
    assert(held.length == 0);
    held = nextHeldRoute(true, 200, 6000, held);  // a new length shows at once and restarts the hold
    held = nextHeldRoute(true, 0, 6000 + ROUTE_HOLD_MS, held);
    assert(held.length == 200);
    // The hold is time, not frames: many frames in the same millisecond window keep it.
    for (int frame = 0; frame < 1000; ++frame) held = nextHeldRoute(true, 0, 6100, held);
    assert(held.length == 200);
    assert(nextHeldRoute(true, 0, 0, HeldRoute{}).length == 0);
    assert(nextHeldRoute(false, 0, 6100, held).length == 0);  // no route and no length: nothing kept
    std::puts("DistanceText: all checks passed");
    return 0;
}
