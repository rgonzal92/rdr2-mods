// Checks BatchLogic's rules without the game. Build and run with `make test`.
#include "../src/BatchLogic.h"

#include <cassert>
#include <cstdio>
#include <cstring>

// A recipe list of three products, as the plugin reads it once a frame.
static Snapshot list(int crafted, int first, int second, int third) {
    Snapshot snapshot = {};
    snapshot.crafted = crafted;
    snapshot.size = 3;
    snapshot.rows[0] = {0xA1, COST_CRAFTING, first};
    snapshot.rows[1] = {0xB2, COST_CRAFTING_FIRE, second};
    snapshot.rows[2] = {0xC3, COST_CRAFTING_KNIFE, third};
    return snapshot;
}

static void testGlobalIndexes() {
    // Global_1911914 is an array of 175 rows of 9 values behind a length; f_1 of a row is its
    // list entry. The values after the array are Global_1911914.f_1576 and .f_1582.
    assert(rowEntryGlobal(0) == 1911916);
    assert(rowEntryGlobal(1) == 1911925);
    assert(rowEntryGlobal(RECIPE_LIST_ROWS - 1) == 1911914 + 1 + 9 * 174 + 1);
    assert(RECIPE_ROW_COUNT == 1913490);
    assert(CRAFTING_ACTIVE == 1913496);
}

static void testCraftedRow() {
    const Snapshot before = list(40, 2, 0, 5);
    // The game counted one craft and one product went up by one: that row.
    assert(craftedRow(before, list(41, 3, 0, 5)) == 0);
    assert(craftedRow(before, list(41, 2, 0, 6)) == 2);
    // Eaten, used or poured: counted, nothing stowed.
    assert(craftedRow(before, list(41, 2, 0, 5)) == -1);
    // A product arrived without a craft being counted.
    assert(craftedRow(before, list(40, 3, 0, 5)) == -1);
    // Anything but one craft and one item is not a single finished craft (a loaded save, say).
    assert(craftedRow(before, list(42, 3, 0, 5)) == -1);
    assert(craftedRow(before, list(41, 4, 0, 5)) == -1);
    assert(craftedRow(before, list(41, 3, 1, 5)) == -1);
    assert(craftedRow(before, list(41, 3, 0, 4)) == -1);
    assert(craftedRow(before, list(39, 3, 0, 5)) == -1);

    // The list was rebuilt in between: rows no longer line up.
    Snapshot shorter = list(41, 3, 0, 5);
    shorter.size = 2;
    assert(craftedRow(before, shorter) == -1);
    Snapshot swapped = list(41, 3, 0, 5);
    swapped.rows[0].item = 0xD4;  // the row now shows another product
    assert(craftedRow(before, swapped) == -1);
    // Only the cost shown for a row changed (the game moved it to an affordable variant after
    // its own craft): still that row. The plugin pays with the cost from `before`.
    Snapshot otherCost = list(41, 3, 0, 5);
    otherCost.rows[0].costKey = COST_CRAFTING_2;
    assert(craftedRow(before, otherCost) == 0);

    assert(craftedRow(Snapshot{}, Snapshot{}) == -1);
}

static void testSetsOwned() {
    const int quantity[] = {1, 2, 6};
    const int plenty[] = {30, 60, 180};
    assert(setsOwned(plenty, quantity, 3) == 30);
    const int shortOfSecond[] = {30, 7, 180};
    assert(setsOwned(shortOfSecond, quantity, 3) == 3);
    const int noneOfThird[] = {30, 60, 5};
    assert(setsOwned(noneOfThird, quantity, 3) == 0);
    assert(setsOwned(plenty, quantity, 1) == 30);
    const int exactlyOne[] = {1, 2, 6};
    assert(setsOwned(exactlyOne, quantity, 3) == 1);
    // No recipe, or a recipe the catalog should not hold: nothing can be made.
    assert(setsOwned(plenty, quantity, 0) == 0);
    const int zeroQuantity[] = {1, 0, 6};
    assert(setsOwned(plenty, zeroQuantity, 3) == 0);
    const int negative[] = {-1, 60, 180};
    assert(setsOwned(negative, quantity, 3) == 0);
}

static void testBatchSize() {
    // Counts are taken after the game's own item: 30 sets at the start leave 29.
    // The game checks again before it offers another: use everything. 1 + 29 = 30 in one go.
    assert(batchSize(50, 29, NO_LIMIT, true) == 29);
    // The game has already decided: its next craft must still be paid for and must still fit,
    // so one set and one place are left for it. 1 + 28 now, the 30th with the next animation.
    assert(batchSize(50, 29, NO_LIMIT, false) == 28);
    // The setting counts the game's own item.
    assert(batchSize(50, 200, NO_LIMIT, true) == 49);
    assert(batchSize(50, 200, NO_LIMIT, false) == 49);
    assert(batchSize(50, 50, NO_LIMIT, false) == 49);
    assert(batchSize(50, 49, NO_LIMIT, false) == 48);
    // What the player can still carry limits it the same way.
    assert(batchSize(50, 200, 4, true) == 4);
    assert(batchSize(50, 200, 4, false) == 3);
    assert(batchSize(50, 3, 2, true) == 2);
    // Nothing left to use or no room: the game's item stays the only one.
    assert(batchSize(50, 0, NO_LIMIT, true) == 0);
    assert(batchSize(50, 1, NO_LIMIT, false) == 0);
    assert(batchSize(50, 200, 0, true) == 0);
    assert(batchSize(50, 200, 1, false) == 0);
    assert(batchSize(50, 200, -3, true) == 0);
    assert(batchSize(50, -1, NO_LIMIT, true) == 0);
    // 1 is the game's own behaviour, 0 and less switch the plugin off.
    assert(batchSize(1, 200, NO_LIMIT, true) == 0);
    assert(batchSize(0, 200, NO_LIMIT, true) == 0);
    assert(batchSize(-5, 200, NO_LIMIT, true) == 0);
    assert(batchSize(2, 200, NO_LIMIT, false) == 1);
}

static void testGameChecksAgain() {
    const AnimEvents nothing = {false, false}, stowed = {true, false}, decided = {false, true}, both = {true, true};
    // Cooking and fire crafting: the item is stowed, and a moment later the game decides once
    // whether "Cook Another" is offered. A batch between the two is seen by that decision.
    assert(gameChecksAgain(COST_CRAFTING_KNIFE, nothing, stowed, false));
    assert(gameChecksAgain(COST_CRAFTING_GRILL, stowed, nothing, false));  // seen one frame late
    assert(gameChecksAgain(COST_CRAFTING_FIRE, nothing, stowed, false));
    assert(gameChecksAgain(COST_CRAFTING_FIRE_2, nothing, stowed, false));
    // The decision fell in the same frames: it may have been taken before the batch.
    assert(!gameChecksAgain(COST_CRAFTING_KNIFE, nothing, both, false));
    assert(!gameChecksAgain(COST_CRAFTING_KNIFE, stowed, decided, false));
    assert(!gameChecksAgain(COST_CRAFTING_KNIFE, decided, stowed, false));
    // The decision fell earlier still: the game is already waiting at the prompt.
    assert(!gameChecksAgain(COST_CRAFTING_KNIFE, nothing, stowed, true));
    assert(!gameChecksAgain(COST_CRAFTING_FIRE, stowed, nothing, true));
    // The stow was not seen at all (an interrupted craft hands the item over without it).
    assert(!gameChecksAgain(COST_CRAFTING_KNIFE, nothing, nothing, false));
    assert(!gameChecksAgain(COST_CRAFTING_KNIFE, nothing, decided, false));
    // Ammunition, arrows, thrown weapons, baits and horse meal: the game decides about the
    // next one in the same moment it finishes one, whatever the animation says.
    assert(!gameChecksAgain(COST_CRAFTING, nothing, stowed, false));
    assert(!gameChecksAgain(COST_CRAFTING_2, nothing, stowed, false));
    assert(!gameChecksAgain(COST_CRAFTING_3, stowed, nothing, false));
    assert(!gameChecksAgain(COST_CRAFTING_4, nothing, stowed, false));
    assert(!gameChecksAgain(0, nothing, stowed, false));
}

static void testRecipeType() {
    ItemTraits item = {};
    assert(recipeType(item) == 0);
    item.food = true;
    assert(recipeType(item) == joaat("FOOD"));
    item = {};
    item.ammo = true;
    assert(recipeType(item) == joaat("AMMO"));
    item.thrown = true;  // a thrown weapon is counted as a weapon, not as ammunition
    assert(recipeType(item) == joaat("WEAPONS"));
    item = {};
    item.weapon = true;
    assert(recipeType(item) == joaat("WEAPONS"));
    item = {};
    item.medicine = true;
    assert(recipeType(item) == joaat("REMEDIES"));
    item = {};
    item.remedy = true;
    assert(recipeType(item) == joaat("REMEDIES"));
    item.horseItem = true;  // a horse medicine is horse care
    assert(recipeType(item) == joaat("HORSE_CARE"));
    item = {};
    item.hunting = true;
    assert(recipeType(item) == joaat("HUNTING"));
    // The game's order: food, ammunition, horse care, remedies, weapons, hunting.
    item.food = true;
    assert(recipeType(item) == joaat("FOOD"));
    item = {};
    item.hunting = item.medicine = true;
    assert(recipeType(item) == joaat("REMEDIES"));
    item.ammo = true;
    assert(recipeType(item) == joaat("AMMO"));
    item = {};
    item.ammo = item.horseItem = true;
    assert(recipeType(item) == joaat("AMMO"));
    item = {};
    item.weapon = item.hunting = true;
    assert(recipeType(item) == joaat("WEAPONS"));
}

static void testHeldItemStat() {
    assert(heldItemStat(false, false, false) == 0);
    assert(heldItemStat(true, false, false) == joaat("DEADEYE_ITEMS"));
    assert(heldItemStat(true, true, false) == joaat("MEDICINE_AND_DEADEYE_ITEMS"));
    assert(heldItemStat(false, true, false) == joaat("MEDICINE_ITEMS"));
    assert(heldItemStat(false, false, true) == joaat("LOCK_BREAKER_ITEMS"));
    assert(heldItemStat(false, true, true) == joaat("MEDICINE_ITEMS"));
    assert(heldItemStat(true, false, true) == joaat("DEADEYE_ITEMS"));
}

static void testHerbStat() {
    // Hashes as listed from the game's own table (interactive_campfire.c, func_524 and func_841).
    assert(herbStat(0x556A9746) == 0xC8657493);  // CONSUMABLE_HERB_GINSENG -> HERB_ALASKAN_GINSENG
    assert(herbStat(0x3E6C4053) == 0x2179C985);  // CONSUMABLE_HERB_CURRANT -> HERB_BLACK_CURRANT
    assert(herbStat(0x95D230D6) == 0x3AB00E21);  // CONSUMABLE_HERB_SAGE -> HERB_RED_SAGE
    assert(herbStat(0xD9318A6A) == 0x7A95FD10);  // PROVISION_RO_FLOWER_ACUNAS_STAR, the first row
    assert(herbStat(0xE06E5D84) == 0x0A2626F5);  // CONSUMABLE_HERB_YARROW, the last row
    assert(herbStat(0xE61255CE) == 0x4CD245AF);  // CONSUMABLE_HERB_WILD_MINT
    assert(herbStat(joaat("CONSUMABLE_MOONSHINE")) == 0);
    assert(herbStat(0) == 0);
    assert(sizeof HERB_STATS / sizeof HERB_STATS[0] == 39);
    // Every row at once: the sum over the game's table, computed from its hashes.
    Hash sum = 0;
    for (const HerbStat& herb : HERB_STATS) sum += herb.item ^ herb.stat;
    assert(sum == 0xE58ACA2E);
}

static void testLine() {
    Line line;
    line << "item " << Hex{0xE61255CE} << " sets " << 30 << " room " << -4 << " zero " << 0;
    assert(!std::strcmp(line.text, "item 0xE61255CE sets 30 room -4 zero 0"));
    assert(line.length == static_cast<int>(std::strlen(line.text)));

    Line small;
    small << Hex{0x1A} << " " << -2147483647 - 1 << " " << 2147483647;
    assert(!std::strcmp(small.text, "0x0000001A -2147483648 2147483647"));

    // A new line is empty whatever was in its memory before.
    assert(Line().length == 0 && Line().text[0] == '\0');

    // Text that does not fit is cut, never written past the end.
    Line full;
    for (int i = 0; i < 100; ++i) full << "0123456789";
    assert(full.length == Line::CAPACITY);
    assert(static_cast<int>(std::strlen(full.text)) == Line::CAPACITY);
    full << 12345 << Hex{0xFFFFFFFF};
    assert(full.length == Line::CAPACITY);
    assert(full.text[Line::CAPACITY] == '\0');
}

static void testToastQueue() {
    // Global_1911774: hold, since, timeout, count, the array's length, then 20 entries of 4.
    assert(TOAST_QUEUE == 1911774 && TOAST_FIRST + 4 * TOAST_ROOM == 85);

    // The game queued "1 of 0xA1" for its own craft: the batch raises that entry's count.
    Slot queue[85] = {};
    queue[TOAST_LENGTH] = TOAST_ROOM;
    queue[TOAST_COUNT] = 2;
    queue[TOAST_FIRST] = 0xB2, queue[TOAST_FIRST + 1] = 1;
    queue[TOAST_FIRST + 4] = 0xA1, queue[TOAST_FIRST + 5] = 1;
    assert(countInToast(queue, 0xA1, 29) == Toast::Raised);
    assert(queue[TOAST_FIRST + 5] == 30 && queue[TOAST_FIRST + 1] == 1 && queue[TOAST_COUNT] == 2);

    // An entry that already shows a count is not the game's craft: the batch gets its own.
    assert(countInToast(queue, 0xA1, 5) == Toast::Appended);
    assert(queue[TOAST_COUNT] == 3 && queue[TOAST_FIRST + 5] == 30);
    assert(queue[TOAST_FIRST + 8] == 0xA1 && queue[TOAST_FIRST + 9] == 5);
    assert(queue[TOAST_FIRST + 10] == 0 && queue[TOAST_FIRST + 11] == 0);

    // Of two entries for the product, the later one is the craft just made.
    Slot twice[85] = {};
    twice[TOAST_LENGTH] = TOAST_ROOM;
    twice[TOAST_COUNT] = 2;
    twice[TOAST_FIRST] = 0xA1, twice[TOAST_FIRST + 1] = 1;
    twice[TOAST_FIRST + 4] = 0xA1, twice[TOAST_FIRST + 5] = 1;
    assert(countInToast(twice, 0xA1, 3) == Toast::Raised);
    assert(twice[TOAST_FIRST + 1] == 1 && twice[TOAST_FIRST + 5] == 4);

    // A full queue is left alone, and so is one of another length (another game build).
    Slot full[85] = {};
    full[TOAST_LENGTH] = TOAST_ROOM;
    full[TOAST_COUNT] = TOAST_ROOM;
    assert(countInToast(full, 0xA1, 3) == Toast::Dropped);
    assert(full[TOAST_COUNT] == TOAST_ROOM && full[TOAST_FIRST] == 0);
    Slot other[85] = {};
    other[TOAST_LENGTH] = 10;
    assert(countInToast(other, 0xA1, 3) == Toast::Dropped && other[TOAST_COUNT] == 0);
}

static void testProgressPrompt() {
    // Global_1945188: 48 rows of 18 behind a length; the next global is .f_865.
    assert(1 + PROMPT_ROWS * 18 == 865);
    assert(promptGlobal(0, PROMPT_PRIORITY) == 1945189 && promptGlobal(0, PROMPT_HANDLE) == 1945192);
    assert(promptGlobal(1, PROMPT_FLAGS) == 1945190 + 18 && promptGlobal(47, PROMPT_OWNER) == 1945205 + 18 * 47);

    // The progress bar of cooking and fire crafting, as the scripts register it (flags 2 | 128,
    // plus 4 while it is switched off).
    assert(isProgressPrompt(2, 130, 0, INPUT_CONTEXT_A, 0));
    assert(isProgressPrompt(2, 134, 0, INPUT_CONTEXT_A, 0));
    // "Pour" is on the same key; a free row has no flags; another key is another prompt.
    assert(!isProgressPrompt(1, 130, 1, INPUT_CONTEXT_A, 3));
    assert(!isProgressPrompt(2, 0, 0, INPUT_CONTEXT_A, 0));
    assert(!isProgressPrompt(2, 130, 0, 0x12345678, 0));
    assert(INPUT_CONTEXT_A == 0x5181713D);
}

int main() {
    testGlobalIndexes();
    testCraftedRow();
    testSetsOwned();
    testBatchSize();
    testGameChecksAgain();
    testRecipeType();
    testHeldItemStat();
    testHerbStat();
    testLine();
    testToastQueue();
    testProgressPrompt();
    std::puts("BatchLogic: all checks passed");
    return 0;
}
