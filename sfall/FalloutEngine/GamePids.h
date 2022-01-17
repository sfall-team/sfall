/******************************************************************
   Note: This file is arranged differently than all the other
         header files. Rather than having all of the items in the
         order in which they appear in the item list, they will
         be sorted, but the Pid number defines will remain as they
         are in the item list. Please do not alter numbers, unless
         it is to fix errors found through mapper.

         SKIPPING PIDs 60-70   BECAUSE THEY ARE DESKS AND BOOKCASES
         SKIPPING PIDs 145-158 BECAUSE THEY ARE DESKS AND BOOKCASES
         SKIPPING PIDs 164-179 BECAUSE THEY ARE FALLOUT 1 SPECIFIC
         SKIPPING PIDs 181-204 BECAUSE THEY ARE FALLOUT 1 SPECIFIC
         SKIPPING PIDs 213-217 BECAUSE THEY ARE FALLOUT 1 SPECIFIC
         SKIPPING PIDs 230-231 BECAUSE THEY ARE FALLOUT 1 SPECIFIC
         SKIPPING PID  238     BECAUSE THEY ARE FALLOUT 1 SPECIFIC

   Purpose: This file will contain defines for all the item
            prototypes for the game. These prototypes can be
            found in mapper using <F1> or selecting items. All
            defines in here will need to be prepended with PID_
            for the ease of everyone. Additionally, Please do not
            make duplicate names for the same item. Most should
            have the correct name.

   Structure: The following is the structure of this file.
                I.    Armor
                II.   Weapons
                III.  Ammo
                IV.   Medical
                V.    Containers
                VI.   Books
                VII.  Tools
                VIII. Misc.
                IX.   Active_Items
                X.    Flying Weapons

******************************************************************/

#pragma once

namespace fo
{

// Some FO2 PIDs possibly used by engine
enum ProtoID : unsigned long
{
	/******************************************************************
	***************       Armor                         ***************
	******************************************************************/

	PID_LEATHER_ARMOR                   = 1,
	PID_METAL_ARMOR                     = 2,
	PID_POWERED_ARMOR                   = 3,
	PID_COMBAT_ARMOR                    = 17,
	PID_LEATHER_JACKET                  = 74,
	PID_PURPLE_ROBE                     = 113,
	PID_HARDENED_POWER_ARMOR            = 232,
	PID_BROTHERHOOD_COMBAT_ARMOR        = 239,
	PID_TESLA_ARMOR                     = 240,
	PID_CURED_LEATHER_ARMOR             = 265,
	PID_ADVANCED_POWER_ARMOR            = 348,
	PID_ADVANCED_POWER_ARMOR_MK2        = 349,
	PID_LEATHER_ARMOR_MK_II             = 379,
	PID_METAL_ARMOR_MK_II               = 380,
	PID_COMBAT_ARMOR_MK_II              = 381,
	PID_BRIDGEKEEPERS_ROBE              = 524,

	/******************************************************************
	***************       Weapons                       ***************
	******************************************************************/

	PID_KNIFE                           = 4,
	PID_CLUB                            = 5,
	PID_SLEDGEHAMMER                    = 6,
	PID_SPEAR                           = 7,
	PID_10MM_PISTOL                     = 8,
	PID_10MM_SMG                        = 9,
	PID_HUNTING_RIFLE                   = 10,
	PID_FLAMER                          = 11,
	PID_MINIGUN                         = 12,
	PID_ROCKET_LAUNCHER                 = 13,
	PID_PLASMA_RIFLE                    = 15,
	PID_LASER_PISTOL                    = 16,
	PID_DESERT_EAGLE                    = 18,
	PID_ROCK                            = 19,
	PID_CROWBAR                         = 20,
	PID_BRASS_KNUCKLES                  = 21,
	PID_14MM_PISTOL                     = 22,
	PID_ASSAULT_RIFLE                   = 23,
	PID_PLASMA_PISTOL                   = 24,
	PID_FRAG_GRENADE                    = 25,
	PID_PLASMA_GRENADE                  = 26,
	PID_PULSE_GRENADE                   = 27,
	PID_GATLING_LASER                   = 28,
	PID_THROWING_KNIFE                  = 45,
	PID_SHOTGUN                         = 94,
	PID_SUPER_SLEDGE                    = 115,
	PID_RIPPER                          = 116,
	PID_LASER_RIFLE                     = 118,
	PID_ALIEN_LASER_PISTOL              = 120,
	PID_9MM_MAUSER                      = 122,
	PID_SNIPER_RIFLE                    = 143,
	PID_MOLOTOV_COCKTAIL                = 159,
	PID_CATTLE_PROD                     = 160,
	PID_RED_RYDER_BB_GUN                = 161,
	PID_RED_RYDER_LE_BB_GUN             = 162,
	PID_TURBO_PLASMA_RIFLE              = 233,
	PID_SPIKED_KNUCKLES                 = 234,
	PID_POWER_FIST                      = 235,
	PID_COMBAT_KNIFE                    = 236,
	PID_223_PISTOL                      = 241,
	PID_COMBAT_SHOTGUN                  = 242,
	PID_JONNY_BB_GUN                    = 261,
	PID_HK_CAWS                         = 268,
	PID_ROBO_ROCKET_LAUNCHER            = 270,
	PID_SHARP_SPEAR                     = 280,
	PID_SCOPED_HUNTING_RIFLE            = 287,
	PID_EYEBALL_FIST_1                  = 290,
	PID_EYEBALL_FIST_2                  = 291,
	PID_BOXING_GLOVES                   = 292,
	PID_PLATED_BOXING_GLOVES            = 293,
	PID_HK_P90C                         = 296,
	PID_SPRINGER_RIFLE                  = 299,
	PID_ZIP_GUN                         = 300,
	PID_44_MAGNUM_REVOLVER              = 313,
	PID_SWITCHBLADE                     = 319,
	PID_SHARPENED_POLE                  = 320,
	PID_PLANT_SPIKE                     = 365,
	PID_DEATHCLAW_CLAW_1                = 371,
	PID_DEATHCLAW_CLAW_2                = 372,
	PID_TOMMY_GUN                       = 283,
	PID_GREASE_GUN                      = 332,
	PID_BOZAR                           = 350,
	PID_LIGHT_SUPPORT_WEAPON            = 355,
	PID_FN_FAL                          = 351,
	PID_HK_G11                          = 352,
	PID_INDEPENDENT                     = 353,
	PID_PANCOR_JACKHAMMER               = 354,
	PID_SHIV                            = 383,
	PID_WRENCH                          = 384,
	PID_LOUISVILLE_SLUGGER              = 386,
	PID_SOLAR_SCORCHER                  = 390,  // No ammo
	PID_SAWED_OFF_SHOTGUN               = 385,  // 12 ga.
	PID_M60                             = 387,  // 7.62
	PID_NEEDLER_PISTOL                  = 388,  // HN Needler
	PID_AVENGER_MINIGUN                 = 389,  // 5mm JHP
	PID_HK_G11E                         = 391,  // 4.7mm Caseless
	PID_M72_GAUSS_RIFLE                 = 392,  // 2mm EC
	PID_PHAZER                          = 393,  // Small Energy
	PID_PK12_GAUSS_PISTOL               = 394,  // 2mm EC
	PID_VINDICATOR_MINIGUN              = 395,  // 4.7mm Caseless
	PID_YK32_PULSE_PISTOL               = 396,  // Small Energy
	PID_YK42B_PULSE_RIFLE               = 397,  // Micro Fusion
	PID_44_MAGNUM_SPEEDLOADER           = 398,
	PID_SUPER_CATTLE_PROD               = 399,
	PID_IMPROVED_FLAMETHROWER           = 400,
	PID_LASER_RIFLE_EXT_CAP             = 401,
	PID_MAGNETO_LASER_PISTOL            = 402,
	PID_FN_FAL_NIGHT_SCOPE              = 403,
	PID_DESERT_EAGLE_EXT_MAG            = 404,
	PID_ASSAULT_RIFLE_EXT_MAG           = 405,
	PID_PLASMA_PISTOL_EXT_CART          = 406,
	PID_MEGA_POWER_FIST                 = 407,
	PID_HOLY_HAND_GRENADE               = 421,  // Special don't use this
	PID_GOLD_NUGGET                     = 423,
	PID_URANIUM_ORE                     = 426,
	PID_FLAME_BREATH                    = 427,
	PID_REFINED_ORE                     = 486,
	PID_SPECIAL_BOXING_GLOVES           = 496,  // DO NOT USE, SPECIAL, VERY VERY SPECIAL, GOT IT?
	PID_SPECIAL_PLATED_BOXING_GLOVES    = 497,  // NO YOU CANNOT USE THIS, NO I SAID
	PID_GUN_TURRET_WEAPON               = 498,
	PID_FN_FAL_HPFA                     = 500,
	PID_LIL_JESUS_WEAPON                = 517,
	PID_DUAL_MINIGUN                    = 518,
	PID_HEAVY_DUAL_MINIGUN              = 520,
	PID_WAKIZASHI_BLADE                 = 522,

	/******************************************************************
	***************       Ammo                          ***************
	******************************************************************/

	PID_EXPLOSIVE_ROCKET                = 14,
	PID_10MM_JHP                        = 29,
	PID_10MM_AP                         = 30,
	PID_44_MAGNUM_JHP                   = 31,
	PID_FLAMETHROWER_FUEL               = 32,
	PID_14MM_AP                         = 33,
	PID_223_FMJ                         = 34,
	PID_5MM_JHP                         = 35,
	PID_5MM_AP                          = 36,
	PID_ROCKET_AP                       = 37,
	PID_SMALL_ENERGY_CELL               = 38,
	PID_MICRO_FUSION_CELL               = 39,
	PID_SHOTGUN_SHELLS                  = 95,
	PID_44_FMJ_MAGNUM                   = 111,
	PID_9MM_BALL                        = 121,
	PID_BBS                             = 163,
	PID_ROBO_ROCKET_AMMO                = 274,
	PID_45_CALIBER_AMMO                 = 357,
	PID_2MM_EC_AMMO                     = 358,
	PID_4_7MM_CASELESS                  = 359,
	PID_9MM_AMMO                        = 360,
	PID_HN_NEEDLER_CARTRIDGE            = 361,
	PID_HN_AP_NEEDLER_CARTRIDGE         = 362,
	PID_7_62MM_AMMO                     = 363,
	PID_FLAMETHROWER_FUEL_MK_II         = 382,

	/******************************************************************
	***************       Medical                       ***************
	******************************************************************/

	PID_STIMPAK                         = 40,
	PID_FIRST_AID_KIT                   = 47,
	PID_RADAWAY                         = 48,
	PID_ANTIDOTE                        = 49,
	PID_MENTATS                         = 53,
	PID_MUTATED_FRUIT                   = 71,
	PID_BUFFOUT                         = 87,
	PID_DOCTORS_BAG                     = 91,
	PID_RAD_X                           = 109,
	PID_PSYCHO                          = 110,
	PID_SUPER_STIMPAK                   = 144,
	PID_JET                             = 259,
	PID_JET_ANTIDOTE                    = 260,
	PID_BROC_FLOWER                     = 271,
	PID_XANDER_ROOT                     = 272,
	PID_HEALING_POWDER                  = 273,
	PID_MEAT_JERKY                      = 284,
	PID_HYPODERMIC_NEEDLE               = 318,
	PID_MUTAGENIC_SYRUM                 = 329,
	PID_HEART_PILLS                     = 333,
	PID_HYPO_POISON                     = 334,
	PID_FIELD_MEDIC_KIT                 = 408,
	PID_PARAMEDICS_BAG                  = 409,
	PID_MONUMENT_CHUNCK                 = 424,
	PID_MEDICAL_SUPPLIES                = 428,

	/******************************************************************
	***************       Container                     ***************
	******************************************************************/

	PID_FRIDGE                          = 42,
	PID_ICE_CHEST_LEFT                  = 43,
	PID_ICE_CHEST_RIGHT                 = 44,
	PID_BAG                             = 46,
	PID_BACKPACK                        = 90,
	PID_BROWN_BAG                       = 93,
	PID_FOOTLOCKER_CLEAN_LEFT           = 128,
	PID_FOOTLOCKER_RUSTY_LEFT           = 129,
	PID_FOOTLOCKER_CLEAN_RIGHT          = 130,
	PID_FOOTLOCKER_RUSTY_RIGHT          = 131,
	PID_LOCKER_CLEAN_LEFT               = 132,
	PID_LOCKER_RUSTY_LEFT               = 133,
	PID_LOCKER_CLEAN_RIGHT              = 134,
	PID_LOCKER_RUSTY_RIGHT              = 135,
	PID_WALL_LOCKER_CLEAN_LEFT          = 136,
	PID_WALL_LOCKER_CLEAN_RIGHT         = 137,
	PID_WALL_LOCKER_RUSTY_LEFT          = 138,
	PID_WALL_LOCKER_RUSTY_RIGHT         = 139,
	PID_CONTAINER_WOOD_CRATE            = 180,
	PID_VAULT_DWELLER_BONES             = 211,
	PID_SMALL_POT                       = 243,
	PID_TALL_POT                        = 244,
	PID_CHEST                           = 245,
	PID_LEFT_ARROYO_BOOKCASE            = 246,
	PID_RIGHT_ARROYO_BOOKCASE           = 247,
	PID_OLIVE_POT                       = 248,
	PID_FLOWER_POT                      = 249,
	PID_HUMAN_BONES                     = 250,
	PID_ANNA_BONES                      = 251,
	PID_CRASHED_VERTI_BIRD              = 330,
	PID_GRAVESITE_1                     = 344,
	PID_GRAVESITE_2                     = 345,
	PID_GRAVESITE_3                     = 346,
	PID_GRAVESITE_4                     = 347,
	PID_LG_LT_AMMO_CRATE                = 367,
	PID_SM_LT_AMMO_CRATE                = 368,
	PID_LG_RT_AMMO_CRATE                = 369,
	PID_SM_RT_AMMO_CRATE                = 370,
	PID_LF_GRAVESITE_1                  = 374,
	PID_LF_GRAVESITE_2                  = 375,
	PID_LF_GRAVESITE_3                  = 376,
	PID_STONE_HEAD                      = 425,
	PID_WAGON_RED                       = 434,
	PID_WAGON_GREY                      = 435,
	PID_CAR_TRUNK                       = 455,
	PID_JESSE_CONTAINER                 = 467,  // Engine item
	PID_WALL_SAFE                       = 501,
	PID_FLOOR_SAFE                      = 502,
	PID_POOL_TABLE_1                    = 510,
	PID_POOL_TABLE_2                    = 511,
	PID_POOL_TABLE_3                    = 512,
	PID_POOL_TABLE_4                    = 513,
	PID_POOL_TABLE_5                    = 514,
	PID_POOL_TABLE_6                    = 515,
	PID_POOR_BOX                        = 521,

	/******************************************************************
	***************       Books                         ***************
	******************************************************************/

	PID_BIG_BOOK_OF_SCIENCE             = 73,
	PID_DEANS_ELECTRONICS               = 76,
	PID_FIRST_AID_BOOK                  = 80,
	PID_SCOUT_HANDBOOK                  = 86,
	PID_GUNS_AND_BULLETS                = 102,
	PID_CATS_PAW                        = 225,
	PID_TECHNICAL_MANUAL                = 228,
	PID_CHEMISTRY_MANUAL                = 237,

	PID_SLAG_MESSAGE                    = 263,
	PID_CATS_PAW_ISSUE_5                = 331,

	/******************************************************************
	***************       Tools                         ***************
	******************************************************************/

	PID_DYNAMITE                        = 51,
	PID_GEIGER_COUNTER                  = 52,
	PID_STEALTH_BOY                     = 54,
	PID_MOTION_SENSOR                   = 59,
	PID_MULTI_TOOL                      = 75,
	PID_ELECTRONIC_LOCKPICKS            = 77,
	PID_LOCKPICKS                       = 84,
	PID_PLASTIC_EXPLOSIVES              = 85,
	PID_ROPE                            = 127,
	PID_SUPER_TOOL_KIT                  = 308,
	PID_EXP_LOCKPICK_SET                = 410,
	PID_ELEC_LOCKPICK_MKII              = 411,

	/******************************************************************
	***************       Misc.                         ***************
	******************************************************************/

	PID_BOTTLE_CAPS                     = 41,
	PID_RESERVED_ITEM00                 = 50,  // Reserved item! Don't use! Broken!
	PID_WATER_CHIP                      = 55,
	PID_DOG_TAGS                        = 56,
	PID_ELECTRONIC_BUG                  = 57,
	PID_HOLODISK                        = 58,
	PID_BRIEFCASE                       = 72,
	PID_FUZZY_PAINTING                  = 78,
	PID_FLARE                           = 79,
	PID_IGUANA_ON_A_STICK               = 81,
	PID_KEY                             = 82,
	PID_KEYS                            = 83,
	PID_WATCH                           = 88,
	PID_MOTOR                           = 89,
	PID_SCORPION_TAIL                   = 92,
	PID_RED_PASS_KEY                    = 96,
	PID_BLUE_PASS_KEY                   = 97,
	PID_PUMP_PARTS                      = 98,
	PID_GOLD_LOCKET                     = 99,
	PID_RADIO                           = 100,
	PID_LIGHTER                         = 101,
	PID_MEAT_ON_A_STICK                 = 103,
	PID_TAPE_RECORDER                   = 104,
	PID_NUKE_KEY                        = 105,
	PID_NUKA_COLA                       = 106,
	PID_ALIEN_SIDE                      = 107,
	PID_ALIEN_FORWARD                   = 108,
	PID_URN                             = 112,
	PID_TANGLERS_HAND                   = 114,
	PID_FLOWER                          = 117,
	PID_NECKLACE                        = 119,
	PID_PSYCHIC_NULLIFIER               = 123,
	PID_BEER                            = 124,
	PID_BOOZE                           = 125,
	PID_WATER_FLASK                     = 126,
	PID_ACCESS_CARD                     = 140,
	PID_BLACK_COC_BADGE                 = 141,
	PID_RED_COC_BADGE                   = 142,
	PID_BARTER_TANDI                    = 212,
	PID_BARTER_LIGHT_HEALING            = 218,
	PID_BARTER_MEDIUM_HEALING           = 219,
	PID_BARTER_HEAVY_HEALING            = 220,
	PID_SECURITY_CARD                   = 221,
	PID_TOGGLE_SWITCH                   = 222,
	PID_YELLOW_PASS_KEY                 = 223,
	PID_SMALL_STATUETTE                 = 224,
	PID_BOX_OF_NOODLES                  = 226,
	PID_FROZEN_DINNER                   = 227,
	PID_MOTIVATOR                       = 229,
	PID_ANNA_GOLD_LOCKET                = 252,
	PID_CAR_FUEL_CELL_CONTROLLER        = 253,
	PID_CAR_FUEL_INJECTION              = 254,
	PID_DAY_PASS                        = 255,
	PID_FAKE_CITIZENSHIP                = 256,
	PID_CORNELIUS_GOLD_WATCH            = 257,
	PID_HY_MAG_PART                     = 258,
	PID_RUBBER_BOOTS                    = 262,
	PID_SMITH_COOL_ITEM                 = 264,
	PID_VIC_RADIO                       = 266,
	PID_VIC_WATER_FLASK                 = 267,
	PID_ROBOT_PARTS                     = 269,
	PID_TROPHY_OF_RECOGNITION           = 275,
	PID_GECKO_PELT                      = 276,
	PID_GOLDEN_GECKO_PELT               = 277,
	PID_FLINT                           = 278,
	PID_NEURAL_INTERFACE                = 279,
	PID_DIXON_EYE                       = 281,
	PID_CLIFTON_EYE                     = 282,
	PID_RADSCORPION_PARTS               = 285,
	PID_FIREWOOD                        = 286,
	PID_CAR_FUEL_CELL                   = 288,
	PID_SHOVEL                          = 289,
	PID_HOLODISK_FAKE_V13               = 294,
	PID_CHEEZY_POOFS                    = 295,
	PID_PLANK                           = 297,
	PID_TRAPPER_TOWN_KEY                = 298,
	PID_CLIPBOARD                       = 301,
	PID_GECKO_DATA_DISK                 = 302,
	PID_REACTOR_DATA_DISK               = 303,
	PID_DECK_OF_TRAGIC_CARDS            = 304,
	PID_YELLOW_REACTOR_KEYCARD          = 305,
	PID_RED_REACTOR_KEYCARD             = 306,
	PID_PLASMA_TRANSFORMER              = 307,
	PID_TALISMAN                        = 309,
	PID_GAMMA_GULP_BEER                 = 310,
	PID_ROENTGEN_RUM                    = 311,
	PID_PART_REQUISITION_FORM           = 312,
	PID_BLUE_CONDOM                     = 314,
	PID_GREEN_CONDOM                    = 315,
	PID_RED_CONDOM                      = 316,
	PID_COSMETIC_CASE                   = 317,
	PID_CYBERNETIC_BRAIN                = 321,
	PID_HUMAN_BRAIN                     = 322,
	PID_CHIMP_BRAIN                     = 323,
	PID_ABNORMAL_BRAIN                  = 324,
	PID_DICE                            = 325,
	PID_LOADED_DICE                     = 326,
	PID_EASTER_EGG                      = 327,
	PID_MAGIC_8_BALL                    = 328,
	PID_MOORE_BAD_BRIEFCASE             = 335,
	PID_MOORE_GOOD_BRIEFCASE            = 336,
	PID_LYNETTE_HOLO                    = 337,
	PID_WESTIN_HOLO                     = 338,
	PID_SPY_HOLO                        = 339,
	PID_DR_HENRY_PAPERS                 = 340,
	PID_PRESIDENTIAL_PASS               = 341,
	PID_RANGER_PIN                      = 342,
	PID_RANGER_MAP                      = 343,
	PID_COMPUTER_VOICE_MODULE           = 356,
	PID_GECK                            = 366,
	PID_V15_KEYCARD                     = 373,
	PID_V15_COMPUTER_PART               = 377,
	PID_COOKIE                          = 378,
	PID_OIL_CAN                         = 412,
	PID_STABLES_ID_BADGE                = 413,
	PID_VAULT_13_SHACK_KEY              = 414,
	PID_SPECTACLES                      = 415,  // DO NOT USE THIS IN YOUR SCRIPTS, THIS IS SPECIAL CASE
	PID_EMPTY_JET                       = 416,  // DO NOT USE THIS IN YOUR SCRIPTS, THIS IS SPECIAL CASE
	PID_OXYGEN_TANK                     = 417,  // DO NOT USE THIS IN YOUR SCRIPTS, THIS IS SPECIAL CASE
	PID_POISON_TANK                     = 418,  // DO NOT USE THIS IN YOUR SCRIPTS, THIS IS SPECIAL CASE
	PID_MINE_PART                       = 419,  // DO NOT USE THIS IN YOUR SCRIPTS, THIS IS SPECIAL CASE
	PID_MORNING_STAR_MINE               = 420,
	PID_EXCAVATOR_CHIP                  = 422,
	PID_GOLD_TOOTH                      = 429,
	PID_HOWITZER_SHELL                  = 430,
	PID_RAMIREZ_BOX_CLOSED              = 431,
	PID_RAMIREZ_BOX_OPEN                = 432,
	PID_MIRROR_SHADES                   = 433,
	PID_DECK_OF_CARDS                   = 436,
	PID_MARKED_DECK_OF_CARDS            = 437,
	PID_TEMPLE_KEY                      = 438,
	PID_POCKET_LINT                     = 439,
	PID_BIO_GEL                         = 440,
	PID_BLONDIE_DOG_TAG                 = 441,
	PID_ANGEL_EYES_DOG_TAG              = 442,
	PID_TUCO_DOG_TAG                    = 443,
	PID_RAIDERS_MAP                     = 444,
	PID_SHERIFF_BADGE                   = 445,
	PID_VERTIBIRD_PLANS                 = 446,
	PID_BISHOPS_HOLODISK                = 447,
	PID_ACCOUNT_BOOK                    = 448,
	PID_ECON_HOLODISK                   = 449,
	PID_TORN_PAPER_1                    = 450,
	PID_TORN_PAPER_2                    = 451,
	PID_TORN_PAPER_3                    = 452,
	PID_PASSWORD_PAPER                  = 453,
	PID_EXPLOSIVE_SWITCH                = 454,
	PID_CELL_DOOR_KEY                   = 456,
	PID_ELRON_FIELD_REP                 = 457,
	PID_ENCLAVE_HOLODISK_5              = 458,
	PID_ENCLAVE_HOLODISK_1              = 459,
	PID_ENCLAVE_HOLODISK_2              = 460,
	PID_ENCLAVE_HOLODISK_3              = 461,
	PID_ENCLAVE_HOLODISK_4              = 462,
	PID_EVACUATION_HOLODISK             = 463,
	PID_EXPERIMENT_HOLODISK             = 464,
	PID_MEDICAL_HOLODISK                = 465,
	PID_PASSWORD_HOLODISK               = 466,
	PID_SMITTY_MEAL                     = 468,
	PID_ROT_GUT                         = 469,
	PID_BALL_GAG                        = 470,
	PID_BECKY_BOOK                      = 471,
	PID_ELRON_MEMBER_HOLO               = 472,
	PID_MUTATED_TOE                     = 473,
	PID_DAISIES                         = 474,
	PID_ENLIGHTENED_ONE_LETTER          = 476,
	PID_BROADCAST_HOLODISK              = 477,
	PID_SIERRA_MISSION_HOLODISK         = 478,
	PID_NAV_COMPUTER_PARTS              = 479,
	PID_KITTY_SEX_DRUG_AGILITY          = 480,  // + 1 agility for 1 hr
	PID_KITTY_SEX_DRUG_INTELLIGENCE     = 481,  // + 1 iq for 1 hr
	PID_KITTY_SEX_DRUG_STRENGTH         = 482,  // + 1 strength for 1 hr
	PID_FALLOUT_2_HINTBOOK              = 483,  // no touchy
	PID_PLAYERS_EAR                     = 484,
	PID_MASTICATORS_EAR                 = 485,
	PID_MEMO_FROM_FRANCIS               = 487,
	PID_K9_MOTIVATOR                    = 488,
	PID_SPECIAL_BOXER_WEAPON            = 489,
	PID_NCR_HISTORY_HOLODISK            = 490,
	PID_MR_NIXON_DOLL                   = 491,
	PID_TANKER_FOB                      = 492,
	PID_ELRON_TEACH_HOLO                = 493,
	PID_KOKOWEEF_MINE_SCRIP             = 494,
	PID_PRES_ACCESS_KEY                 = 495,
	PID_DERMAL_PIP_BOY_DISK             = 499,  // AGAIN, VERY SPECIAL, NOT FOR YOU
	PID_MEM_CHIP_BLUE                   = 503,
	PID_MEM_CHIP_GREEN                  = 504,
	PID_MEM_CHIP_RED                    = 505,
	PID_MEM_CHIP_YELLOW                 = 506,
	PID_DECOMPOSING_BODY                = 507,
	PID_BLOW_UP_DOLL                    = 508,
	PID_POPPED_BLOW_UP_DOLL             = 509,
	PID_PIP_BOY_MEDICAL_ENHANCER        = 516,
	PID_TYPHON_TREASURE                 = 519,
	PID_SURVEY_MAP                      = 523,

	/******************************************************************
	***************       Active Items                  ***************
	******************************************************************/

	PID_ACTIVE_FLARE                    = 205,
	PID_ACTIVE_DYNAMITE                 = 206,
	PID_ACTIVE_GEIGER_COUNTER           = 207,
	PID_ACTIVE_MOTION_SENSOR            = 208,
	PID_ACTIVE_PLASTIC_EXPLOSIVE        = 209,
	PID_ACTIVE_STEALTH_BOY              = 210,

	/******************************************************************
	***************       Flying Weapons                ***************
	******************************************************************/

	PID_FLYING_ROCKET                   = 0x5000001,
	PID_FLYING_PLASMA_BALL              = 0x5000002,
	PID_FLYING_KNIFE                    = 0x5000006,
	PID_FLYING_SPEAR                    = 0x5000007,
	PID_FLYING_LASER_BLAST              = 0x5000009,
	PID_FLYING_PLASMA_BLAST             = 0x500000A,
	PID_FLYING_ELECTRICITY_BOLT         = 0x500000B,


	/******************************************************************
	***************       Critter Type                  ***************
	******************************************************************/

	PID_Player                          = 0x01000000,
	PID_GORIS                           = 0x01000098,


	/******************************************************************
	***************       Scenery Type                  ***************
	******************************************************************/

	PID_RAD_GOO_1                       = 0x020003D9,
	PID_RAD_GOO_4                       = 0x020003DC,
	PID_DRIVABLE_CAR                    = 0x020003F1, // index 1009


	/******************************************************************
	***************       Misc Type                     ***************
	******************************************************************/

	PID_CORPSE_BLOOD                    = 0x05000004,
};

}
