#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Objects.h"

namespace sfall
{

static int unjamTimeState;
static int maxCountLoadProto = 512;

long Objects::uniqueID = UniqueID::Start; // current counter id, saving to sfallgv.sav

// Assigns a new unique identifier to an object if it has not been previously assigned
// the identifier is saved with the object in the saved game and this can used in various script
// player ID = 18000, all party members have ID = 18000 + its pid (file number of prototype)
long __fastcall Objects::SetObjectUniqueID(fo::GameObject* obj) {
	long id = obj->id;
	if (id > UniqueID::Start || (id >= PLAYER_ID && id < 83536)) return id; // 65535 maximum possible number of prototypes

	if ((DWORD)uniqueID >= (DWORD)UniqueID::End) uniqueID = UniqueID::Start;
	obj->id = ++uniqueID;
	return uniqueID;
}

// Assigns a unique ID in the negative range (0xFFFFFFF6 - 0x8FFFFFF7)
long __fastcall Objects::SetSpecialID(fo::GameObject* obj) {
	long id = obj->id;
	if (id <= -10 || id > UniqueID::Start) return id;

	if ((DWORD)uniqueID >= (DWORD)UniqueID::End) uniqueID = UniqueID::Start;
	id = -9 - (++uniqueID - UniqueID::Start);
	obj->id = id;
	return id;
}

void Objects::SetNewEngineID(fo::GameObject* obj) {
	if (obj->id > UniqueID::Start) return;
	obj->id = fo::func::new_obj_id();
}

static void __declspec(naked) item_identical_hack() {
	using namespace fo::Fields;
	__asm {
		mov  ecx, [edi]; // item id
		cmp  ecx, Start; // start unique ID
		jg   notIdentical;
		mov  eax, [esi + scriptId];
		cmp  eax, ebx;
notIdentical:
		retn; // if ZF == 0 then item is not identical
	}
}

static void __declspec(naked) new_obj_id_hook() {
	__asm {
		mov  eax, 83535;
		cmp  dword ptr ds:[FO_VAR_cur_id], eax;
		jle  pickNewID;
		retn;
pickNewID: // skip PM range (18000 - 83535)
		mov  ds:[FO_VAR_cur_id], eax;
		jmp  fo::funcoffs::new_obj_id_;
	}
}

// Reassignment of ID values to all critters upon first loading the map
static void map_fix_critter_id() {
	long npcStartID = 4096;
	fo::GameObject* obj = fo::func::obj_find_first();
	while (obj) {
		if (obj->Type() == fo::OBJ_TYPE_CRITTER && obj->id < PLAYER_ID) {
			obj->id = npcStartID++;
		}
		obj = fo::func::obj_find_next();
	}
}

static void __declspec(naked) map_load_file_hack() {
	__asm {
		jz   mapVirgin;
		retn;
mapVirgin:
		call fo::funcoffs::wmMapIsSaveable_;
		test eax, eax;
		jnz  savedable;
		retn;
savedable:
		call map_fix_critter_id;
		xor  eax, eax; // set ZF
		retn;
	}
}

static void __declspec(naked) queue_add_hack() {
	using namespace fo;
	using namespace Fields;
	__asm {
		mov  [edx + 8], edi; // queue.object
		mov  [edx], esi;     // queue.time
		//
		test edi, edi;
		jnz  fix;
		retn;
fix:
		mov  eax, [edi + protoId];
		and  eax, 0x0F000000;
		jnz  notItem; // object it's not item?
		push ecx;
		push edx;
		mov  ecx, edi;
		call Objects::SetSpecialID;
		pop  edx;
		pop  ecx;
		retn;
notItem:
		cmp  ecx, script_timer_event; // QueueType
		je   end;
		cmp  eax, OBJ_TYPE_CRITTER << 24;
		jne  end;
		push ecx;
		push edx;
		mov  ecx, edi;
		call Objects::SetObjectUniqueID;
		pop  edx;
		pop  ecx;
end:
		xor  edi, edi; // fix don't set 'Used' flag for critter object
		retn;
	}
}

void Objects::SetAutoUnjamLockTime(DWORD time) {
	if (!unjamTimeState) BlockCall(0x4A364A); // disable auto unjam at midnight

	if (time > 0) {
		SafeWrite8(0x4831D9, (BYTE)time);
		if (unjamTimeState == 2) {
			SafeWrite8(0x4831DA, 0x7C);
		}
		unjamTimeState = 1;
	} else {
		SafeWrite8(0x4831DA, 0xEB); // disable auto unjam
		unjamTimeState = 2;
	}
}

void RestoreObjUnjamAllLocks() {
	if (unjamTimeState) {
		SafeWrite8(0x4A364A, 0xE8);
		SafeWrite32(0x4A364B, 0xFFFF9E69);
		SafeWrite8(0x4831DA, 0x7C);
		SafeWrite8(0x4831D9, 24);
		unjamTimeState = 0;
	}
}

static void __declspec(naked) proto_ptr_hack() {
	__asm {
		mov  ecx, maxCountLoadProto;
		cmp  ecx, 4096;
		jae  skip;
		cmp  eax, ecx;
		jb   end;
		add  ecx, 256;
		mov  maxCountLoadProto, ecx;
skip:
		cmp  eax, ecx;
end:
		retn;
	}
}

void Objects::LoadProtoAutoMaxLimit() {
	MakeCall(0x4A21B2, proto_ptr_hack);
}

void Objects::init() {
	LoadGameHook::OnGameReset() += []() {
		RestoreObjUnjamAllLocks();
	};

	HookCall(0x4A38A5, new_obj_id_hook);
	SafeWrite8(0x4A38B3, 0x90); // fix increment ID

	MakeCall(0x477A0E, item_identical_hack); // item with unique ID don't put to items stack

	// Fixes mapper bug with assign id's to critters (applies to maps not yet visited)
	MakeCall(0x482E6B, map_load_file_hack);
	SafeWrite8(0x482E71, 0x85); // jz > jnz
	// Additional fixes ID for queue events
	MakeCall(0x4A25BA, queue_add_hack);
}

}
