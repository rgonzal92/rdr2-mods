// Turns a distance into the text the plugin draws. Holds no game calls and no C runtime calls,
// so it can be tested without the game (see test/DistanceTextTest.cpp).
#pragma once

// Writes the distance as whole metres followed by " m", e.g. "87 m" or "1235 m", into `out`,
// which must hold DISTANCE_TEXT_SIZE characters. Halves round up. A distance that is negative,
// not a number, or 100 km or more is not a real one on this map and is written as "? m".
const int DISTANCE_TEXT_SIZE = 8;  // "99999 m" and the terminator

inline void distanceText(float metres, char* out) {
    char digits[5];
    int count = 0;
    if (metres >= 0.0f && metres < 99999.5f) {  // false for NaN as well
        unsigned int whole = static_cast<unsigned int>(metres + 0.5f);
        do {
            digits[count++] = static_cast<char>('0' + whole % 10);
            whole /= 10;
        } while (whole);
        while (count) *out++ = digits[--count];
    } else {
        *out++ = '?';
    }
    *out++ = ' ';
    *out++ = 'm';
    *out = '\0';
}

// Writes a length along the game's GPS route as "route " and the distance, e.g. "route 1235 m",
// into `out`, which must hold ROUTE_TEXT_SIZE characters. Same rounding and "? m" rule as above.
const int ROUTE_TEXT_SIZE = 14;  // "route 99999 m" and the terminator

inline void routeDistanceText(float metres, char* out) {
    const char prefix[] = "route ";
    for (int i = 0; i < 6; ++i) *out++ = prefix[i];
    distanceText(metres, out);
}

// The GPS route length to draw, and the game time in milliseconds of its last positive reading.
struct HeldRoute {
    int length = 0;
    int readMs = 0;
};

// How long the last good length is kept while a route is found but its length reads 0 or less.
// Only meant to cover a brief gap, so a length that belonged to an earlier route, or a route
// whose length really reached 0, does not stay on screen.
const int ROUTE_HOLD_MS = 500;

// The route length to draw this frame. A positive length shows at once whether or not the game
// reports the route as found; Rockstar's scripts read the length without asking. Without a
// found route, a length of 0 or less clears it at once.
inline HeldRoute nextHeldRoute(bool found, int length, int nowMs, HeldRoute held) {
    if (length > 0) return HeldRoute{length, nowMs};
    if (!found || held.length <= 0 || nowMs - held.readMs > ROUTE_HOLD_MS) return HeldRoute{};
    return held;
}
