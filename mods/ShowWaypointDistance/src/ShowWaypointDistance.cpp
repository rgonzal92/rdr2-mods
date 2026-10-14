// ShowWaypointDistance: a ScriptHookRDR2 plugin that draws, below the mini-map, the straight-line
// distance to the player's map waypoint, or, with no waypoint set, the length of the GPS route
// the game draws to its own objective blip, labelled "route".
//
// Recreated from the disassembly of ShowWaypointDistance.asi (built 2020-12-08). The position,
// size and colour of the text are that version's. Changes from it:
//   - measures on the map plane; the original measured in three dimensions against a waypoint
//     whose height the game reports as 0, so its number never fell below the player's altitude
//   - draws only while the game shows the radar; the original drew whenever a waypoint was set,
//     also over cutscenes, the cinematic camera and shop catalogues
//   - whole metres with a unit; the original showed a bare number with one decimal
//   - calls the game's current text commands; the original called commands the game emptied
//     in build 1436 and relied on the script hook to redirect them
//   - also measures the game's own GPS routes (the yellow ones); the original showed nothing
//     without a player waypoint
//   - no keyboard hook, no C runtime, no unused tables of model names

#include "DistanceText.h"

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// The compiler refers to this marker in any code that uses floats and expects the C runtime to
// define it. The plugin is built without that runtime, so it is defined here.
extern "C" int _fltused = 0;

// Game natives, by hash.
const unsigned long long IS_WAYPOINT_ACTIVE = 0x202B1BBFC6AB5EE4;           // MAP::IS_WAYPOINT_ACTIVE() -> BOOL
const unsigned long long GET_WAYPOINT_COORDS = 0x29B30D07C3F7873B;          // MAP::_GET_WAYPOINT_COORDS() -> Vector3, z always 0
const unsigned long long GET_GPS_BLIP_ROUTE_FOUND = 0x869DAACBBE9FA006;     // PATH::GET_GPS_BLIP_ROUTE_FOUND() -> BOOL
const unsigned long long GET_GPS_BLIP_ROUTE_LENGTH = 0xBBB45C3CF5C8AA85;    // PATH::GET_GPS_BLIP_ROUTE_LENGTH() -> int, metres along the route
const unsigned long long GET_GAME_TIMER = 0x4F67E8ECA7D3F667;               // MISC::GET_GAME_TIMER() -> int, milliseconds
const unsigned long long IS_RADAR_HIDDEN = 0x1B82FD5FFA4D666E;              // HUD::IS_RADAR_HIDDEN() -> BOOL
const unsigned long long PLAYER_PED_ID = 0x096275889B8E0EE0;                // PLAYER::PLAYER_PED_ID() -> Ped
const unsigned long long GET_ENTITY_COORDS = 0xA86D5F069399F44D;            // ENTITY::GET_ENTITY_COORDS(Entity entity, BOOL alive, BOOL realCoords) -> Vector3
const unsigned long long GET_DISTANCE_BETWEEN_COORDS = 0x0BE7F4E3CDBAFB28;  // MISC::GET_DISTANCE_BETWEEN_COORDS(float x1, y1, z1, x2, y2, z2, BOOL useZ) -> float
const unsigned long long VAR_STRING = 0xFA925AC00EB830B9;                   // MISC::VAR_STRING(int flags, const char* label, ...) -> const char*
const unsigned long long BG_SET_TEXT_SCALE = 0xA1253A3C870B6843;            // UIDEBUG::_BG_SET_TEXT_SCALE(float scaleX, float scaleY)
const unsigned long long BG_SET_TEXT_COLOR = 0x16FA5CE47F184F1E;            // UIDEBUG::_BG_SET_TEXT_COLOR(int red, int green, int blue, int alpha)
const unsigned long long BG_DISPLAY_TEXT = 0x16794E044C9EFB58;              // UIDEBUG::_BG_DISPLAY_TEXT(const char* text, float x, float y)

// A vector as the game returns it: each part in its own 8 bytes.
struct Vector3 {
    float x; unsigned int padX;
    float y; unsigned int padY;
    float z; unsigned int padZ;
};

// An argument travels as 64 bits: a float as its bit pattern, a pointer whole.
static unsigned long long arg(int value) { return static_cast<unsigned int>(value); }
static unsigned long long arg(float value) {
    unsigned int bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return bits;
}
static unsigned long long arg(const char* value) { return reinterpret_cast<unsigned long long>(value); }

// Calls a native and returns where the script hook keeps the result. The next call reuses that
// place, so copy the result out first.
template <typename... Args>
static unsigned long long* call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(arg(args)), ...);
    return nativeCall();
}

// Where and how the original drew: screen fractions from the top left corner, white.
const float TEXT_X = 0.097f, TEXT_Y = 0.97f, TEXT_SCALE = 0.4f;
// The original asked for a drop shadow through a command the game no longer has. This draws a
// black copy a little down and to the right first, which needs nothing but the commands above.
const float SHADOW_OFFSET = 0.001f;

static void drawText(const char* text, float x, float y, int shade) {
    call(BG_SET_TEXT_SCALE, TEXT_SCALE, TEXT_SCALE);
    call(BG_SET_TEXT_COLOR, shade, shade, shade, 255);
    // The game's own scripts turn a C string into display text this way.
    const char* shown = reinterpret_cast<const char*>(*call(VAR_STRING, 10, "LITERAL_STRING", text));
    call(BG_DISPLAY_TEXT, shown, x, y);
}

static void drawShadowed(const char* text) {
    drawText(text, TEXT_X + SHADOW_OFFSET, TEXT_Y + SHADOW_OFFSET, 0);
    drawText(text, TEXT_X, TEXT_Y, 255);
}

static void scriptMain() {
    HeldRoute heldRoute;  // last good GPS route length, kept a moment over a reading of 0
    for (;;) {
        // The text lasts one frame, so it is drawn again every frame.
        if (static_cast<int>(*call(IS_RADAR_HIDDEN))) {
            heldRoute = HeldRoute{};
        } else if (static_cast<int>(*call(IS_WAYPOINT_ACTIVE))) {
            heldRoute = HeldRoute{};
            const int playerPed = static_cast<int>(*call(PLAYER_PED_ID));
            const Vector3 player = *reinterpret_cast<Vector3*>(call(GET_ENTITY_COORDS, playerPed, 0, 0));
            const Vector3 waypoint = *reinterpret_cast<Vector3*>(call(GET_WAYPOINT_COORDS));
            float metres;
            __builtin_memcpy(&metres, call(GET_DISTANCE_BETWEEN_COORDS, player.x, player.y, 0.0f,
                                           waypoint.x, waypoint.y, 0.0f, 0), sizeof metres);
            char text[DISTANCE_TEXT_SIZE];
            distanceText(metres, text);
            drawShadowed(text);
        } else {
            // The route the game draws to an objective blip that forces GPS. Neither native takes
            // a blip: they read the route the map shows now.
            const bool found = static_cast<int>(*call(GET_GPS_BLIP_ROUTE_FOUND));
            const int length = static_cast<int>(*call(GET_GPS_BLIP_ROUTE_LENGTH));
            const int nowMs = static_cast<int>(*call(GET_GAME_TIMER));
            heldRoute = nextHeldRoute(found, length, nowMs, heldRoute);
            if (heldRoute.length > 0) {
                char text[ROUTE_TEXT_SIZE];
                routeDistanceText(static_cast<float>(heldRoute.length), text);
                drawShadowed(text);
            }
        }
        scriptWait(0);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    else if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
