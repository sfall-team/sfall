#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Inventory.h"
#include "..\PartyControl.h"
#include "Common.h"

#include "InventoryHs.h"

namespace sfall
{
/*
static void RemoveInvenObjHook_Script(fo::GameObject* source, fo::GameObject* item, long count, long rmType) {
	BeginHook();
	argCount = 5;

	args[0] = (DWORD)source;
	args[1] = (DWORD)item;
	args[2] = count;
	args[3] = rmType; // RMOBJ_*
	args[4] = 0; // target only from item_move_func_

	RunHookScript(HOOK_REMOVEINVENOBJ);
	EndHook();
}

void RemoveInvenObjHook_Invoke(fo::GameObject* source, fo::GameObject* item, long count, long rmType) {
	if (HookScripts::HookHasScript(HOOK_REMOVEINVENOBJ)) RemoveInvenObjHook_Script(source, item, count, rmType);
}
*/

static long rmObjType = -1;

void __stdcall SetRemoveObjectType(long rmType) {
	rmObjType = rmType;
}

static __declspec(naked) void RemoveObjHook() {
	static const DWORD RemoveObjHookRet = 0x477497;
	__asm {
		mov  ecx, [esp + 8]; // call addr
		cmp  rmObjType, -1;
		cmovne ecx, rmObjType;
		mov  rmObjType, -1;
		cmp  ecx, -2;
		je   skipHook;
		HookBegin;
		mov  args[0], eax;   // source
		mov  args[4], edx;   // item
		mov  args[8], ebx;   // count
		mov  args[12], ecx;  // RMOBJ_* (called func)
		xor  esi, esi;
		xor  ecx, 0x47761D;  // from item_move_func_
		cmovz esi, ebp;      // target
		mov  args[16], esi;
		push eax;
		push edx;
	}

	argCount = 5;
	RunHookScript(HOOK_REMOVEINVENOBJ);
	EndHook();

	__asm {
		pop  edx;
		pop  eax;
skipHook:
		push edi;
		push ebp;
		sub  esp, 0x0C;
		jmp  RemoveObjHookRet;
	}
}

static __declspec(naked) void MoveCostHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		call fo::funcoffs::critter_compute_ap_from_distance_;
		mov  args[8], eax;
		push ebx;
		push ecx;
		mov  ebx, eax;
	}

	argCount = 3;
	RunHookScript(HOOK_MOVECOST);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static int __fastcall SwitchHandHook_Script(fo::GameObject* item, fo::GameObject* itemReplaced, DWORD addr) {
	if (itemReplaced && fo::func::item_get_type(itemReplaced) == fo::item_type_weapon && fo::func::item_get_type(item) == fo::item_type_ammo) {
		return -1; // to prevent inappropriate hook call after dropping ammo on weapon
	}

	BeginHook();
	argCount = 3;

	args[0] = (addr < 0x47136D) ? 1 : 2;    // slot: 1 - left, 2 - right
	args[1] = (DWORD)item;
	args[2] = (DWORD)itemReplaced;

	RunHookScript(HOOK_INVENTORYMOVE);
	int result = PartyControl::SwitchHandHook(item);
	if (result != -1) {
		cRetTmp = 0;
		HookCommon::SetHSReturn(result);
	}
	result = (cRet > 0) ? rets[0] : -1;
	EndHook();

	return result;
}

/*
	This hook is called every time an item is placed into either hand slot via inventory screen drag&drop
	If switch_hand_ function is not called, item is not placed anywhere (it remains in main inventory)
*/
static __declspec(naked) void SwitchHandHook() {
	__asm {
		pushadc;
		mov  ecx, eax;           // item being moved
		mov  edx, [edx];         // other item
		mov  eax, [esp + 12];    // back address
		push eax;
		call SwitchHandHook_Script;
		cmp  eax, -1;            // ret value
		popadc;
		jne  skip;
		jmp  fo::funcoffs::switch_hand_;
skip:
		retn;
	}
}

/* Common inventory move hook */
static int __fastcall InventoryMoveHook_Script(DWORD itemReplace, DWORD item, int type) {
	BeginHook();
	argCount = 3;

	args[0] = type;         // event type
	args[1] = item;         // item being dropped
	args[2] = itemReplace;  // item being replaced here

	RunHookScript(HOOK_INVENTORYMOVE);

	int result = (cRet > 0) ? rets[0] : -1; // -1 - can move
	EndHook();

	return result;
}

// This hack is called when an armor is dropped into the armor slot at inventory screen
static __declspec(naked) void UseArmorHack() {
	static const DWORD UseArmorHack_back = 0x4713AF; // normal operation (old 0x4713A9)
	static const DWORD UseArmorHack_skip = 0x471481; // skip code, prevent wearing armor
	__asm {
		mov  ecx, ds:[FO_VAR_i_worn];       // replacement item (override code)
		mov  edx, [esp + 0x58 - 0x40];      // item
		push ecx;
		push 3;                             // event: armor slot
		call InventoryMoveHook_Script;      // ecx - replacement item
		cmp  eax, -1;                       // ret value
		pop  ecx;
		jne  skip;
		jmp  UseArmorHack_back;
skip:
		jmp  UseArmorHack_skip;
	}
}

static __declspec(naked) void MoveInventoryHook() {
	__asm {
		pushadc;
		xor eax, eax;
		mov ecx, eax;                    // no item replace
		cmp dword ptr ds:[FO_VAR_curr_stack], 0;
		jle noCont;
		mov ecx, eax;                    // contaner ptr
		mov eax, 5;
noCont:
		push eax;                        // event: 0 - main backpack, 5 - contaner
		call InventoryMoveHook_Script;   // edx - item
		cmp  eax, -1;                    // ret value
		popadc;
		jne  skip;
		jmp  fo::funcoffs::item_add_force_;
skip:
		retn;
	}
}

// Hooks into dropping item from inventory to ground
// - allows to prevent item being dropped if 0 is returned with set_sfall_return
// - called for every item when dropping multiple items in stack (except caps)
// - when dropping caps it called always once
// - if 0 is returned while dropping caps, selected amount - 1 will still disappear from inventory (fixed)
static DWORD nextHookDropSkip = 0;
static int dropResult = -1;
static const DWORD InvenActionObjDropRet = 0x473874;
static __declspec(naked) void InvenActionCursorObjDropHook() {
	if (nextHookDropSkip) {
		nextHookDropSkip = 0;
		goto skipHook;
	} else {
		using namespace fo::Fields;
		__asm {
			cmp  dword ptr [esp], 0x47379A + 5;  // caps call address
			jnz  notCaps;
			mov  [edx + charges], ebx;           // edx - caps, ebx - amount-1
			add  dword ptr [edx + charges], 1;
notCaps:
			pushadc;
			xor  ecx, ecx;                       // no itemReplace
			push 6;                              // event: item drop ground
			call InventoryMoveHook_Script;       // edx - item
			mov  dropResult, eax;                // ret value
			popadc;
			cmp  dword ptr [esp], 0x47379A + 5;  // caps call address
			jz   capsMultiDrop;
		}
	}

	if (dropResult == -1) {
skipHook:
		__asm call fo::funcoffs::obj_drop_;
	}
	__asm retn;

/* for only caps multi drop */
capsMultiDrop:
	if (dropResult == -1) {
		nextHookDropSkip = 1;
		__asm {
			push eax;
			push 0x47379F;
			call SetRemoveObjectType; // call addr for HOOK_REMOVEINVENOBJ
			pop  eax;
			call fo::funcoffs::item_remove_mult_;
			retn;
		}
	}
	__asm add esp, 4;
	__asm jmp InvenActionObjDropRet; // no caps drop
}

static __declspec(naked) void InvenActionExplosiveDropHack() {
	__asm {
		pushadc;
		xor  ecx, ecx;                 // no itemReplace
		push 6;                        // event: item drop ground
		call InventoryMoveHook_Script; // edx - item
		cmp  eax, -1;                  // ret value
		popadc;
		jnz  noDrop;
		mov  dword ptr ds:[FO_VAR_dropped_explosive], ebp; // overwritten engine code (ebp = 1)
		mov  nextHookDropSkip, ebp;
		retn;
noDrop:
		add  esp, 4;
		jmp  InvenActionObjDropRet; // no drop
	}
}

static int __fastcall DropIntoContainer(DWORD ptrCont, DWORD item, DWORD addrCall) {
	int type = 5;                   // event: move to container (Crafty compatibility)

	if (addrCall == 0x47147C + 5) { // drop out contaner
		ptrCont = 0;
		type = 0;                   // event: move to main backpack
	}
	return InventoryMoveHook_Script(ptrCont, item, type);
}

static __declspec(naked) void DropIntoContainerHack() {
	static const DWORD DropIntoContainer_back = 0x47649D; // normal operation
	static const DWORD DropIntoContainer_skip = 0x476503; // exit drop_into_container_
	__asm {
		test ecx, ecx;
		js   skipDrop;
		push ecx; //pushadc;
		mov  edx, esi;         // item
		mov  ecx, ebp;         // contaner ptr
		push [esp + 0x10 + 4]; // call address
		call DropIntoContainer;
		cmp  eax, -1;          // ret value
		pop  ecx; //popadc;
		jne  skipDrop;
		jmp  DropIntoContainer_back;
skipDrop:
		mov  eax, -1;
		jmp  DropIntoContainer_skip;
	}
}

static __declspec(naked) void DropIntoContainerHandSlotHack() {
	static const DWORD DropIntoContainerRet = 0x471481;
	__asm {
		call fo::funcoffs::drop_into_container_;
		jmp  DropIntoContainerRet;
	}
}

static __declspec(naked) void DropAmmoIntoWeaponHook() {
	static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
	__asm {
		pushadc;
		mov  ecx, ebp;              // weapon ptr
		mov  edx, [esp + 16];       // item var: ammo_
		push 4;                     // event: weapon reloading
		call InventoryMoveHook_Script;
		cmp  eax, -1;               // ret value
		jne  donothing;
		popadc;
		jmp  fo::funcoffs::item_w_can_reload_;
donothing:
		add  esp, 4*4; // destroy all pushed values and return address
		xor  eax, eax; // result 0
		jmp  DropAmmoIntoWeaponHack_return;
	}
}

static __declspec(naked) void PickupObjectHack() {
	__asm {
		cmp  edi, ds:[FO_VAR_obj_dude];
		je   runHook;
		xor  edx, edx; // engine handler
		retn;
runHook:
		push eax;
		push ecx;
		mov  edx, ecx;
		xor  ecx, ecx;                 // no itemReplace
		push 7;                        // event: item pickup
		call InventoryMoveHook_Script; // edx - item
		mov  edx, eax;                 // ret value
		pop  ecx;
		pop  eax;
		inc  edx; // 0 - engine handler, otherwise cancel pickup
		retn;
	}
}

static __declspec(naked) void InvenPickupHook() {
	__asm {
		call fo::funcoffs::mouse_click_in_;
		test eax, eax;
		jnz  runHook;
		retn;
runHook:
		cmp  dword ptr ds:[FO_VAR_curr_stack], 0;
		jnz  skip;
		mov  edx, [esp + 0x58 - 0x40 + 4]; // item
		xor  ecx, ecx;                     // no itemReplace
		push 8;                            // event: drop item on character portrait
		call InventoryMoveHook_Script;
		cmp  eax, -1;  // ret value
		je   skip;
		xor  eax, eax; // 0 - cancel, otherwise engine handler
skip:
		retn;
	}
}

/* Common InvenWield script hooks */
static long __fastcall InvenWieldHook_Script(fo::GameObject* critter, fo::GameObject* item, long slot, long isWield, long isRemove) {
	if (!isWield) {
		// for the critter, the right slot is always the active slot
		if (slot == fo::INVEN_TYPE_LEFT_HAND && critter != *fo::ptr::obj_dude) return 1;
		// check the current active slot for the player
		if (slot != fo::INVEN_TYPE_WORN && critter == *fo::ptr::obj_dude) {
			long _slot = (slot != fo::INVEN_TYPE_LEFT_HAND);
			if (_slot != *fo::ptr::itemCurrentItem) return 1; // item in non-active slot
		}
	}
	BeginHook();
	argCount = 5;

	args[0] = (DWORD)critter;
	args[1] = (DWORD)item;
	args[2] = slot;
	args[3] = isWield; // unwield/wield event
	args[4] = isRemove;

	RunHookScript(HOOK_INVENWIELD);

	long result = (cRet == 0 || rets[0] == -1);
	EndHook();

	return result; // 1 - use engine handler
}

static __declspec(noinline) bool InvenWieldHook_ScriptPart(long isWield, long isRemove = 0) {
	argCount = 5;

	args[3] = isWield; // unwield/wield event
	args[4] = isRemove;

	RunHookScript(HOOK_INVENWIELD);

	bool result = (cRet == 0 || rets[0] == -1);
	EndHook();

	return result; // true - use engine handler
}

static __declspec(naked) void InvenWieldFuncHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // slot
		pushad;
	}

	// right hand slot?
	if (args[2] != fo::INVEN_TYPE_RIGHT_HAND && fo::func::item_get_type((fo::GameObject*)args[1]) != fo::item_type_armor) {
		args[2] = fo::INVEN_TYPE_LEFT_HAND;
	}
	InvenWieldHook_ScriptPart(1); // wield event

	__asm {
		test al, al;
		popad;
		jz   skip;
		jmp  fo::funcoffs::invenWieldFunc_;
skip:
		mov  eax, -1;
		retn;
	}
}

// called when unwielding weapons
static __declspec(naked) void InvenUnwieldFuncHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[8], edx; // slot
		pushadc;
	}

	// set slot
	if (args[2] == 0) { // left hand slot?
		args[2] = fo::INVEN_TYPE_LEFT_HAND;
	}

	// get item
	args[1] = (DWORD)fo::util::GetItemPtrSlot((fo::GameObject*)args[0], (fo::InvenType)args[2]);

	InvenWieldHook_ScriptPart(0); // unwield event

	__asm {
		test al, al;
		popadc;
		jz   skip;
		jmp  fo::funcoffs::invenUnwieldFunc_;
skip:
		mov  eax, -1;
		retn;
	}
}

static __declspec(naked) void CorrectFidForRemovedItemHook() {
	using namespace fo::ObjectFlag;
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // item flag
		pushadc;
	}

	// set slot
	if (args[2] & fo::ObjectFlag::Right_Hand) {       // right hand slot
		args[2] = fo::INVEN_TYPE_RIGHT_HAND;
	} else if (args[2] & fo::ObjectFlag::Left_Hand) { // left hand slot
		args[2] = fo::INVEN_TYPE_LEFT_HAND;
	} else {
		args[2] = fo::INVEN_TYPE_WORN;                // armor slot
	}

	InvenWieldHook_ScriptPart(0, 1); // unwield event (armor by default)

	// engine handler is not overridden
	__asm {
		popadc;
		jmp  fo::funcoffs::correctFidForRemovedItem_;
	}
}

long InvenWieldHook_Invoke(fo::GameObject* critter, fo::GameObject* item, long flags) {
	if (!HookScripts::HookHasScript(HOOK_INVENWIELD)) return 1;

	long slot = fo::INVEN_TYPE_WORN;
	if (flags & fo::ObjectFlag::Right_Hand) {       // right hand slot
		slot = fo::INVEN_TYPE_RIGHT_HAND;
	} else if (flags & fo::ObjectFlag::Left_Hand) { // left hand slot
		slot = fo::INVEN_TYPE_LEFT_HAND;
	}
	return InvenWieldHook_Script(critter, item, slot, 0, 0);
}

static __declspec(naked) void item_drop_all_hack() {
	using namespace fo::ObjectFlag;
	__asm {
		mov  ecx, 1;
		push eax;
		mov  [esp + 0x40 - 0x2C + 8], ecx; // itemIsEquipped
		push ecx; // remove event
		push 0;   // unwield event
		inc  ecx; // INVEN_TYPE_LEFT_HAND (2)
		test ah, Left_Hand >> 24;
		jnz  skip;
		test ah, Worn >> 24;
		setz cl; // set INVEN_TYPE_WORN or INVEN_TYPE_RIGHT_HAND
skip:
		push ecx;      // slot
		mov  edx, esi; // item
		mov  ecx, edi; // critter
		call InvenWieldHook_Script;
		//mov  [esp + 0x40 - 0x2C + 8], eax; // itemIsEquipped (eax - hook return result)
		pop  eax;
		retn;
	}
}

static bool hookInvenWieldIsInject = false;

// called from bugfixes for obj_drop_
__declspec(naked) void InvenUnwield_HookDrop() { // ecx - critter, edx - item
	using namespace fo;
	using namespace Fields;
	using namespace ObjectFlag;
	__asm {
		cmp hookInvenWieldIsInject, 1;
		je  runHook;
		retn;
runHook:
		pushadc;
		mov  eax, INVEN_TYPE_LEFT_HAND;
		test byte ptr [edx + flags + 3], Left_Hand >> 24;
		jnz  isLeft;
		test byte ptr [edx + flags + 3], Worn >> 24;
		setz al;  // set INVEN_TYPE_WORN or INVEN_TYPE_RIGHT_HAND
isLeft:
		push 1;   // remove event
		push 0;   // unwield event
		push eax; // slot
		call InvenWieldHook_Script; // ecx - critter, edx - item
		// engine handler is not overridden
		popadc;
		retn;
	}
}

// called from bugfixes for op_move_obj_inven_to_obj_
__declspec(naked) void InvenUnwield_HookMove() { // eax - item, edx - critter
	__asm {
		cmp hookInvenWieldIsInject, 1;
		je  runHook;
		retn;
runHook:
		pushadc;
		mov  ecx, edx;
		mov  edx, eax;
		push 1;   // remove event
		xor  eax, eax;
		push eax; // unwield event
		push eax; // slot
		call InvenWieldHook_Script; // ecx - critter, edx - item
		// engine handler is not overridden
		popadc;
		retn;
	}
}

// called when unwelding dude weapon and armor
static __declspec(naked) void op_move_obj_inven_to_obj_hook() {
	using namespace fo;
	using namespace ObjectFlag;
	__asm {
		cmp  eax, ds:[FO_VAR_obj_dude];
		je   runHook;
		jmp  fo::funcoffs::item_move_all_;
runHook:
		push eax;
		push edx;
		mov  ecx, eax; // keep source
		mov  edx, ds:[FO_VAR_itemCurrentItem]; // get player's active slot
		test edx, edx;
		jz   left;
		call fo::funcoffs::inven_right_hand_;
		jmp  skip;
left:
		call fo::funcoffs::inven_left_hand_;
skip:
		test eax, eax;
		jz   noWeapon;
		push 1; // remove event
		push 0; // unwield event
		mov  ebx, INVEN_TYPE_LEFT_HAND;
		sub  ebx, edx;
		push ebx;      // slot: INVEN_TYPE_LEFT_HAND or INVEN_TYPE_RIGHT_HAND
		mov  edx, eax; // weapon
		mov  ebx, ecx; // keep source
		call InvenWieldHook_Script; // ecx - source
		// engine handler is not overridden
noWeapon:
		mov  edx, [esp + 0x30 - 0x20 + 12]; // armor
		test edx, edx;
		jz   noArmor;
		xor  eax, eax;
		push 1;   // remove event
		push eax; // unwield event
		push eax; // slot: INVEN_TYPE_WORN
		mov  ecx, ebx; // source
		call InvenWieldHook_Script;
		// engine handler is not overridden
noArmor:
		pop  edx;
		pop  eax;
		jmp  fo::funcoffs::item_move_all_;
	}
}

void AdjustFidHook(DWORD vanillaFid) {
	if (!HookScripts::HookHasScript(HOOK_ADJUSTFID)) return;

	BeginHook();
	argCount = 2;

	args[0] = vanillaFid;
	args[1] = *fo::ptr::i_fid; // modified FID by sfall code
	RunHookScript(HOOK_ADJUSTFID);

	if (cRet > 0) {
		*fo::ptr::i_fid = rets[0];
	}
	EndHook();
}

void Inject_RemoveInvenObjHook() {
	MakeJump(0x477492, RemoveObjHook); // old 0x477490
}

void Inject_MoveCostHook() {
	const DWORD moveCostHkAddr[] = {0x417665, 0x44B88A};
	HookCalls(MoveCostHook, moveCostHkAddr);
}

void Inject_InventoryMoveHook() {
	const DWORD switchHandHkAddr[] = {
		0x4712E3, // left slot
		0x47136D  // right slot
	};
	HookCalls(SwitchHandHook, switchHandHkAddr);
	MakeJump(0x4713A9, UseArmorHack); // old 0x4713A3
	MakeJump(0x476491, DropIntoContainerHack);
	const DWORD dropIntoContHandHkAddr[] = {0x471338, 0x4712AB};
	MakeJumps(DropIntoContainerHandSlotHack, dropIntoContHandHkAddr);
	HookCall(0x471200, MoveInventoryHook);
	HookCall(0x476549, DropAmmoIntoWeaponHook); // old 0x476588
	const DWORD actionCurObjDropHkAddr[] = {
		0x473851, 0x47386F,
		0x47379A  // caps multi drop
	};
	HookCalls(InvenActionCursorObjDropHook, actionCurObjDropHkAddr);
	MakeCall(0x473807, InvenActionExplosiveDropHack, 1); // drop active explosives

	MakeCall(0x49B660, PickupObjectHack);
	SafeWrite32(0x49B665, 0x850FD285); // test edx, edx
	SafeWrite32(0x49B669, 0xC2);       // jnz  0x49B72F
	SafeWrite8(0x49B66E, 0xFE);        // cmp edi > cmp esi

	HookCall(0x471457, InvenPickupHook);
}

void Inject_InvenWieldHook() {
	const DWORD invWieldFuncHkAddr[] = {
		0x47275E, // inven_wield_
		0x495FDF  // partyMemberCopyLevelInfo_
	};
	HookCalls(InvenWieldFuncHook, invWieldFuncHkAddr);
	const DWORD invUnwieldFuncHkAddr[] = {
		0x45967D, // op_metarule_
		0x472A5A, // inven_unwield_
		0x495F0B  // partyMemberCopyLevelInfo_
	};
	HookCalls(InvenUnwieldFuncHook, invUnwieldFuncHkAddr);
	const DWORD fidRemovedItemHkAddr[] = {
		0x45680C, // op_rm_obj_from_inven_
		0x45C4EA  // op_move_obj_inven_to_obj_
	};
	HookCalls(CorrectFidForRemovedItemHook, fidRemovedItemHkAddr);
	HookCall(0x45C4F6, op_move_obj_inven_to_obj_hook);
	MakeCall(0x4778AF, item_drop_all_hack, 3);

	hookInvenWieldIsInject = true;
}

void InitInventoryHookScripts() {
	HookScripts::LoadHookScript("hs_removeinvenobj", HOOK_REMOVEINVENOBJ);
	HookScripts::LoadHookScript("hs_movecost", HOOK_MOVECOST);
	HookScripts::LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	HookScripts::LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	HookScripts::LoadHookScript("hs_adjustfid", HOOK_ADJUSTFID);
}

}
