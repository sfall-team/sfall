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


/******************************************************************************/
/* FALLOUT2.EXE structs should be placed here  */
/******************************************************************************/

// TODO: make consistent naming for all FO structs

struct TGameObj;

/*   26 */
#pragma pack(push, 1)
struct TInvenRec {
	TGameObj *object;
	__int32 count;
};
#pragma pack(pop)

/* 15 */
#pragma pack(push, 1)
struct TGameObj {
	__int32 ID;
	__int32 tile;
	char gap_8[2];
	char field_A;
	char gap_B[17];
	__int32 rotation;
	__int32 artFID;
	char gap_24[4];
	__int32 elevation;
	__int32 invenCount;
	__int32 field_30;
	TInvenRec *invenTablePtr;
	char gap_38[4];
	__int32 itemCharges;
	__int32 critterAP_weaponAmmoPid;
	char gap_44[16];
	__int32 lastTarget;
	char gap_58[12];
	__int32 pid;
	char gap_68[16];
	__int32 scriptID;
	char gap_7C[4];
	__int32 script_index;
	char gap_84[7];
	char field_0;
};
#pragma pack(pop)

/*    9 */
#pragma pack(push, 1)
struct TComputeAttack {
	TGameObj *attacker;
	char gap_4[4];
	TGameObj *weapon;
	char gap_C[4];
	__int32 damageAttacker;
	__int32 flagsAttacker;
	__int32 rounds;
	char gap_1C[4];
	TGameObj *target;
	__int32 targetTile;
	__int32 bodyPart;
	__int32 damageTarget;
	__int32 flagsTarget;
	__int32 knockbackValue;
};
#pragma pack(pop)


/*   22 */
#pragma pack(push, 1)
struct TScript {
	__int32 script_id;
	char gap_4[4];
	__int32 elevation_and_tile;
	__int32 spatial_radius;
	char gap_10[4];
	__int32 script_index;
	__int32 program_ptr;
	__int32 self_obj_id;
	char gap_20[8];
	__int32 scr_return;
	char gap_2C[4];
	__int32 fixed_param;
	TGameObj *self_obj;
	TGameObj *source_obj;
	TGameObj *target_obj;
	__int32 script_overrides;
	char field_44;
	char gap_45[15];
	__int32 procedure_table[28];
};
#pragma pack(pop)


/*   25 */
#pragma pack(push, 1)
struct TProgram {
	const char* fileName;
	__int32 *codeStackPtr;
	char gap_8[8];
	__int32 *codePtr;
	__int32 field_14;
	char gap_18[4];
	__int32 *dStackPtr;
	__int32 *aStackPtr;
	__int32 *dStackOffs;
	__int32 *aStackOffs;
	char gap_2C[4];
	__int32 *stringRefPtr;
	char gap_34[4];
	__int32 *procTablePtr;
};
#pragma pack(pop)

struct ItemButtonItem {
	TGameObj* item;
	__int32 field_2;
	__int32 field_3;
	__int32 field_4;
	__int32 mode;
	__int32 field_6;
};

struct PerkInfo {
	char* Name;
	char* Desc;
	__int32 Image;
	__int32 Ranks;
	__int32 Level;
	__int32 Stat;
	__int32 StatMag;
	__int32 Skill1;
	__int32 Skill1Mag;
	__int32 Type;
	__int32 Skill2;
	__int32 Skill2Mag;
	__int32 Str;
	__int32 Per;
	__int32 End;
	__int32 Chr;
	__int32 Int;
	__int32 Agl;
	__int32 Lck;
};

struct DBFile {
	__int32 fileType;
	void* handle;
};

struct sElevator {
	__int32 ID1;
	__int32 Elevation1;
	__int32 Tile1;
	__int32 ID2;
	__int32 Elevation2;
	__int32 Tile2;
	__int32 ID3;
	__int32 Elevation3;
	__int32 Tile3;
	__int32 ID4;
	__int32 Elevation4;
	__int32 Tile4;
};

#pragma pack(push, 1)
struct FRM {
	__int32 id;			//0x00
	__int32 unused;		//0x04
	__int16 frames;		//0x08
	__int16 xshift[6];		//0x0a
	__int16 yshift[6];		//0x16
	__int32 framestart[6];//0x22
	__int32 size;			//0x3a
	__int16 width;			//0x3e
	__int16 height;		//0x40
	__int32 frmsize;		//0x42
	__int16 xoffset;		//0x46
	__int16 yoffset;		//0x48
	unsigned char pixels[80 * 36];	//0x4a
};
#pragma pack(pop)

struct MessageNode {
	__int32 number;
	__int32 flags;
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
	__int32 numMsgs;
	MessageNode *nodes;

	MessageList() {
		nodes = nullptr;
		numMsgs = 0;
	}
} MessageList;


struct sArt {
	__int32 flags;
	char path[16];
	char* names;
	__int32 d18;
	__int32 total;
};

struct CritStruct {
	union {
		struct {
			__int32 DamageMultiplier;
			__int32 EffectFlags;
			__int32 StatCheck;
			__int32 StatMod;
			__int32 FailureEffect;
			__int32 Message;
			__int32 FailMessage;
		};
		__int32 values[7];
	};
};

#pragma pack(push, 1)
struct SkillInfo
{
  __int32 name;
  __int32 desc;
  __int32 attr;
  __int32 image;
  __int32 base;
  __int32 statMulti;
  __int32 statA;
  __int32 statB;
  __int32 skillPointMulti;
  __int32 Exp;
  __int32 f;
};
#pragma pack(pop)


struct TraitInfo {
	char* Name;
	char* Desc;
	__int32 Image;
};

//fallout2 path node structure
struct sPath {
	char* path;
	void* pDat;
	__int32 isDat;
	sPath* next;
};

struct sProtoBase {
	__int32 pid;
	__int32 message_num;
	__int32 fid;
};

struct sProtoTile {
	sProtoBase base;	
	__int32 flags;
	__int32 flags_ext;
	__int32 material;
	__int32 field_18;
};

struct sProtoObj {
	sProtoBase base;
	__int32 light_distance;
	__int32 light_intensity;
	__int32 flags;
};

struct sProtoItem {
	sProtoObj obj;	
	__int32 flags_ext;
	__int32 sid;
	ItemType type;
};

struct sProtoWeapon
{
	sProtoItem item;
	__int32 animation_code;
	__int32 min_damage;
	__int32 max_damage;
	__int32 dt;
	__int32 max_range1;
	__int32 max_range2;
	__int32 proj_pid;
	__int32 min_st;
	__int32 mp_cost1;
	__int32 mp_cost2;
	__int32 crit_fail_table;
	__int32 perk;
	__int32 rounds;
	__int32 caliber;
	__int32 ammo_type_pid;
	__int32 max_ammo;
	__int32 sound_id;
	__int32 field_68;
	__int32 material;
	__int32 size;
	__int32 weight;
	__int32 cost;
	__int32 inv_fid;
	__int8 SndID;
};

//for holding window info
struct WINinfo {
	__int32 ref;
	__int32 flags;
	RECT wRect;
	unsigned __int32 width;
	unsigned __int32 height;
	__int32 clearColour;
	__int32 unknown2;
	__int32 unknown3;
	unsigned char *surface;         // bytes frame data ref to palette
	__int32 buttonListP;
	__int32 unknown5;//buttonptr?
	__int32 unknown6;
	__int32 unknown7;
	__int32 drawFuncP;
};
