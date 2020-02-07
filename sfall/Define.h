#pragma once

enum ObjType : char
{
	OBJ_TYPE_ITEM = 0,
	OBJ_TYPE_CRITTER = 1,
	OBJ_TYPE_SCENERY = 2,
	OBJ_TYPE_WALL = 3,
	OBJ_TYPE_TILE = 4,
	OBJ_TYPE_MISC = 5,
	OBJ_TYPE_SPATIAL = 6
};

enum ArtType : char
{
	OBJ_TYPE_INTRFACE   = 6,
	OBJ_TYPE_INVEN      = 7,
	OBJ_TYPE_HEAD       = 8,
	OBJ_TYPE_BACKGROUND = 9,
	OBJ_TYPE_SKILLDEX   = 10
};

// Some FO2 PIDs possibly used by engine
enum ProtoId : long
{
	PID_MOTION_SENSOR = 59,
	PID_SUPER_STIMPAK = 144,
	PID_ACTIVE_GEIGER_COUNTER = 207,
	PID_ACTIVE_STEALTH_BOY = 210,
	PID_JET = 259,
	PID_CAR_TRUNK = 455,
	PID_JESSE_CONTAINER = 467,

	PID_Player = 16777216,
	PID_DRIVABLE_CAR = 33555441,

	// misc type
	PID_CORPSE_BLOOD = 0x05000004,
};

//XXXXXXXXXXXXXXXXXXXXX
//XX Critter defines XX
//XXXXXXXXXXXXXXXXXXXXX

// Trait defines //
#define TRAIT_PERK  (0)
#define TRAIT_OBJECT (1)
#define TRAIT_TRAIT  (2)


enum Perk : long
{
	PERK_bonus_awareness = 0,
	PERK_bonus_hth_attacks = 1,
	PERK_bonus_hth_damage = 2,
	PERK_bonus_move = 3,
	PERK_bonus_ranged_damage = 4,
	PERK_bonus_rate_of_fire = 5,
	PERK_earlier_sequence = 6,
	PERK_faster_healing = 7,
	PERK_more_criticals = 8,
	PERK_night_vision = 9,
	PERK_presence = 10,
	PERK_rad_resistance = 11,
	PERK_toughness = 12,
	PERK_strong_back = 13,
	PERK_sharpshooter = 14,
	PERK_silent_running = 15,
	PERK_survivalist = 16,
	PERK_master_trader = 17,
	PERK_educated = 18,
	PERK_healer = 19,
	PERK_fortune_finder = 20,
	PERK_better_criticals = 21,
	PERK_empathy = 22,
	PERK_slayer = 23,
	PERK_sniper = 24,
	PERK_silent_death = 25,
	PERK_action_boy = 26,
	PERK_mental_block = 27,
	PERK_lifegiver = 28,
	PERK_dodger = 29,
	PERK_snakeater = 30,
	PERK_mr_fixit = 31,
	PERK_medic = 32,
	PERK_master_thief = 33,
	PERK_speaker = 34,
	PERK_heave_ho = 35,
	PERK_friendly_foe = 36,
	PERK_pickpocket = 37,
	PERK_ghost = 38,
	PERK_cult_of_personality = 39,
	PERK_scrounger = 40,
	PERK_explorer = 41,
	PERK_flower_child = 42,
	PERK_pathfinder = 43,
	PERK_animal_friend = 44,
	PERK_scout = 45,
	PERK_mysterious_stranger = 46,
	PERK_ranger = 47,
	PERK_quick_pockets = 48,
	PERK_smooth_talker = 49,
	PERK_swift_learner = 50,
	PERK_tag = 51,
	PERK_mutate = 52,
	PERK_add_nuka = 53,
	PERK_add_buffout = 54,
	PERK_add_mentats = 55,
	PERK_add_psycho = 56,
	PERK_add_radaway = 57,
	PERK_weapon_long_range = 58,
	PERK_weapon_accurate = 59,
	PERK_weapon_penetrate = 60,
	PERK_weapon_knockback = 61,
	PERK_armor_powered = 62,
	PERK_armor_combat = 63,
	PERK_weapon_scope_range = 64,
	PERK_weapon_fast_reload = 65,
	PERK_weapon_night_sight = 66,
	PERK_weapon_flameboy = 67,
	PERK_armor_advanced_1 = 68,
	PERK_armor_advanced_2 = 69,
	PERK_add_jet = 70,
	PERK_add_tragic = 71,
	PERK_armor_charisma = 72,
	PERK_gecko_skinning_perk = 73,
	PERK_dermal_armor_perk = 74,
	PERK_dermal_enhancement_perk = 75,
	PERK_phoenix_armor_perk = 76,
	PERK_phoenix_enhancement_perk = 77,
	PERK_vault_city_inoculations_perk = 78,
	PERK_adrenaline_rush_perk = 79,
	PERK_cautious_nature_perk = 80,
	PERK_comprehension_perk = 81,
	PERK_demolition_expert_perk = 82,
	PERK_gambler_perk = 83,
	PERK_gain_strength_perk = 84,
	PERK_gain_perception_perk = 85,
	PERK_gain_endurance_perk = 86,
	PERK_gain_charisma_perk = 87,
	PERK_gain_intelligence_perk = 88,
	PERK_gain_agility_perk = 89,
	PERK_gain_luck_perk = 90,
	PERK_harmless_perk = 91,
	PERK_here_and_now_perk = 92,
	PERK_hth_evade_perk = 93,
	PERK_kama_sutra_perk = 94,
	PERK_karma_beacon_perk = 95,
	PERK_light_step_perk = 96,
	PERK_living_anatomy_perk = 97,
	PERK_magnetic_personality_perk = 98,
	PERK_negotiator_perk = 99,
	PERK_pack_rat_perk = 100,
	PERK_pyromaniac_perk = 101,
	PERK_quick_recovery_perk = 102,
	PERK_salesman_perk = 103,
	PERK_stonewall_perk = 104,
	PERK_thief_perk = 105,
	PERK_weapon_handling_perk = 106,
	PERK_vault_city_training_perk = 107,
	PERK_alcohol_hp_bonus1_perk = 108,
	PERK_alcohol_hp_bonus2_perk = 109,
	PERK_alcohol_hp_neg1_perk = 110,
	PERK_alcohol_hp_neg2_perk = 111,
	PERK_autodoc_hp_bonus1_perk = 112,
	PERK_autodoc_hp_bonus2_perk = 113,
	PERK_autodoc_hp_neg1_perk = 114,
	PERK_autodoc_hp_neg2_perk = 115,
	PERK_expert_excrement_expediter_perk = 116,
	PERK_weapon_knockout_perk = 117,
	PERK_jinxed_perk = 118,
	PERK_count = 119
};

enum Trait : long
{
	TRAIT_fast_metabolism = 0,
	TRAIT_bruiser = 1,
	TRAIT_small_frame = 2,
	TRAIT_one_hander = 3,
	TRAIT_finesse = 4,
	TRAIT_kamikaze = 5,
	TRAIT_heavy_handed = 6,
	TRAIT_fast_shot = 7,
	TRAIT_bloody_mess = 8,
	TRAIT_jinxed = 9,
	TRAIT_good_natured = 10,
	TRAIT_drug_addict = 11,
	TRAIT_drug_resistant = 12,
	TRAIT_sex_appeal = 13,
	TRAIT_skilled = 14,
	TRAIT_gifted = 15,
	TRAIT_count = 16,
};

// proto.h: stats //

enum Stat : long
{
	STAT_st = 0,
	STAT_pe = 1,
	STAT_en = 2,
	STAT_ch = 3,
	STAT_iq = 4,
	STAT_ag = 5,
	STAT_lu = 6,
	///  strength, perception, endurance, charisma, intelligence, agility,
	///  luck,   // luck MUST be the last basic stat
	// derived stats
	STAT_max_hit_points = 7,
	STAT_max_move_points = 8,
	STAT_ac = 9,
	STAT_unused = 10,
	STAT_melee_dmg = 11,
	STAT_carry_amt = 12,
	STAT_sequence = 13,
	STAT_heal_rate = 14,
	STAT_crit_chance = 15,
	STAT_better_crit = 16,
	STAT_dmg_thresh = 17,
	STAT_dmg_thresh_laser = 18,
	STAT_dmg_thresh_fire = 19,
	STAT_dmg_thresh_plasma = 20,
	STAT_dmg_thresh_electrical = 21,
	STAT_dmg_thresh_emp = 22,
	STAT_dmg_thresh_explosion = 23,
	STAT_dmg_resist = 24,
	STAT_dmg_resist_laser = 25,
	STAT_dmg_resist_fire = 26,
	STAT_dmg_resist_plasma = 27,
	STAT_dmg_resist_electrical = 28,
	STAT_dmg_resist_emp = 29,
	STAT_dmg_resist_explosion = 30,
	STAT_rad_resist = 31,
	STAT_poison_resist = 32,
	// poison_resist MUST be the last derived stat
	// nonderived stats
	STAT_age = 33,
	STAT_gender = 34,
	// gender MUST be the last nonderived stat
	STAT_current_hp = 35,
	STAT_current_poison = 36,
	STAT_current_rad = 37,
	STAT_real_max_stat = 38
};

#define STAT_max_derived   STAT_poison_resist
#define STAT_max_stat  STAT_current_hp

// Script data types
#define VAR_TYPE_INT    (0xC001)
#define VAR_TYPE_FLOAT  (0xA001)
#define VAR_TYPE_STR    (0x9801)
#define VAR_TYPE_STR2   (0x9001)

// extra stat-like values that are treated specially
enum PCStat : long
{
	PCSTAT_unspent_skill_points = 0,
	PCSTAT_level = 1,
	PCSTAT_experience = 2,
	PCSTAT_reputation = 3,
	PCSTAT_karma = 4,
	PCSTAT_max_pc_stat = 5,
};

enum Skill : long
{
	SKILL_SMALL_GUNS = 0,
	SKILL_BIG_GUNS = 1,
	SKILL_ENERGY_WEAPONS = 2,
	SKILL_UNARMED_COMBAT = 3,
	SKILL_MELEE = 4,
	SKILL_THROWING = 5,
	SKILL_FIRST_AID = 6,
	SKILL_DOCTOR = 7,
	SKILL_SNEAK = 8,
	SKILL_LOCKPICK = 9,
	SKILL_STEAL = 10,
	SKILL_TRAPS = 11,
	SKILL_SCIENCE = 12,
	SKILL_REPAIR = 13,
	SKILL_CONVERSANT = 14,
	SKILL_BARTER = 15,
	SKILL_GAMBLING = 16,
	SKILL_OUTDOORSMAN = 17,
	SKILL_count = 18
};

enum CritterFlags : long
{
	CFLG_Sneak        = 0x01,   // Can sneak ?
	CFLG_Barter       = 0x02,   // Can trade with
	CFLG_Level        = 0x04,   // Level received ?
	CFLG_Addict       = 0x08,   // Drug addiction ?
	CFLG_NoSteal      = 0x20,   // Can't be stolen from
	CFLG_NoDrop       = 0x40,   // Doesn't drop items
	CFLG_NoLimbs      = 0x80,   // Can't lose limbs
	CFLG_NoAges       = 0x100,  // Dead body does not disappear
	CFLG_NoHeal       = 0x200,  // Damage is not healed with time
	CFLG_Invulnerable = 0x400,  // Is Invulnerable (cannot be hurt)
	CFLG_NoFlatten    = 0x800,  // Doesn't flatten on death (leaves no dead body)
	CFLG_SpecialDeath = 0x1000, // Has a special type of death
	CFLG_RangeHth     = 0x2000, // Has extra hand-to-hand range
	CFLG_NoKnockBack  = 0x4000, // Can't be knocked back
};

//XXXXXXXXXXXXXXXXXXXXXXXXXX
//XX Combat Flags defines XX
//XXXXXXXXXXXXXXXXXXXXXXXXXX

enum DamageFlag : long
{
	DAM_KNOCKED_OUT = 0x1,
	DAM_KNOCKED_DOWN = 0x2,
	DAM_CRIP_LEG_LEFT = 0x4,
	DAM_CRIP_LEG_RIGHT = 0x8,
	DAM_CRIP_ARM_LEFT = 0x10,
	DAM_CRIP_ARM_RIGHT = 0x20,
	DAM_BLIND = 0x40,
	DAM_DEAD = 0x80,
	DAM_HIT = 0x100,
	DAM_CRITICAL = 0x200,
	DAM_ON_FIRE = 0x400,
	DAM_BYPASS = 0x800,
	DAM_EXPLODE = 0x1000,
	DAM_DESTROY = 0x2000,
	DAM_DROP = 0x4000,
	DAM_LOSE_TURN = 0x8000,
	DAM_HIT_SELF = 0x10000,
	DAM_LOSE_AMMO = 0x20000,
	DAM_DUD = 0x40000,
	DAM_HURT_SELF = 0x80000,
	DAM_RANDOM_HIT = 0x100000,
	DAM_CRIP_RANDOM = 0x200000,
	DAM_BACKWASH = 0x400000,
	DAM_PERFORM_REVERSE = 0x800000,
};

//XXXXXXXXXXXXXXXXXXXX
//XX Object defines XX
//XXXXXXXXXXXXXXXXXXXX

enum ItemType : long
{
	item_type_armor = 0,
	item_type_container = 1,
	item_type_drug = 2,
	item_type_weapon = 3,
	item_type_ammo = 4,
	item_type_misc_item = 5,
	item_type_key = 6,
};

// Inventory Equates
enum InvenType : long
{
	INVEN_TYPE_WORN = 0,
	INVEN_TYPE_RIGHT_HAND = 1,
	INVEN_TYPE_LEFT_HAND = 2,
};

enum AttackType : long
{
	ATKTYPE_LWEAPON_PRIMARY   = 0,
	ATKTYPE_LWEAPON_SECONDARY = 1,
	ATKTYPE_RWEAPON_PRIMARY   = 2,
	ATKTYPE_RWEAPON_SECONDARY = 3,
	ATKTYPE_PUNCH             = 4,
	ATKTYPE_KICK              = 5,
	ATKTYPE_LWEAPON_RELOAD    = 6,
	ATKTYPE_RWEAPON_RELOAD    = 7,
	ATKTYPE_STRONGPUNCH       = 8,
	ATKTYPE_HAMMERPUNCH       = 9,
	ATKTYPE_HAYMAKER          = 10,
	ATKTYPE_JAB               = 11,
	ATKTYPE_PALMSTRIKE        = 12,
	ATKTYPE_PIERCINGSTRIKE    = 13,
	ATKTYPE_STRONGKICK        = 14,
	ATKTYPE_SNAPKICK          = 15,
	ATKTYPE_POWERKICK         = 16,
	ATKTYPE_HIPKICK           = 17,
	ATKTYPE_HOOKKICK          = 18,
	ATKTYPE_PIERCINGKICK      = 19
};

enum KillType : long
{
	KILL_TYPE_men             = 0,
	KILL_TYPE_women           = 1,
	KILL_TYPE_children        = 2,
	KILL_TYPE_super_mutant    = 3,
	KILL_TYPE_ghoul           = 4,
	KILL_TYPE_brahmin         = 5,
	KILL_TYPE_radscorpion     = 6,
	KILL_TYPE_rat             = 7,
	KILL_TYPE_floater         = 8,
	KILL_TYPE_centaur         = 9,
	KILL_TYPE_robot           = 10,
	KILL_TYPE_dog             = 11,
	KILL_TYPE_manti           = 12,
	KILL_TYPE_deathclaw       = 13,
	KILL_TYPE_plant           = 14,
	KILL_TYPE_gecko           = 15,
	KILL_TYPE_alien           = 16,
	KILL_TYPE_giant_ant       = 17,
	KILL_TYPE_big_bad_boss    = 18,
	KILL_TYPE_count
};

#define PLAYER_ID             (18000)

#define OBJFLAG_CAN_WEAR_ITEMS (0xF000000)

#define OBJFLAG_HELD_IN_RIGHT (0x10000)
#define OBJFLAG_HELD_IN_LEFT  (0x20000)
#define OBJFLAG_WORN          (0x40000)

#define CRITTER_BROKEN_RARM   (0x10)
#define CRITTER_BROKEN_LARM   (0x20)
#define CRITTER_EYEDAMAGE     (0x40)

#define WEAPON_TWO_HANDED     (0x200)

#define AUTOMAP_MAX           (160)

enum TicksTime : unsigned long
{
	ONE_GAME_YEAR         = 315360000
};

enum RollResult
{
	ROLL_CRITICAL_FAILURE = 0x0,
	ROLL_FAILURE = 0x1,
	ROLL_SUCCESS = 0x2,
	ROLL_CRITICAL_SUCCESS = 0x3,
};

enum WinButtonFlags : long
{
	WIN_OwnerFlag         = 0x1,
	WIN_UnknownFlag2      = 0x2,
	WIN_MoveOnTop         = 0x4,
	WIN_Hidden            = 0x8,
	WIN_Exclusive         = 0x10,
	WIN_Transparent       = 0x20,
	WIN_UnknownFlag40     = 0x40,
	WIN_UnknownFlag80     = 0x80,
	WIN_ScriptWindow      = 0x100,
	WIN_itsButton         = 0x10000,
};

enum QueueType : long
{
	drug_effect_event  = 0,  // critter use drug
	knockout_event     = 1,  // critter
	addict_event       = 2,  // critter
	script_timer_event = 3,  // any object
	game_time_event    = 4,  // no object
	poison_event       = 5,  // dude
	radiation_event    = 6,  // dude
	flare_time_event   = 7,  // item
	explode_event      = 8,  // item
	item_trickle_event = 9,
	sneak_event        = 10, // dude
	explode_fail_event = 11, // item
	map_update_event   = 12,
	gsound_sfx_event   = 13  // no object
};

enum DialogOutFlags : long
{
	DIALOGOUT_NORMAL     = 0x01, // uses regular graphic
	DIALOGOUT_SMALL      = 0x02, // uses smaller graphic
	DIALOGOUT_ALIGN_LEFT = 0x04, // text aligned to left
	DIALOGOUT_ALIGN_TOP  = 0x08, // text aligned to top
	DIALOGOUT_YESNO      = 0x10, // DONE button replaced by YES/NO buttons
	DIALOGOUT_CLEAN      = 0x20  // no buttons
};
