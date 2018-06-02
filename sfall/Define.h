#pragma once

// PIDS of some items
#define PID_ACTIVE_GEIGER_COUNTER    207
#define PID_ACTIVE_STEALTH_BOY       210
#define PID_CAR_TRUNK                455

#define PID_DRIVABLE_CAR             33555441
#define PID_JET                      259
#define PID_JESSE_CONTAINER          467
#define PID_MOTION_SENSOR            59
#define PID_Player                   16777216
#define PID_SUPER_STIMPAK            144

//XXXXXXXXXXXXXXXXXXXXX
//XX Critter defines XX
//XXXXXXXXXXXXXXXXXXXXX

// Trait defines //
#define TRAIT_PERK  (0)
#define TRAIT_OBJECT (1)
#define TRAIT_TRAIT  (2)

// Perk Trait Types
#define PERK_bonus_awareness                 (0)
#define PERK_bonus_hth_attacks               (1)
#define PERK_bonus_hth_damage                (2)
#define PERK_bonus_move                      (3)
#define PERK_bonus_ranged_damage             (4)
#define PERK_bonus_rate_of_fire              (5)
#define PERK_earlier_sequence                (6)
#define PERK_faster_healing                  (7)
#define PERK_more_criticals                  (8)
#define PERK_night_vision                    (9)
#define PERK_presence                        (10)
#define PERK_rad_resistance                  (11)
#define PERK_toughness                       (12)
#define PERK_strong_back                     (13)
#define PERK_sharpshooter                    (14)
#define PERK_silent_running                  (15)
#define PERK_survivalist                     (16)
#define PERK_master_trader                   (17)
#define PERK_educated                        (18)
#define PERK_healer                          (19)
#define PERK_fortune_finder                  (20)
#define PERK_better_criticals                (21)
#define PERK_empathy                         (22)
#define PERK_slayer                          (23)
#define PERK_sniper                          (24)
#define PERK_silent_death                    (25)
#define PERK_action_boy                      (26)
///#define PERK_mental_block        (27)  -- Disabled FO2
#define PERK_lifegiver                       (28)
#define PERK_dodger                          (29)
#define PERK_snakeater                       (30)
#define PERK_mr_fixit                        (31)
#define PERK_medic                           (32)
#define PERK_master_thief                    (33)
#define PERK_speaker                         (34)
#define PERK_heave_ho                        (35)
///#define PERK_friendly_foe        (36)  -- Disabled FO2
#define PERK_pickpocket                      (37)
#define PERK_ghost                           (38)
#define PERK_cult_of_personality             (39)
///#define PERK_scrounger           (40)  -- Disabled FO2
#define PERK_explorer                        (41)
///#define PERK_flower_child        (42)  -- Disabled FO2
#define PERK_pathfinder                      (43)
///#define PERK_animal_friend       (44)  -- Disabled FO2
#define PERK_scout                           (45)
#define PERK_mysterious_stranger             (46)
#define PERK_ranger                          (47)
#define PERK_quick_pockets                   (48)
#define PERK_smooth_talker                   (49)
#define PERK_swift_learner                   (50)
#define PERK_tag                             (51)
#define PERK_mutate                          (52)
#define PERK_add_nuka                        (53)
#define PERK_add_buffout                     (54)
#define PERK_add_mentats                     (55)
#define PERK_add_psycho                      (56)
#define PERK_add_radaway                     (57)
#define PERK_weapon_long_range               (58)
#define PERK_weapon_accurate                 (59)
#define PERK_weapon_penetrate                (60)
#define PERK_weapon_knockback                (61)
#define PERK_armor_powered                   (62)
#define PERK_armor_combat                    (63)
// Note: Fallout 2: New Perks      
#define PERK_weapon_scope_range              (64)
#define PERK_weapon_fast_reload              (65)
#define PERK_weapon_night_sight              (66)
#define PERK_weapon_flameboy                 (67)
#define PERK_armor_advanced_1                (68)
#define PERK_armor_advanced_2                (69)
#define PERK_add_jet                         (70)
#define PERK_add_tragic                      (71)
#define PERK_armor_charisma                  (72)
#define PERK_gecko_skinning_perk             (73)
#define PERK_dermal_armor_perk               (74)
#define PERK_dermal_enhancement_perk         (75)
#define PERK_phoenix_armor_perk              (76)
#define PERK_phoenix_enhancement_perk        (77)
#define PERK_vault_city_inoculations_perk    (78)
#define PERK_adrenaline_rush_perk            (79)
#define PERK_cautious_nature_perk            (80)
#define PERK_comprehension_perk              (81)
#define PERK_demolition_expert_perk          (82)
#define PERK_gambler_perk                    (83)
#define PERK_gain_strength_perk              (84)
#define PERK_gain_perception_perk            (85)
#define PERK_gain_endurance_perk             (86)
#define PERK_gain_charisma_perk              (87)
#define PERK_gain_intelligence_perk          (88)
#define PERK_gain_agility_perk               (89)
#define PERK_gain_luck_perk                  (90)
#define PERK_harmless_perk                   (91)
#define PERK_here_and_now_perk               (92)
#define PERK_hth_evade_perk                  (93)
#define PERK_kama_sutra_perk                 (94)
#define PERK_karma_beacon_perk               (95)
#define PERK_light_step_perk                 (96)
#define PERK_living_anatomy_perk             (97)
#define PERK_magnetic_personality_perk       (98)
#define PERK_negotiator_perk                 (99)
#define PERK_pack_rat_perk                   (100)
#define PERK_pyromaniac_perk                 (101)
#define PERK_quick_recovery_perk             (102)
#define PERK_salesman_perk                   (103)
#define PERK_stonewall_perk                  (104)
#define PERK_thief_perk                      (105)
#define PERK_weapon_handling_perk            (106)
#define PERK_vault_city_training_perk        (107)
#define PERK_alcohol_hp_bonus1_perk          (108)
#define PERK_alcohol_hp_bonus2_perk          (109)
#define PERK_alcohol_hp_neg1_perk            (110)
#define PERK_alcohol_hp_neg2_perk            (111)
#define PERK_autodoc_hp_bonus1_perk          (112)
#define PERK_autodoc_hp_bonus2_perk          (113)
#define PERK_autodoc_hp_neg1_perk            (114)
#define PERK_autodoc_hp_neg2_perk            (115)
#define PERK_expert_excrement_expediter_perk (116)
#define PERK_weapon_knockout_perk            (117)
#define PERK_jinxed_perk                     (118)
#define PERK_count                           (119)

// Trait Types
#define TRAIT_fast_metabolism  (0)
#define TRAIT_bruiser          (1)
#define TRAIT_small_frame      (2)
#define TRAIT_one_hander       (3)
#define TRAIT_finesse          (4)
#define TRAIT_kamikaze         (5)
#define TRAIT_heavy_handed     (6)
#define TRAIT_fast_shot        (7)
#define TRAIT_bloody_mess      (8)
#define TRAIT_jinxed           (9)
#define TRAIT_good_natured    (10)
#define TRAIT_drug_addict     (11)
#define TRAIT_drug_resistant  (12)
#define TRAIT_sex_appeal      (13)
#define TRAIT_skilled         (14)
#define TRAIT_gifted          (15)
#define TRAIT_count           (16)

// proto.h: stats //
// SPECIAL System stats
#define STAT_st              (0)
#define STAT_pe              (1)
#define STAT_en              (2)
#define STAT_ch              (3)
#define STAT_iq              (4)
#define STAT_ag              (5)
#define STAT_lu              (6)
#define STAT_max_hp          (7)
#define STAT_max_hit_points  (7)
///  strength, perception, endurance, charisma, intelligence, agility,
///  luck,   // luck MUST be the last basic stat
// derived stats
#define STAT_max_ap             (8)
#define STAT_max_move_points    (8)
#define STAT_ac                 (9)
///#define STAT_unarmed_dmg        (10)
#define STAT_unused             (10)
#define STAT_melee_dmg          (11)
#define STAT_carry_amt          (12)
#define STAT_sequence           (13)
#define STAT_heal_rate          (14)
#define STAT_crit_chance        (15)
#define STAT_better_crit        (16)
#define STAT_dmg_thresh         (17)
#define STAT_dmg_thresh_laser   (18)
#define STAT_dmg_thresh_fire    (19)
#define STAT_dmg_thresh_plasma  (20)
#define STAT_dmg_thresh_electrical (21)
#define STAT_dmg_thresh_emp        (22)
#define STAT_dmg_thresh_explosion  (23)
#define STAT_dmg_resist            (24)
#define STAT_dmg_resist_laser      (25)
#define STAT_dmg_resist_fire       (26)
#define STAT_dmg_resist_plasma     (27)
#define STAT_dmg_resist_electrical (28)
#define STAT_dmg_resist_emp        (29)
#define STAT_dmg_resist_explosion  (30)
#define STAT_rad_resist            (31)
#define STAT_poison_resist         (32)
#define STAT_max_derived   STAT_poison_resist
// poison_resist MUST be the last derived stat
// nonderived stats
#define STAT_age                   (33)
#define STAT_gender                (34)
// gender MUST be the last nonderived stat
#define STAT_current_hp            (35)
#define STAT_current_poison        (36)
#define STAT_current_rad           (37)
#define STAT_real_max_stat         (38)
// extra stat-like values that are treated specially
#define STAT_max_stat  STAT_current_hp

#define PCSTAT_unspent_skill_points (0)
#define PCSTAT_level                (1)
#define PCSTAT_experience           (2)
#define PCSTAT_reputation           (3)
#define PCSTAT_karma                (4)
#define PCSTAT_max_pc_stat          (5)

#define SKILL_SMALL_GUNS       (0)
#define SKILL_BIG_GUNS         (1)
#define SKILL_ENERGY_WEAPONS   (2)
#define SKILL_UNARMED_COMBAT   (3)
#define SKILL_MELEE            (4)
#define SKILL_THROWING         (5)
#define SKILL_FIRST_AID        (6)
#define SKILL_DOCTOR           (7)
#define SKILL_SNEAK            (8)
#define SKILL_LOCKPICK         (9)
#define SKILL_STEAL            (10)
#define SKILL_TRAPS            (11)
#define SKILL_SCIENCE          (12)
#define SKILL_REPAIR           (13)
#define SKILL_CONVERSANT       (14)
#define SKILL_BARTER           (15)
#define SKILL_GAMBLING         (16)
#define SKILL_OUTDOORSMAN      (17)
#define SKILL_count            (18)

//XXXXXXXXXXXXXXXXXXXXXXXXXX
//XX Combat Flags defines XX
//XXXXXXXXXXXXXXXXXXXXXXXXXX

#define DAM_KNOCKED_OUT        (0x1)
#define DAM_KNOCKED_DOWN       (0x2)
#define DAM_CRIP_LEG_LEFT      (0x4)
#define DAM_CRIP_LEG_RIGHT     (0x8)
#define DAM_CRIP_ARM_LEFT      (0x10)
#define DAM_CRIP_ARM_RIGHT     (0x20)
#define DAM_BLIND              (0x40)
#define DAM_DEAD               (0x80)
#define DAM_HIT                (0x100)
#define DAM_CRITICAL           (0x200)
#define DAM_ON_FIRE            (0x400)
#define DAM_BYPASS             (0x800)
#define DAM_EXPLODE            (0x1000)
#define DAM_DESTROY            (0x2000)
#define DAM_DROP               (0x4000)
#define DAM_LOSE_TURN          (0x8000)
#define DAM_HIT_SELF           (0x10000)
#define DAM_LOSE_AMMO          (0x20000)
#define DAM_DUD                (0x40000)
#define DAM_HURT_SELF          (0x80000)
#define DAM_RANDOM_HIT         (0x100000)
#define DAM_CRIP_RANDOM        (0x200000)
#define DAM_BACKWASH           (0x400000)
#define DAM_PERFORM_REVERSE    (0x800000)

//XXXXXXXXXXXXXXXXXXXX
//XX Object defines XX
//XXXXXXXXXXXXXXXXXXXX

#define item_type_armor        (0)
#define item_type_container    (1)
#define item_type_drug         (2)
#define item_type_weapon       (3)
#define item_type_ammo         (4)
#define item_type_misc_item    (5)
#define item_type_key_item     (6)

#define OBJFLAG_CAN_WEAR_ITEMS (0xf000000)

#define OBJFLAG_HELD_IN_RIGHT (0x10000)
#define OBJFLAG_HELD_IN_LEFT  (0x20000)
#define OBJFLAG_WORN          (0x40000)

#define CRITTER_BROKEN_RARM   (0x10)
#define CRITTER_BROKEN_LARM   (0x20)
#define CRITTER_EYEDAMAGE     (0x40)


#define WEAPON_TWO_HANDED      (0x200)


#define ROLL_CRITICAL_FAILURE (0)
#define ROLL_FAILURE          (1)
#define ROLL_SUCCESS          (2)
#define ROLL_CRITICAL_SUCCESS (3)

//XXXXXXXXXXXXXXXXXXXXXXX
//XX Procedure defines XX
//XXXXXXXXXXXXXXXXXXXXXXX

#define combat_is_starting_p_proc (26)
#define combat_is_over_p_proc     (27)
