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
#include <dsound.h>

#include "Enums.h"

namespace sfall
{

struct Rectangle {
	long x, y, width, height;

	long right() { return x + (width - 1); }
	long bottom() { return y + (height - 1); }
};

}

namespace fo
{

/******************************************************************************/
/* FALLOUT2.EXE structs should be placed here  */
/******************************************************************************/

#pragma pack(push, 1)

// TODO: make consistent naming for all FO structs

struct GameObject;
struct Program;
struct ScriptInstance;

struct Art {
	long flags;
	char path[16];
	char* names;
	long d18;
	long total;
};

struct AnimationSet {
	long currentAnim;
	long counter;
	long totalAnimCount;
	long flags;

	struct Animation {
		long animType;
		GameObject* source;
		GameObject* target;
		long data1;
		long elevation;
		long animCode;
		long delay;
		long(__fastcall *callFunc)(DWORD, DWORD);
		long(__fastcall *callFunc3)(DWORD, DWORD);
		long flags;
		long data2;
		long frmPtr;
	} animations[55];
};

static_assert(sizeof(AnimationSet) == 2656, "Incorrect AnimationSet definition.");

struct AnimationSad {
	long flags;
	GameObject* source;
	long fid;
	long animCode;
	long ticks;
	long tpf;       // fps
	long animSetSlot;
	long pathCount; // len
	long animStep;  // current step in rotationData/pathData
	short dstTile;
	char rotation1;
	char rotation2;

	union {
		long rotationData[800];
		struct BuildPathData {
			long tile;
			long elevation;
			long sX;
			long sY;
		} pathData[200];
	};
};

static_assert(sizeof(AnimationSad) == 3240, "Incorrect AnimationSad definition.");

// Bounding rectangle, used by tile_refresh_rect and related functions.
struct BoundRect {
	long x;
	long y;
	long offx; // right
	long offy; // bottom

	BoundRect() {};

	BoundRect(sfall::Rectangle rect) {
		x = rect.x;
		y = rect.y;
		offx = rect.right();
		offy = rect.bottom();
	}

	BoundRect(RECT* rect) {
		x = rect->left;
		y = rect->top;
		offx = rect->right;
		offy = rect->bottom;
	}
};

struct RectList {
	union {
		BoundRect rect;
		RECT wRect;
	};
	RectList* nextRect;
};

// Game objects (items, critters, etc.), including those stored in inventories.
struct GameObject {
	long id;
	long tile;
	long x;
	long y;
	long sx;
	long sy;
	long frm; // current frame
	long rotation;
	long artFid;
	long flags;
	long elevation;
	long invenSize;
	long invenMax;
	struct InvenItem {
		GameObject *object;
		long count;
	} *invenTable;

	union {
		struct {
			long updatedFlags;
			// for weapons - ammo in magazine, for ammo - amount of ammo in last ammo pack
			long charges;
			// current type of ammo loaded in magazine
			long ammoPid;
			long unused[8]; // offset 0x44
		} item;
		struct {
			long reaction; // unused?
			// 1 - combat, 2 - enemies out of sight, 4 - running away
			long combatState;
			// aka action points
			long movePoints;
			long damageFlags;
			long damageLastTurn;
			long aiPacket;
			long teamNum;
			GameObject* whoHitMe;
			long health;
			long rads;
			long poison;

			inline bool IsDead() {
				return ((damageFlags & DamageFlag::DAM_DEAD) != 0);
			}
			inline bool IsNotDead() {
				return ((damageFlags & DamageFlag::DAM_DEAD) == 0);
			}
			inline bool IsActive() {
				return ((damageFlags & (DamageFlag::DAM_KNOCKED_OUT | DamageFlag::DAM_LOSE_TURN)) == 0);
			}
			inline bool IsNotActive() {
				return ((damageFlags & (DamageFlag::DAM_KNOCKED_OUT | DamageFlag::DAM_LOSE_TURN)) != 0);
			}
			inline bool IsActiveNotDead() {
				return ((damageFlags & (DamageFlag::DAM_DEAD | DamageFlag::DAM_KNOCKED_OUT | DamageFlag::DAM_LOSE_TURN)) == 0);
			}
			inline bool IsNotActiveOrDead() {
				return ((damageFlags & (DamageFlag::DAM_DEAD | DamageFlag::DAM_KNOCKED_OUT | DamageFlag::DAM_LOSE_TURN)) != 0);
			}
			inline bool IsFleeing() {
				return ((combatState & CombatStateFlag::InFlee) != 0);
			}

			// Gets the current target or the attacker who dealt damage in the previous combat turn
			inline GameObject* getHitTarget() {
				return whoHitMe;
			}
			inline long getAP() {
				return movePoints;
			}
		} critter;
	};
	DWORD protoId; // object PID
	long cid; // combat ID (don't change while in combat)
	long lightDistance;
	long lightIntensity;
	DWORD outline;
	long scriptId; // SID 0x0Y00XXXX: Y - type: 0=s_system, 1=s_spatial, 2=s_time, 3=s_item, 4=s_critter; XXXX - index in scripts.lst; 0xFFFFFFFF no attached script
	GameObject* owner;
	long scriptIndex;

	inline char Type() {
		return (protoId >> 24);
	}
	inline char TypeFid() {
		return ((artFid >> 24) & 0x0F);
	}

	inline bool IsCritter() {
		return (Type() == fo::ObjType::OBJ_TYPE_CRITTER);
	}
	inline bool IsNotCritter() {
		return (Type() != fo::ObjType::OBJ_TYPE_CRITTER);
	}
	inline bool IsItem() {
		return (Type() == fo::ObjType::OBJ_TYPE_ITEM);
	}
	inline bool IsNotItem() {
		return (Type() != fo::ObjType::OBJ_TYPE_ITEM);
	}
};

// Results of compute_attack_() function.
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
	long targetDamage;
	long targetFlags;
	long knockbackValue;
	GameObject* mainTarget;
	long numExtras;
	GameObject* extraTarget[6];
	long extraBodyPart[6];
	long extraDamage[6];
	long extraFlags[6];
	long extraKnockbackValue[6];
};

struct CombatGcsd {
	GameObject* source;
	GameObject* target;
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
struct ScriptInstance {
	long id;
	long next;
	long elevationAndTile; // first 3 bits - elevation, rest - tile number
	long spatialRadius;
	long flags;
	long scriptIdx;
	Program *program;
	long ownerObjectId;
	long localVarOffset; // data
	long numLocalVars;
	long returnValue;
	long action;
	long fixedParam;
	GameObject *selfObject;
	GameObject *sourceObject;
	GameObject *targetObject;
	long actionNum;
	long scriptOverrides;
	long field_48;
	long howMuch;
	long field_50;
	long procedureTable[28];
	long gap[7];
};

// Script run-time data
struct Program {
	const char* fileName; // path and file name of the script "scripts\*.int"
	long *codeStackPtr;
	long field_8;
	long field_C;
	long codePosition;  // position in the code stack when reading script opcodes
	long field_14;      // unused?
	long field_18;      // unused?
	long *dStackPtr;
	long *aStackPtr;
	long *dStackOffs;
	long *aStackOffs;
	long field_2C;
	long *stringRefPtr;
	long *procTablePtr;
	long *codeStack;    // same as codeStackPtr
	long savedEnv[12];  // saved register values
	long field_6C;      // unused?
	long field_70;      // unused?
	long field_74;      // unused?
	long timerTick;     // unused?
	long func_7C;       // always null?
	short flags;
	short fetchOpcode;
	long currentScriptWin; // current window for executing script
	long shouldRemove;
};

static_assert(sizeof(Program) == 140, "Incorrect Program definition.");

struct ProgramList {
	Program* progPtr;
	ProgramList* next;
	ProgramList* prev;
};

struct ItemButtonItem {
	GameObject* item;
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

struct ElevatorExit {
	long id;
	long elevation;
	long tile;
};

struct ElevatorFrms {
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

	FrmFile() {};

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
struct UnlistedFrm {
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

	UnlistedFrm() {
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
	~UnlistedFrm() {
		if (frames != nullptr)
			delete[] frames;
	}
};

//for holding a message
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

struct StatInfo {
	const char* name;
	const char* description;
	long image;
	long minValue;
	long maxValue;
	long defaultValue;
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

struct ObjectTable {
	GameObject*  object;
	ObjectTable* nextObject;
};

struct PremadeChar {
	char path[20];
	DWORD fid;
	char unknown[20];
};

// In-memory PROTO structure, not the same as PRO file format.
struct Proto {
	struct Tile {
		long scriptId;
		Material material;
	};

	struct Item {
		struct Weapon {
			long animationCode;
			long minDamage;
			long maxDamage;
			long damageType;
			long maxRange[2];
			long projectilePid;
			long minStrength;
			long movePointCost[2];
			long critFailTable;
			long perk;
			long burstRounds;
			long caliber;
			long ammoPid;
			long maxAmmo;
			// shot sound ID
			long soundId;
			long gap_68;
		};

		struct Ammo {
			long caliber;
			long packSize;
			long acAdjust;
			long drAdjust;
			long damageMult;
			long damageDiv;
			char gap_3c[48];
		};

		struct Armor {
			long armorClass;
			// for each DamageType
			long damageResistance[7];
			// for each DamageType
			long damageThreshold[7];
			long perk;
			long maleFID;
			long femaleFID;
		};

		struct Container {
			// container size capacity (not weight)
			long maxSize;
			// 1 - has use animation, 0 - no animation
			long openFlags;
		};

		struct Drug {
			long stats[3];
			long immediateEffect[3];
			struct DelayedEffect {
				// delay for the effect
				long duration;
				// effect amount for each stat
				long effect[3];
			} delayed[2];
			long addictionRate;
			long addictionEffect;
			long addictionOnset;
			char gap_68[4];
		};

		struct Misc {
			long powerPid;
			long powerType;
			long maxCharges;
		};

		struct Key {
			long keyCode;
		};

		long flags;
		long flagsExt;
		long scriptId; // SID 0x0Y00XXXX: Y - type: 0=s_system, 1=s_spatial, 2=s_time, 3=s_item, 4=s_critter; XXXX - index in scripts.lst; 0xFFFFFFFF no attached script
		ItemType type;

		union {
			Weapon weapon;
			Ammo ammo;
			Armor armor;
			Container container;
			Drug drug;
			Misc misc;
			Key key;
		};
		Material material; // should be at 0x6C
		long size;
		long weight;
		long cost;
		long inventoryFid;
		BYTE soundId;
	};

	struct Critter {
		struct Stats {
			long strength;
			long perception;
			long endurance;
			long charisma;
			long intelligence;
			long agility;
			long luck;
			long health;
			// max move points (action points)
			long movePoints;
			long armorClass;
			// not used by engine
			long unarmedDamage;
			long meleeDamage;
			long carryWeight;
			long sequence;
			long healingRate;
			long criticalChance;
			long betterCriticals;
			// for each DamageType
			long damageThreshold[7];
			// for each DamageType
			long damageResistance[7];
			long radiationResistance;
			long poisonResistance;
			long age;
			long gender;
		};

		long flags;
		long flagsExt;
		long scriptId;
		long critterFlags;

		Stats base;
		Stats bonus;

		long skills[SKILL_count];

		long bodyType;
		long experience;
		long killType;
		long damageType;
		long headFid;
		long aiPacket;
		long teamNum;
	};

	struct Scenery {
		struct Door {
			long openFlags;
			long keyCode;
		};
		struct Stairs {
			long elevationAndTile;
			long mapId;
		};
		struct Elevator {
			long id;
			long level;
		};

		long flags;
		long flagsExt;
		long scriptId;
		ScenerySubType type;
		union {
			Door door;
			Stairs stairs;
			Elevator elevator;
		};
		Material material;
		char gap_30[4];
		BYTE soundId;
	};

	struct Wall {
		long flags;
		long flagsExt;
		long scriptId;
		Material material;
	};

	struct Misc {
		long flags;
		long flagsExt;
	};

	long pid;
	long messageNum;
	long fid;
	// range 0-8 in hexes
	long lightDistance;
	// range 0 - 65536
	long lightIntensity;
	union {
		Tile tile;
		Item item;
		Critter critter;
		Scenery scenery;
		Wall wall;
		Misc misc;
	};
};

static_assert(offsetof(Proto, item) + offsetof(Proto::Item, material) == 0x6C, "Incorrect Proto definition.");

struct ScriptListInfoItem {
	char fileName[16];
	long numLocalVars;
};

struct WinRegion { // sizeof = 0x88 (0x8C in the engine code)
	char  name[32];
	long  field_20;
	long  field_24;
	long  field_28;
	long  field_2C;
	long  field_30;
	long  field_34;
	long  field_38;
	long  field_3C;
	long  field_40;
	Program* procScript;
	long  proc_48;
	long  proc_4C;
	long  procEnter;
	long  procLeave;
	long  field_58;
	long  field_5C;
	long  field_60;
	long  field_64;
	long  flags_68;
	long  field_6C;
	long  field_70;
	long  field_74;
	void* func_78;
	void* func_7C;
	long  field_80;
	long  field_84;
};

//for holding window info
struct Window {
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
	char  name[32];
	long  wID; // window position in the _window_index array
	long  width;
	long  height;
	WinRegion* regions;
	long  region2;
	long  countRegions;
	long  region4;
	long* buttons;
	long  numButtons;
	long  setPositionX;
	long  setPositionY;
	long  clearColour;
	long  flags;
	float randX;
	float randY;
};

struct LSData {
	char  signature[24];
	short majorVer;
	short minorVer;
	char  charR;
	char  playerName[32];
	char  comment[30];
	char  unused1;
	short realMonth;
	short realDay;
	short realYear;
	short unused2;
	long  realTime;
	short gameMonth;
	short gameDay;
	short gameYear;
	short unused3;
	long  gameTime;
	short mapElev;
	short mapNumber;
	char  mapName[16];
};

struct AIcap {
	long name;
	long packet_num;
	long max_dist;
	long min_to_hit;
	long min_hp;
	long aggression;
	long hurt_too_much;
	long secondary_freq;
	long called_freq;
	long font;
	long color;
	long outline_color;
	long chance;
	long combat_message_data[24];
	long area_attack_mode;
	long run_away_mode;
	long best_weapon;
	long distance;
	long attack_who;
	long chem_use;
	long chem_primary_desire;
	long chem_primary_desire1;
	long chem_primary_desire2;
	long disposition;
	long body_type;
	long general_type;

	inline AIpref::distance getDistance() {
		return (AIpref::distance)distance;
	}
	inline AIpref::run_away_mode getRunAwayMode() {
		return (AIpref::run_away_mode)run_away_mode;
	}

};

struct Queue {
	DWORD time;
	long  type;
	GameObject* object;
	DWORD* data;
	Queue* next;
};

struct QueueRadiation {
	long level;
	long init; // 1 - for removing effect
};

struct QueueDrug {
	DWORD pid;
	fo::Stat stat0;
	fo::Stat stat1;
	fo::Stat stat2;
	long amount0;
	long amount1;
	long amount2;
};

struct QueueAddict {
	long  init;      // 1 - perk is not active yet
	DWORD drugPid;
	fo::Perk perkId; // effect of addiction
};

struct DrugInfoList {
	DWORD itemPid;
	long  addictGvar;
	long  numEffects;
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
	IDirectSoundBuffer* soundBuffer;
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
	long  position;
};

// aka PartyMemberRecoveryList
struct ObjectListData {
	GameObject* object;
	fo::ScriptInstance* script;
	long* localVarData;
	ObjectListData* nextSaveList; // _itemSaveListHead
};

struct PartyMemberPerkListData {
	long perkData[fo::Perk::PERK_count];
};

struct QuestData {
	long location;
	long description;
	long gvarIndex;
	long displayThreshold;
	long completedThreshold;
};

#pragma pack(pop)

}
