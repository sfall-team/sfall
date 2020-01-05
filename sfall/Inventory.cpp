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

#include "main.h"
#include "FalloutEngine.h"
#include "input.h"
#include "Inventory.h"
#include "LoadGameHook.h"

static DWORD sizeLimitMode;
static DWORD invSizeMaxLimit;

static DWORD reloadWeaponKey = 0;
static DWORD itemFastMoveKey = 0;
static DWORD skipFromContainer = 0;

DWORD& GetActiveItemMode() {
	return ptr_itemButtonItems[(*ptr_itemCurrentItem * 6) + 4];
}

TGameObj* GetActiveItem() {
	return (TGameObj*)ptr_itemButtonItems[*ptr_itemCurrentItem * 6];
}

void InventoryKeyPressedHook(DWORD dxKey, bool pressed, DWORD vKey) {
	if (pressed && reloadWeaponKey && dxKey == reloadWeaponKey && IsMapLoaded() && (GetLoopFlags() & ~(COMBAT | PCOMBAT)) == 0) {
		DWORD maxAmmo, curAmmo;
		TGameObj* item = GetActiveItem();
		__asm {
			mov eax, item;
			call item_w_max_ammo_;
			mov maxAmmo, eax;
			mov eax, item;
			call item_w_curr_ammo_;
			mov curAmmo, eax;
		}
		if (maxAmmo != curAmmo) {
			DWORD &currentMode = GetActiveItemMode();
			DWORD previusMode = currentMode;
			currentMode = 5; // reload mode
			__asm {
				call intface_use_item_;
			}
			if (previusMode != 5) {
				// return to previous active item mode (if it wasn't "reload")
				currentMode = previusMode - 1;
				if (currentMode < 0)
					currentMode = 4;
				__asm {
					call intface_toggle_item_state_;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////

DWORD __stdcall sf_item_total_size(TGameObj* critter) {
	int totalSize;
	__asm {
		mov  eax, critter;
		call item_c_curr_size_;
		mov  totalSize, eax;
	}

	if (((critter->artFid >> 24) & 0x0F) == OBJ_TYPE_CRITTER) {
		TGameObj* item = InvenRightHand(critter);
		if (item && !(item->flags & 0x2000000)) { // ObjectFlag Right_Hand
			totalSize += ItemSize(item);
		}

		TGameObj* itemL = InvenLeftHand(critter);
		if (itemL && item != itemL && !(itemL->flags & 0x1000000)) { // ObjectFlag Left_Hand
			totalSize += ItemSize(itemL);
		}

		item = InvenWorn(critter);
		if (item && !(item->flags & 0x4000000)) { // ObjectFlag Worn
			totalSize += ItemSize(item);
		}
	}
	return totalSize;
}

static int __stdcall CritterGetMaxSize(TGameObj* critter) {
	if (critter == *ptr_obj_dude) return invSizeMaxLimit;

	if (sizeLimitMode != 3) { // selected mode 1 or 2
		if (!(sizeLimitMode & 2) || !(IsPartyMember(critter))) return 0; // if mode 2 is selected, check this party member, otherwise 0
	}

	int statSize = 0;
	char* proto = GetProtoPtr(critter->pid);
	if (proto != nullptr) {
		statSize = *(int*)(proto + 76) + *(int*)(proto + 216); // The unused stat in the base + extra block
	}
	return (statSize > 0) ? statSize : 100; // 100 - default value, for all critters if not set stats
}

static __declspec(naked) void critterIsOverloaded_hack() {
	__asm {
		and  eax, 0xFF;
		jnz  end;
		push ecx;
		push ebx;                // critter
		call CritterGetMaxSize;
		test eax, eax;
		jz   skip;
		push ebx;
		mov  ebx, eax;           // ebx = MaxSize
		call sf_item_total_size;
		cmp  eax, ebx;
		setg al;                 // if CurrSize > MaxSize
		and  eax, 0xFF;
skip:
		pop  ecx;
end:
		retn;
	}
}

static int __fastcall CanAddedItems(TGameObj* critter, TGameObj* item, int count) {
	int sizeMax = CritterGetMaxSize(critter);
	if (sizeMax > 0) {
		int itemsSize = ItemSize(item) * count;
		if (itemsSize + (int)sf_item_total_size(critter) > sizeMax) return -6; // TODO: Switch this to a lower number, and add custom error messages.
	}
	return 0;
}

static const DWORD ItemAddMultRet  = 0x4772A6;
static const DWORD ItemAddMultFail = 0x4771C7;
static __declspec(naked) void item_add_mult_hack() {
	__asm {
		push ecx;
		push ebx;           // items count
		mov  edx, esi;      // item
		call CanAddedItems; // ecx - source;
		pop  ecx;
		test eax, eax;
		jnz  fail;
		jmp  ItemAddMultRet;
fail:
		jmp  ItemAddMultFail;
	}
}

static __declspec(naked) void item_add_mult_hack_container() {
	__asm {
		/* cmp eax, edi */
		mov  eax, -6;
		jl   fail;
		//-------
		push ecx;
		push ebx;           // items count
		mov  edx, esi;      // item
		call CanAddedItems; // ecx - source;
		pop  ecx;
		test eax, eax;
		jnz  fail;
		jmp  ItemAddMultRet;
fail:
		jmp  ItemAddMultFail;
	}
}

static int __fastcall BarterAttemptTransaction(TGameObj* critter, TGameObj* table) {
	int size = CritterGetMaxSize(critter);
	if (size == 0) return 1;

	int sizeTable = sf_item_total_size(table);
	if (sizeTable == 0) return 1;

	size -= sf_item_total_size(critter);
	return (sizeTable <= size) ? 1 : 0;
}

static const DWORD BarterAttemptTransactionPCFail = 0x474C81;
static const DWORD BarterAttemptTransactionPCRet  = 0x474CA8;
static __declspec(naked) void barter_attempt_transaction_hack_pc() {
	__asm {
		/* cmp  eax, edx */
		jg   fail;    // if there's no available weight
		//------
		mov  ecx, edi;                   // source (pc)
		mov  edx, ebp;                   // npc table
		call BarterAttemptTransaction;
		test eax, eax;
		jz   fail;
		jmp  BarterAttemptTransactionPCRet;
fail:
		mov  esi, 31;
		jmp  BarterAttemptTransactionPCFail;
	}
}

static const DWORD BarterAttemptTransactionPMFail = 0x474CD8;
static const DWORD BarterAttemptTransactionPMRet  = 0x474D01;
static __declspec(naked) void barter_attempt_transaction_hack_pm() {
	__asm {
		/* cmp  eax, edx */
		jg   fail;    // if there's no available weight
		//------
		mov  ecx, ebx;                  // target (npc)
		mov  edx, esi;                  // pc table
		call BarterAttemptTransaction;
		test eax, eax;
		jz   fail;
		jmp  BarterAttemptTransactionPMRet;
fail:
		mov  ecx, 32;
		jmp  BarterAttemptTransactionPMFail;
	}
}

static __declspec(naked) void loot_container_hook_btn() {
	__asm {
		push ecx;
		push edx;                            // source current weight
		mov  edx, eax;                       // target
		mov  ecx, [esp + 0x150 - 0x1C + 12]; // source
		call BarterAttemptTransaction;
		pop  edx;
		pop  ecx;
		test eax, eax;
		jz   fail;
		mov  eax, ebp;                       // target
		jmp  item_total_weight_;
fail:
		mov  eax, edx;
		inc  eax;                            // weight + 1
		retn;
	}
}

static char InvenFmt[32];
static const char* InvenFmt1 = "%s %d/%d %s %d/%d";
static const char* InvenFmt2 = "%s %d/%d";
static const char* InvenFmt3 = "%d/%d | %d/%d";

static void __cdecl DisplaySizeStats(TGameObj* critter, const char* &message, DWORD &size, DWORD &sizeMax) {
	int limitMax = CritterGetMaxSize(critter);
	if (limitMax == 0) {
		strcpy(InvenFmt, InvenFmt2); // default fmt
		return;
	}

	sizeMax = limitMax;
	size = sf_item_total_size(critter);

	const char* msg = MsgSearch(ptr_inventry_message_file, 35);
	message = (msg != nullptr) ? msg : "";

	strcpy(InvenFmt, InvenFmt1);
}

static const DWORD DisplayStatsRet = 0x4725E5;
static __declspec(naked) void display_stats_hack() {
	__asm {
		mov  ecx, esp;
		sub  ecx, 4;
		push ecx;   // sizeMax
		sub  ecx, 4;
		push ecx;   // size
		sub  ecx, 4;
		push ecx;   // size message
		push eax;   // critter
		call DisplaySizeStats;
		pop  eax;
		mov  edx, STAT_carry_amt;
		jmp  DisplayStatsRet;
	}
}

static char SizeMsgBuf[32];
static const char* _stdcall SizeInfoMessage(TGameObj* item) {
	int size = ItemSize(item);
	if (size == 1) {
		const char* message = MsgSearch(ptr_proto_main_msg_file, 543);
		if (message == nullptr)
			strcpy(SizeMsgBuf, "It occupies 1 unit.");
		else
			strncpy_s(SizeMsgBuf, message, _TRUNCATE);
	} else {
		const char* message = MsgSearch(ptr_proto_main_msg_file, 542);
		if (message == nullptr)
			sprintf(SizeMsgBuf, "It occupies %d units.", size);
		else
			_snprintf_s(SizeMsgBuf, _TRUNCATE, message, size);
	}
	return SizeMsgBuf;
}

static __declspec(naked) void inven_obj_examine_func_hook() {
	__asm {
		call inven_display_msg_;
		push edx;
		push ecx;
		push esi;
		call SizeInfoMessage;
		pop  ecx;
		pop  edx;
		jmp  inven_display_msg_;
	}
}

static const DWORD ControlUpdateInfoRet = 0x44912A;
static void __declspec(naked) gdControlUpdateInfo_hack() {
	__asm {
		mov  ebx, eax;
		push eax;               // critter
		call CritterGetMaxSize;
		push eax;               // sizeMax
		push ebx;
		call sf_item_total_size;
		push eax;               // size
		mov  eax, ebx;
		mov  edx, STAT_carry_amt;
		jmp  ControlUpdateInfoRet;
	}
}
/////////////////////////////////////////////////////////////////

static char SuperStimMsg[128];
static int __fastcall SuperStimFix(TGameObj* item, TGameObj* target) {
	if (item->pid != PID_SUPER_STIMPAK || !target || (target->pid & 0xFF000000) != (OBJ_TYPE_CRITTER << 24)) { // 0x01000000
		return 0;
	}

	long curr_hp = StatLevel(target, STAT_current_hp);
	long max_hp = StatLevel(target, STAT_max_hit_points);
	if (curr_hp < max_hp) return 0;

	DisplayConsoleMessage(SuperStimMsg);
	return -1;
}

static const DWORD protinst_use_item_on_Ret = 0x49C5F4;
static void __declspec(naked) protinst_use_item_on_hack() {
	__asm {
		push ecx;
		mov  ecx, ebx;     // ecx - item
		call SuperStimFix; // edx - target
		pop  ecx;
		test eax, eax;
		jnz  end;
		mov  ebp, -1;      // overwritten engine code
		retn;
end:
		add  esp, 4;       // destroy ret
		jmp  protinst_use_item_on_Ret; // exit
	}
}

static void __declspec(naked) SetDefaultAmmo() {
	__asm {
		push ecx;
		mov  ecx, edx;                     // ecx = item
		mov  eax, edx;
		call item_get_type_;
		cmp  eax, item_type_weapon;        // is it item_type_weapon?
		jne  end;                          // no
		cmp  dword ptr [ecx + 0x3C], 0;    // is there any ammo in the weapon?
		jne  end;                          // yes
		sub  esp, 4;
		mov  edx, esp;
		mov  eax, [ecx + 0x64];            // eax = weapon pid
		call proto_ptr_;
		mov  edx, [esp];
		mov  eax, [edx + 0x5C];            // eax = default ammo pid
		mov  [ecx + 0x40], eax;            // set current ammo proto
		add  esp, 4;
end:
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) inven_action_cursor_hack() {
	__asm {
		mov  edx, [esp + 0x6C - 0x50 + 4];         // source_item
		call SetDefaultAmmo;
		cmp  dword ptr [esp + 0x6C - 0x54 + 4], 0; // overwritten engine code
		retn;
	}
}

static void __declspec(naked) item_add_mult_hook() {
	__asm {
		push edx;
		call SetDefaultAmmo;
		pop  edx;
		mov  eax, ecx;    // restore
		jmp  item_add_force_;
	}
}

static void __declspec(naked) inven_pickup_hack() {
	__asm {
		mov  eax, ds:[_i_wid];
		call GNW_find_;
		mov  ebx, [eax + 8 + 0];                  // ebx = _i_wid.rect.x
		mov  ecx, [eax + 8 + 4];                  // ecx = _i_wid.rect.y
		lea  eax, [ebx + 176];                    // x_start
		add  ebx, 176 + 60;                       // x_end
		lea  edx, [ecx + 37];                     // y_start
		add  ecx, 37 + 100;                       // y_end
		retn;
	}
}

static void __declspec(naked) loot_container_hack_scroll() {
	__asm {
		cmp  esi, 0x150;                          // source_down
		je   scroll;
		cmp  esi, 0x148;                          // source_up
		jne  end;
scroll:
		mov  eax, ds:[_i_wid];
		call GNW_find_;
		push edx;
		push ecx;
		push ebx;
		mov  ebx, [eax + 8 + 0];                  // ebx = _i_wid.rect.x
		mov  ecx, [eax + 8 + 4];                  // ecx = _i_wid.rect.y
		lea  eax, [ebx + 297];                    // x_start
		add  ebx, 297 + 64;                       // x_end
		lea  edx, [ecx + 37];                     // y_start
		add  ecx, 37 + 6 * 48;                    // y_end
		call mouse_click_in_;
		pop  ebx;
		pop  ecx;
		pop  edx;
		test eax, eax;
		jz   end;
		cmp  esi, 0x150;                          // source_down
		je   targetDown;
		mov  esi, 0x18D;                          // target_up
		jmp  end;
targetDown:
		mov  esi, 0x191;                          // target_down
end:
		mov  eax, ds:[_curr_stack];
		retn;
	}
}

static void __declspec(naked) barter_inventory_hack_scroll() {
	__asm {
		mov  esi, eax;
		cmp  esi, 0x150;                          // source_down
		je   scroll;
		cmp  esi, 0x148;                          // source_up
		jne  skip;
scroll:
		mov  eax, ds:[_i_wid];
		call GNW_find_;
		push edx;
		push ecx;
		push ebx;
		push ebp;
		push edi;
		mov  ebp, [eax + 8 + 0];
		mov  edi, [eax + 8 + 4];
		mov  ebx, ebp;                            // ebx = _i_wid.rect.x
		mov  ecx, edi;                            // ecx = _i_wid.rect.y
		lea  eax, [ebp + 395];                    // x_start
		add  ebx, 395 + 64;                       // x_end
		lea  edx, [edi + 35];                     // y_start
		add  ecx, 35 + 3 * 48;                    // y_end
		call mouse_click_in_;
		test eax, eax;
		jz   notTargetScroll;
		cmp  esi, 0x150;                          // source_down
		je   targetDown;
		mov  esi, 0x18D;                          // target_up
		jmp  end;
targetDown:
		mov  esi, 0x191;                          // target_down
		jmp  end;
notTargetScroll:
		mov  ebx, ebp;
		mov  ecx, edi;
		lea  eax, [ebp + 250];                    // x_start
		add  ebx, 250 + 64;                       // x_end
		lea  edx, [edi + 20];                     // y_start
		add  ecx, 20 + 3 * 48;                    // y_end
		call mouse_click_in_;
		test eax, eax;
		jz   notTargetBarter;
		cmp  esi, 0x150;                          // source_down
		je   barterTargetDown;
		mov  esi, 0x184;                          // target_barter_up
		jmp  end;
barterTargetDown:
		mov  esi, 0x176;                          // target_barter_down
		jmp  end;
notTargetBarter:
		mov  ebx, ebp;
		mov  ecx, edi;
		lea  eax, [ebp + 165];                    // x_start
		add  ebx, 165 + 64;                       // x_end
		lea  edx, [edi + 20];                     // y_start
		add  ecx, 20 + 3 * 48;                    // y_end
		call mouse_click_in_;
		test eax, eax;
		jz   end;
		cmp  esi, 0x150;                          // source_down
		je   barterSourceDown;
		mov  esi, 0x149;                          // source_barter_up
		jmp  end;
barterSourceDown:
		mov  esi, 0x151;                          // source_barter_down
end:
		pop  edi;
		pop  ebp;
		pop  ebx;
		pop  ecx;
		pop  edx;
		mov  eax, esi;
skip:
		cmp  eax, 0x11;
		retn;
	}
}

static void __declspec(naked) op_inven_unwield_hook() {
	__asm {
		mov  ecx, [eax + 0x64];
		and  ecx, 0x0F000000;
		cmp  ecx, OBJ_TYPE_CRITTER << 24;
		jne  skip;
		test byte ptr [eax + 0x44], DAM_KNOCKED_OUT;
		jz   skip;
		push 0x505AFC; // "But is already Inactive (Dead/Stunned/Invisible)"
		call debug_printf_;
		add  esp, 4;
end:
		retn;
skip:
		call inven_unwield_;
		// update interface slot
		cmp  ebx, ds:[_obj_dude];
		jne  end;
		xor  eax, eax; // no animate
		mov  ebx, eax;
		dec  ebx;      // modeRight (-1)
		mov  edx, ebx; // modeLeft (-1)
		jmp  intface_update_items_;
	}
}

static void __declspec(naked) op_wield_obj_critter_hook() {
	__asm {
		test byte ptr [eax + 0x44], DAM_KNOCKED_OUT;
		jz   skip;
		mov  eax, -1;
		retn;
skip:
		jmp  inven_wield_;
	}
}

static const DWORD DoMoveTimer_Ret = 0x476920;
static void __declspec(naked) do_move_timer_hook() {
	__asm {
		cmp eax, 4;
		jnz end;
		pushadc;
	}

	KeyDown(itemFastMoveKey); // check pressed

	__asm {
		cmp  skipFromContainer, 0;
		jz   noSkip;
		cmp  dword ptr [esp + 0x14 + 16], 0x474A43;
		jnz  noSkip;
		test eax, eax;
		setz al;
noSkip:
		test eax, eax;  // set if pressed
		popadc;
		jz   end;
		add  esp, 4;    // destroy ret
		jmp  DoMoveTimer_Ret;
end:
		jmp  setup_move_timer_win_;
	}
}

static void __declspec(naked) do_move_timer_hack() {
	__asm {
		mov  ebx, 1;
		call GetLoopFlags;
		test eax, BARTER;
		cmovz ebx, ebp; // set max when not in barter
		retn;
	}
}

static int invenApCost, invenApCostDef;
static char invenApQPReduction;
static const DWORD inven_ap_cost_Ret = 0x46E812;
static void __declspec(naked) inven_ap_cost_hack() {
	_asm {
		mul byte ptr invenApQPReduction;
		mov edx, invenApCost;
		jmp inven_ap_cost_Ret;
	}
}

static bool onlyOnceAP = false;
inline static void ApplyInvenApCostPatch() {
	MakeJump(0x46E80B, inven_ap_cost_hack);
	onlyOnceAP = true;
}

void __fastcall SetInvenApCost(int cost) {
	invenApCost = cost;
	if (!onlyOnceAP) ApplyInvenApCostPatch();
}

long __stdcall GetInvenApCost() {
	long perkLevel = PerkLevel(*ptr_obj_dude, PERK_quick_pockets);
	return invenApCost - (invenApQPReduction * perkLevel);
}

void InventoryReset() {
	invenApCost = invenApCostDef;
}

void InventoryInit() {
	long weightWidth = 135;

	sizeLimitMode = GetConfigInt("Misc", "CritterInvSizeLimitMode", 0);
	if (sizeLimitMode > 0 && sizeLimitMode <= 7) {
		if (sizeLimitMode >= 4) {
			sizeLimitMode -= 4;
			// item_total_weight_ patch
			SafeWrite8(0x477EB3, 0xEB);
			SafeWrite8(0x477EF5, 0);
			SafeWrite8(0x477F11, 0);
			SafeWrite8(0x477F29, 0);
		}
		invSizeMaxLimit = GetConfigInt("Misc", "CritterInvSizeLimit", 100);

		// Check item_add_multi (picking stuff from the floor, etc.)
		HookCall(0x4771BD, item_add_mult_hack); // jle addr
		SafeWrite16(0x47726F, 0x9090);
		MakeJump(0x477271, item_add_mult_hack_container);
		MakeCall(0x42E688, critterIsOverloaded_hack);

		// Check player's capacity when bartering
		SafeWrite16(0x474C7A, 0x9090);
		MakeJump(0x474C7C, barter_attempt_transaction_hack_pc);

		// Check player's capacity when using "Take All" button
		HookCall(0x47410B, loot_container_hook_btn);

		// Display total weight/size on the inventory screen
		MakeJump(0x4725E0, display_stats_hack);
		SafeWrite32(0x4725FF, (DWORD)&InvenFmt);
		SafeWrite8(0x47260F, 0x20);
		SafeWrite32(0x4725F9, 0x9C + 0x0C);
		SafeWrite8(0x472606, 0x10 + 0x0C);
		SafeWrite8(0x472638, 0); // x offset position
		weightWidth = 150;

		// Display item size when examining
		HookCall(0x472FFE, inven_obj_examine_func_hook);

		if (sizeLimitMode > 1) {
			// Check party member's capacity when bartering
			SafeWrite16(0x474CD1, 0x9090);
			MakeJump(0x474CD3, barter_attempt_transaction_hack_pm);

			// Display party member's current/max inventory size on the combat control panel
			MakeJump(0x449125, gdControlUpdateInfo_hack);
			SafeWrite32(0x44913E, (DWORD)InvenFmt3);
			SafeWrite8(0x449145, 0x0C + 0x08);
			SafeWrite8(0x449150, 0x10 + 0x08);
		}
	}
	// Adjust the max text width of the total weight display on the inventory screen
	SafeWrite32(0x472632, weightWidth);

	if (GetConfigInt("Misc", "SuperStimExploitFix", 0)) {
		Translate("sfall", "SuperStimExploitMsg", "You cannot use a super stim on someone who is not injured!", SuperStimMsg);
		MakeCall(0x49C3D9, protinst_use_item_on_hack);
	}

	reloadWeaponKey = GetConfigInt("Input", "ReloadWeaponKey", 0);

	invenApCost = invenApCostDef = GetConfigInt("Misc", "InventoryApCost", 4);
	invenApQPReduction = GetConfigInt("Misc", "QuickPocketsApCostReduction", 2);
	if (invenApCostDef != 4 || invenApQPReduction != 2) {
		ApplyInvenApCostPatch();
	}

	if (GetConfigInt("Misc", "StackEmptyWeapons", 0)) {
		MakeCall(0x4736C6, inven_action_cursor_hack);
		HookCall(0x4772AA, item_add_mult_hook);
	}

	// Do not call the 'Move Items' window when using drag and drop to reload weapons in the inventory
	int ReloadReserve = GetConfigInt("Misc", "ReloadReserve", -1);
	if (ReloadReserve >= 0) {
		SafeWrite32(0x47655F, ReloadReserve);     // mov  eax, ReloadReserve
		SafeWrite32(0x476563, 0x097EC139);        // cmp  ecx, eax; jle  0x476570
		SafeWrite16(0x476567, 0xC129);            // sub  ecx, eax
		SafeWrite8(0x476569, 0x91);               // xchg ecx, eax
	};

	itemFastMoveKey = GetConfigInt("Input", "ItemFastMoveKey", DIK_LCONTROL);
	if (itemFastMoveKey > 0) {
		HookCall(0x476897, do_move_timer_hook);
		// Do not call the 'Move Items' window when taking items from containers or corpses
		skipFromContainer = GetConfigInt("Input", "FastMoveFromContainer", 0);
	}

	if (GetConfigInt("Misc", "ItemCounterDefaultMax", 0)) {
		MakeCall(0x4768A3, do_move_timer_hack);
	}

	// Move items from bag/backpack to the main inventory list by dragging them on the character portrait (similar to Fallout 1 behavior)
	MakeCall(0x471452, inven_pickup_hack);

	// Move items to player's main inventory instead of the opened bag/backpack when confirming a trade
	SafeWrite32(0x475CF2, _stack);

	// Enable mouse scroll control in barter and loot screens when the cursor is hovering over other lists
	if (UseScrollWheel) {
		MakeCall(0x473E66, loot_container_hack_scroll);
		MakeCall(0x4759F1, barter_inventory_hack_scroll);
		*ptr_max = 100;
	};

	// Check the DAM_KNOCKED_OUT flag for wield_obj_critter/inven_unwield script functions
	// Note: the flag is not checked for the metarule(METARULE_INVEN_UNWIELD_WHO, x) function
	HookCall(0x45B0CE, op_inven_unwield_hook); // with fix to update interface slot after unwielding
	HookCall(0x45693C, op_wield_obj_critter_hook);
}
