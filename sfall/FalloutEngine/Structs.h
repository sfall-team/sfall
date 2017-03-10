/*
* sfall
* Copyright (C) 2008-2016 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <Windows.h>

#include "Enums.h"

namespace fo
{

/******************************************************************************/
/* FALLOUT2.EXE structs should be placed here  */
/******************************************************************************/

// TODO: make consistent naming for all FO structs

struct GameObject;
struct Program;
struct ScriptInstance;

/*   26 */
#pragma pack(push, 1)
struct TInvenRec {
	GameObject *object;
	long count;
};
#pragma pack(pop)

// Game objects (items, critters, etc.), including those stored in inventories.
#pragma pack(push, 1)
struct GameObject {
	long id;
	long tile;
	long x;
	long y;
	long sx;
	long sy;
	long frm;
	long rotation;
	long artFid;
	long flags;
	long elevation;
	long invenSize;
	long invenMax;
	TInvenRec *invenTable;
	union {
		struct {
			char gap_38[4];
			// for weapons - ammo in magazine, for ammo - amount of ammo in last ammo pack
			long charges;
			// current type of ammo loaded in magazine
			long ammoPid;
			char gap_44[32];
		} item;
		struct {
			long reaction;
			// 1 - combat, 2 - enemies out of sight, 4 - running away
			long combatState;
			// aka action points
			long movePoints;
			long damageFlags;
			long damageLastTurn;
			long aiPacket;
			long teamNum;
			long whoHitMe;
			long health;
			long rads;
			long poison;
		} critter;
	};
	long pid;
	long cid;
	long lightDistance;
	long lightIntensity;
	char outline[4];
	long scriptId;
	GameObject* owner;
	long scriptIndex;

	inline char type() {
		return pid >> 24;
	}
};
#pragma pack(pop)

// Results of compute_attack_() function.
#pragma pack(push, 1)
struct ComputeAttackResult {
	GameObject* attacker;
	long hitMode;
	GameObject* weapon;
	long field_C;
	long attackerDamage;
	long attackerFlags;
	long numRounds;
	long message;
	GameObject* target;
	long targetTile;
	long bodyPart;
	long damage;
	long flags;
	long knockbackValue;
	GameObject* mainTarget;
	long numExtras;
	GameObject* extraTarget[6];
	long extraBodyPart[6];
	long extraDamage[6];
	long extraFlags[6];
	long extraKnockbackValue[6];
};
#pragma pack(pop)

// Script instance attached to an object or tile (spatial script).
#pragma pack(push, 1)
struct ScriptInstance {
	long id;
	long next;
	// first 3 bits - elevation, rest - tile number
	long elevationAndTile;
	long spatialRadius;
	long flags;
	long scriptIdx;
	Program *program;
	long selfObjectId;
	long localVarOffset;
	long numLocalVars;
	long returnValue;
	long action;
	long fixedParam;
	GameObject *selfObject;
	GameObject *sourceObject;
	GameObject *targetObject;
	long actionNum;
	long scriptOverrides;
	char gap_48[4];
	long howMuch;
	char gap_50[4];
	long procedureTable[28];
};
#pragma pack(pop)


/*   25 */
#pragma pack(push, 1)
struct Program {
	const char* fileName;
	long *codeStackPtr;
	char gap_8[8];
	long *codePtr;
	long field_14;
	char gap_18[4];
	long *dStackPtr;
	long *aStackPtr;
	long *dStackOffs;
	long *aStackOffs;
	char gap_2C[4];
	long *stringRefPtr;
	char gap_34[4];
	long *procTablePtr;
};
#pragma pack(pop)

struct ItemButtonItem {
	GameObject* item;
	long flags;
	long primaryAttack;
	long secondaryAttack;
	long mode;
	long fid;
};

// When gained, the perk increases Stat by StatMod, which may be negative. All other perk effects come from being
// specifically checked for by scripts or the engine. If a primary stat requirement is negative, that stat must be
// below the value specified (e.g., -7 indicates a stat must be less than 7). Operator is only non-zero when there
// are two skill requirements. If set to 1, only one of those requirements must be met; if set to 2, both must be met.
struct PerkInfo {
	const char* name;
	const char* description;
	long image;
	long ranks;
	long levelMin;
	long stat;
	long statMod;
	long skill1;
	long skill1Min;
	long skillOperator;
	long skill2;
	long skill2Min;
	long strengthMin;
	long perceptionMin;
	long enduranceMin;
	long charismaMin;
	long intelligenceMin;
	long agilityMin;
	long luckMin;
};

struct DbFile {
	long fileType;
	void* handle;
};

struct ElevatorExit {
	long id;
	long elevation;
	long tile;
};

#pragma pack(push, 1)
struct FRM {
	long id;			//0x00
	long unused;		//0x04
	short frames;		//0x08
	short xshift[6];		//0x0a
	short yshift[6];		//0x16
	long framestart[6];//0x22
	long size;			//0x3a
	short width;			//0x3e
	short height;		//0x40
	long frmsize;		//0x42
	short xoffset;		//0x46
	short yoffset;		//0x48
	BYTE pixels[80 * 36];	//0x4a
};
#pragma pack(pop)

struct MessageNode {
	long number;
	long flags;
	char* audio;
	char* message;

	MessageNode() {
		number = 0;
		flags = 0;
		audio = nullptr;
		message = nullptr;
	}
};

//for holding msg array
typedef struct MessageList {
	long numMsgs;
	MessageNode *nodes;

	MessageList() {
		nodes = nullptr;
		numMsgs = 0;
	}
} MessageList;


struct sArt {
	long flags;
	char path[16];
	char* names;
	long d18;
	long total;
};

struct CritStruct {
	union {
		struct {
			long DamageMultiplier;
			long EffectFlags;
			long StatCheck;
			long StatMod;
			long FailureEffect;
			long Message;
			long FailMessage;
		};
		long values[7];
	};
};

#pragma pack(push, 1)
struct SkillInfo
{
	char* name;
	char* desc;
	long attr;
	long image;
	long base;
	long statMulti;
	long statA;
	long statB;
	long skillPointMulti;
	long Exp;
	long f;
};
#pragma pack(pop)

struct StatInfo {
	char* dame;
	char* description;
	long image;
	long minValue;
	long maxValue;
	long defaultValue;
};

struct TraitInfo {
	char* Name;
	char* Desc;
	long Image;
};

//fallout2 path node structure
struct sPath {
	char* path;
	void* pDat;
	long isDat;
	sPath* next;
};

struct PremadeChar {
	char path[20];
	DWORD fid;
	char unknown[20];
};

struct sProtoBase {
	long pid;
	long message_num;
	long fid;
};

struct sProtoTile {
	sProtoBase base;	
	long flags;
	long flags_ext;
	long material;
	long field_18;
};

struct sProtoObj {
	sProtoBase base;
	long light_distance;
	long light_intensity;
	long flags;
};

struct sProtoItem {
	sProtoObj obj;	
	long flags_ext;
	long sid;
	ItemType type;
};

struct sProtoWeapon
{
	sProtoItem item;
	long animation_code;
	long min_damage;
	long max_damage;
	long dt;
	long max_range1;
	long max_range2;
	long proj_pid;
	long min_st;
	long mp_cost1;
	long mp_cost2;
	long crit_fail_table;
	long perk;
	long rounds;
	long caliber;
	long ammo_type_pid;
	long max_ammo;
	long sound_id;
	long field_68;
	long material;
	long size;
	long weight;
	long cost;
	long inv_fid;
	BYTE SndID;
};

struct sProtoCritter {
	sProtoObj obj;
	long flags_ext;
	long sid;
	long critter_flags;
	long base_stat_srength;
	long base_stat_prception;
	long base_stat_endurance;
	long base_stat_charisma;
	long base_stat_intelligence;
	long base_stat_agility;
	long base_stat_luck;
	long base_stat_hp;
	long base_stat_ap;
	long base_stat_ac;
	// not used by engine
	long base_stat_unarmed_damage;
	long base_stat_melee_damage;
	long base_stat_carry_weight;
	long base_stat_sequence;
	long base_stat_healing_rate;
	long base_stat_critical_chance;
	long base_stat_better_criticals;
	long base_dt_normal;
	long base_dt_laser;
	long base_dt_fire;
	long base_dt_plasma;
	long base_dt_electrical;
	long base_dt_emp;
	long base_dt_explode;
	long base_dr_normal;
	long base_dr_laser;
	long base_dr_fire;
	long base_dr_plasma;
	long base_dr_electrical;
	long base_dr_emp;
	long base_dr_explode;
	long base_dr_radiation;
	long base_dr_poison;
	long base_age;
	long base_gender;
	long bonus_stat_srength;
	long bonus_stat_prception;
	long bonus_stat_endurance;
	long bonus_stat_charisma;
	long bonus_stat_intelligence;
	long bonus_stat_agility;
	long bonus_stat_luck;
	long bonus_stat_hp;
	long bonus_stat_ap;
	long bonus_stat_ac;
	long bonus_stat_unarmed_damage;
	long bonus_stat_melee_damage;
	long bonus_stat_carry_weight;
	long bonus_stat_sequence;
	long bonus_stat_healing_rate;
	long bonus_stat_critical_chance;
	long bonus_stat_better_criticals;
	long bonus_dt_normal;
	long bonus_dt_laser;
	long bonus_dt_fire;
	long bonus_dt_plasma;
	long bonus_dt_electrical;
	long bonus_dt_emp;
	long bonus_dt_explode;
	long bonus_dr_normal;
	long bonus_dr_laser;
	long bonus_dr_fire;
	long bonus_dr_plasma;
	long bonus_dr_electrical;
	long bonus_dr_emp;
	long bonus_dr_explode;
	long bonus_dr_radiation;
	long bonus_dr_poison;
	long bonus_age;
	long bonus_gender;
	long skill_small_guns;
	long skill_big_guns;
	long skill_energy_weapons;
	long skill_unarmed;
	long skill_melee_weapons;
	long skill_throwing;
	long skill_first_aid;
	long skill_doctor;
	long skill_sneak;
	long skill_lockpick;
	long skill_steal;
	long skill_traps;
	long skill_science;
	long skill_repair;
	long skill_speech;
	long skill_barter;
	long skill_gambling;
	long skill_outdoorsman;
	long body_type;
	long exp_val;
	long kill_type;
	long damage_type;
	long head_fid;
	long ai_packet;
	long team_num;
};

struct ScriptListInfoItem {
	char fileName[16];
	long numLocalVars;
};

//for holding window info
struct WINinfo {
	long ref;
	long flags;
	RECT wRect;
	long width;
	long height;
	long clearColour;
	long unknown2;
	long unknown3;
	BYTE *surface; // bytes frame data ref to palette
	long buttonListP;
	long unknown5;//buttonptr?
	long unknown6;
	long unknown7;
	long drawFuncP;
};

}
