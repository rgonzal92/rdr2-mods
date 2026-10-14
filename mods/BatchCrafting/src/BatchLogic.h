// The rules of BatchCrafting. Holds no game calls and no C runtime calls, so it can be tested
// without the game (see test/BatchLogicTest.cpp).
#pragma once

using Hash = unsigned int;
// Script values, and the parts of the structures natives fill in, are 8 bytes each.
using Slot = unsigned long long;

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
static_assert(joaat("ITEMS_CRAFTED") == 0xE7580C82 && joaat("StowItem") == 0xC82DBF75,
              "joaat must reproduce the hashes in the game's scripts");

// A recipe is a product plus one of these cost keys; the key names the ingredient set. The first
// four are crafted anywhere, the last four need a fire (KNIFE is plain cooked meat, GRILL is
// seasoned meat). The scripts spell only COST_CRAFTING, _FIRE, _KNIFE and _GRILL; the other
// names were found by hashing, and the hashes are the ones in the game's item catalog.
constexpr Hash COST_CRAFTING = joaat("COST_CRAFTING");
constexpr Hash COST_CRAFTING_2 = joaat("COST_CRAFTING_2");
constexpr Hash COST_CRAFTING_3 = joaat("COST_CRAFTING_3");
constexpr Hash COST_CRAFTING_4 = joaat("COST_CRAFTING_4");
constexpr Hash COST_CRAFTING_FIRE = joaat("COST_CRAFTING_FIRE");
constexpr Hash COST_CRAFTING_FIRE_2 = joaat("COST_CRAFTING_FIRE_2");
constexpr Hash COST_CRAFTING_KNIFE = joaat("COST_CRAFTING_KNIFE");
constexpr Hash COST_CRAFTING_GRILL = joaat("COST_CRAFTING_GRILL");
static_assert(COST_CRAFTING_2 == 0x2C92EF12 && COST_CRAFTING_3 == 0x1DEFD1CC && COST_CRAFTING_4 == 0xCE46B27B &&
              COST_CRAFTING_FIRE_2 == 0x583BB03D, "the variant keys must be the ones in the item catalog");

// The crafting menu's recipe list: Global_1911914 in the decompiled scripts of game build
// 1.0.1491.50. It is an array of 175 rows of 9 values behind its length, followed by the number
// of rows in use (.f_1576) and the "player is crafting" flag (.f_1582). The second value of a
// row (.f_1) is the row's entry in the menu's data, which holds the product and the cost key
// the row shows now ("eOutputItem" and "eCost"). A game update can move script globals: check
// these against the new scripts before using the plugin on another build.
const int RECIPE_LIST = 1911914;
const int RECIPE_LIST_ROWS = 175;
const int RECIPE_ROW_COUNT = RECIPE_LIST + 1576;
const int CRAFTING_ACTIVE = RECIPE_LIST + 1582;
constexpr int rowEntryGlobal(int row) { return RECIPE_LIST + 1 + 9 * row + 1; }

// What the plugin reads once a frame while the player is crafting: the game's count of crafted
// items, and for every row of the recipe list the product and cost shown and how many of that
// product the player holds.
struct Candidate {
    Hash item;
    Hash costKey;
    int count;
};
struct Snapshot {
    int crafted;
    int size;
    Candidate rows[RECIPE_LIST_ROWS];
};

// The row whose product the game crafted and put away between two snapshots, or -1. That is one
// craft counted and one more of exactly one product. Eating or using the result counts a craft
// and stows nothing; anything larger is not one finished craft.
inline int craftedRow(const Snapshot& before, const Snapshot& now) {
    if (now.crafted != before.crafted + 1 || now.size != before.size) return -1;
    int row = -1;
    for (int i = 0; i < now.size; ++i) {
        if (now.rows[i].item != before.rows[i].item) return -1;
        const int gained = now.rows[i].count - before.rows[i].count;
        if (gained == 0) continue;
        if (gained != 1 || row != -1) return -1;
        row = i;
    }
    return row;
}

// How many times a recipe can be paid for: the scarcest ingredient decides. `owned` and
// `quantity` hold one value per ingredient.
inline int setsOwned(const int* owned, const int* quantity, int ingredients) {
    if (ingredients <= 0) return 0;
    int sets = -1;
    for (int i = 0; i < ingredients; ++i) {
        if (quantity[i] <= 0 || owned[i] < quantity[i]) return 0;
        const int enough = owned[i] / quantity[i];
        if (sets == -1 || enough < sets) sets = enough;
    }
    return sets;
}

// Stands for "no carry limit" where room is counted.
const int NO_LIMIT = 1 << 30;

// How many items to add to the one the game just made. `sets` and `room` are what the player
// can pay for and can still carry now, after that item; `maxPerAnimation` counts it too.
//
// The game decides whether another craft may follow in the moment it finishes one, and what
// follows is not checked again: with nothing left to pay it would hand out a free item, and with
// no room left it would take the ingredients and add nothing. So unless the game is certain to
// decide after the batch (`gameChecksAgain`), one set and one place are left for it, and the
// last item of a pile of ingredients takes one more animation.
constexpr int batchSize(int maxPerAnimation, int sets, int room, bool gameChecksAgain) {
    const int keep = gameChecksAgain ? 0 : 1;
    int size = maxPerAnimation - 1;
    if (sets - keep < size) size = sets - keep;
    if (room - keep < size) size = room - keep;
    return size > 0 ? size : 0;
}

// Two animation events on the player, as seen in one frame. `stow` is the one on which the game
// adds a cooked or fire-crafted item to the satchel. `decide` is either of the two on which the
// game then decides, once, whether "Cook Another" or "Craft Another" is offered.
struct AnimEvents {
    bool stow, decide;
};

// True when the game is certain to take that decision after a batch made in this frame, so the
// batch may use the last ingredients and the last room. That holds only for recipes made at the
// fire, and only when the stow event was seen in this frame or the one before (the plugin may
// run before or after the game's script in a frame) with no deciding event in either, and the
// game is not already waiting at that prompt. Every other craft decides in the moment it
// finishes.
constexpr bool gameChecksAgain(Hash costKey, AnimEvents previousFrame, AnimEvents thisFrame, bool atAnotherPrompt) {
    const bool atFire = costKey == COST_CRAFTING_FIRE || costKey == COST_CRAFTING_FIRE_2 ||
                        costKey == COST_CRAFTING_KNIFE || costKey == COST_CRAFTING_GRILL;
    return atFire && !atAnotherPrompt && (previousFrame.stow || thisFrame.stow) && !previousFrame.decide && !thisFrame.decide;
}

// What the game's scripts ask of a product to file it under a recipe type.
struct ItemTraits {
    bool food;       // a cooked consumable
    bool ammo;       // of the item group AMMO
    bool thrown;     // tagged as a thrown weapon
    bool weapon;     // of the item group WEAPON
    bool horseItem;  // tagged as a horse item
    bool medicine;   // tagged as medicine
    bool remedy;     // tagged as a remedy
    bool hunting;    // tagged as a hunting item
};

// The recipe type the game counts a crafted product under, in the order its scripts test, or 0
// when none fits.
constexpr Hash recipeType(const ItemTraits& item) {
    if (item.food) return joaat("FOOD");
    if (item.ammo && !item.thrown) return joaat("AMMO");
    if (item.horseItem) return joaat("HORSE_CARE");
    if (item.medicine || item.remedy) return joaat("REMEDIES");
    if (item.weapon || item.ammo) return joaat("WEAPONS");
    if (item.hunting) return joaat("HUNTING");
    return 0;
}

// The statistic in which the game keeps how many items of a tagged kind the player holds, or 0.
// The game's scripts raise it on every add and lower it on every removal.
constexpr Hash heldItemStat(bool deadEye, bool medicine, bool lockBreaker) {
    if (deadEye) return medicine ? joaat("MEDICINE_AND_DEADEYE_ITEMS") : joaat("DEADEYE_ITEMS");
    if (medicine) return joaat("MEDICINE_ITEMS");
    if (lockBreaker) return joaat("LOCK_BREAKER_ITEMS");
    return 0;
}

// The herbs whose use as an ingredient the game counts, each with the name it is counted under.
// The table is the one in the game's scripts (interactive_campfire.c, func_524 and func_841).
struct HerbStat {
    Hash item, stat;
};
constexpr HerbStat HERB_STATS[] = {
    {joaat("PROVISION_RO_FLOWER_ACUNAS_STAR"), joaat("HERB_ACUNAS_STAR_ORCHID")},
    {joaat("CONSUMABLE_HERB_GINSENG"), joaat("HERB_ALASKAN_GINSENG")},
    {joaat("CONSUMABLE_HERB_BAY_BOLETE"), joaat("HERB_BAY_BOLETE")},
    {joaat("CONSUMABLE_HERB_BLACK_BERRY"), joaat("HERB_BLACK_BERRY")},
    {joaat("CONSUMABLE_HERB_CURRANT"), joaat("HERB_BLACK_CURRANT")},
    {joaat("CONSUMABLE_HERB_BURDOCK_ROOT"), joaat("HERB_BURDOCK_ROOT")},
    {joaat("CONSUMABLE_HERB_CHANTERELLES"), joaat("HERB_CHANTERELLES")},
    {joaat("PROVISION_RO_FLOWER_CIGAR"), joaat("HERB_CIGAR_ORCHID")},
    {joaat("PROVISION_RO_FLOWER_CLAMSHELL"), joaat("HERB_CLAMSHELL_ORCHID")},
    {joaat("CONSUMABLE_HERB_COMMON_BULRUSH"), joaat("HERB_COMMON_BULRUSH")},
    {joaat("CONSUMABLE_HERB_CREEPING_THYME"), joaat("HERB_CREEPING_THYME")},
    {joaat("PROVISION_RO_FLOWER_DRAGONS"), joaat("HERB_DRAGONS_MOUTH_ORCHID")},
    {joaat("CONSUMABLE_HERB_ENGLISH_MACE"), joaat("HERB_ENGLISH_MACE")},
    {joaat("CONSUMABLE_HERB_EVERGREEN_HUCKLEBERRY"), joaat("HERB_EVERGREEN_HUCKLEBERRY")},
    {joaat("PROVISION_RO_FLOWER_GHOST"), joaat("HERB_GHOST_ORCHID")},
    {joaat("CONSUMABLE_HERB_INDIAN_TOBACCO"), joaat("HERB_INDIAN_TOBACCO")},
    {joaat("PROVISION_RO_FLOWER_LADY_OF_NIGHT"), joaat("HERB_LADY_OF_NIGHT_ORCHID")},
    {joaat("PROVISION_RO_FLOWER_LADY_SLIPPER"), joaat("HERB_LADY_SLIPPER_ORCHID")},
    {joaat("CONSUMABLE_HERB_MILKWEED"), joaat("HERB_MILKWEED")},
    {joaat("PROVISION_RO_FLOWER_MOCCASIN"), joaat("HERB_MOCCASIN_FLOWER_ORCHID")},
    {joaat("PROVISION_RO_FLOWER_NIGHT_SCENTED"), joaat("HERB_NIGHT_SCENTED_ORCHID")},
    {joaat("CONSUMABLE_HERB_OLEANDER_SAGE"), joaat("HERB_OLEANDER_SAGE")},
    {joaat("CONSUMABLE_HERB_OREGANO"), joaat("HERB_OREGANO")},
    {joaat("CONSUMABLE_HERB_PARASOL_MUSHROOM"), joaat("HERB_PARASOL_MUSHROOM")},
    {joaat("CONSUMABLE_HERB_PRAIRIE_POPPY"), joaat("HERB_PRAIRIE_POPPY")},
    {joaat("PROVISION_RO_FLOWER_QUEENS"), joaat("HERB_QUEENS_ORCHID")},
    {joaat("CONSUMABLE_HERB_RAMS_HEAD"), joaat("HERB_RAMS_HEAD")},
    {joaat("PROVISION_RO_FLOWER_RAT_TAIL"), joaat("HERB_RAT_TAIL_ORCHID")},
    {joaat("CONSUMABLE_HERB_RED_RASPBERRY"), joaat("HERB_RED_RASPBERRY")},
    {joaat("CONSUMABLE_HERB_SAGE"), joaat("HERB_RED_SAGE")},
    {joaat("PROVISION_RO_FLOWER_SPARROWS"), joaat("HERB_SPARROWS_EGG_ORCHID")},
    {joaat("PROVISION_RO_FLOWER_SPIDER"), joaat("HERB_SPIDER_ORCHID")},
    {joaat("CONSUMABLE_HERB_VANILLA_FLOWER"), joaat("HERB_VANILLA_FLOWER")},
    {joaat("CONSUMABLE_HERB_VIOLET_SNOWDROP"), joaat("HERB_VIOLET_SNOWDROP")},
    {joaat("CONSUMABLE_HERB_WILD_CARROTS"), joaat("HERB_WILD_CARROTS")},
    {joaat("CONSUMABLE_HERB_WILD_FEVERFEW"), joaat("HERB_WILD_FEVERFEW")},
    {joaat("CONSUMABLE_HERB_WILD_MINT"), joaat("HERB_WILD_MINT")},
    {joaat("CONSUMABLE_HERB_WINTERGREEN_BERRY"), joaat("HERB_WINTERGREEN_BERRY")},
    {joaat("CONSUMABLE_HERB_YARROW"), joaat("HERB_YARROW")},
};

// The name an ingredient's use is counted under, or 0 for an ingredient that is not counted.
constexpr Hash herbStat(Hash item) {
    for (const HerbStat& herb : HERB_STATS) {
        if (herb.item == item) return herb.stat;
    }
    return 0;
}

// The "item added" messages the game has not shown yet: Global_1911774 in the decompiled
// scripts. While the first value is set, the game's scripts queue their item messages here
// instead of showing them, and short_update shows the queue once the hold is lifted or has
// lasted longer than its timeout. An entry is 4 values: item, count, two flags.
const int TOAST_QUEUE = 1911774;
const int TOAST_ROOM = 20;
enum { TOAST_HOLD, TOAST_SINCE, TOAST_TIMEOUT, TOAST_COUNT, TOAST_LENGTH, TOAST_FIRST };

// Puts the items of a batch into the queued messages: on the message the game queued for its
// own craft of `product` (the last one that shows a single item), or else as a message of its
// own. A queue that is full or laid out differently is left alone.
enum class Toast { Raised, Appended, Dropped };
inline Toast countInToast(Slot* queue, Hash product, int added) {
    if (static_cast<int>(queue[TOAST_LENGTH]) != TOAST_ROOM) return Toast::Dropped;
    const int count = static_cast<int>(queue[TOAST_COUNT]);
    for (int i = (count < TOAST_ROOM ? count : TOAST_ROOM) - 1; i >= 0; --i) {
        Slot* const entry = queue + TOAST_FIRST + 4 * i;
        if (static_cast<Hash>(entry[0]) != product || static_cast<int>(entry[1]) != 1) continue;
        entry[1] = static_cast<Slot>(1 + added);
        return Toast::Raised;
    }
    if (count < 0 || count >= TOAST_ROOM) return Toast::Dropped;
    Slot* const entry = queue + TOAST_FIRST + 4 * count;
    entry[0] = product;
    entry[1] = static_cast<Slot>(added);
    entry[2] = entry[3] = 0;
    queue[TOAST_COUNT] = static_cast<Slot>(count + 1);
    return Toast::Appended;
}

// The button prompts of the game's scripts: Global_1945188, 48 rows of 18 values behind a
// length. The fields of a row the plugin reads:
const int PROMPTS = 1945188;
const int PROMPT_ROWS = 48;
enum { PROMPT_PRIORITY = 0, PROMPT_FLAGS = 1, PROMPT_TRANSPORT = 2, PROMPT_HANDLE = 3, PROMPT_CONTROL = 4, PROMPT_TYPE = 5, PROMPT_OWNER = 16 };
constexpr int promptGlobal(int row, int field) { return PROMPTS + 1 + 18 * row + field; }
const int PROMPT_REGISTERED = 2, PROMPT_OFF = 4;
constexpr Hash INPUT_CONTEXT_A = joaat("INPUT_CONTEXT_A");

// Whether a row is the progress bar of cooking and fire crafting, by the values the crafting
// scripts register it with. "Cook Another", "Pour" and "Fast Travel" at the fire have the same
// values and do not fill by themselves, and the world's "Pour" has other values: the caller asks
// the game whether the prompt fills by itself and which script owns it.
constexpr bool isProgressPrompt(int priority, int flags, int transport, Hash control, int type) {
    return (flags & PROMPT_REGISTERED) && control == INPUT_CONTEXT_A && priority == 2 && transport == 0 && type == 0;
}

// A hash, written to a Line as 0x and eight digits.
struct Hex {
    Hash value;
};

// One line of the log, put together without the C runtime. Text that does not fit is cut.
struct Line {
    static const int CAPACITY = 255;
    char text[CAPACITY + 1];
    int length = 0;

    Line() { text[0] = '\0'; }

    Line& operator<<(const char* words) {
        while (*words && length < CAPACITY) text[length++] = *words++;
        text[length] = '\0';
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
        if (number < 0) digits[count++] = '-';
        char reversed[12];
        for (int i = 0; i < count; ++i) reversed[i] = digits[count - 1 - i];
        reversed[count] = '\0';
        return *this << reversed;
    }
    Line& operator<<(Hex hash) {
        char digits[11] = "0x";
        for (int i = 0; i < 8; ++i) digits[2 + i] = "0123456789ABCDEF"[(hash.value >> (28 - 4 * i)) & 15];
        digits[10] = '\0';
        return *this << digits;
    }
};
