/*
* sfall
* Copyright (C) 2008-2015 The sfall team
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
/* FALLOUT2.EXE structs should be placed here  */
/******************************************************************************/


struct TGameObj;

/*   26 */
#pragma pack(push, 1)
struct TInvenRec
{
	TGameObj *object;
	int count;
};
#pragma pack(pop)

/* 15 */
#pragma pack(push, 1)
struct TGameObj
{
	int ID;
	int tile;
	char gap_8[2];
	char field_A;
	char gap_B[17];
	int rotation;
	int artFID;
	char gap_24[4];
	int elevation;
	int invenCount;
	int field_30;
	TInvenRec *invenTablePtr;
	char gap_38[4];
	int itemCharges;
	int critterAP_weaponAmmoPid;
	char gap_44[16];
	int lastTarget;
	char gap_58[12];
	int pid;
	char gap_68[16];
	int scriptID;
	char gap_7C[4];
	int script_index;
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
	int damageAttacker;
	int flagsAttacker;
	int rounds;
	char gap_1C[4];
	TGameObj *target;
	int targetTile;
	int bodyPart;
	int damageTarget;
	int flagsTarget;
	int knockbackValue;
};
#pragma pack(pop)


/*   22 */
#pragma pack(push, 1)
struct TScript
{
	int script_id;
	char gap_4[4];
	int elevation_and_tile;
	int spatial_radius;
	char gap_10[4];
	int script_index;
	int program_ptr;
	int self_obj_id;
	char gap_20[8];
	int scr_return;
	char gap_2C[4];
	int fixed_param;
	TGameObj *self_obj;
	TGameObj *source_obj;
	TGameObj *target_obj;
	int script_overrides;
	char field_44;
	char gap_45[15];
	int procedure_table[28];
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
	int *codeStackPtr;
	char gap_8[8];
	int *codePtr;
	int field_14;
	char gap_18[4];
	int *dStackPtr;
	int *aStackPtr;
	int *dStackOffs;
	int *aStackOffs;
	char gap_2C[4];
	int *stringRefPtr;
	char gap_34[4];
	int *procTablePtr;
};
#pragma pack(pop)