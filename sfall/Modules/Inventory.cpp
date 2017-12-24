/*
 *    sfall
 *    Copyright (C) 2011  Timeslip
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "PartyControl.h"
#include "HookScripts.h"
#include "LoadGameHook.h"

#include "Inventory.h"

namespace sfall
{

static Delegate<DWORD> onAdjustFid;

static DWORD mode;
static DWORD maxItemSize;
static DWORD reloadWeaponKey = 0;

long& GetActiveItemMode() {
	return fo::var::itemButtonItems[fo::var::itemCurrentItem].mode;
}

fo::GameObject* GetActiveItem() {
	return fo::var::itemButtonItems[fo::var::itemCurrentItem].item;
}

void InventoryKeyPressedHook(DWORD dxKey, bool pressed, DWORD vKey) {
	// TODO: move this out into a script
	if (pressed && reloadWeaponKey && dxKey == reloadWeaponKey && IsMapLoaded() && (GetLoopFlags() & ~(COMBAT | PCOMBAT)) == 0) {
		DWORD maxAmmo, curAmmo;
		fo::GameObject* item = GetActiveItem();
		maxAmmo = fo::func::item_w_max_ammo(item);
		curAmmo = fo::func::item_w_curr_ammo(item);
		if (maxAmmo != curAmmo) {
			long &currentMode = GetActiveItemMode();
			long previusMode = currentMode;
			currentMode = 5; // reload mode
			fo::func::intface_use_item();
			if (previusMode != 5) {
				// return to previous active item mode (if it wasn't "reload")
				currentMode = previusMode - 1;
				if (currentMode < 0) {
					currentMode = 4;
				}
				fo::func::intface_toggle_item_state();
			}
		}
	}
}

//TODO: Do we actually want to include this in the limit anyway?
static __declspec(naked) DWORD item_total_size(void* critter) {
	__asm {
		push    ebx;
		push    ecx;
		push    edx;
		push    esi;
		push    edi;
		push    ebp;
		mov     ebp, eax;
		test    eax, eax;
		jz      loc_477F33;
		lea     edi, [eax+2Ch];
		xor     edx, edx;
		mov     ebx, [edi];
		xor     esi, esi;
		test    ebx, ebx;
		jle     loc_477ED3;
		xor     ebx, ebx;
loc_477EB7:
		mov     ecx, [edi+8];
		mov     eax, [ecx+ebx];
		call    fo::funcoffs::item_size_
		imul    eax, [ecx+ebx+4];
		add     ebx, 8;
		inc     edx;
		mov     ecx, [edi];
		add     esi, eax;
		cmp     edx, ecx;
		jl      loc_477EB7;
loc_477ED3:
		mov     eax, [ebp+20h];
		and     eax, 0F000000h;
		sar     eax, 18h;
		cmp     eax, 1;
		jnz     loc_477F31;
		mov     eax, ebp;
		call    fo::funcoffs::inven_right_hand_
		mov     edx, eax;
		test    eax, eax;
		jz      loc_477EFD;
		test    byte ptr [eax+27h], 2;
		jnz     loc_477EFD;
		call    fo::funcoffs::item_size_
		add     esi, eax;
loc_477EFD:
		mov     eax, ebp;
		call    fo::funcoffs::inven_left_hand_
		test    eax, eax;
		jz      loc_477F19;
		cmp     edx, eax;
		jz      loc_477F19;
		test    byte ptr [eax+27h], 1;
		jnz     loc_477F19;
		call    fo::funcoffs::item_size_
		add     esi, eax;
loc_477F19:
		mov     eax, ebp;
		call    fo::funcoffs::inven_worn_
		test    eax, eax;
		jz      loc_477F31;
		test    byte ptr [eax+27h], 4;
		jnz     loc_477F31;
		call    fo::funcoffs::item_size_
		add     esi, eax;
loc_477F31:
		mov     eax, esi;
loc_477F33:
		pop     ebp;
		pop     edi;
		pop     esi;
		pop     edx;
		pop     ecx;
		pop     ebx;
		retn;
	}
}

/*static const DWORD ObjPickupFail=0x49B70D;
static const DWORD ObjPickupEnd=0x49B6F8;
static const DWORD size_limit;
static __declspec(naked) void  ObjPickupHook() {
	__asm {
		cmp edi, ds:[FO_VAR_obj_dude];
		jnz end;
end:
		lea edx, [esp+0x10];
		mov eax, ecx;
		jmp ObjPickupEnd;
	}
}*/

static __declspec(naked) int CritterCheck() {
	__asm {
		push ebx;
		push edx;
		sub esp, 4;
		mov ebx, eax;

		cmp eax, dword ptr ds:[FO_VAR_obj_dude];
		je single;
		test mode, 3;
		jnz run;
		test mode, 2;
		jz fail;
		call fo::funcoffs::isPartyMember_;
		test eax, eax;
		jz end;
run:
		test mode, 8;
		jz single;
		mov edx, esp;
		mov eax, ebx;
		call fo::funcoffs::proto_ptr_;
		mov eax, [esp];
		mov eax, [eax + 0xB0 + 40]; //The unused stat in the extra block
		jmp end;
single:
		mov eax, maxItemSize;
		jmp end;
fail:
		xor eax, eax;
end:
		add esp, 4;
		pop edx;
		pop ebx;
		retn;
	}
}

static const DWORD IsOverloadedEnd = 0x42E68D;
static __declspec(naked) void CritterIsOverloadedHook() {
	__asm {
		and eax, 0xff;
		jnz end;
		mov eax, ebx;
		call CritterCheck;
		test eax, eax;
		jz end;
		xchg eax, ebx;
		call item_total_size;
		cmp eax, ebx;
		setg al;
		and eax, 0xff;
end:
		jmp IsOverloadedEnd;
	}
}

static const DWORD ItemAddMultiRet = 0x4772A6;
static const DWORD ItemAddMultiFail = 0x4771C7;
static __declspec(naked) void ItemAddMultiHook1() {
	__asm {
		push ebp;
		mov eax, ecx;
		call CritterCheck;
		test eax, eax;
		jz end;
		mov ebp, eax;
		mov eax, esi;
		call fo::funcoffs::item_size_
		mov edx, eax;
		imul edx, ebx;
		mov eax, ecx;
		call item_total_size;
		add edx, eax;
		cmp edx, ebp;
		jle end;
		mov eax, -6; //TODO: Switch this to a lower number, and add custom error messages.
		pop ebp;
		jmp ItemAddMultiFail;
end:
		pop ebp;
		jmp ItemAddMultiRet;
	}
}

static __declspec(naked) void ItemAddMultiHook2() {
	__asm {
		cmp eax, edi;
		jl fail;
		jmp ItemAddMultiHook1;
fail:
		mov eax, -6;
		jmp ItemAddMultiFail;
	}
}

static const DWORD BarterAttemptTransactionHook1Fail = 0x474C81;
static const DWORD BarterAttemptTransactionHook1End = 0x474CA8;
static __declspec(naked) void BarterAttemptTransactionHook1() {
	__asm {
		cmp eax, edx;
		jg fail;
		mov eax, edi;
		call CritterCheck;
		test eax, eax;
		jz end;
		mov edx, eax;
		mov eax, edi;
		call item_total_size;
		sub edx, eax;
		mov eax, ebp;
		call item_total_size;
		cmp eax, edx;
		jle end;
fail:
		mov esi, 0x1f;
		jmp BarterAttemptTransactionHook1Fail;
end:
		jmp BarterAttemptTransactionHook1End;
	}
}

static const DWORD BarterAttemptTransactionHook2Fail = 0x474CD8;
static const DWORD BarterAttemptTransactionHook2End = 0x474D01;
static __declspec(naked) void BarterAttemptTransactionHook2() {
	__asm {
		cmp eax, edx;
		jg fail;
		mov eax, ebx;
		call CritterCheck;
		test eax, eax;
		jz end;
		mov edx, eax;
		mov eax, ebx;
		call item_total_size;
		sub edx, eax;
		mov eax, esi;
		call item_total_size;
		cmp eax, edx;
		jle end;
fail:
		mov ecx, 0x20;
		jmp BarterAttemptTransactionHook2Fail;
end:
		jmp BarterAttemptTransactionHook2End;
	}
}

static char SizeStr[16];
static char InvenFmt[32];
static const char* InvenFmt1 = "%s %d/%d  %s %d/%d";
static const char* InvenFmt2 = "%s %d/%d";

static const char* _stdcall GetInvenMsg() {
	const char* tmp = fo::MessageSearch(&fo::var::inventry_message_file, 35);
	if (!tmp) return "S:";
	else return tmp;
}

static void _stdcall strcpy_wrapper(char* buf, const char* str) {
	strcpy(buf, str);
}

static const DWORD DisplayStatsEnd = 0x4725E5;
static __declspec(naked) void DisplayStatsHook() {
	__asm {
		call CritterCheck;
		jz nolimit;
		push eax;
		mov eax, ds:[FO_VAR_stack];
		push ecx;
		push InvenFmt1;
		push offset InvenFmt;
		call strcpy_wrapper;
		pop ecx;
		mov eax, ds:[FO_VAR_stack];
		call item_total_size;
		push eax;
		push ecx;
		call GetInvenMsg;
		pop ecx;
		push eax;
		jmp end;
nolimit:
		push ecx;
		push InvenFmt2;
		push offset InvenFmt;
		call strcpy_wrapper;
		pop ecx;
		push eax;
		push eax;
		push eax;
end:
		mov eax, ds:[FO_VAR_stack];
		mov edx, 0xc;
		jmp DisplayStatsEnd;
	}
}

static char SizeMsgBuf[32];
static const char* _stdcall FmtSizeMsg(int size) {
	if(size==1) {
		const char* tmp = fo::MessageSearch(&fo::var::proto_main_msg_file, 543);
		if(!tmp) strcpy(SizeMsgBuf, "It occupies 1 unit.");
		else sprintf(SizeMsgBuf, tmp, size);
	} else {
		const char* tmp = fo::MessageSearch(&fo::var::proto_main_msg_file, 542);
		if(!tmp) sprintf(SizeMsgBuf, "It occupies %d units.", size);
		else sprintf(SizeMsgBuf, tmp, size);
	}
	return SizeMsgBuf;
}

static __declspec(naked) void InvenObjExamineFuncHook() {
	__asm {
		call fo::funcoffs::inven_display_msg_
		push edx;
		push ecx;
		mov eax, esi;
		call fo::funcoffs::item_size_
		push eax;
		call FmtSizeMsg;
		pop ecx;
		pop edx;
		call fo::funcoffs::inven_display_msg_
		retn;
	}
}

static std::string superStimMsg;
static int _stdcall SuperStimFix2(fo::GameObject* item, fo::GameObject* target) {
	if (!item || !target) return 0;
	DWORD itm_pid = item->protoId, target_pid = target->protoId;
	if ((target_pid & 0xff000000) != 0x01000000) return 0;
	if ((itm_pid & 0xff000000) != 0) return 0;
	if ((itm_pid & 0xffffff) != 144) return 0;
	DWORD curr_hp, max_hp;
	curr_hp = fo::func::stat_level(target, fo::STAT_current_hp);
	max_hp = fo::func::stat_level(target, fo::STAT_max_hit_points);
	if (curr_hp < max_hp) return 0;
	fo::func::display_print(superStimMsg.c_str());
	return 1;
}

static const DWORD UseItemHookRet = 0x49C3D3;
static void __declspec(naked) SuperStimFix() {
	__asm {
		push eax;
		push ecx;
		push edx;

		push edx;
		push ebx;
		call SuperStimFix2;
		pop edx;
		pop ecx;
		test eax, eax;
		jz end;
		pop eax;
		xor eax, eax;
		retn;
end:
		pop eax;
		push ecx;
		push esi;
		push edi;
		push ebp;
		sub esp, 0x14;
		jmp UseItemHookRet;
	}
}

static int invenapcost;
static char invenapqpreduction;
void _stdcall SetInvenApCost(int a) {
	invenapcost = a;
}
static const DWORD inven_ap_cost_hook_ret = 0x46E816;
static void __declspec(naked) inven_ap_cost_hook() {
	_asm {
		movzx ebx, byte ptr invenapqpreduction;
		mul bl;
		mov edx, invenapcost;
		sub edx, eax;
		mov eax, edx;
		jmp inven_ap_cost_hook_ret;
	}
}

static const DWORD add_check_for_item_ammo_cost_back = 0x4266EE;
// adds check for weapons which require more than 1 ammo for single shot (super cattle prod & mega power fist)
static void __declspec(naked) add_check_for_item_ammo_cost() {
	__asm {
		push    edx
		push    ebx
		sub     esp, 4
		call    fo::funcoffs::item_w_curr_ammo_
		mov     ebx, eax
		mov     eax, ecx // weapon
		mov     edx, esp
		mov     dword ptr [esp], 1
		pushad
		push    1 // hook type
		call    AmmoCostHookWrapper
		add     esp, 4
		popad
		mov     eax, [esp]
		cmp     eax, ebx
		jle     enoughammo
		xor     eax, eax // this will force "Out of ammo"
		jmp     end
enoughammo:
		mov     eax, 1 // this will force success
end:
		add     esp, 4
		pop     ebx
		pop     edx
		jmp     add_check_for_item_ammo_cost_back; // jump back
	}
}

static const DWORD divide_burst_rounds_by_ammo_cost_back = 0x4234B9;
static void __declspec(naked) divide_burst_rounds_by_ammo_cost() {
	__asm {
		// ecx - current ammo, eax - burst rounds; need to set ebp
		push edx
		sub     esp, 4
		mov     ebp, eax
		mov     eax, edx // weapon
		mov     dword ptr [esp], 1
		mov     edx, esp // *rounds
		pushad
		push    2
		call    AmmoCostHookWrapper
		add     esp, 4
		popad
		mov     edx, 0
		mov     eax, ebp // rounds in burst
		imul    dword ptr [esp] // so much ammo is required for this burst
		cmp     eax, ecx
		jle     skip
		mov     eax, ecx // if more than current ammo, set it to current
skip:
		idiv    dword ptr [esp] // divide back to get proper number of rounds for damage calculations
		mov     ebp, eax
		add     esp, 4
		pop edx
		// end overwriten code
		jmp     divide_burst_rounds_by_ammo_cost_back; // jump back
	}
}

static void __declspec(naked) SetDefaultAmmo() {
	using namespace fo;
	__asm {
		push    eax
		push    ebx
		push    edx
		xchg    eax, edx
		mov     ebx, eax
		call    fo::funcoffs::item_get_type_
		cmp     eax, item_type_weapon // is it item_type_weapon?
		jne     end // no
		cmp     dword ptr [ebx+0x3C], 0 // is there any ammo in the weapon?
		jne     end // yes
		sub     esp, 4
		mov     edx, esp
		mov     eax, [ebx+0x64] // eax = weapon pid
		call    fo::funcoffs::proto_ptr_
		mov     edx, [esp]
		mov     eax, [edx+0x5C] // eax = default ammo pid
		mov     [ebx+0x40], eax // set current ammo proto
		add     esp, 4
end:
		pop     edx
		pop     ebx
		pop     eax
		retn
	}
}

static const DWORD inven_action_cursor_hack_End = 0x4736CB;
static void __declspec(naked) inven_action_cursor_hack() {
	__asm {
		mov     edx, [esp+0x1C]
		call    SetDefaultAmmo
		cmp     dword ptr [esp+0x18], 0
		jmp     inven_action_cursor_hack_End
	}
}

static void __declspec(naked) item_add_mult_hook() {
	__asm {
		call    SetDefaultAmmo
		jmp     fo::funcoffs::item_add_force_
	}
}

static void __declspec(naked) inven_pickup_hook() {
	__asm {
		mov  eax, ds:[FO_VAR_i_wid]
		call fo::funcoffs::GNW_find_
		mov  ebx, [eax+0x8+0x0]                   // ebx = _i_wid.rect.x
		mov  ecx, [eax+0x8+0x4]                   // ecx = _i_wid.rect.y
		mov  eax, 176
		add  eax, ebx                             // x_start
		add  ebx, 176+60                          // x_end
		mov  edx, 37
		add  edx, ecx                             // y_start
		add  ecx, 37+100                          // y_end
		call fo::funcoffs::mouse_click_in_
		test eax, eax
		jz   end
		mov  edx, ds:[FO_VAR_curr_stack]
		test edx, edx
		jnz  end
		cmp  edi, 1006                            // Hands?
		jae  skip                                 // Yes
skip:
		xor  eax, eax
end:
		retn
	}
}

static void __declspec(naked) loot_container_hack2() {
	__asm {
		cmp  esi, 0x150                           // source_down
		je   scroll
		cmp  esi, 0x148                           // source_up
		jne  end
scroll:
		push edx
		push ecx
		push ebx
		mov  eax, ds:[FO_VAR_i_wid]
		call fo::funcoffs::GNW_find_
		mov  ebx, [eax+0x8+0x0]                   // ebx = _i_wid.rect.x
		mov  ecx, [eax+0x8+0x4]                   // ecx = _i_wid.rect.y
		mov  eax, 297
		add  eax, ebx                             // x_start
		add  ebx, 297+64                          // x_end
		mov  edx, 37
		add  edx, ecx                             // y_start
		add  ecx, 37+6*48                         // y_end
		call fo::funcoffs::mouse_click_in_
		pop  ebx
		pop  ecx
		pop  edx
		test eax, eax
		jz   end
		cmp  esi, 0x150                           // source_down
		je   targetDown
		mov  esi, 0x18D                           // target_up
		jmp  end
targetDown:
		mov  esi, 0x191                           // target_down
end:
		mov  eax, ds:[FO_VAR_curr_stack]
		retn
	}
}

static void __declspec(naked) barter_inventory_hack2() {
	__asm {
		push edx
		push ecx
		push ebx
		xchg esi, eax
		cmp  esi, 0x150                           // source_down
		je   scroll
		cmp  esi, 0x148                           // source_up
		jne  end
scroll:
		mov  eax, ds:[FO_VAR_i_wid]
		call fo::funcoffs::GNW_find_
		mov  ebx, [eax+0x8+0x0]                   // ebx = _i_wid.rect.x
		mov  ecx, [eax+0x8+0x4]                   // ecx = _i_wid.rect.y
		push ebx
		push ecx
		mov  eax, 395
		add  eax, ebx                             // x_start
		add  ebx, 395+64                          // x_end
		mov  edx, 35
		add  edx, ecx                             // y_start
		add  ecx, 35+3*48                         // y_end
		call fo::funcoffs::mouse_click_in_
		pop  ecx
		pop  ebx
		test eax, eax
		jz   notTargetScroll
		cmp  esi, 0x150                           // source_down
		je   targetDown
		mov  esi, 0x18D                           // target_up
		jmp  end
targetDown:
		mov  esi, 0x191                           // target_down
		jmp  end
notTargetScroll:
		push ebx
		push ecx
		mov  eax, 250
		add  eax, ebx                             // x_start
		add  ebx, 250+64                          // x_end
		mov  edx, 20
		add  edx, ecx                             // y_start
		add  ecx, 20+3*48                         // y_end
		call fo::funcoffs::mouse_click_in_
		pop  ecx
		pop  ebx
		test eax, eax
		jz   notTargetBarter
		cmp  esi, 0x150                           // source_down
		je   barterTargetDown
		mov  esi, 0x184                           // target_barter_up
		jmp  end
barterTargetDown:
		mov  esi, 0x176                           // target_barter_down
		jmp  end
notTargetBarter:
		mov  eax, 165
		add  eax, ebx                             // x_start
		add  ebx, 165+64                          // x_end
		mov  edx, 20
		add  edx, ecx                             // y_start
		add  ecx, 20+3*48                         // y_end
		call fo::funcoffs::mouse_click_in_
		test eax, eax
		jz   end
		cmp  esi, 0x150                           // source_down
		je   barterSourceDown
		mov  esi, 0x149                           // source_barter_up
		jmp  end
barterSourceDown:
		mov  esi, 0x151                           // source_barter_down
end:
		pop  ebx
		pop  ecx
		pop  edx
		mov  eax, esi
		cmp  eax, 0x11
		retn
	}
}

int __stdcall ItemCountFixStdcall(fo::GameObject* who, fo::GameObject* item) {
	int count = 0;
	for (int i = 0; i < who->invenSize; i++) {
		auto tableItem = &who->invenTable[i];
		if (tableItem->object == item) {
			count += tableItem->count;
		} else if (fo::func::item_get_type(tableItem->object) == fo::item_type_container) {
			count += ItemCountFixStdcall(tableItem->object, item);
		}
	}
	return count;
}

void __declspec(naked) ItemCountFix() {
	__asm {
		push ebx;
		push ecx;
		push edx; // save state
		push edx; // item
		push eax; // container-object
		call ItemCountFixStdcall;
		pop edx;
		pop ecx;
		pop ebx; // restore
		retn;
	}
}

// reimplementation of adjust_fid engine function
// Differences from vanilla:
// - doesn't use art_vault_guy_num as default art, uses current critter FID instead
// - invokes onAdjustFid delegate that allows to hook into FID calculation
DWORD __stdcall adjust_fid_replacement2() {
	using namespace fo;

	DWORD fid;
	if ((var::inven_dude->artFid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER) {
		DWORD frameNum;
		DWORD weaponAnimCode = 0;
		if (PartyControl::IsNpcControlled()) {
			// if NPC is under control, use current FID of critter
			frameNum = var::inven_dude->artFid & 0xFFF;
		} else {
			// vanilla logic:
			frameNum = var::art_vault_guy_num;
			auto critterPro = GetProto(var::inven_pid);
			if (critterPro != nullptr) {
				frameNum = critterPro->fid & 0xFFF;
			}
			if (var::i_worn != nullptr) {
				auto armorPro = GetProto(var::i_worn->protoId);
				DWORD armorFrameNum = func::stat_level(var::inven_dude, STAT_gender) == GENDER_FEMALE
					? armorPro->item.armor.femaleFrameNum
					: armorPro->item.armor.maleFrameNum;

				if (armorFrameNum != -1) {
					frameNum = armorFrameNum;
				}
			}
		}
		auto itemInHand = func::intface_is_item_right_hand()
			? var::i_rhand
			: var::i_lhand;

		if (itemInHand != nullptr) {
			auto itemPro = GetProto(itemInHand->protoId);
			if (itemPro->item.type == item_type_weapon) {
				weaponAnimCode = itemPro->item.weapon.animationCode;
			}
		}
		fid = func::art_id(OBJ_TYPE_CRITTER, frameNum, 0, weaponAnimCode, 0);
	} else {
		fid = var::inven_dude->artFid;
	}
	var::i_fid = fid;
	onAdjustFid.invoke(fid);
	return var::i_fid;
}

void __declspec(naked) adjust_fid_replacement() {
	__asm {
		pushad;
		call adjust_fid_replacement2;
		popad;
		mov eax, [FO_VAR_i_fid];
		retn;
	}
}

void InventoryReset() {
	invenapcost = GetConfigInt("Misc", "InventoryApCost", 4);
}

void Inventory::init() {
	OnKeyPressed() += InventoryKeyPressedHook;
	LoadGameHook::OnGameReset() += InventoryReset;

	MakeJump(fo::funcoffs::adjust_fid_, adjust_fid_replacement);

	mode = GetConfigInt("Misc", "CritterInvSizeLimitMode", 0);
	invenapcost = GetConfigInt("Misc", "InventoryApCost", 4);
	invenapqpreduction = GetConfigInt("Misc", "QuickPocketsApCostReduction", 2);
	MakeJump(0x46E80B, inven_ap_cost_hook);
	if (mode > 7) {
		mode = 0;
	}
	if (mode >= 4) {
		mode -= 4;
		SafeWrite8(0x477EB3, 0xeb);
	}
	if (mode) {
		maxItemSize = GetConfigInt("Misc", "CritterInvSizeLimit", 100);

		//Check item_add_multi (picking stuff from the floor, etc.)
		HookCall(0x4771BD, &ItemAddMultiHook1);
		MakeJump(0x47726D, ItemAddMultiHook2);
		MakeJump(0x42E688, CritterIsOverloadedHook);

		//Check capacity of player and barteree when bartering
		MakeJump(0x474C78, BarterAttemptTransactionHook1);
		MakeJump(0x474CCF, BarterAttemptTransactionHook2);

		//Display total weight on the inventory screen
		SafeWrite32(0x4725FF, (DWORD)&InvenFmt);
		MakeJump(0x4725E0, DisplayStatsHook);
		SafeWrite8(0x47260F, 0x20);
		SafeWrite32(0x4725F9, 0x9c + 0xc);
		SafeWrite8(0x472606, 0x10 + 0xc);
		SafeWrite32(0x472632, 150);
		SafeWrite8(0x472638, 0);

		//Display item weight when examining
		HookCall(0x472FFE, &InvenObjExamineFuncHook);
	}

	if (GetConfigInt("Misc", "SuperStimExploitFix", 0)) {
		superStimMsg = Translate("sfall", "SuperStimExploitMsg", "You cannot use a super stim on someone who is not injured!");
		MakeJump(0x49C3CC, SuperStimFix);
	}

	if (GetConfigInt("Misc", "CheckWeaponAmmoCost", 0)) {
		MakeJump(0x4266E9, add_check_for_item_ammo_cost);
		MakeJump(0x4234B3, divide_burst_rounds_by_ammo_cost);
	}

	reloadWeaponKey = GetConfigInt("Input", "ReloadWeaponKey", 0);

	if (GetConfigInt("Misc", "StackEmptyWeapons", 0)) {
		MakeJump(0x4736C6, inven_action_cursor_hack);
		HookCall(0x4772AA, &item_add_mult_hook);
	}

	// Do not call the 'Move Items' window when using drap and drop to reload weapons in the inventory
	int ReloadReserve = GetConfigInt("Misc", "ReloadReserve", 1);
	if (ReloadReserve >= 0) {
		SafeWrite32(0x47655F, ReloadReserve);     // mov  eax, ReloadReserve
		SafeWrite32(0x476563, 0x097EC139);        // cmp  ecx, eax; jle  0x476570
		SafeWrite16(0x476567, 0xC129);            // sub  ecx, eax
		SafeWrite8(0x476569, 0x91);               // xchg ecx, eax
	};

	// Move items out of bag/backpack and back into the main inventory list by dragging them to character's image
	// (similar to Fallout 1 behavior)
	HookCall(0x471457, &inven_pickup_hook);

	// Move items to player's main inventory instead of the opened bag/backpack when confirming a trade
	SafeWrite32(0x475CF2, FO_VAR_stack);

	// Enable mouse scroll control in barter and loot screens when the cursor is hovering over other lists
	if (useScrollWheel) {
		MakeCall(0x473E66, loot_container_hack2);
		MakeCall(0x4759F1, barter_inventory_hack2);
		fo::var::max = 100;
	};

	// Fix item_count function returning incorrect value when there is a container-item inside
	MakeJump(0x47808C, ItemCountFix); // replacing item_count_ function
}

Delegate<DWORD>& Inventory::OnAdjustFid() {
	return onAdjustFid;
}



}
