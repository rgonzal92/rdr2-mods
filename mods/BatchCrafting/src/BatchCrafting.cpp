// BatchCrafting: a ScriptHookRDR2 plugin that makes a batch of items from one crafting or
// cooking animation.
//
// The game makes one item per animation. This plugin leaves that alone and watches for the
// moment the game puts its item away. In that moment it makes the rest of the batch itself: it
// adds more of the same product, removes the ingredients for exactly as many as arrived, and
// writes the statistics the game's scripts write for a craft. It works at camp fires, in the
// player's own camp and in the satchel's crafting menu (Rest > Craft), for every recipe in the
// crafting menu: ammunition, arrows, thrown weapons, tonics, horse items, baits and cooked meat.
//
// How it stays honest:
//   - the recipe comes from the game: product and ingredient set from the row of the crafting
//     menu, ingredients and amounts from the item catalog. The plugin holds no recipe table
//   - a batch is never larger than the ingredients in the satchel pay for, or than the player
//     can carry by the game's own limits. Meat still inside a carcass, and meat or pelts on the
//     horse, are not counted: the game turns those into ingredients one craft at a time
//   - the products are added first and counted again, and the ingredients are removed only
//     when all of them arrived. If the count is off, or an ingredient cannot be removed,
//     everything is put back, what was put back is written to the log, and the plugin stops
//     until the game is restarted
//   - the game decides whether another craft may follow in the moment it finishes one, and does
//     not check again. A batch that used the last ingredients would get a free item out of that,
//     so one set of ingredients and one place are left for the game, unless the game is certain
//     to decide after the batch (see gameChecksAgain in BatchLogic.h). The last item of a pile
//     of ingredients can therefore take a second animation
//   - nothing happens when the result is eaten, used or poured instead of stowed, in the mission
//     that teaches cooking and crafting, when a carcass or a piece of meat is cooked directly
//     (the Cook prompt) or in the recipe menu reached with Back after such a cook, or on a game
//     build whose recipe list is laid out differently
//
// Differences from crafting the same number by hand: the extra items give no Dead Eye
// experience, do not count toward the camp's "craft three things" task, and are not counted as
// cooked (only as crafted, made and seasoned).
//
// Two more things, both in the crafting menu opened with Craft or Cook:
//   - the "item added" message shows the batch's count. The game's scripts can hold their item
//     messages back in a queue; the plugin holds them while the menu is open, writes the count
//     into the message the game queued for its own item, and lets the game show it
//   - the plugin holds the speed-up key of cooking and fire crafting for the player, so the
//     progress bar fills in about 4 to 5 seconds instead of 10. Coffee has no such bar and
//     brews as before, and a carcass or piece of meat cooked with the Cook prompt is left alone
//
// Every batch, and every craft that got none, is written to BatchCrafting.log in the game folder.
//
// No code of the game is patched and nothing is searched for in its memory.

#include "BatchLogic.h"

// Functions imported from ScriptHookRDR2.dll (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();
__declspec(dllimport) unsigned long long* getGlobalPtr(int index);

// Functions imported from KERNEL32.dll (see Kernel32.def).
extern "C" {
__declspec(dllimport) unsigned int GetPrivateProfileIntA(const char* section, const char* key, int fallback, const char* file);
__declspec(dllimport) void* CreateFileA(const char* name, unsigned long access, unsigned long share, void* security,
                                        unsigned long creation, unsigned long flags, void* templateFile);
__declspec(dllimport) int WriteFile(void* file, const void* buffer, unsigned long size, unsigned long* written, void* overlapped);
__declspec(dllimport) int CloseHandle(void* handle);
}

// Game natives, by hash.
const Slot PLAYER_PED_ID = 0x096275889B8E0EE0;                                            // PLAYER::PLAYER_PED_ID() -> Ped
const Slot GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH = 0x8E34C953364A76DD;  // SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(Hash scriptHash) -> int
const Slot HAS_ANIM_EVENT_FIRED = 0x5851CC48405F4A07;                                     // ENTITY::HAS_ANIM_EVENT_FIRED(Entity entity, Hash actionHash) -> BOOL
const Slot DATABINDING_IS_ENTRY_VALID = 0x1E7130793AAAAB8D;                               // DATABINDING::DATABINDING_IS_ENTRY_VALID(Hash entryId) -> BOOL
const Slot DATABINDING_READ_DATA_INT_FROM_PARENT = 0xFFC566A4801F6B40;                    // DATABINDING::_DATABINDING_READ_DATA_INT_FROM_PARENT(Any entryId, const char* name) -> int
const Slot STAT_ID_IS_VALID = 0xC48FE1971C9743FF;                                         // STATS::STAT_ID_IS_VALID(StatId* statId) -> BOOL
const Slot STAT_ID_GET_INT = 0x767FBC2AC802EF3E;                                          // STATS::STAT_ID_GET_INT(StatId* statId, int* value) -> BOOL
const Slot STAT_ID_INCREMENT_INT = 0x6A0184E904CDF25E;                                    // STATS::_STAT_ID_INCREMENT_INT(StatId* statId, int value)
const Slot STAT_ID_DECREMENT_INT = 0xBD861AE8A5181ED7;                                    // STATS::_STAT_ID_DECREMENT_INT(StatId* statId, int value)
const Slot ITEMDATABASE_IS_KEY_VALID = 0x6D5D51B188333FD1;                                // ITEMDATABASE::_ITEMDATABASE_IS_KEY_VALID(Hash key, int mode) -> BOOL
const Slot ITEMDATABASE_FILLOUT_ITEM_INFO = 0xFE90ABBCBFDC13B2;                           // ITEMDATABASE::ITEMDATABASE_FILLOUT_ITEM_INFO(Hash key, ItemInfo* info) -> BOOL
const Slot ITEMDATABASE_FILLOUT_ACQUIRE_COST = 0x74F7928816E4E181;                        // ITEMDATABASE::_ITEMDATABASE_FILLOUT_ACQUIRE_COST(Hash key, Hash costKey, AcquireCost* cost) -> BOOL
const Slot ITEMDATABASE_DOES_ITEM_HAVE_TAG = 0xFF5FB5605AD56856;                          // ITEMDATABASE::_ITEMDATABASE_DOES_ITEM_HAVE_TAG(Hash item, Hash tag, Hash tagType) -> BOOL
const Slot ITEMDATABASE_GET_ITEM_PATHSET = 0xF4452CE83118C738;                            // ITEMDATABASE::_ITEMDATABASE_GET_ITEM_PATHSET(Hash key, Hash pathset) -> int
const Slot INVENTORY_IS_USING_BACKUP_INVENTORY = 0x7C7E4AB748EA3B07;                      // INVENTORY::_INVENTORY_IS_USING_BACKUP_INVENTORY() -> BOOL
const Slot INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID = 0xE787F05DFC977BDE;           // INVENTORY::_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID(int inventoryId, Hash item, BOOL p2) -> int
const Slot INVENTORY_IS_INVENTORY_ITEM_FLAG_ENABLED = 0x245D07651B1D183B;                 // INVENTORY::_INVENTORY_IS_INVENTORY_ITEM_FLAG_ENABLED(Hash item, int flag) -> BOOL
const Slot INVENTORY_FITS_SLOT_ID = 0x780C5B9AE2819807;                                   // INVENTORY::_INVENTORY_FITS_SLOT_ID(Hash item, Hash slotId) -> BOOL
const Slot GET_ITEM_ROLE_MAX_LEVEL_COUNT = 0xADDD1E7C0ECF7D95;                            // INVENTORY::_GET_ITEM_ROLE_MAX_LEVEL_COUNT(int inventoryId, Hash item) -> int
const Slot INVENTORY_GET_GUID_FROM_ITEMID = 0x886DFD3E185C8A89;                           // INVENTORY::INVENTORY_GET_GUID_FROM_ITEMID(int inventoryId, Guid* parent, Hash item, Hash slotId, Guid* out) -> BOOL
const Slot INVENTORY_IS_GUID_VALID = 0xB881CA836CC4B6D4;                                  // INVENTORY::_INVENTORY_IS_GUID_VALID(Guid* guid) -> BOOL
const Slot INVENTORY_ADD_ITEM_WITH_GUID = 0xCB5D11F9508A928D;                             // INVENTORY::_INVENTORY_ADD_ITEM_WITH_GUID(int inventoryId, Guid* item, SlotInfo* parent, Hash item, Hash slotId, int amount, Hash addReason) -> BOOL
const Slot INVENTORY_REMOVE_INVENTORY_ITEM_WITH_ITEMID = 0xB4158C8C9A3B5DCE;              // INVENTORY::_INVENTORY_REMOVE_INVENTORY_ITEM_WITH_ITEMID(int inventoryId, Hash item, int amount, Hash removeReason) -> BOOL
const Slot IS_AMMO_VALID = 0x1F7977C9101F807F;                                            // WEAPON::_IS_AMMO_VALID(Hash ammoType) -> BOOL
const Slot IS_WEAPON_VALID = 0x937C71165CF334B3;                                          // WEAPON::IS_WEAPON_VALID(Hash weapon) -> BOOL
const Slot GET_PED_AMMO_BY_TYPE = 0x39D22031557946C1;                                     // WEAPON::GET_PED_AMMO_BY_TYPE(Ped ped, Hash ammoType) -> int
const Slot GET_MAX_AMMO = 0xDC16122C7A20C933;                                             // WEAPON::GET_MAX_AMMO(Ped ped, int* ammo, Hash weapon) -> BOOL
const Slot GET_WEAPON_TYPE_FROM_AMMO_TYPE = 0x7AA043F6C41D151E;                           // WEAPON::_GET_WEAPON_TYPE_FROM_AMMO_TYPE(Hash ammoType) -> Hash
const Slot ADD_AMMO_TO_PED_BY_TYPE = 0x106A811C6D3035F3;                                  // WEAPON::_ADD_AMMO_TO_PED_BY_TYPE(Ped ped, Hash ammoType, int amount, Hash addReason)
const Slot REMOVE_AMMO_FROM_PED_BY_TYPE = 0xB6CFEC32E3742779;                             // WEAPON::_REMOVE_AMMO_FROM_PED_BY_TYPE(Ped ped, Hash ammoType, int amount, Hash removeReason)
const Slot GET_GAME_TIMER = 0x4F67E8ECA7D3F667;                                           // MISC::GET_GAME_TIMER() -> int
const Slot GET_HASH_OF_THREAD = 0x724CB89D35B283D0;                                       // SCRIPT::_GET_HASH_OF_THREAD(int threadId) -> Hash
const Slot UI_PROMPT_IS_VALID = 0x347469FBDD1589A9;                                       // HUD::_UI_PROMPT_IS_VALID(Prompt prompt) -> BOOL
const Slot UI_PROMPT_HAS_HOLD_AUTO_FILL_MODE = 0x8010BEBD0D5ED5BC;                        // HUD::_UI_PROMPT_HAS_HOLD_AUTO_FILL_MODE(Prompt prompt) -> BOOL
const Slot IS_CONTROL_PRESSED = 0xF3A21BCD95725A4A;                                       // PAD::IS_CONTROL_PRESSED(int control, Hash action) -> BOOL
const Slot SET_CONTROL_VALUE_NEXT_FRAME = 0xE8A25867FBA3B05E;                             // PAD::SET_CONTROL_VALUE_NEXT_FRAME(int control, Hash action, float value) -> BOOL

// The three game scripts that craft, and the one that teaches it ("Exit Pursued by a Bruised
// Ego" hands out its ingredients and counts what the player makes of them).
constexpr Hash CRAFTING_SCRIPTS[] = {joaat("player_camp"), joaat("interactive_campfire"), joaat("simple_crafting")};
constexpr Hash LESSON_SCRIPT = joaat("hunting1");
static_assert(CRAFTING_SCRIPTS[0] == 0x38EB3D5B && CRAFTING_SCRIPTS[1] == 0xA64E7B5F && CRAFTING_SCRIPTS[2] == 0xDBBA239B,
              "joaat must reproduce the script hashes");

// The camp state word, Global_1392040 in the decompiled scripts of game build 1.0.1491.50. Only
// the lesson sets the first two bits; while one is set the crafting menu is cut down to its
// recipe. The fire scripts hold the third up while they wait at "Cook Another" or "Craft Another".
const int CAMP_STATE = 1392040;
const Slot LESSON_BITS = (1 << 12) | (1 << 13);
const Slot AT_ANOTHER_PROMPT = 1 << 10;

// The animation events of BatchLogic's AnimEvents, as the game's fire scripts test them. The
// scripts hold only the hashes; the names were found by hashing.
constexpr Hash STOW_EVENT = joaat("StowItem");
constexpr Hash DECIDE_EVENTS[] = {joaat("FinalBite"), joaat("ShowMenu")};
static_assert(DECIDE_EVENTS[0] == 0xF183F10C && DECIDE_EVENTS[1] == 0xB7F65C18, "the events must be the ones in the game's scripts");

// Names from the game's scripts.
constexpr Hash ITEM_GROUP_AMMO = joaat("AMMO");
constexpr Hash ITEM_GROUP_WEAPON = joaat("WEAPON");
constexpr Hash ITEM_GROUP_CONSUMABLE = joaat("CONSUMABLE");
constexpr Hash SLOTID_SATCHEL = joaat("SLOTID_SATCHEL");
constexpr Hash ADD_REASON_DEFAULT = joaat("ADD_REASON_DEFAULT");
constexpr Hash REMOVE_REASON_USED = joaat("REMOVE_REASON_USED");
static_assert(SLOTID_SATCHEL == 0x409F50CB && ADD_REASON_DEFAULT == 0x2CD419DC && REMOVE_REASON_USED == 0x2188E0A3,
              "joaat must reproduce the hashes in the game's scripts");

// The tag type the game's scripts pass when they ask whether an item has a tag.
const Hash TAG_TYPE = 1120943070;
// The item flag that marks a cooked consumable.
const int ITEM_FLAG_COOKED = 1 << 24;

// Sizes of the structures natives fill in, in slots, as the game's scripts reserve them. The
// cost structure must be told how many ingredients and unlocks it has room for, in two slots.
const int ITEM_INFO_SLOTS = 8;  // the scripts reserve 7; the item group is slot 2
const int ACQUIRE_COST_SLOTS = 47;
const int COST_ROWS_SLOT = 3, COST_ROOM_SLOT = 4, COST_FIRST_ROW_SLOT = 5, COST_UNLOCK_ROOM_SLOT = 36;
const int COST_ROOM = 15, COST_UNLOCK_ROOM = 10;

// The settings from BatchCrafting.ini, read when the plugin loads.
static int maxPerAnimation;
static bool holdSpeedUpKey;
// How long a hold on the game's item messages lasts if the plugin does not lift it, as the
// game's own scripts set it.
const int TOAST_HOLD_MS = 2000;
// Set when a batch did not go as counted: the plugin then makes no more batches.
static bool stopped;

// An argument travels as 64 bits: a pointer whole.
static Slot arg(int value) { return static_cast<unsigned int>(value); }
static Slot arg(Hash value) { return value; }
static Slot arg(Slot value) { return value; }
static Slot arg(const char* value) { return reinterpret_cast<Slot>(value); }
static Slot arg(Slot* value) { return reinterpret_cast<Slot>(value); }
static Slot arg(float value) {
    union {
        float number;
        unsigned int bits;
    } slot = {value};
    return slot.bits;
}

// Calls a native whose result is a 32-bit value.
template <typename... Args>
static int call(Slot native, Args... args) {
    nativeInit(native);
    (nativePush64(arg(args)), ...);
    return static_cast<int>(*nativeCall());
}

// Appends one line to BatchCrafting.log in the game folder, the current folder of the game.
static void log(const Line& line) {
    const unsigned long FILE_APPEND_DATA = 4, FILE_SHARE_READ = 1, OPEN_ALWAYS = 4, FILE_ATTRIBUTE_NORMAL = 0x80;
    void* const file = CreateFileA(".\\BatchCrafting.log", FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == reinterpret_cast<void*>(-1LL)) return;
    unsigned long written;
    WriteFile(file, line.text, static_cast<unsigned long>(line.length), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    CloseHandle(file);
}

static bool running(Hash script) {
    return call(GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, script) > 0;
}

static bool hasTag(Hash item, Hash tag) {
    return call(ITEMDATABASE_DOES_ITEM_HAVE_TAG, item, tag, TAG_TYPE);
}

// The player's inventory, as the game's scripts pick it: a few missions swap in a second one.
static int inventoryId() {
    return call(INVENTORY_IS_USING_BACKUP_INVENTORY) ? 5 : 1;
}

// Where the game keeps an item: ammunition (bullets, arrows and thrown weapons) with the
// player's weapons, the rest in the satchel. Weapons and anything the catalog does not know are
// neither, and a recipe with such an item gets no batch.
enum class Kind { Satchel, Ammo, Other };

static Kind kindOf(Hash item, Hash* group = nullptr) {
    Slot info[ITEM_INFO_SLOTS] = {};
    if (!item || !call(ITEMDATABASE_IS_KEY_VALID, item, 0) || !call(ITEMDATABASE_FILLOUT_ITEM_INFO, item, info)) return Kind::Other;
    const Hash itemGroup = static_cast<Hash>(info[2]);
    if (group) *group = itemGroup;
    if (itemGroup == ITEM_GROUP_WEAPON) return Kind::Other;
    if (itemGroup == ITEM_GROUP_AMMO) return call(IS_AMMO_VALID, item) ? Kind::Ammo : Kind::Other;
    return Kind::Satchel;
}

// How many of an item the player holds, counted where the game's scripts count it.
static int count(int ped, Hash item, Kind kind) {
    if (kind == Kind::Ammo) return call(GET_PED_AMMO_BY_TYPE, ped, item);
    if (kind == Kind::Satchel) return call(INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID, inventoryId(), item, false);
    return 0;
}

// Adds items the way the game's scripts add a crafted product.
static void give(int ped, Hash item, Kind kind, int amount) {
    if (kind == Kind::Ammo) {
        call(ADD_AMMO_TO_PED_BY_TYPE, ped, item, amount, ADD_REASON_DEFAULT);
        return;
    }
    // A satchel item hangs below the character in the inventory: look up the character, then
    // the item's stack below it, and add to that. `character` carries the slot in a fifth part.
    const int inventory = inventoryId();
    Slot none[4] = {}, character[5] = {}, stack[4] = {};
    call(INVENTORY_GET_GUID_FROM_ITEMID, inventory, none, joaat("CHARACTER"), joaat("SLOTID_NONE"), character);
    if (!call(INVENTORY_IS_GUID_VALID, character)) return;
    character[4] = SLOTID_SATCHEL;
    call(INVENTORY_GET_GUID_FROM_ITEMID, inventory, character, item, SLOTID_SATCHEL, stack);
    call(INVENTORY_ADD_ITEM_WITH_GUID, inventory, stack, character, item, SLOTID_SATCHEL, amount, ADD_REASON_DEFAULT);
}

// Removes items the way the game's scripts remove an ingredient, and returns how many went:
// the ammunition command reports nothing, so the count is the only witness. (For meat the
// scripts pass another reason while the horse stands near; they do nothing else with the reason.)
static int take(int ped, Hash item, Kind kind, int amount) {
    const int before = count(ped, item, kind);
    if (kind == Kind::Ammo) call(REMOVE_AMMO_FROM_PED_BY_TYPE, ped, item, amount, REMOVE_REASON_USED);
    else call(INVENTORY_REMOVE_INVENTORY_ITEM_WITH_ITEMID, inventoryId(), item, amount, REMOVE_REASON_USED);
    return before - count(ped, item, kind);
}

// A statistic is named by two hashes. The game's scripts write "ITEMS_CRAFTED" as {0, name}
// and the per-item ones as {name, item}.
static void addToStat(Hash base, Hash permutation, int amount) {
    Slot stat[2] = {base, permutation};
    call(STAT_ID_INCREMENT_INT, stat, amount);
}

// Moves the counter of medicine, Dead Eye and lock breaker items the player holds, which the
// game's scripts keep in step with every add and removal in the satchel. It is named {name, 0},
// and the scripts ask whether it exists before they write it.
static void addToHeldItems(Hash item, int amount) {
    Slot stat[2] = {heldItemStat(hasTag(item, joaat("CI_TAG_ITEM_DEADEYE")), hasTag(item, joaat("CI_TAG_ITEM_MEDICINE")),
                                 hasTag(item, joaat("CI_TAG_ITEM_LOCK_BREAKER"))),
                    0};
    if (!stat[0] || !call(STAT_ID_IS_VALID, stat)) return;
    if (amount > 0) call(STAT_ID_INCREMENT_INT, stat, amount);
    else call(STAT_ID_DECREMENT_INT, stat, -amount);
}

// How many more of a product the player can carry, by the limits the game's scripts apply
// before they add one. Writes the item's limit to the log line.
static int roomFor(int ped, Hash item, Kind kind, Line& line) {
    // The game has a second way to look up the limit of items without a set of satchel
    // upgrades. No craftable product is such an item, so that way is not rebuilt here.
    if (!call(ITEMDATABASE_GET_ITEM_PATHSET, item, joaat("DEFAULT")) ||
        (kind == Kind::Satchel && !call(INVENTORY_FITS_SLOT_ID, item, SLOTID_SATCHEL))) {
        line << " limit unknown";
        return 0;
    }
    const int limit = call(GET_ITEM_ROLE_MAX_LEVEL_COUNT, inventoryId(), item);
    line << " limit " << limit;
    int room = NO_LIMIT;
    if (limit >= 0) {  // below zero means no limit
        // For ammunition the game compares the limit with the inventory's count in one place and
        // with the weapon's in another. The larger count is the safe one.
        int held = call(INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID, inventoryId(), item, false);
        if (kind == Kind::Ammo && count(ped, item, kind) > held) held = count(ped, item, kind);
        room = limit - held;
    }
    if (kind == Kind::Ammo && !hasTag(item, joaat("CI_TAG_ITEM_THROWN"))) {
        // Bullets and arrows are also held to what the weapon takes; without it, to nothing.
        const Hash weapon = static_cast<Hash>(call(GET_WEAPON_TYPE_FROM_AMMO_TYPE, item));
        Slot weaponLimit = 0;
        if (call(IS_WEAPON_VALID, weapon) && weapon != joaat("WEAPON_UNARMED")) call(GET_MAX_AMMO, ped, &weaponLimit, weapon);
        const int weaponRoom = static_cast<int>(weaponLimit) - count(ped, item, kind);
        line << " weapon limit " << static_cast<int>(weaponLimit);
        if (weaponRoom < room) room = weaponRoom;
    }
    return room;
}

// Makes the rest of a batch of `item` by the recipe `costKey`, after the game made one. Returns
// how many it made; 0 also when a batch had to be undone.
static int makeBatch(int ped, Hash item, Hash costKey, bool gameChecksAgain) {
    Line line;
    line << "item " << Hex{item} << " cost " << Hex{costKey};
    if (stopped) {
        log(line << ": no batch, the plugin stopped after an error");
        return 0;
    }

    // The recipe, from the item catalog.
    Hash group = 0;
    const Kind kind = kindOf(item, &group);
    static Slot cost[ACQUIRE_COST_SLOTS];
    for (Slot& slot : cost) slot = 0;
    cost[COST_ROOM_SLOT] = COST_ROOM;
    cost[COST_UNLOCK_ROOM_SLOT] = COST_UNLOCK_ROOM;
    if (kind == Kind::Other || !call(ITEMDATABASE_FILLOUT_ACQUIRE_COST, item, costKey, cost)) {
        log(line << ": no batch, the catalog has no such recipe");
        return 0;
    }
    const int ingredients = static_cast<int>(cost[COST_ROWS_SLOT]);
    Hash ingredient[COST_ROOM];
    Kind ingredientKind[COST_ROOM];
    int quantity[COST_ROOM], owned[COST_ROOM];
    if (ingredients < 1 || ingredients > COST_ROOM) {
        log(line << ": no batch, the recipe has " << ingredients << " ingredients");
        return 0;
    }
    for (int i = 0; i < ingredients; ++i) {
        ingredient[i] = static_cast<Hash>(cost[COST_FIRST_ROW_SLOT + 2 * i]);
        quantity[i] = static_cast<int>(cost[COST_FIRST_ROW_SLOT + 2 * i + 1]);
        ingredientKind[i] = kindOf(ingredient[i]);
        if (ingredientKind[i] == Kind::Other) {
            log(line << ": no batch, ingredient " << Hex{ingredient[i]} << " is not a satchel item or ammunition");
            return 0;
        }
        owned[i] = count(ped, ingredient[i], ingredientKind[i]);
    }

    const int sets = setsOwned(owned, quantity, ingredients);
    line << " sets " << sets;
    const int room = roomFor(ped, item, kind, line);
    const int size = batchSize(maxPerAnimation, sets, room, gameChecksAgain);
    if (room == NO_LIMIT) line << " room any";
    else line << " room " << room;
    line << (gameChecksAgain ? " use all" : " keep one") << " batch " << size;
    if (size == 0) {
        log(line);
        return 0;
    }

    // Products first, counted again before anything is paid: the game's limits were asked
    // above, but nothing in its scripts shows what the add commands do with more than fits.
    const int had = count(ped, item, kind);
    give(ped, item, kind, size);
    const int added = count(ped, item, kind) - had;
    line << " added " << added;
    if (added != size) {
        // The limits above were wrong for this item, so the place kept for the game's next
        // craft cannot be trusted either. Take back what arrived and make no more batches.
        const int back = added > 0 ? take(ped, item, kind, added) : 0;
        stopped = true;
        log(line << ": unexpected, " << back << " taken back, no ingredients used; no more batches until the game is restarted");
        return 0;
    }
    for (int i = 0; i < ingredients; ++i) {
        const int gone = take(ped, ingredient[i], ingredientKind[i], added * quantity[i]);
        if (gone == added * quantity[i]) continue;
        // Undo: the products go, and every ingredient taken so far comes back.
        const int back = take(ped, item, kind, added);
        if (gone > 0) give(ped, ingredient[i], ingredientKind[i], gone);
        for (int taken = 0; taken < i; ++taken) give(ped, ingredient[taken], ingredientKind[taken], added * quantity[taken]);
        stopped = true;
        log(line << ": ingredient " << Hex{ingredient[i]} << " lost " << gone << ", all ingredients returned, " << back
                 << " products taken back; no more batches until the game is restarted");
        return 0;
    }

    // The statistics the game's scripts write for each craft, for `added` crafts.
    for (int i = 0; i < ingredients; ++i) {
        if (ingredientKind[i] == Kind::Satchel) addToHeldItems(ingredient[i], -added * quantity[i]);
        if (const Hash herb = herbStat(ingredient[i])) addToStat(joaat("USED_IN_RECIPE"), herb, added * quantity[i]);
    }
    addToStat(0, joaat("ITEMS_CRAFTED"), added);
    if (kind == Kind::Satchel) addToHeldItems(item, added);
    addToStat(joaat("MADE"), item, added);
    ItemTraits traits = {};
    traits.food = group == ITEM_GROUP_CONSUMABLE && call(INVENTORY_IS_INVENTORY_ITEM_FLAG_ENABLED, item, ITEM_FLAG_COOKED);
    traits.ammo = group == ITEM_GROUP_AMMO;
    traits.thrown = hasTag(item, joaat("CI_TAG_ITEM_THROWN"));
    traits.horseItem = hasTag(item, joaat("CI_TAG_ITEM_HORSE_ITEM"));
    traits.medicine = hasTag(item, joaat("CI_TAG_ITEM_MEDICINE"));
    traits.remedy = hasTag(item, joaat("CI_TAG_ITEM_REMEDY"));
    traits.hunting = hasTag(item, joaat("CI_TAG_ITEM_HUNTING"));
    // For a product of no recipe type the game's scripts count the item as made a second time.
    if (const Hash type = recipeType(traits)) addToStat(joaat("MADE_RECIPE_TYPE"), type, added);
    else addToStat(joaat("MADE"), item, added);
    if (costKey == COST_CRAFTING_GRILL) addToStat(0, joaat("FOOD_SEASONED"), added);
    log(line);
    return added;
}

// Reads the game's count of crafted items into `snapshot`, and with `menu` the crafting menu's
// recipe list as well (without it the snapshot has no rows). Returns how many rows show a
// product.
// ponytail: asks the game about every row every frame while the player crafts, some hundreds of
// native calls. Remember each row's kind between frames if that ever shows in the frame time.
static int read(int ped, Snapshot& snapshot, bool menu) {
    Slot crafted[2] = {0, joaat("ITEMS_CRAFTED")}, value = 0;
    call(STAT_ID_GET_INT, crafted, &value);
    snapshot.crafted = static_cast<int>(value);
    snapshot.size = menu ? static_cast<int>(*getGlobalPtr(RECIPE_ROW_COUNT)) : 0;
    if (snapshot.size < 0 || snapshot.size > RECIPE_LIST_ROWS) snapshot.size = 0;
    int listed = 0;
    for (int i = 0; i < snapshot.size; ++i) {
        Candidate& row = snapshot.rows[i];
        row = {0, 0, 0};
        const Slot entry = *getGlobalPtr(rowEntryGlobal(i));
        if (!call(DATABINDING_IS_ENTRY_VALID, entry)) continue;
        row.item = static_cast<Hash>(call(DATABINDING_READ_DATA_INT_FROM_PARENT, entry, "eOutputItem"));
        row.costKey = static_cast<Hash>(call(DATABINDING_READ_DATA_INT_FROM_PARENT, entry, "eCost"));
        row.count = count(ped, row.item, kindOf(row.item));
        if (row.item) ++listed;
    }
    return listed;
}

// The progress bar of cooking and fire crafting that is on screen or about to be, or 0: its
// handle, and its row's flags. A bar that is switched on wins: the game can leave a switched-off
// one behind in a lower row (a Cook from the satchel after an interrupted craft does not delete
// the bar of that craft).
static int progressPrompt(int* flags) {
    if (static_cast<int>(*getGlobalPtr(PROMPTS)) != PROMPT_ROWS) return 0;  // another game build
    int found = 0;
    for (int row = 0; row < PROMPT_ROWS; ++row) {
        const auto field = [row](int index) { return static_cast<int>(*getGlobalPtr(promptGlobal(row, index))); };
        const int handle = field(PROMPT_HANDLE);
        if (!handle || !isProgressPrompt(field(PROMPT_PRIORITY), field(PROMPT_FLAGS), field(PROMPT_TRANSPORT),
                                         static_cast<Hash>(field(PROMPT_CONTROL)), field(PROMPT_TYPE)))
            continue;
        if (!call(UI_PROMPT_IS_VALID, handle) || !call(UI_PROMPT_HAS_HOLD_AUTO_FILL_MODE, handle)) continue;
        const Hash owner = static_cast<Hash>(call(GET_HASH_OF_THREAD, field(PROMPT_OWNER)));
        if (owner != CRAFTING_SCRIPTS[0] && owner != CRAFTING_SCRIPTS[1]) continue;
        const int rowFlags = field(PROMPT_FLAGS);
        if (found && (rowFlags & PROMPT_OFF)) continue;
        found = handle;
        *flags = rowFlags;
        if (!(rowFlags & PROMPT_OFF)) break;
    }
    return found;
}

// The game's scripts read a key from one of two sets of controls, 0 or 2. Their own check of a
// prompt's key reads set 2 and their own simulated input writes set 0, and nothing shows which
// one the prompts themselves read, so the bar's key is read from and held in both.
const int CONTROL_SETS[] = {0, 2};

// The progress bar the plugin is following.
struct Progress {
    int handle;
    int startedAt;  // game time when the bar was switched on, 0 before
    bool holding;   // the bar was seen switched on with its key up: the plugin holds the key
};

// Holds the progress bar's key for the player while the bar is on, which fills it in about 4 to
// 5 seconds instead of 10 (3.8 to 4.9 measured in the game). How long each bar took is written
// to the log.
//
// Nothing faster is done, on purpose. A bar given a shorter time of its own ended at once, and
// the cooking script then started its next animation while the game still held the command
// that started the last one (given for 6 seconds). The script waits for that animation without
// a time limit: the player sat frozen at a switched-off "Cook". With the key held, the timing
// is that of a player holding it.
//
// A craft that is interrupted (the player is shot at, say) leaves its bar behind, and the fire's
// other prompts switch it on again while the player sits there. The game lowers the crafting
// flag in that moment, so without the flag a bar is neither timed nor is its key held: the same
// key is "Pour" and "Fast Travel" at the fire.
//
// The same key is also "Cook Another", and the game's prompts appear not to count a key that
// was down already when they came on until it has been let go. So the hold begins one frame
// after the bar was seen on with the key up: a player who keeps "Cook Another" down a moment
// too long does not lose the speed-up for that bar.
static void speedUp(Progress& progress, bool crafting) {
    int flags = 0;
    const int handle = progressPrompt(&flags);
    if (handle != progress.handle) {
        if (progress.startedAt)
            log(Line() << "progress bar took " << call(GET_GAME_TIMER) - progress.startedAt
                       << (progress.holding ? " ms with the key held by the plugin" : " ms, the key was never free for the plugin to hold"));
        progress = {handle, 0, false};
    }
    if (!handle || (flags & PROMPT_OFF) || !crafting) {
        progress.startedAt = 0;
        progress.holding = false;
        return;
    }
    if (!progress.startedAt) progress.startedAt = call(GET_GAME_TIMER);
    if (!progress.holding) {
        progress.holding = !call(IS_CONTROL_PRESSED, CONTROL_SETS[0], INPUT_CONTEXT_A) && !call(IS_CONTROL_PRESSED, CONTROL_SETS[1], INPUT_CONTEXT_A);
        return;
    }
    for (const int controls : CONTROL_SETS) call(SET_CONTROL_VALUE_NEXT_FRAME, controls, INPUT_CONTEXT_A, 1.0f);
}

static void scriptMain() {
    log(Line() << "BatchCrafting started, at most " << maxPerAnimation << " per animation"
               << (stopped ? ", still stopped after an earlier error" : ""));
    static Snapshot snapshots[2];
    Snapshot* before = &snapshots[0];
    Snapshot* now = &snapshots[1];
    bool haveBefore = false, wasInMenu = false;
    bool holdingToasts = false;  // this thread set the hold on the game's item messages
    Progress progress = {0, 0, false};
    AnimEvents previousEvents = {false, false};
    for (;; scriptWait(0)) {
        const int ped = call(PLAYER_PED_ID);
        // getGlobalPtr does not check that the game has given the global any memory, and a
        // plugin can be running before the game's scripts are. While a crafting script runs, it
        // has. The recipe list's length is the check that this is the build the indexes are for.
        if (!ped || !(running(CRAFTING_SCRIPTS[0]) || running(CRAFTING_SCRIPTS[1]) || running(CRAFTING_SCRIPTS[2])) ||
            static_cast<int>(*getGlobalPtr(RECIPE_LIST)) != RECIPE_LIST_ROWS) {
            haveBefore = wasInMenu = false;
            progress = {0, 0, false};
            if (holdingToasts) getGlobalPtr(TOAST_QUEUE)[TOAST_HOLD] = 0;
            holdingToasts = false;
            continue;
        }
        const AnimEvents events = {
            call(HAS_ANIM_EVENT_FIRED, ped, STOW_EVENT) != 0,
            call(HAS_ANIM_EVENT_FIRED, ped, DECIDE_EVENTS[0]) || call(HAS_ANIM_EVENT_FIRED, ped, DECIDE_EVENTS[1]),
        };
        // The flag goes up when the crafting menu is opened with Craft or Cook and stays up
        // through every animation until the player leaves. The game lowers it before it hands
        // over the item of an interrupted craft, and does not raise it for the Cook prompt on a
        // carcass or a piece of meat, nor for the menu reached with Back from there. Without it
        // only the count of crafts is read, so that such a craft still shows in the log.
        const bool inMenu = static_cast<int>(*getGlobalPtr(CRAFTING_ACTIVE)) != 0;
        const int listed = read(ped, *now, inMenu);
        if (inMenu && !wasInMenu) log(Line() << "crafting menu opened, " << now->crafted << " items crafted so far");
        wasInMenu = inMenu;

        const Slot campState = *getGlobalPtr(CAMP_STATE);
        const bool lesson = (campState & LESSON_BITS) || running(LESSON_SCRIPT);
        if (holdSpeedUpKey && !lesson) speedUp(progress, inMenu);

        // Hold the game's item messages back while the menu is open, so that the message for a
        // craft is still in the queue when its batch is made. Only an empty queue is held, and
        // never one another script holds. Set anew every frame, so the game lifts the hold by
        // itself soon after the plugin stops running.
        Slot* const toasts = getGlobalPtr(TOAST_QUEUE);
        const bool wantHold = inMenu && !lesson && !stopped && static_cast<int>(toasts[TOAST_LENGTH]) == TOAST_ROOM;
        if (wantHold && static_cast<int>(toasts[TOAST_COUNT]) == 0 && (holdingToasts || static_cast<int>(toasts[TOAST_HOLD]) == 0)) {
            toasts[TOAST_HOLD] = 1;
            toasts[TOAST_SINCE] = arg(call(GET_GAME_TIMER));
            toasts[TOAST_TIMEOUT] = TOAST_HOLD_MS;
            holdingToasts = true;
        }

        const int row = haveBefore ? craftedRow(*before, *now) : -1;
        if (row != -1 && !lesson) {
            // The recipe as the row showed it before the craft: the game may move a row to
            // another ingredient set once the one it used has run out.
            const Hash item = before->rows[row].item, costKey = before->rows[row].costKey;
            const int added = makeBatch(ped, item, costKey,
                                        gameChecksAgain(costKey, previousEvents, events, (campState & AT_ANOTHER_PROMPT) != 0));
            if (added > 0) {
                const Toast toast = countInToast(toasts, item, added);
                log(Line() << (toast == Toast::Raised ? "message: the game's own now shows " : toast == Toast::Appended
                               ? "message: the game's own was shown already, a second one shows " : "message: none, the queue was not usable for ")
                           << (toast == Toast::Raised ? added + 1 : added));
            }
            read(ped, *now, inMenu);  // the batch changed the counts
        } else if (haveBefore && now->crafted > before->crafted) {
            log(Line() << "craft counted, no batch: "
                       << (!inMenu ? "not in a crafting menu opened with Craft or Cook"
                           : lesson ? "the crafting lesson is running" : "not exactly one listed product was stowed")
                       << " (" << now->size << " rows, " << listed << " with a product)");
        }
        // Messages are queued, or the menu is closed: let the game show them.
        if (holdingToasts && (!wantHold || static_cast<int>(toasts[TOAST_COUNT]) > 0)) {
            toasts[TOAST_HOLD] = 0;
            holdingToasts = false;
        }
        haveBefore = true;
        Snapshot* const swap = before;
        before = now;
        now = swap;
        previousEvents = events;
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) {
        // The file sits beside the plugin, in the game folder, which is the current folder while
        // plugins load.
        maxPerAnimation = static_cast<int>(GetPrivateProfileIntA("BATCH", "MAX_PER_ANIMATION", 50, ".\\BatchCrafting.ini"));
        holdSpeedUpKey = GetPrivateProfileIntA("BATCH", "HOLD_SPEED_UP_KEY", 1, ".\\BatchCrafting.ini") != 0;
        if (maxPerAnimation > 1) scriptRegister(module, scriptMain);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (maxPerAnimation > 1) scriptUnregister(module);
    }
    return 1;
}
