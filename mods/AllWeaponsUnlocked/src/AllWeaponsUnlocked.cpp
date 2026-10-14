// AllWeaponsUnlocked: a ScriptHookRDR2 plugin that puts every weapon on sale from the start.
//
// Recreated from the disassembly of AllWeaponsUnlocked.asi 1.1.0 (built 2022-01-27), which
// marked 113 hard-coded weapon hashes as unlocked and visible. Changes from that version:
//   - marks weapons visible only. Shops read just that flag, and buying a weapon unlocks it.
//     "Unlocked" is the game's record of which weapon types the player owns: with every weapon
//     unlocked it armed enemies and lawmen from the full pool from the first fight, stopped
//     withholding loot and picked-up guns, put the Ornate Dagger on sale without its hunt, and
//     kept all of that in the save
//   - adds 30 weapons it left out
//   - drops its 17 entries that are not weapons (unarmed and the animal attacks)
//   - skips weapons the game has no unlock entry for
//   - re-applies once a second instead of every frame

// Functions imported from ScriptHookRDR2.dll, with the signatures encoded in the original's
// import table (see ScriptHookRDR2.def).
struct HINSTANCE__;
__declspec(dllimport) void scriptRegister(HINSTANCE__* module, void (*scriptMain)());
__declspec(dllimport) void scriptUnregister(HINSTANCE__* module);
__declspec(dllimport) void scriptWait(unsigned long milliseconds);
__declspec(dllimport) void nativeInit(unsigned long long hash);
__declspec(dllimport) void nativePush64(unsigned long long value);
__declspec(dllimport) unsigned long long* nativeCall();

using Hash = unsigned int;

// Game natives, by hash.
const unsigned long long GET_WEAPON_UNLOCK = 0x865F36299079FB75;    // WEAPON::_GET_WEAPON_UNLOCK(Hash weaponHash) -> Hash
const unsigned long long UNLOCK_SET_VISIBLE = 0x46B901A8ECDB5A61;   // UNLOCK::UNLOCK_SET_VISIBLE(Hash unlockHash, BOOL toggle)

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
static_assert(joaat("weapon_revolver_cattleman") == 0x169F59F7, "joaat must reproduce the hashes found in the original");

constexpr Hash WEAPONS[] = {
    joaat("weapon_revolver_cattleman"),
    joaat("weapon_melee_knife"),
    joaat("weapon_shotgun_doublebarrel"),
    joaat("weapon_melee_lantern"),
    joaat("weapon_repeater_carbine"),
    joaat("weapon_revolver_schofield_bill"),
    joaat("weapon_rifle_boltaction_bill"),
    joaat("weapon_melee_knife_bill"),
    joaat("weapon_shotgun_sawedoff_charles"),
    joaat("weapon_bow_charles"),
    joaat("weapon_melee_knife_charles"),
    joaat("weapon_thrown_tomahawk"),
    joaat("weapon_revolver_schofield_dutch"),
    joaat("weapon_revolver_schofield_dutch_dualwield"),
    joaat("weapon_melee_knife_dutch"),
    joaat("weapon_revolver_cattleman_hosea"),
    joaat("weapon_revolver_cattleman_hosea_dualwield"),
    joaat("weapon_shotgun_semiauto_hosea"),
    joaat("weapon_melee_knife_hosea"),
    joaat("weapon_revolver_doubleaction_javier"),
    joaat("weapon_thrown_throwing_knives_javier"),
    joaat("weapon_melee_knife_javier"),
    joaat("weapon_revolver_cattleman_john"),
    joaat("weapon_repeater_winchester_john"),
    joaat("weapon_melee_knife_john"),
    joaat("weapon_revolver_cattleman_kieran"),
    joaat("weapon_melee_knife_kieran"),
    joaat("weapon_revolver_cattleman_lenny"),
    joaat("weapon_sniperrifle_rollingblock_lenny"),
    joaat("weapon_melee_knife_lenny"),
    joaat("weapon_revolver_doubleaction_micah"),
    joaat("weapon_revolver_doubleaction_micah_dualwield"),
    joaat("weapon_melee_knife_micah"),
    joaat("weapon_revolver_cattleman_sadie"),
    joaat("weapon_revolver_cattleman_sadie_dualwield"),
    joaat("weapon_repeater_carbine_sadie"),
    joaat("weapon_thrown_throwing_knives"),
    joaat("weapon_melee_knife_sadie"),
    joaat("weapon_revolver_cattleman_sean"),
    joaat("weapon_melee_knife_sean"),
    joaat("weapon_revolver_schofield_uncle"),
    joaat("weapon_shotgun_doublebarrel_uncle"),
    joaat("weapon_melee_knife_uncle"),
    joaat("weapon_revolver_doubleaction"),
    joaat("weapon_rifle_boltaction"),
    joaat("weapon_revolver_schofield"),
    joaat("weapon_rifle_springfield"),
    joaat("weapon_repeater_winchester"),
    joaat("weapon_rifle_varmint"),
    joaat("weapon_pistol_volcanic"),
    joaat("weapon_shotgun_sawedoff"),
    joaat("weapon_pistol_semiauto"),
    joaat("weapon_pistol_mauser"),
    joaat("weapon_repeater_henry"),
    joaat("weapon_shotgun_pump"),
    joaat("weapon_bow"),
    joaat("weapon_thrown_molotov"),
    joaat("weapon_melee_hatchet_hewing"),
    joaat("weapon_melee_machete"),
    joaat("weapon_revolver_doubleaction_exotic"),
    joaat("weapon_revolver_schofield_golden"),
    joaat("weapon_thrown_dynamite"),
    joaat("weapon_melee_davy_lantern"),
    joaat("weapon_lasso"),
    joaat("weapon_kit_binoculars"),
    joaat("weapon_kit_camera"),
    joaat("weapon_fishingrod"),
    joaat("weapon_sniperrifle_rollingblock"),
    joaat("weapon_shotgun_semiauto"),
    joaat("weapon_shotgun_repeating"),
    joaat("weapon_sniperrifle_carcano"),
    joaat("weapon_melee_broken_sword"),
    joaat("weapon_melee_knife_bear"),
    joaat("weapon_melee_knife_civil_war"),
    joaat("weapon_melee_knife_jawbone"),
    joaat("weapon_melee_knife_miner"),
    joaat("weapon_melee_knife_vampire"),
    joaat("weapon_melee_torch"),
    joaat("weapon_melee_lantern_electric"),
    joaat("weapon_melee_hatchet"),
    joaat("weapon_melee_ancient_hatchet"),
    joaat("weapon_melee_cleaver"),
    joaat("weapon_melee_hatchet_double_bit"),
    joaat("weapon_melee_hatchet_double_bit_rusted"),
    joaat("weapon_melee_hatchet_hunter"),
    joaat("weapon_melee_hatchet_hunter_rusted"),
    joaat("weapon_melee_hatchet_viking"),
    joaat("weapon_revolver_cattleman_mexican"),
    joaat("weapon_revolver_cattleman_pig"),
    joaat("weapon_revolver_schofield_calloway"),
    joaat("weapon_pistol_mauser_drunk"),
    joaat("weapon_shotgun_doublebarrel_exotic"),
    joaat("weapon_sniperrifle_rollingblock_exotic"),
    joaat("weapon_thrown_tomahawk_ancient"),
    joaat("weapon_melee_torch_crowd"),
    joaat("weapon_melee_hatchet_meleeonly"),
    // Missing from 1.1.0, added in this rebuild.
    joaat("weapon_bow_improved"),
    joaat("weapon_kit_binoculars_improved"),
    joaat("weapon_kit_camera_advanced"),
    joaat("weapon_kit_detector"),
    joaat("weapon_kit_metal_detector"),
    joaat("weapon_lasso_reinforced"),
    joaat("weapon_melee_knife_horror"),
    joaat("weapon_melee_knife_rustic"),
    joaat("weapon_melee_knife_trader"),
    joaat("weapon_melee_lantern_halloween"),
    joaat("weapon_melee_machete_collector"),
    joaat("weapon_melee_machete_horror"),
    joaat("weapon_moonshinejug"),
    joaat("weapon_moonshinejug_mp"),
    joaat("weapon_pistol_m1899"),
    joaat("weapon_repeater_evans"),
    joaat("weapon_revolver_cattleman_dualwield"),
    joaat("weapon_revolver_doubleaction_dualwield"),
    joaat("weapon_revolver_doubleaction_gambler"),
    joaat("weapon_revolver_lemat"),
    joaat("weapon_revolver_navy"),
    joaat("weapon_revolver_navy_crossover"),
    joaat("weapon_revolver_schofield_dualwield"),
    joaat("weapon_rifle_elephant"),
    joaat("weapon_thrown_bolas"),
    joaat("weapon_thrown_bolas_hawkmoth"),
    joaat("weapon_thrown_bolas_intertwined"),
    joaat("weapon_thrown_bolas_ironspiked"),
    joaat("weapon_thrown_poisonbottle"),
    joaat("weapon_thrown_tomahawk_meleeonly"),
};

static Hash getWeaponUnlock(Hash weapon) {
    nativeInit(GET_WEAPON_UNLOCK);
    nativePush64(weapon);
    return static_cast<Hash>(*nativeCall());
}

static void setVisible(Hash unlock) {
    nativeInit(UNLOCK_SET_VISIBLE);
    nativePush64(unlock);
    nativePush64(true);
    nativeCall();
}

// ponytail: re-applies on a timer, because a new game and a mission replay hide every weapon
// again; detect those instead if a delay of up to a second ever matters.
const unsigned long REAPPLY_INTERVAL_MS = 1000;

static void scriptMain() {
    for (;;) {
        for (Hash weapon : WEAPONS) {
            Hash unlock = getWeaponUnlock(weapon);
            if (!unlock) continue;  // the game has no unlock entry for this weapon
            setVisible(unlock);
        }
        scriptWait(REAPPLY_INTERVAL_MS);
    }
}

extern "C" int DllMain(HINSTANCE__* module, unsigned long reason, void*) {
    const unsigned long DLL_PROCESS_DETACH = 0, DLL_PROCESS_ATTACH = 1;
    if (reason == DLL_PROCESS_ATTACH) scriptRegister(module, scriptMain);
    else if (reason == DLL_PROCESS_DETACH) scriptUnregister(module);
    return 1;
}
