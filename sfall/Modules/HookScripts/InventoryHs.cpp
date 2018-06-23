
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Inventory.h"
#include "..\PartyControl.h"
#include "Common.h"

#include "InventoryHs.h"

namespace sfall
{

static const DWORD RemoveObjHookRet = 0x477497;
static void __declspec(naked) RemoveObjHook() {
	__asm {
		mov ecx, [esp + 8]; // call addr
		HookBegin;
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		mov args[12], ecx;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_REMOVEINVENOBJ);
	EndHook();

	__asm {
		popad;
		push edi;
		push ebp;
		sub  esp, 0x0C;
		jmp RemoveObjHookRet;
	}
}

static void __declspec(naked) MoveCostHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		call fo::funcoffs::critter_compute_ap_from_distance_
		mov  args[8], eax;
		pushad;
	}

	argCount = 3;
	RunHookScript(HOOK_MOVECOST);
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		cmovge eax, dword ptr rets[0];
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
	EndHook();

	return (cRet > 0) ? rets[0] : -1;
}

static int __fastcall SwitchHandHook_Script(fo::GameObject* item, fo::GameObject* itemReplaced, DWORD addr) {
	if (itemReplaced && fo::GetItemType(itemReplaced) == fo::item_type_weapon && fo::GetItemType(item) == fo::item_type_ammo) {
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
		SetHSReturn(result);
	}
	EndHook();

	return (cRet > 0) ? rets[0] : -1;
}

/*
	This hook is called every time an item is placed into either hand slot via inventory screen drag&drop
	If switch_hand_ function is not called, item is not placed anywhere (it remains in main inventory)
*/
static void _declspec(naked) SwitchHandHook() {
	__asm {
		pushad;
		mov  ecx, eax;           // item being moved
		mov  edx, [edx];         // other item
		mov  eax, [esp + 32];    // back address
		push eax;
		call SwitchHandHook_Script;
		cmp  eax, -1;            // ret value
		popad;
		jne  skip;
		call fo::funcoffs::switch_hand_;
skip:
		retn;
	}
}

static const DWORD UseArmorHack_back = 0x4713AF; // normal operation (old 0x4713A9)
static const DWORD UseArmorHack_skip = 0x471481; // skip code, prevent wearing armor
// This hack is called when an armor is dropped into the armor slot at inventory screen
static void _declspec(naked) UseArmorHack() {
	__asm {
		mov  ecx, ds:[FO_VAR_i_worn];       // replacement item (override code)
		mov  edx, [esp + 0x58 - 0x40];      // item
		pushad;
		push 3;                             // event: armor slot
		call InventoryMoveHook_Script;
		cmp  eax, -1;                       // ret value
		popad;
		jne skip;
		jmp UseArmorHack_back;
skip:
		jmp UseArmorHack_skip;
	}
}

static void _declspec(naked) MoveInventoryHook() {
	__asm {
		pushad;
		xor ecx, ecx;                    // no item replace
		mov ebx, ecx;
		cmp dword ptr ds:[FO_VAR_curr_stack], 0;
		jle noCont;
		mov ecx, eax;                    // contaner ptr
		mov ebx, 5;
noCont:
		push ebx;                        // event: 0 - main backpack, 5 - contaner
		call InventoryMoveHook_Script;   // edx - item
		cmp  eax, -1;                    // ret value
		popad;
		jne  skip;
		call fo::funcoffs::item_add_force_;
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
static void __declspec(naked) InvenActionCursorObjDropHook() {
	if (nextHookDropSkip) {
		nextHookDropSkip = 0;
		goto skipHook;
	} else {
		__asm {
			pushad;
			xor  ecx, ecx;                       // no itemReplace
			push 6;                              // event: item drop ground
			call InventoryMoveHook_Script;       // edx - item
			mov  dropResult, eax;                // ret value
			popad;
			cmp  dword ptr [esp], 0x47379A + 5;  // caps call address
			jz   capsMultiDrop;
		}
	}

	if (dropResult == -1) {
skipHook:
		_asm call fo::funcoffs::obj_drop_;
	}
	_asm retn;

/* for only caps multi drop */
capsMultiDrop:
	if (dropResult == -1) {
		nextHookDropSkip = 1;
		_asm call fo::funcoffs::item_remove_mult_;
		_asm retn;
	}
	_asm add esp, 4;
	_asm jmp InvenActionObjDropRet;    // no caps drop
}

static void __declspec(naked) InvenActionExplosiveDropHack() {
	__asm {
		pushad;
		xor  ecx, ecx;                       // no itemReplace
		push 6;                              // event: item drop ground
		call InventoryMoveHook_Script;       // edx - item
		cmp  eax, -1;                        // ret value
		popad;
		jnz noDrop;
		mov dword ptr ds:[FO_VAR_dropped_explosive], ebp; // overwritten engine code (ebp = 1)
		mov nextHookDropSkip, ebp;
		retn;
noDrop:
		add esp, 4;
		jmp InvenActionObjDropRet;           // no drop
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

static const DWORD DropIntoContainer_back = 0x47649D; // normal operation
static const DWORD DropIntoContainer_skip = 0x476503;
static void __declspec(naked) DropIntoContainerHack() {
	__asm {
		pushad;
		mov ecx, ebp;                // contaner ptr
		mov edx, esi;                // item
		mov eax, [esp + 0x10 + 32];  // call address
		push eax;
		call DropIntoContainer;
		cmp  eax, -1;                // ret value
		popad;
		jne  skipdrop;
		jmp DropIntoContainer_back;
skipdrop:
		mov eax, -1;
		jmp DropIntoContainer_skip;
	}
}

static const DWORD DropIntoContainerRet = 0x471481;
static void __declspec(naked) DropIntoContainerHandSlotHack() {
	__asm {
		call fo::funcoffs::drop_into_container_;
		jmp DropIntoContainerRet;
	}
}

static const DWORD DropAmmoIntoWeaponHack_back = 0x47658D; // proceed with reloading
static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
static void _declspec(naked) DropAmmoIntoWeaponHack() {
	__asm {
		pushad;
		mov ecx, ebp;              // weapon ptr
		mov edx, [esp + 32];       // item var: ammo_
		push 4;                    // event: weapon reloading
		call InventoryMoveHook_Script;
		cmp  eax, -1;              // ret value
		popad;
		jne  donothing;
		mov ebx, 1;   // overwritten code
		jmp DropAmmoIntoWeaponHack_back;
donothing:
		xor eax, eax; // result 0
		jmp DropAmmoIntoWeaponHack_return;
	}
}

/* Common InvenWield hook */
static void InvenWieldHook_Script(int flag) {

	argCount = 4;
	args[3] = flag;  // invenwield flag
	RunHookScript(HOOK_INVENWIELD);
	EndHook();
}

static void _declspec(naked) InvenWieldFuncHook() {
	using namespace fo;
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // slot
		pushad;
	}

	// right hand slot?
	if (args[2] != INVEN_TYPE_RIGHT_HAND && GetItemType((GameObject*)args[1]) != item_type_armor) {
		args[2] = INVEN_TYPE_LEFT_HAND;
	}

	InvenWieldHook_Script(1); // wield flag

	_asm popad;
	if (cRet == 0 || rets[0] == -1) {
		_asm call funcoffs::invenWieldFunc_;
	}
	_asm retn;
}

// called when unwielding weapons
static void _declspec(naked) InvenUnwieldFuncHook() {
	__asm {
		HookBegin;
		mov args[0], eax;   // critter
		mov args[8], edx;   // slot
		pushad;
	}

	// set slot
	if (args[2] == 0) { // left hand slot?
		args[2] = fo::INVEN_TYPE_LEFT_HAND;
	}
	args[1] = (DWORD)fo::GetItemPtrSlot((fo::GameObject*)args[0], (fo::InvenType)args[2]); // get item

	InvenWieldHook_Script(0); // unwield flag

	_asm popad;
	if (cRet == 0 || rets[0] == -1) {
		_asm call fo::funcoffs::invenUnwieldFunc_;
	}
	_asm retn;
}

static void _declspec(naked) CorrectFidForRemovedItemHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // item flag
		pushad;
	}

	// set slot
	if (args[2] & fo::ObjectFlag::Right_Hand) {       // right hand slot
		args[2] = fo::INVEN_TYPE_RIGHT_HAND;
	} else if (args[2] & fo::ObjectFlag::Left_Hand) { // left hand slot
		args[2] = fo::INVEN_TYPE_LEFT_HAND;
	} else {
		args[2] = fo::INVEN_TYPE_WORN;                // armor slot
	}

	InvenWieldHook_Script(0); // unwield flag (armor by default)

	_asm popad;
	if (cRet == 0 || rets[0] == -1) {
		_asm call fo::funcoffs::correctFidForRemovedItem_;
	}
	_asm retn;
}

void AdjustFidHook(DWORD vanillaFid) {
	BeginHook();
	argCount = 1;
	args[0] = vanillaFid;
	RunHookScript(HOOK_ADJUSTFID);
	if (cRet > 0) {
		fo::var::i_fid = rets[0];
	}
	EndHook();
}

void Inject_RemoveInvenObjHook() {
	MakeJump(0x477492, RemoveObjHook); // old 0x477490
}

void Inject_MoveCostHook() {
	HookCalls(MoveCostHook, { 0x417665, 0x44B88A });
}

void Inject_InventoryMoveHook() {
	HookCalls(SwitchHandHook, {
		0x4712E3, // left slot
		0x47136D  // right slot
	});
	MakeJump(0x4713A9, UseArmorHack); // old 0x4713A3
	MakeJump(0x476491, DropIntoContainerHack);
	MakeJump(0x471338, DropIntoContainerHandSlotHack);
	MakeJump(0x4712AB, DropIntoContainerHandSlotHack);
	HookCall(0x471200, MoveInventoryHook);
	MakeJump(0x476588, DropAmmoIntoWeaponHack);
	HookCalls(InvenActionCursorObjDropHook, {
		0x473851, 0x47386F,
		0x47379A  // caps multi drop
	});
	MakeCall(0x473807, InvenActionExplosiveDropHack);  // drop active explosives
	SafeWrite8(0x47380C, 0x90);
}

void Inject_InvenWieldHook() {
	HookCalls(InvenWieldFuncHook, { 0x47275E, 0x495FDF });
	HookCalls(InvenUnwieldFuncHook, { 0x45967D, 0x472A5A, 0x495F0B });
	HookCalls(CorrectFidForRemovedItemHook, { 0x45680C, 0x45C4EA });
}

void InitInventoryHookScripts() {

	LoadHookScript("hs_removeinvenobj", HOOK_REMOVEINVENOBJ);
	LoadHookScript("hs_movecost", HOOK_MOVECOST);
	LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	LoadHookScript("hs_adjustfid", HOOK_ADJUSTFID);

	Inventory::OnAdjustFid() += AdjustFidHook;
}

}
