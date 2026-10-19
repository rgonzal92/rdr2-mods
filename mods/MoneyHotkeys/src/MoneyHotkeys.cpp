// MoneyHotkeys: a ScriptHookRDR2 plugin that changes the player's cash by $500 per key press:
// "]" adds $500, "[" takes $500 away, never below $0.
//
// It uses the same money calls as the game's own scripts. Those also count the change in the
// game's career cash stat; this plugin does not, so hotkey money is not counted as earned.

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
using KeyboardHandler = void (*)(unsigned long key, unsigned short repeats, unsigned char scanCode, int isExtended,
                                 int isWithAlt, int wasDownBefore, int isUpNow);
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void keyboardHandlerRegister(KeyboardHandler handler);
__declspec(dllimport) void keyboardHandlerUnregister(KeyboardHandler handler);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

// Game natives, by hash.
const unsigned long long PLAYER_ID = 0x217E9DC48139933D;                       // PLAYER::PLAYER_ID() -> Player
const unsigned long long IS_PLAYER_PLAYING = 0xBFFB35986CAAE58C;               // PLAYER::IS_PLAYER_PLAYING(Player player) -> BOOL
const unsigned long long MONEY_GET_CASH_BALANCE = 0x0C02DABFA3B98176;          // MONEY::_MONEY_GET_CASH_BALANCE() -> int
const unsigned long long MONEY_INCREMENT_CASH_BALANCE = 0xBC3422DC91667621;    // MONEY::_MONEY_INCREMENT_CASH_BALANCE(int amount, Hash addReason) -> BOOL
const unsigned long long MONEY_DECREMENT_CASH_BALANCE = 0x466BC8769CF26A7A;    // MONEY::_MONEY_DECREMENT_CASH_BALANCE(int amount) -> BOOL

// joaat("ADD_REASON_DEFAULT"), the reason the game's scripts give for most cash they add.
const unsigned int ADD_REASON_DEFAULT = 0x2CD419DC;

// Money is counted in cents: the game's cheat menu adds $500 as 50000.
const int CENTS_PER_PRESS = 50000;

// Windows virtual-key codes of the "[" and "]" keys on a US layout.
const unsigned long VK_OEM_4 = 0xDB, VK_OEM_6 = 0xDD;

// Presses not yet paid out: +1 per "]", -1 per "[". The keyboard handler runs on the game's
// window thread and the script loop on its script thread, so both use atomic operations.
static int pending;

// Calls a native whose arguments and result are all 32-bit values.
template <typename... Args>
static int call(unsigned long long native, Args... args) {
    nativeInit(native);
    (nativePush64(static_cast<unsigned int>(args)), ...);
    return static_cast<int>(*nativeCall());
}

static void onKey(unsigned long key, unsigned short, unsigned char, int, int, int wasDownBefore, int isUpNow) {
    if (wasDownBefore || isUpNow) return;  // only a fresh press: holding the key does not repeat
    if (key == VK_OEM_6) __atomic_add_fetch(&pending, 1, __ATOMIC_SEQ_CST);
    else if (key == VK_OEM_4) __atomic_sub_fetch(&pending, 1, __ATOMIC_SEQ_CST);
}

static void scriptMain() {
    for (;;) {
        const int presses = __atomic_exchange_n(&pending, 0, __ATOMIC_SEQ_CST);
        // Outside play (the main menu, a loading screen) the presses are dropped.
        if (presses && call(IS_PLAYER_PLAYING, call(PLAYER_ID))) {
            if (presses > 0) {
                call(MONEY_INCREMENT_CASH_BALANCE, presses * CENTS_PER_PRESS, ADD_REASON_DEFAULT);
            } else {
                // Never below $0: the game's own scripts clamp the amount to the balance the same way.
                const int balance = call(MONEY_GET_CASH_BALANCE);
                const int amount = -presses * CENTS_PER_PRESS < balance ? -presses * CENTS_PER_PRESS : balance;
                if (amount > 0) call(MONEY_DECREMENT_CASH_BALANCE, amount);
            }
        }
        scriptWait(0);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) {
        scriptRegister(module, scriptMain);
        keyboardHandlerRegister(onKey);
    } else if (reason == DLL_PROCESS_DETACH) {
        scriptUnregister(module);
        keyboardHandlerUnregister(onKey);
    }
    return 1;
}
