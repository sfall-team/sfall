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

struct sRectangle {
	long x, y, width, height;

	long right() { return x + (width - 1); }
	long bottom() { return y + (height - 1); }
};

/******************************************************************************/
/* FALLOUT2.EXE structs should be placed here  */
/******************************************************************************/

#pragma pack(push, 1)

struct TGameObj;
struct TProgram;
struct TScript;

struct sArt {
	long flags;
	char path[16];
	char* names;
	long d18;
	long total;
};

// Bounding rectangle, used by tile_refresh_rect and related functions.
struct BoundRect {
	long x;
	long y;
	long offx; // right
	long offy; // bottom
};

struct RectList {
	union {
		BoundRect rect;
		RECT wRect;
	};
	RectList* nextRect;
};

// Game objects (items, critters, etc.), including those stored in inventories.
struct TGameObj {
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
	struct TInvenRec {
		TGameObj *object;
		long count;
	} *invenTable;

	union {
		struct {
			char updatedFlags[4];
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
			TGameObj* whoHitMe;
			long health;
			long rads;
			long poison;
		} critter;
	};
	DWORD protoId;
	long cid;
	long lightDistance;
	long lightIntensity;
	DWORD outline;
	long scriptId;
	TGameObj* owner;
	long scriptIndex;

	inline char Type() {
		return (protoId >> 24);
	}
	inline char TypeFid() {
		return ((artFid >> 24) & 0x0F);
	}
};

// Results of compute_attack_() function.
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

struct CombatGcsd {
	TGameObj* source;
	TGameObj* target;
	long freeAP;
	long bonusToHit;
	long bonusDamage;
	long minDamage;
	long maxDamage;
	long changeFlags;
	DWORD flagsSource;
	DWORD flagsTarget;
};

// Script instance attached to an object or tile (spatial script).
struct TScript {
	long id;
	long next;
	long elevationAndTile; // first 3 bits - elevation, rest - tile number
	long spatialRadius;
	long flags;
	long scriptIdx;
	TProgram *program;
	long selfObjectId;
	long localVarOffset;
	long numLocalVars;
	long returnValue;
	long action;
	long fixedParam;
	TGameObj *selfObject;
	TGameObj *sourceObject;
	TGameObj *targetObject;
	long actionNum;
	long scriptOverrides;
	long field_48;
	long howMuch;
	long field_50;
	long procedureTable[28];
};

// Script run-time data
struct TProgram {
	const char* fileName; // path and file name of the script "scripts\*.int"
	long *codeStackPtr;
	long field_8;
	long field_C;
	long *codePtr;
	long field_14;
	long field_18;
	long *dStackPtr;
	long *aStackPtr;
	long *dStackOffs;
	long *aStackOffs;
	long field_2C;
	long *stringRefPtr;
	long field_34;      // procTablePtr
	long *procTablePtr; // field_38
	long regs[12];
	long field_6C;
	long field_70;
	long field_74;
	long field_78;
	long field_7C;
	union {
		long flags;
		struct {
			char flags1;
			char flags2;
			char flags3;
			char flags4;
		};
	};
	long currentScriptWin; // current window for executing script
	long field_88;
};

static_assert(sizeof(TProgram) == 140, "Incorrect TProgram definition.");

struct ItemButtonItem {
	TGameObj* item;
	union {
		long flags;
		struct {
			char cantUse;
			char itsWeapon;
			short unkFlag;
		};
	};
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

struct sElevatorExit {
	long id;
	long elevation;
	long tile;
};

struct sElevatorFrms {
	DWORD main;
	DWORD buttons;
};

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

// structures for holding frms loaded with fallout2 functions
typedef class FrmFrameData { // sizeof 12 + 1 byte
public:
	WORD width;
	WORD height;
	DWORD size;   // width * height
	WORD x;
	WORD y;
	BYTE data[1]; // begin frame image data
} FrmFrameData;

struct FrmFile {            // sizeof 2954
	long id;                // 0x00
	short fps;              // 0x04
	short actionFrame;      // 0x06
	short frames;           // 0x08
	short xshift[6];        // 0x0A
	short yshift[6];        // 0x16
	long oriFrameOffset[6]; // 0x22
	long frameAreaSize;     // 0x3A
	union {
		FrmFrameData* frameData;
		struct {
			short width;    // 0x3E
			short height;   // 0x40
		};
	};
	long frameSize;         // 0x42
	short xoffset;          // 0x46
	short yoffset;          // 0x48
	union {                 // 0x4A
		BYTE *pixelData;
		BYTE pixels[80 * 36]; // for tiles FRM
	};

	// Returns a pointer to the data of the frame in the direction
	FrmFrameData* GetFrameData(long dir, long frame) {
		BYTE* offsDirectionFrame = (BYTE*)&frameData;
		if (dir > 0 && dir < 6) {
			offsDirectionFrame += oriFrameOffset[dir];
		}
		if (frame > 0) {
			int maxFrames = frames - 1;
			if (frame > maxFrames) frame = maxFrames;
			while (frame-- > 0) {
				offsDirectionFrame += ((FrmFrameData*)offsDirectionFrame)->size + (sizeof(FrmFrameData) - 1);
			}
		}
		return (FrmFrameData*)offsDirectionFrame;
	}
};

static_assert(sizeof(FrmFile) == 2954, "Incorrect FrmFile definition.");

// structures for loading unlisted frms
struct UNLSTDfrm {
	DWORD version;
	WORD FPS;
	WORD actionFrame;
	WORD numFrames;
	WORD xCentreShift[6];
	WORD yCentreShift[6];
	DWORD oriOffset[6];
	DWORD frameAreaSize;

	struct Frame {
		WORD width;
		WORD height;
		DWORD size;
		WORD x;
		WORD y;
		BYTE *indexBuff;

		Frame() {
			width = 0;
			height = 0;
			size = 0;
			x = 0;
			y = 0;
			indexBuff = nullptr;
		}
		~Frame() {
			if (indexBuff != nullptr)
				delete[] indexBuff;
		}
	} *frames;

	UNLSTDfrm() {
		version = 0;
		FPS = 0;
		actionFrame = 0;
		numFrames = 0;
		for (int i = 0; i < 6; i++) {
			xCentreShift[i] = 0;
			yCentreShift[i] = 0;
			oriOffset[i] = 0;
		}
		frameAreaSize = 0;
		frames = nullptr;
	}
	~UNLSTDfrm() {
		if (frames != nullptr)
			delete[] frames;
	}
};

//for holding a message
struct MSGNode {
	long number;
	long flags;
	char* audio;
	char* message;

	MSGNode() {
		number = 0;
		flags = 0;
		audio = nullptr;
		message = nullptr;
	}
};

//for holding msg array
typedef struct MSGList {
	long numMsgs;
	MSGNode *nodes;

	MSGList() {
		nodes = nullptr;
		numMsgs = 0;
	}
} MSGList;

struct CritInfo {
	union {
		struct {
			// This is divided by 2, so a value of 3 does 1.5x damage, and 8 does 4x damage.
			long damageMult;
			// This is a flag bit field (DAM_*) controlling what effects the critical causes.
			long effectFlags;
			// This makes a check against a (SPECIAL) stat. Values of 2 (endurance), 5 (agility), and 6 (luck) are used, but other stats will probably work as well. A value of -1 indicates that no check is to be made.
			long statCheck;
			// Affects the outcome of the stat check, if one is made. Positive values make it easier to pass the check, and negative ones make it harder.
			long statMod;
			// Another bit field, using the same values as EffectFlags. If the stat check is failed, these are applied in addition to the earlier ones.
			long failureEffect;
			// The message to show when this critical occurs, taken from combat.msg .
			long message;
			// Shown instead of Message if the stat check is failed.
			long failMessage;
		};
		long values[7];
	};
};

struct SkillInfo {
	const char* name;
	const char* description;
	long attr;
	long image;
	long base;
	long statMulti;
	long statA;
	long statB;
	long skillPointMulti;
	// Default experience for using the skill: 25 for Lockpick, Steal, Traps, and First Aid, 50 for Doctor, and 100 for Outdoorsman.
	long experience;
	// 1 for Lockpick, Steal, Traps; 0 otherwise
	long f;
};

struct TraitInfo {
	const char* name;
	const char* description;
	long image;
};

//fallout2 path node structure
struct PathNode {
	char* path;
	void* pDat;
	long isDat;
	PathNode* next;
};

struct PremadeChar {
	char path[20];
	DWORD fid;
	char unknown[20];
};

struct ScriptListInfoItem {
	char fileName[16];
	long numLocalVars;
};

//for holding window info
struct WINinfo {
	long wID; // window position in the _window_index array
	long flags;
	union {
		RECT wRect;
		BoundRect rect;
	};
	long  width;
	long  height;
	long  clearColour;
	long  randX;   // not used by engine
	long* randY;   // used by sfall for additional surfaces
	BYTE* surface; // bytes frame data ref to palette
	long* buttonsList;
	long  buttonT1; // buttonptr?
	long  buttonT2;
	long* menuBar;
	void  (__cdecl *drawFunc)(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width); // trans_buf_to_buf_
};

struct sWindow {
	char name[32];
	long wID; // window position in the _window_index array
	long width;
	long height;
	long region1;
	long region2;
	long region3;
	long region4;
	long *buttons;
	long numButtons;
	long setPositionX;
	long setPositionY;
	long clearColour;
	long flags;
	float randX;
	float randY;
};

struct LSData {
	char signature[24];
	short majorVer;
	short minorVer;
	char charR;
	char playerName[32];
	char comment[30];
	char unused1;
	short realMonth;
	short realDay;
	short realYear;
	short unused2;
	long realTime;
	short gameMonth;
	short gameDay;
	short gameYear;
	short unused3;
	long gameTime;
	short mapElev;
	short mapNumber;
	char mapName[16];
};

struct Queue {
	DWORD time;
	long  type;
	TGameObj* object;
	DWORD* data;
	Queue* next;
};

struct QueueRadiation {
	long level;
	long init; // 1 - for removing effect
};

struct FloatText {
	long  flags;
	void* unknown0;
	long  unknown1;
	long  unknown2;
	long  unknown3;
	long  unknown4;
	long  unknown5;
	long  unknown6;
	long  unknown7;
	long  unknown8;
	long  unknown9;
	void* unknown10;
};

struct SubTitleList {
	long  text;
	long  frame;
	long* next;
};

struct ACMSoundData {
	void* OpenFunc;
	void* CloseFunc;
	void* ReadFunc;
	void* WriteFunc;
	void* SeekFunc;
	void* TellFunc;
	void* FileSizeFunc;
	long  openAudioIndex;
	long  memData;
	long  soundBuffer;
	long  dwSize;              // begin DSBUFFERDESC structure
	long  dwFlags;
	long  dwBufferBytes;
	long  dwReserved;
	WAVEFORMATEX* lpwfxFormat; // end DSBUFFERDESC structure
	long  soundMode;
	long  state;
	long  mode;
	long  lastPosition;
	long  volume;
	long  field_50;
	long  field_54;
	long  field_58;
	long  field_5C;
	long  fileSize;
	long  field_64;
	long  field_68;
	long  readLimit;
	long  field_70;
	long  field_74;
	long  numBuffers;
	long  dataSize;
	long  field_80;
	long  soundTag;
	void* CallBackFunc;
	long  field_8C;
	long  field_90;
	void* managerList;
	ACMSoundData* self;
};

struct AudioDecode {
	void* ReadFunc;
	void* openfile_data;
	void* read_data;
	long  read_data_size;
	long  field_10;
	long  countReadBytes;
	long  signature;
	long  count;
	long  field_20;
	long  field_24;
	long  field_28;
	long  field_2C;
	long  field_30;
	long  data;
	long  field_38;
	long  field_3C;
	long  out_Channels;
	long  out_SampleRate;
	long  out_Length;
	long  field_4C;
	long  field_50;
};

struct AudioFile {
	long  flags;
	void* open_file;
	AudioDecode* decoderData;
	long  length;
	long  sample_rate;
	long  channels;
	long  tell;
};

#pragma pack(pop)
