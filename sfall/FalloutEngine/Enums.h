/*
 *    sfall
 *    Copyright (C) 2008-2026  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "GamePids.h"

namespace fo
{

enum Animation : long
{
	ANIM_stand = 0,
	ANIM_walk = 1,
	ANIM_jump_begin = 2,
	ANIM_jump_end = 3,
	ANIM_climb_ladder = 4,
	ANIM_falling = 5,
	ANIM_up_stairs_right = 6,
	ANIM_up_stairs_left = 7,
	ANIM_down_stairs_right = 8,
	ANIM_down_stairs_left = 9,
	ANIM_magic_hands_ground = 10,
	ANIM_magic_hands_middle = 11,
	ANIM_magic_hands_up = 12,
	ANIM_dodge_anim = 13,
	ANIM_hit_from_front = 14,
	ANIM_hit_from_back = 15,
	ANIM_throw_punch = 16,
	ANIM_kick_leg = 17,
	ANIM_throw_anim = 18,
	ANIM_running = 19,
	ANIM_fall_back = 20,
	ANIM_fall_front = 21,
	ANIM_bad_landing = 22,
	ANIM_big_hole = 23,
	ANIM_charred_body = 24,
	ANIM_chunks_of_flesh = 25,
	ANIM_dancing_autofire = 26,
	ANIM_electrify = 27,
	ANIM_sliced_in_half = 28,
	ANIM_burned_to_nothing = 29,
	ANIM_electrified_to_nothing = 30,
	ANIM_exploded_to_nothing = 31,
	ANIM_melted_to_nothing = 32,
	ANIM_fire_dance = 33,
	ANIM_fall_back_blood = 34,
	ANIM_fall_front_blood = 35,
	ANIM_prone_to_standing = 36,
	ANIM_back_to_standing = 37,
	ANIM_take_out = 38,
	ANIM_put_away = 39,
	ANIM_parry_anim = 40,
	ANIM_thrust_anim = 41,
	ANIM_swing_anim = 42,
	ANIM_point = 43,
	ANIM_unpoint = 44,
	ANIM_fire_single = 45,
	ANIM_fire_burst = 46,
	ANIM_fire_continuous = 47,
	ANIM_fall_back_sf = 48,
	ANIM_fall_front_sf = 49,
	ANIM_bad_landing_sf = 50,
	ANIM_big_hole_sf = 51,
	ANIM_charred_body_sf = 52,
	ANIM_chunks_of_flesh_sf = 53,
	ANIM_dancing_autofire_sf = 54,
	ANIM_electrify_sf = 55,
	ANIM_sliced_in_half_sf = 56,
	ANIM_burned_to_nothing_sf = 57,
	ANIM_electrified_to_nothing_sf = 58,
	ANIM_exploded_to_nothing_sf = 59,
	ANIM_melted_to_nothing_sf = 60,
	ANIM_fire_dance_sf = 61,
	ANIM_fall_back_blood_sf = 62,
	ANIM_fall_front_blood_sf = 63,
	ANIM_called_shot_pic = 64,
};

enum AnimCommand : long
{
	RB_UNRESERVED = 0x1,
	RB_RESERVED   = 0x2,
	RB_DONTSTAND  = 0x4,
	RB_UNKNOWN    = 0x100,
	RB_END_ANIM   = 0x200
};

enum AnimationType : long {
	ANIM_TYPE_MOVE_TO_OBJECT = 0,
	ANIM_TYPE_MOVE_TO_TILE = 1,
	ANIM_TYPE_MOVE_TO_TILE_STRAIGHT = 2,
	ANIM_TYPE_MOVE_TO_TILE_STRAIGHT_AND_WAIT_FOR_COMPLETE = 3,
	ANIM_TYPE_ANIMATE = 4,
	ANIM_TYPE_ANIMATE_REVERSED = 5,
	ANIM_TYPE_ANIMATE_AND_HIDE = 6,
	ANIM_TYPE_ROTATE_TO_TILE = 7,
	ANIM_TYPE_ROTATE_CLOCKWISE = 8,
	ANIM_TYPE_ROTATE_COUNTER_CLOCKWISE = 9,
	ANIM_TYPE_HIDE = 10,
	ANIM_TYPE_CALLBACK = 11,
	ANIM_TYPE_CALLBACK3 = 12,
	ANIM_TYPE_SET_FLAG = 14,
	ANIM_TYPE_UNSET_FLAG = 15,
	ANIM_TYPE_TOGGLE_FLAT = 16,
	ANIM_TYPE_SET_FID = 17,
	ANIM_TYPE_TAKE_OUT_WEAPON = 18,
	ANIM_TYPE_SET_LIGHT_DISTANCE = 19,
	ANIM_TYPE_MOVE_ON_STAIRS = 20,
	ANIM_TYPE_CHECK_FALLING = 23,
	ANIM_TYPE_TOGGLE_OUTLINE = 24,
	ANIM_TYPE_ANIMATE_FOREVER = 25,
	ANIM_TYPE_PING = 26,
	ANIM_TYPE_CONTINUE = 28
};

enum BodyPart : long {
	Head     = 0,
	ArmLeft  = 1,
	ArmRight = 2,
	Torso    = 3,
	LegRight = 4,
	LegLeft  = 5,
	Eyes     = 6,
	Groin    = 7,
	Uncalled = 8
};

enum CritterFlags : long
{
	Sneak        = 0x01,   // Can sneak ? (unused)
	Barter       = 0x02,   // Can trade with
	Level        = 0x04,   // Level received ? (unused)
	Addict       = 0x08,   // Drug addiction ? (unused)
	NoSteal      = 0x20,   // Can't be stolen from
	NoDrop       = 0x40,   // Doesn't drop items
	NoLimbs      = 0x80,   // Can't lose limbs
	NoAges       = 0x100,  // Dead body does not disappear
	NoHeal       = 0x200,  // Damage is not healed with time
	Invulnerable = 0x400,  // Is Invulnerable (cannot be hurt)
	NoFlatten    = 0x800,  // Doesn't flatten on death (leaves no dead body)
	SpecialDeath = 0x1000, // Has a special type of death
	RangeHtH     = 0x2000, // Has extra hand-to-hand range
	NoKnockBack  = 0x4000, // Can't be knocked back
};

enum ItemFlags : unsigned long // for FlagsExt
{
	// Weapon Flags:
	BigGun       = 0x00000100,
	TwoHand      = 0x00000200,
	EnergyGun    = 0x00000400, // sfall flag

	// Action Flags:
	Use          = 0x00000800, // object can be used

	HealingItem  = 0x04000000, // sfall healing drug flag
	HiddenItem   = 0x08000000  // item is hidden
};

enum MiscFlags : unsigned long
{
	Opened       = 0x00000001,
	CantUse      = 0x00000010, // determines whether the weapon can be used in combat (sfall flag)
	Locked       = 0x02000000,
	Jammed       = 0x04000000,
};

enum DamageFlag : unsigned long
{
	DAM_KNOCKED_OUT     = 0x1,
	DAM_KNOCKED_DOWN    = 0x2,
	DAM_CRIP_LEG_LEFT   = 0x4,
	DAM_CRIP_LEG_RIGHT  = 0x8,
	DAM_CRIP_ARM_LEFT   = 0x10,
	DAM_CRIP_ARM_RIGHT  = 0x20,
	DAM_BLIND           = 0x40,
	DAM_DEAD            = 0x80,
	DAM_HIT             = 0x100,
	DAM_CRITICAL        = 0x200,
	DAM_ON_FIRE         = 0x400,
	DAM_BYPASS          = 0x800,
	DAM_EXPLODE         = 0x1000,
	DAM_DESTROY         = 0x2000,
	DAM_DROP            = 0x4000,
	DAM_LOSE_TURN       = 0x8000,
	DAM_HIT_SELF        = 0x10000,
	DAM_LOSE_AMMO       = 0x20000,
	DAM_DUD             = 0x40000,
	DAM_HURT_SELF       = 0x80000,
	DAM_RANDOM_HIT      = 0x100000,
	DAM_CRIP_RANDOM     = 0x200000,
	DAM_BACKWASH        = 0x400000,
	DAM_PERFORM_REVERSE = 0x800000,
	// sfall flags
	DAM_KNOCKOUT_WOKEN  = 0x40000000, // internal for op_critter_state_
	DAM_PRESERVE_FLAGS  = 0x80000000  // used for attack_complex
};

enum DamageType
{
	DMG_normal       = 0,
	DMG_laser        = 1,
	DMG_fire         = 2,
	DMG_plasma       = 3,
	DMG_electrical   = 4,
	DMG_emp          = 5,
	DMG_explosion    = 6,
	DMG_BYPASS_ARMOR = 256,
	DMG_NOANIMATE    = 512,
};

enum Gender
{
	GENDER_MALE   = 0,
	GENDER_FEMALE = 1,
};

// Some global variables referenced by engine
enum GlobalVar : long
{
	GVAR_PLAYER_REPUTATION = 0,
	GVAR_ENEMY_ARROYO = 7,
	GVAR_PLAYER_GOT_CAR = 18,
	GVAR_NUKA_COLA_ADDICT = 21,
	GVAR_BUFF_OUT_ADDICT = 22,
	GVAR_MENTATS_ADDICT = 23,
	GVAR_PSYCHO_ADDICT = 24,
	GVAR_RADAWAY_ADDICT = 25,
	GVAR_ALCOHOL_ADDICT = 26,
	GVAR_LOAD_MAP_INDEX = 27,
	GVAR_TOWN_REP_ARROYO = 47,
	GVAR_ADDICT_TRAGIC = 295,
	GVAR_ADDICT_JET = 296,
	GVAR_CAR_BLOWER = 439,
	GVAR_CAR_UPGRADE_FUEL_CELL_REGULATOR = 453,
	GVAR_NEW_RENO_CAR_UPGRADE = 455,
	GVAR_NEW_RENO_SUPER_CAR = 456,
	GVAR_MODOC_SHITTY_DEATH = 491,
	GVAR_FALLOUT_2 = 494,
	GVAR_CAR_PLACED_TILE = 633,
};

// Physical material type, used for items and tiles.
enum class Material : long
{
	Glass = 0x0,
	Metal = 0x1,
	Plastic = 0x2,
	Wood = 0x3,
	Dirt = 0x4,
	Stone = 0x5,
	Cement = 0x6,
	Leather = 0x7
};

namespace ObjectFlag {
	enum ObjectFlag : unsigned long {
		Hidden       = 0x00000001,
		NoSave       = 0x00000004, // WalkThru flag for doors
		Flat         = 0x00000008,
		NoBlock      = 0x00000010,
		Lighting     = 0x00000020,
		NoRemove     = 0x00000400,
		MultiHex     = 0x00000800,
		NoHighlight  = 0x00001000,
		Used         = 0x00002000, // set if there was/is any event for the object
		TransRed     = 0x00004000,
		TransNone    = 0x00008000,
		TransWall    = 0x00010000,
		TransGlass   = 0x00020000,
		TransSteam   = 0x00040000,
		TransEnergy  = 0x00080000,
		Left_Hand    = 0x01000000,
		Right_Hand   = 0x02000000,
		Worn         = 0x04000000,
		Unused       = 0x08000000, // reserved
		WallTransEnd = 0x10000000,
		LightThru    = 0x20000000,
		Seen         = 0x40000000,
		ShootThru    = 0x80000000,
	};
}

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

//XXXXXXXXXXXXXXXXXXXXX
//XX Critter defines XX
//XXXXXXXXXXXXXXXXXXXXX

enum TraitType : long
{
	TRAIT_PERK = 0,
	TRAIT_OBJECT = 1,
	TRAIT_TRAIT = 2
};

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

enum class ScenerySubType : long
{
	DOOR = 0,
	STAIRS = 1,
	ELEVATOR = 2,
	LADDER_BOTTOM = 3,
	LADDER_TOP = 4,
	GENERIC = 5
};

// proto.h: stats //

enum Stat : long
{
	STAT_st = 0, // strength
	STAT_pe = 1, // perception
	STAT_en = 2, // endurance
	STAT_ch = 3, // charisma
	STAT_iq = 4, // intelligence
	STAT_ag = 5, // agility
	STAT_lu = 6, // luck

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

	// non-derived stats
	STAT_age = 33,
	STAT_gender = 34,
	// gender MUST be the last non-derived stat

	STAT_current_hp = 35,
	STAT_current_poison = 36,
	STAT_current_rad = 37,

	STAT_real_max_stat = 38,
	STAT_base_count = 7
};

namespace Scripts {
	enum ScriptProc : long
	{
		no_p_proc = 0,
		start = 1,
		spatial_p_proc = 2,
		description_p_proc = 3,
		pickup_p_proc = 4,
		drop_p_proc = 5,
		use_p_proc = 6,
		use_obj_on_p_proc = 7,
		use_skill_on_p_proc = 8,
		none_x_bad = 9,
		none_x_bad2 = 10,
		talk_p_proc = 11,
		critter_p_proc = 12,
		combat_p_proc = 13,
		damage_p_proc = 14,
		map_enter_p_proc = 15,
		map_exit_p_proc = 16,
		create_p_proc = 17,
		destroy_p_proc = 18,
		none_x_bad3 = 19,
		none_x_bad4 = 20,
		look_at_p_proc = 21,
		timed_event_p_proc = 22,
		map_update_p_proc = 23,
		push_p_proc = 24,
		is_dropping_p_proc = 25,
		combat_is_starting_p_proc = 26,
		combat_is_over_p_proc = 27,
		count = 28
	};

	enum ScriptTypes : long
	{
		SCRIPT_SYSTEM = 0,
		SCRIPT_SPATIAL = 1,
		SCRIPT_TIME = 2,
		SCRIPT_ITEM = 3,
		SCRIPT_CRITTER = 4,
	};
}

#define STAT_max_derived   STAT_poison_resist
#define STAT_max_stat      STAT_current_hp

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

enum AttackSubType : long
{
	NONE                      = 0,
	UNARMED                   = 1,
	MELEE                     = 2,
	THROWING                  = 3,
	RANGED                    = 4
};

enum BodyType : long
{
	Biped                     = 0,
	Quadruped                 = 1,
	Robotic                   = 2
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

enum {
	PLAYER_ID                 = 18000
};

#define OBJFLAG_CAN_WEAR_ITEMS (0xF000000)

#define OBJFLAG_HELD_IN_RIGHT (0x10000)
#define OBJFLAG_HELD_IN_LEFT  (0x20000)
#define OBJFLAG_WORN          (0x40000)

#define CRITTER_BROKEN_RARM   (0x10)
#define CRITTER_BROKEN_LARM   (0x20)
#define CRITTER_EYEDAMAGE     (0x40)

#define WEAPON_TWO_HANDED     (0x200)

#define AUTOMAP_MAX           (160)

#define MSG_GENDER_CHECK_FLG  (0x80) // bit 8

enum TicksTime : unsigned long
{
	ONE_GAME_YEAR         = 315360000
};

enum HandSlot : unsigned long
{
	Left                  = 0,
	Right                 = 1
};

enum HandSlotMode : long
{
	Unset                 = 0,
	Primary               = 1,
	Primary_Aimed         = 2,
	Secondary             = 3,
	Secondary_Aimed       = 4,
	Reload                = 5,
	UnkMode               = 6
};

enum RollResult
{
	ROLL_CRITICAL_FAILURE = 0x0,
	ROLL_FAILURE = 0x1,
	ROLL_SUCCESS = 0x2,
	ROLL_CRITICAL_SUCCESS = 0x3,
};

enum CombatStateFlag : long
{
	InCombat        = 1,
	EnemyOutOfRange = 2,
	InFlee          = 4,
	ReTarget        = 8 // sfall flag (set in ai_try_attack_ before run away)
};

// Names of structure offsets used in the assembler code
namespace Fields {
	enum CommonObj : long
	{
		id                = 0x00, // saveable
		tile              = 0x04, // saveable
		x                 = 0x08, // saveable
		y                 = 0x0C, // saveable
		sx                = 0x10, // saveable
		sy                = 0x14, // saveable
		currFrame         = 0x18, // saveable
		rotation          = 0x1C, // saveable
		artFid            = 0x20, // saveable
		flags             = 0x24, // saveable
		elevation         = 0x28, // saveable
		inventory         = 0x2C, // saveable

		protoId           = 0x64, // saveable
		cid               = 0x68, // saveable (critter CombatID, don't change while in combat)
		lightDistance     = 0x6C, // saveable
		lightIntensity    = 0x70, // saveable
		outline           = 0x74, // saveable
		scriptId          = 0x78, // saveable
		owner             = 0x7C,
		scriptIndex       = 0x80, // saveable
	};

	enum CritterObj : long
	{
		reaction          = 0x38, // saveable
		combatState       = 0x3C, // saveable
		movePoints        = 0x40, // saveable
		damageFlags       = 0x44, // saveable
		damageLastTurn    = 0x48, // saveable
		aiPacket          = 0x4C, // saveable
		teamNum           = 0x50, // saveable
		whoHitMe          = 0x54, // saveable
		health            = 0x58, // saveable
		rads              = 0x5C, // saveable
		poison            = 0x60, // saveable
	};

	enum ItemObj : long
	{
		miscFlags         = 0x38, // saveable
		charges           = 0x3C, // saveable
		ammoPid           = 0x40, // saveable
	};

	enum SceneryObj : long
	{
		sceneryFlags      = 0x38, // saveable
		doorFlags         = 0x3C, // saveable
	};

	enum ComputeAttack : long
	{
		ctdAttackerFlags  = 0x15, // flags2Source
		ctdTarget         = 0x20,
		ctdMainTarget     = 0x38,
		ctdExtraTarget1   = 0x40,
	};
}

namespace WinFlags {
	enum WinButtonFlags : long
	{
		OwnerFlag         = 0x000001, // sfall Render flag, indicates that the window surface is used for rendering the game scene
		DontMoveTop       = 0x000002, // does not move the window to top when the mouse is clicked in the window area or when it is showing
		MoveOnTop         = 0x000004, // places the window on top when it is created
		Hidden            = 0x000008,
		Exclusive         = 0x000010,
		Transparent       = 0x000020,
		UnknownFlag40     = 0x000040,
		UnknownFlag80     = 0x000080,
		ScriptWindow      = 0x000100,
		itsButton         = 0x010000,
	};
}

namespace AIpref {
	enum class Distance : long
	{
		unset                 = -1,
		stay_close            = 0, // the attacker will stay at a distance no more than 5 hexes from the player (behavior only for party members, defined in ai_move_steps_closer, cai_perform_distance_prefs)
		charge                = 1, // AI will always try to get close to its target before or after attack
		snipe                 = 2, // keep distance, when the distance between the attacker and the target decreases, the attacker will try to move away from the target to a distance of up to 10 hexes
		on_your_own           = 3, // no special behavior defined for this
		stay                  = 4  // the attacker will, if possible, stay at the hex where the combat started (behavior defined in ai_move_steps_closer, ai_move_away)
	};

	// presets for party members
	enum class Disposition : long
	{
		none                  = -1,
		custom                = 0,
		coward                = 1,
		defensive             = 2,
		aggressive            = 3,
		berserk               = 4
	};

	enum class AttackWho : long
	{
		no_attack_mode        = -1,
		whomever_attacking_me = 0, // attack the target that the player is attacking (only for party members)
		strongest             = 1, // attack stronger targets (will always switch to stronger ones in combat)
		weakest               = 2, // attack weaker targets (will always switch to weaker ones in combat)
		whomever              = 3, // anyone, will attack the chosen target until it dies, or until retaliation occurs (combatai_check_retaliation_)
		closest               = 4, // only attack near targets
	};

	enum class RunAway : long
	{
		none                  = -1, // get the value from the cap.min_hp (in cai_get_min_hp_)
		coward                = 0,  // 0%
		finger_hurts          = 1,  // 25% of the lost amount of health
		bleeding              = 2,  // 40% of the lost amount of health
		not_feeling_good      = 3,  // 60% of the lost amount of health
		tourniquet            = 4,  // 75% of the lost amount of health
		never                 = 5   // 100%
	};

	enum class WeaponPref : long
	{
		unset                 = -1, // same as no_pref
		no_pref               = 0,  // Guns > ...
		melee                 = 1,
		melee_over_ranged     = 2,
		ranged_over_melee     = 3,
		ranged                = 4,
		unarmed               = 5,
		unarmed_over_thrown   = 6, // not available for party member in control panel
		random                = 7  // not available for party member in control panel
	};

	enum class AreaAttack : long
	{
		no_pref               = -1, // special logic for NPC (not available for party member in control panel)
		always                = 0,
		sometimes             = 1,  // use random value from cap.secondary_freq
		be_sure               = 2,  // 85% hit chance
		be_careful            = 3,  // 50% hit chance
		be_absolutely_sure    = 4,  // 95% hit chance
	};

	enum class ChemUse : long
	{
		unset                  = -1,
		clean                  = 0,
		stims_when_hurt_little = 1,
		stims_when_hurt_lots   = 2,
		sometimes              = 3,
		anytime                = 4,
		always                 = 5,
	};
}

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

enum InventoryWindowType : unsigned long
{
	// Normal inventory window with quick character sheet.
	INVENTORY_WINDOW_TYPE_NORMAL,

	// Narrow inventory window with just an item scroller that's shown when
	// a "Use item on" is selected from context menu.
	INVENTORY_WINDOW_TYPE_USE_ITEM_ON,

	// Looting/strealing interface.
	INVENTORY_WINDOW_TYPE_LOOT,

	// Barter interface.
	INVENTORY_WINDOW_TYPE_TRADE,

	// Supplementary "Move items" window. Used to set quantity of items when
	// moving items between inventories.
	INVENTORY_WINDOW_TYPE_MOVE_ITEMS,

	// Supplementary "Set timer" window. Internally it's implemented as "Move
	// items" window but with timer overlay and slightly different adjustment
	// mechanics.
	INVENTORY_WINDOW_TYPE_SET_TIMER,

	INVENTORY_WINDOW_TYPE_COUNT,
};

}
