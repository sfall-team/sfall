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


/******************************************************************************/
/* FALLOUT2.EXE structs, function offsets and wrappers should be placed here  */
/******************************************************************************/


struct TGameObj;
struct TProgram;
struct TScript;

/*   26 */
#pragma pack(push, 1)
struct TInvenRec
{
	TGameObj *object;
	long count;
};
#pragma pack(pop)

/* 15 */
#pragma pack(push, 1)
struct TGameObj
{
	long ID;
	long tile;
	long x;
	long y;
	long sx;
	long sy;
	long currentFrm;
	long rotation;
	long artFID;
	long flags;
	long elevation;
	long invenCount;
	long invenMax;
	TInvenRec *invenTablePtr;
	char gap_38[4];
	long itemCharges;
	long critterAP_weaponAmmoPid;
	char gap_44[16];
	long lastTarget;
	char gap_58[12];
	DWORD pid;
	long cid;
	long lightDistance;
	long lightIntensity;
	DWORD outline;
	long scriptID;
	TGameObj* owner;
	long script_index;
	char gap_84[7];
	char field_0;
};
#pragma pack(pop)

/*    9 */
#pragma pack(push, 1)
struct TComputeAttack
{
	TGameObj *attacker;
	char gap_4[4];
	TGameObj *weapon;
	char gap_C[4];
	long damageAttacker;
	long flagsAttacker;
	long rounds;
	char gap_1C[4];
	TGameObj *target;
	long targetTile;
	long bodyPart;
	long damageTarget;
	long flagsTarget;
	long knockbackValue;
};
#pragma pack(pop)

/*   22 */
#pragma pack(push, 1)
struct TScript
{
	long script_id;
	long next;
	// first 3 bits - elevation, rest - tile number
	long elevation_and_tile;
	long spatial_radius;
	long flags;
	long script_index;
	TProgram *program_ptr;
	long self_obj_id;
	long local_var_offset;
	long num_local_vars;
	long scr_return;
	long action;
	long fixed_param;
	TGameObj *self_obj;
	TGameObj *source_obj;
	TGameObj *target_obj;
	long action_num;
	long script_overrides;
	char gap_48[4];
	long how_much;
	char gap_50[4];
	long procedure_table[28];
};
#pragma pack(pop)

enum ScriptTypes
{
	SCRIPT_SYSTEM = 0,
	SCRIPT_SPATIAL = 1,
	SCRIPT_TIME = 2,
	SCRIPT_ITEM = 3,
	SCRIPT_CRITTER = 4,
};

/*   24 */
enum ObjectTypes
{
	OBJ_TYPE_ITEM = 0,
	OBJ_TYPE_CRITTER = 1,
	OBJ_TYPE_SCENERY = 2,
	OBJ_TYPE_WALL = 3,
	OBJ_TYPE_TILE = 4,
	OBJ_TYPE_MISC = 5,
	OBJ_TYPE_SPATIAL = 6,
};

/*   25 */
#pragma pack(push, 1)
struct TProgram
{
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

// structures for holding frms loaded with fallout2 functions
#pragma pack(push, 2)
typedef class FrmSubframeData {
public:
	WORD width;
	WORD height;
	DWORD size;
	WORD x;
	WORD y;
} FrmSubframeData;

typedef class FrmFrameData {
public:
	DWORD version; // version num
	WORD fps; // frames per sec
	WORD actionFrame;
	WORD numFrames; // number of frames per direction
	WORD xCentreShift[6]; // offset from frm centre +=right -=left
	WORD yCentreShift[6]; // offset from frm centre +=down -=up
	DWORD oriOffset[6]; // frame area offset for diff orientations
	DWORD frameAreaSize;
} FrmFrameData;
#pragma pack(pop)

#pragma pack(push, 1)
struct Queue
{
	DWORD time;
	long type;
	TGameObj* object;
	long data;
	Queue* next;
};
#pragma pack(pop)
