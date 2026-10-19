// PearsonIngredients: a ScriptHookRDR2 plugin that, on F9, tops the player's inventory up to
// every ingredient Pearson needs to craft all his satchels and camp upgrades.
//
// Pearson's crafting menu (shop_pearson) counts an ingredient with the same inventory count this
// plugin uses (func_1248 -> func_947), and only looks at pelts on the horse or a carried carcass
// when that count is short. Items added here are taken by the game when the player crafts.

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

// Functions imported from KERNEL32.dll (see Kernel32.def), to write PearsonIngredients.log.
extern "C" {
__declspec(dllimport) void* CreateFileA(const char* name, unsigned long access, unsigned long share, void* security,
                                        unsigned long creation, unsigned long flags, void* templateFile);
__declspec(dllimport) int WriteFile(void* file, const void* buffer, unsigned long size, unsigned long* written, void* overlapped);
__declspec(dllimport) int CloseHandle(void* handle);
}

using Hash = unsigned int;
using Slot = unsigned long long;

// The game's name hash.
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
static_assert(joaat("SLOTID_SATCHEL") == 0x409F50CB && joaat("ADD_REASON_DEFAULT") == 0x2CD419DC,
              "joaat must reproduce the hashes in the game's scripts");

// Game natives, by hash.
const Slot PLAYER_ID = 0x217E9DC48139933D;                                  // PLAYER::PLAYER_ID() -> Player
const Slot IS_PLAYER_PLAYING = 0xBFFB35986CAAE58C;                          // PLAYER::IS_PLAYER_PLAYING(Player player) -> BOOL
const Slot INVENTORY_IS_USING_BACKUP_INVENTORY = 0x7C7E4AB748EA3B07;        // INVENTORY::_INVENTORY_IS_USING_BACKUP_INVENTORY() -> BOOL
const Slot INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID = 0xE787F05DFC977BDE;  // INVENTORY::_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID(int inventoryId, Hash item, BOOL p2) -> int
const Slot INVENTORY_GET_GUID_FROM_ITEMID = 0x886DFD3E185C8A89;             // INVENTORY::INVENTORY_GET_GUID_FROM_ITEMID(int inventoryId, Guid* parent, Hash item, Hash slotId, Guid* out) -> BOOL
const Slot INVENTORY_IS_GUID_VALID = 0xB881CA836CC4B6D4;                    // INVENTORY::_INVENTORY_IS_GUID_VALID(Guid* guid) -> BOOL
const Slot INVENTORY_ADD_ITEM_WITH_GUID = 0xCB5D11F9508A928D;               // INVENTORY::_INVENTORY_ADD_ITEM_WITH_GUID(int inventoryId, Guid* item, SlotInfo* parent, Hash item, Hash slotId, int amount, Hash addReason) -> BOOL

constexpr Hash SLOTID_SATCHEL = joaat("SLOTID_SATCHEL");
constexpr Hash ADD_REASON_DEFAULT = joaat("ADD_REASON_DEFAULT");

// Windows virtual-key code of F9.
const unsigned long VK_F9 = 0x78;

// Every ingredient of Pearson's 24 recipes (7 satchels, 17 camp upgrades), summed: the item
// catalog of build 1.0.1491.50, acquire costs with the key COST_CRAFTING_PEARSON. The query is in
// this mod's README.
struct Ingredient {
    const char* name;
    int amount;
};
constexpr Ingredient INGREDIENTS[] = {
    {"PROVISION_ALLIGATOR_SKIN_PRISTINE", 1},
    {"PROVISION_ANIMAL_CARCASS_PRONGHORN_PERFECT", 1},
    {"PROVISION_ANIMAL_CARCASS_RAM_PERFECT", 1},
    {"PROVISION_ANIMAL_CARCASS_WOLF_PERFECT", 1},
    {"PROVISION_BADGER_PELT_PRISTINE", 1},
    {"PROVISION_BEAVER_FUR_PRISTINE", 2},
    {"PROVISION_BOAR_SKIN_PRISTINE", 6},
    {"PROVISION_BUCK_ANTLERS", 1},
    {"PROVISION_BUCK_FUR_PRISTINE", 1},
    {"PROVISION_BUFFALO_FUR_PRISTINE", 1},
    {"PROVISION_COUGAR_FUR_PRISTINE", 3},
    {"PROVISION_COW_HIDE_PRISTINE", 1},
    {"PROVISION_DEER_HIDE_PRISTINE", 7},
    {"PROVISION_ELK_ANTLERS", 1},
    {"PROVISION_ELK_FUR_PRISTINE", 2},
    {"PROVISION_FOX_FUR_PRISTINE", 1},
    {"PROVISION_GOAT_HAIR_PRISTINE", 2},
    {"PROVISION_IGUANA_SKIN_PRISTINE", 1},
    {"PROVISION_MOOSE_ANTLER", 1},
    {"PROVISION_MUSKRAT_FUR_PRISTINE", 1},
    {"PROVISION_OXEN_HIDE_PRISTINE", 2},
    {"PROVISION_PANTHER_FUR_PRISTINE", 1},
    {"PROVISION_PRONGHORN_FUR_PRISTINE", 2},
    {"PROVISION_RABBIT_PELT_PRISTINE", 1},
    {"PROVISION_RACCOON_FUR_PRISTINE", 1},
    {"PROVISION_SNAKE_SKIN_PRISTINE", 2},
    {"PROVISION_SQUIRREL_PELT_PRISTINE", 1},
    {"PROVISION_WOLF_FUR_PRISTINE", 3},
};
constexpr int totalAmount() {
    int total = 0;
    for (const Ingredient& i : INGREDIENTS) total += i.amount;
    return total;
}
static_assert(sizeof INGREDIENTS / sizeof INGREDIENTS[0] == 28 && totalAmount() == 49,
              "the catalog query gives 28 ingredients, 49 items in all");

// Set by the keyboard handler on the game's window thread, taken by the script loop.
static int f9Pressed;

// An argument travels as 64 bits: a pointer whole.
static Slot arg(int value) { return static_cast<unsigned int>(value); }
static Slot arg(Hash value) { return value; }
static Slot arg(Slot* value) { return reinterpret_cast<Slot>(value); }

// Calls a native whose result is a 32-bit value.
template <typename... Args>
static int call(Slot native, Args... args) {
    nativeInit(native);
    (nativePush64(arg(args)), ...);
    return static_cast<int>(*nativeCall());
}

// One line of PearsonIngredients.log, built from words and numbers.
struct Line {
    static const int CAPACITY = 255;
    char text[CAPACITY + 1];
    int length = 0;

    Line& operator<<(const char* words) {
        while (*words && length < CAPACITY) text[length++] = *words++;
        return *this;
    }
    Line& operator<<(int number) {
        char digits[11];  // -2147483648 has ten digits and a sign
        int count = 0;
        unsigned int rest = number < 0 ? 0u - static_cast<unsigned int>(number) : static_cast<unsigned int>(number);
        do {
            digits[count++] = static_cast<char>('0' + rest % 10);
            rest /= 10;
        } while (rest);
        if (number < 0 && length < CAPACITY) text[length++] = '-';
        while (count && length < CAPACITY) text[length++] = digits[--count];
        return *this;
    }
};

// Appends one line to PearsonIngredients.log in the game folder, the current folder of the game.
static void log(const Line& line) {
    const unsigned long FILE_APPEND_DATA = 4, FILE_SHARE_READ = 1, OPEN_ALWAYS = 4, FILE_ATTRIBUTE_NORMAL = 0x80;
    void* const file = CreateFileA(".\\PearsonIngredients.log", FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == reinterpret_cast<void*>(-1LL)) return;
    unsigned long written;
    WriteFile(file, line.text, static_cast<unsigned long>(line.length), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

// The player's inventory, as the game's scripts pick it: a few missions swap in a second one.
static int inventoryId() {
    return call(INVENTORY_IS_USING_BACKUP_INVENTORY) ? 5 : 1;
}

// How many of an item the player holds, counted the way Pearson's menu counts it.
static int count(Hash item) {
    return call(INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID, inventoryId(), item, 0);
}

// Adds items the way the game's scripts add a satchel item (BatchCrafting's give()): look up the
// character, then the item's stack below it, and add to that. `character` carries the slot in a
// fifth part. Returns whether the game accepted the add.
static bool give(Hash item, int amount) {
    const int inventory = inventoryId();
    Slot none[4] = {}, character[5] = {}, stack[4] = {};
    call(INVENTORY_GET_GUID_FROM_ITEMID, inventory, none, joaat("CHARACTER"), joaat("SLOTID_NONE"), character);
    if (!call(INVENTORY_IS_GUID_VALID, character)) return false;
    character[4] = SLOTID_SATCHEL;
    call(INVENTORY_GET_GUID_FROM_ITEMID, inventory, character, item, SLOTID_SATCHEL, stack);
    return call(INVENTORY_ADD_ITEM_WITH_GUID, inventory, stack, character, item, SLOTID_SATCHEL, amount, ADD_REASON_DEFAULT);
}

// Brings every ingredient up to what all recipes need together; what is already there counts.
static void topUp() {
    Line header;
    log(header << "F9: topping up Pearson's ingredients (needed, had, now)");
    int stillShort = 0;
    for (const Ingredient& ingredient : INGREDIENTS) {
        const Hash item = joaat(ingredient.name);
        const int had = count(item);
        const bool accepted = had >= ingredient.amount || give(item, ingredient.amount - had);
        const int now = count(item);
        if (now < ingredient.amount) ++stillShort;
        Line line;
        log(line << "  " << ingredient.name << " " << ingredient.amount << " " << had << " " << now
                 << (accepted ? "" : " (add refused)"));
    }
    Line footer;
    log(footer << "  " << stillShort << " of 28 ingredients still short");
}

static void onKey(unsigned long key, unsigned short, unsigned char, int, int, int wasDownBefore, int isUpNow) {
    if (key == VK_F9 && !wasDownBefore && !isUpNow) __atomic_store_n(&f9Pressed, 1, __ATOMIC_SEQ_CST);
}

static void scriptMain() {
    for (;;) {
        // Outside play (the main menu, a loading screen) the press is dropped.
        if (__atomic_exchange_n(&f9Pressed, 0, __ATOMIC_SEQ_CST) && call(IS_PLAYER_PLAYING, call(PLAYER_ID))) topUp();
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
