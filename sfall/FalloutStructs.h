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
struct TInvenRec {
	TGameObj *object;
	long count;
};
#pragma pack(pop)

/* 15 */
#pragma pack(push, 1)
struct TGameObj {
	long ID;
	long tile;
	long x;
	long y;
	long sx;
	long sy;
	long currentFrm;
	long rotation;
	long artFid;
	long flags;
	long elevation;
	long invenCount;
	long invenMax;
	TInvenRec *invenTablePtr;
	char gap_38[4];
	long itemCharges;
	long critterAP_weaponAmmoPid;
	char gap_44[16];
	long whoHitMe;
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
struct TComputeAttack {
	TGameObj* attacker;
	long hitMode;
	TGameObj* weapon;
	long field_C;
	long attackerDamage;
	long attackerFlags;
	long numRounds;
	long message;
	TGameObj* target;
	long targetTile;
	long bodyPart;
	long targetDamage;
	long targetFlags;
	long knockbackValue;
	TGameObj* mainTarget;
	long numExtras;
	TGameObj* extraTarget[6];
	long extraBodyPart[6];
	long extraDamage[6];
	long extraFlags[6];
	long extraKnockbackValue[6];
};
#pragma pack(pop)

/*   22 */
#pragma pack(push, 1)
struct TScript {
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
struct TProgram {
	const char* fileName;
	long *codeStackPtr;
	long gap_8;
	long gap_9;
	long *codePtr;
	long field_14;
	long gap_18;
	long *dStackPtr;
	long *aStackPtr;
	long *dStackOffs;
	long *aStackOffs;
	long gap_2C;
	long *stringRefPtr;
	long gap_34;
	long *procTablePtr;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Art {
	long flags;
	char path[16];
	char* names;
	long d18;
	long total;
};
#pragma pack(pop)

// Bounding rectangle, used by tile_refresh_rect and related functions
#pragma pack(push, 1)
struct BoundRect {
	long x;
	long y;
	long offx;
	long offy;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FrmFile {
	long id;				//0x00
	short fps;				//0x04
	short actionFrame;		//0x06
	short frames;			//0x08
	short xshift[6];		//0x0a
	short yshift[6];		//0x16
	long oriFrameOffset[6];	//0x22
	long frameAreaSize;		//0x3a
	short width;			//0x3e
	short height;			//0x40
	long frameSize;			//0x42
	short xoffset;			//0x46
	short yoffset;			//0x48
	BYTE pixels[80 * 36];	//0x4a
};
#pragma pack(pop)

// structures for holding frms loaded with fallout2 functions
#pragma pack(push, 1)
typedef class FrmFrameData { // sizeof 12 + 1 byte
public:
	WORD width;
	WORD height;
	DWORD size;   // width * height
	WORD x;
	WORD y;
	BYTE data[1]; // begin frame data
} FrmFrameData;
#pragma pack(pop)

#pragma pack(push, 2)
typedef class FrmHeaderData { // sizeof 62
public:
	DWORD version;        // version num
	WORD fps;             // frames per sec
	WORD actionFrame;
	WORD numFrames;       // number of frames per direction
	WORD xCentreShift[6]; // shift in the X direction, of frames with orientations [0-5]
	WORD yCentreShift[6]; // shift in the Y direction, of frames with orientations [0-5]
	DWORD oriOffset[6];   // offset of first frame for direction [0-5] from begining of frame area
	DWORD frameAreaSize;  // size of all frames area
} FrmHeaderData;
#pragma pack(pop)

//for holding window info
#pragma pack(push, 1)
struct WINinfo {
	long wID;
	long flags;
	RECT wRect;
	long width;
	long height;
	long clearColour;
	long rand1;
	long rand2;
	BYTE *surface; // bytes frame data ref to palette
	long *buttonsList;
	long unknown5; // buttonptr?
	long unknown6;
	long *menuBar;
	long *drawFunc;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Queue {
	DWORD time;
	long type;
	TGameObj* object;
	long data;
	Queue* next;
};
#pragma pack(pop)
