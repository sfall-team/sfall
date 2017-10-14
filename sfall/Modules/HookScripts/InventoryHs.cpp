
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
		push ecx;
		mov ecx, [esp + 4];
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		mov args[12], ecx;
		pushad;
		push HOOK_REMOVEINVENOBJ;
		call RunHookScript;
		popad;
		hookend;
		push esi;
		push edi;
		push ebp;
		sub esp, 0xc;
		jmp RemoveObjHookRet;
	}
}

static void __declspec(naked) MoveCostHook() {
	__asm {
		hookbegin(3);
		mov args[0], eax;
		mov args[4], edx;
		call fo::funcoffs::critter_compute_ap_from_distance_
			mov args[8], eax;
		pushad;
		push HOOK_MOVECOST;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

static int __stdcall SwitchHandHook2(fo::GameObject* item, fo::GameObject* itemReplaced, DWORD addr) {
	int tmp;
	if (itemReplaced && fo::func::item_get_type(itemReplaced) == fo::item_type_weapon && fo::func::item_get_type(item) == fo::item_type_ammo) {
		return -1; // to prevent inappropriate hook call after dropping ammo on weapon
	}
	BeginHook();
	argCount = 3;
	args[0] = (addr < 0x47136D) ? 1 : 2;
	args[1] = (DWORD)item;
	args[2] = (DWORD)itemReplaced;
	RunHookScript(HOOK_INVENTORYMOVE); // moveinventory
	tmp = PartyControl::SwitchHandHook(item);
	if (tmp != -1) {
		cRetTmp = 0;
		SetHSReturn(tmp);
	}
	EndHook();
	if (cRet > 0) {
		return rets[0];
	}
	return -1;
}

/*
	This hook is called every time an item is placed into either hand slot via inventory screen drag&drop
	If switch_hand_ function is not called, item is not placed anywhere (it remains in main inventory)
*/
static void _declspec(naked) SwitchHandHook() {
	_asm {
		pushad;
		mov ecx, eax;
		mov eax, [esp+32]; // back address
		push eax;
		mov edx, [edx];
		push edx; // other item
		push ecx; // item being moved
		call SwitchHandHook2;
		cmp eax, -1;
		popad;
		jne skip;
		call fo::funcoffs::switch_hand_;
skip:
		retn;
	}
}


static const DWORD UseArmorHack_back = 0x4713A9; // normal operation
static const DWORD UseArmorHack_skip = 0x471481; // skip code, prevent wearing armor
// This hack is called when an armor is dropped into the armor slot at inventory screen
static void _declspec(naked) UseArmorHack() {
	__asm {
		cmp eax, 0;
		jne skip; // not armor
		hookbegin(3);
		mov args[0], 3;
		mov eax, [esp+24]; // item
		mov args[4], eax;
  		mov eax, ds:[FO_VAR_i_worn]
		mov args[8], eax;
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl back;
		cmp rets[0], -1;
		jne skip;
back:
		hookend;
		jmp UseArmorHack_back;
skip:
		hookend;
		jmp UseArmorHack_skip;
	}
}

static void _declspec(naked) MoveInventoryHook() {
	__asm {
		hookbegin(3);
		mov args[0], 0;
		mov args[4], edx;
		mov args[8], 0; // no item being replaced here..
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl skipcheck;
		cmp rets[0], -1;
		jne skipcall;
skipcheck:
		call fo::funcoffs::item_add_force_
skipcall:
		hookend;
		retn;
	}
}

// Hooks into dropping item from inventory to ground
// - allows to prevent item being dropped if 0 is returned with set_sfall_return
// - called for every item when dropping multiple items in stack (except caps)
// - when dropping caps it called always once
// - if 0 is returned while dropping caps, selected amount - 1 will still disappear from inventory
/*
static void __declspec(naked) InvenActionCursor_ObjDrop_Hook() {
	__asm {
		hookbegin(3);
		mov args[0], 5;
		mov args[4], edx; // item being dropped
		mov args[8], 0; // no item being replaced here..
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl skipcheck;
		cmp rets[0], -1;
		jne skipcall;
skipcheck:
		call fo::funcoffs::obj_drop_;
skipcall:
		hookend;
		retn;
	}
}
*/

static const DWORD DropAmmoIntoWeaponHack_back = 0x47658D; // proceed with reloading
static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
static void _declspec(naked) DropAmmoIntoWeaponHack() {
	__asm {
		hookbegin(3);
		mov args[0], 4;
		mov eax, [esp];
		mov args[4], eax;
		mov args[8], ebp;
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl proceedreloading;
		cmp rets[0], -1;
		jne donothing;
proceedreloading:
		hookend;
		mov ebx, 1; // overwritten code
		jmp DropAmmoIntoWeaponHack_back;
donothing:
		hookend;
		mov eax, 0;
		jmp DropAmmoIntoWeaponHack_return;
	}
}


static void _declspec(naked) invenWieldFunc_Hook() {
	using namespace fo;
	__asm {
		hookbegin(4);
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // slot
		mov args[12], 1; // wield flag
		pushad;
		cmp ebx, 1; // right hand slot?
		je skip;
		mov eax, edx;
		call fo::funcoffs::item_get_type_;
		cmp eax, item_type_armor;
		jz skip;
		mov args[8], 2; // INVEN_TYPE_LEFT_HAND
skip:
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end
			defaulthandler :
		call fo::funcoffs::invenWieldFunc_
			end :
		hookend;
		retn;
	}
}

// called when unwielding weapons
static void _declspec(naked) invenUnwieldFunc_Hook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // critter
		mov args[4], 0; // item
		mov args[8], edx; // slot
		mov args[12], 0; // wield flag
		cmp edx, 0; // left hand slot?
		jne notlefthand;
		mov args[8], 2; // left hand
notlefthand:
		pushad;
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end
			defaulthandler :
		call fo::funcoffs::invenUnwieldFunc_;
end:
		hookend;
		retn;
	}
}

static void _declspec(naked) correctFidForRemovedItem_Hook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], 0; // slot
		mov args[12], 0; // wield flag (armor by default)
		test ebx, 0x02000000; // right hand slot?
		jz notrighthand;
		mov args[8], 1; // right hand
notrighthand:
		test ebx, 0x01000000; // left hand slot?
		jz notlefthand;
		mov args[8], 2; // left hand
notlefthand:
		pushad;
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end;
defaulthandler:
		call fo::funcoffs::correctFidForRemovedItem_;
end:
		hookend;
		retn;
	}
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

void InitInventoryHookScripts() {
	LoadHookScript("hs_removeinvenobj", HOOK_REMOVEINVENOBJ);
	MakeJump(0x477490, RemoveObjHook);

	LoadHookScript("hs_movecost", HOOK_MOVECOST);
	HookCalls(MoveCostHook, { 0x417665, 0x44B88A });

	LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	HookCalls(SwitchHandHook, {
		0x4712E3, // left slot
		0x47136D  // right slot
	});
	MakeJump(0x4713A3, UseArmorHack);
	//HookCall(0x4711B3, &DropIntoContainerHook); 
	//HookCall(0x47147C, &DropIntoContainerHook); 
	HookCall(0x471200, MoveInventoryHook);
	//HookCall(0x4712C7, &DropAmmoIntoWeaponHook);
	//HookCall(0x471351, &DropAmmoIntoWeaponHook);
	MakeJump(0x476588, DropAmmoIntoWeaponHack);
	// TODO: consider adding item drop event into INVENTORYMOVE or different hook
	//HookCalls(InvenActionCursor_ObjDrop_Hook, { 0x473851, 0x47386F });

	LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	HookCalls(invenWieldFunc_Hook, { 0x47275E, 0x495FDF });
	HookCalls(invenUnwieldFunc_Hook, { 0x45967D, 0x472A5A, 0x495F0B });
	HookCalls(correctFidForRemovedItem_Hook, { 0x45680C, 0x45C4EA });

	LoadHookScript("hs_adjustfid", HOOK_ADJUSTFID);
	Inventory::OnAdjustFid() += AdjustFidHook;
}

}
