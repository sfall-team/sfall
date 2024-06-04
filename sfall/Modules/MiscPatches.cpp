/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"
#include "..\Translate.h"

#include "LoadGameHook.h"

#include "MiscPatches.h"

namespace sfall
{

static int idle = -1;

static char mapName[16]       = {};
static char patchName[65]     = {};
static char versionString[65] = {};

static int* scriptDialog = nullptr;

static __declspec(naked) void GNW95_process_message_hack() {
	__asm {
		push idle;
		call Sleep;
		cmp  dword ptr ds:[FO_VAR_GNW95_isActive], 0;
		retn;
	}
}

static __declspec(naked) void WeaponAnimHook() {
	__asm {
		cmp edx, 11;
		je  c11;
		cmp edx, 15;
		je  c15;
		jmp fo::funcoffs::art_get_code_;
c11:
		mov edx, 16;
		jmp fo::funcoffs::art_get_code_;
c15:
		mov edx, 17;
		jmp fo::funcoffs::art_get_code_;
	}
}

static __declspec(naked) void register_object_take_out_hack() {
	using namespace fo::Fields;
	__asm {
		push ecx;
		push eax;
		mov  ecx, edx;                            // ID1
		mov  edx, [eax + rotation];               // cur_rot
		inc  edx;
		push edx;                                 // ID3
		xor  ebx, ebx;                            // ID2
		mov  edx, [eax + artFid];                 // fid
		and  edx, 0xFFF;                          // Index
		xor  eax, eax;
		inc  eax;                                 // Obj_Type CRITTER
		call fo::funcoffs::art_id_;
		mov  edx, eax;
		xor  ebx, ebx;
		dec  ebx;                                 // delay -1
		pop  eax;                                 // critter
		call fo::funcoffs::register_object_change_fid_;
		pop  ecx;
		xor  eax, eax;
		retn;
	}
}

static __declspec(naked) void gdAddOptionStr_hack() {
	static const DWORD gdAddOptionStr_hack_Ret = 0x4458FA;
	__asm {
		mov  ecx, ds:[FO_VAR_gdNumOptions];
		add  ecx, '1';
		push ecx;
		jmp  gdAddOptionStr_hack_Ret;
	}
}

static __declspec(naked) void action_use_skill_on_hook_science() {
	using namespace fo;
	__asm {
		cmp esi, ds:[FO_VAR_obj_dude];
		jne end;
		mov eax, KILL_TYPE_robot;
		retn;
end:
		jmp fo::funcoffs::critter_kill_count_type_;
	}
}

static __declspec(naked) void intface_item_reload_hook() {
	__asm {
		push eax;
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];
		call fo::funcoffs::register_clear_;
		xor  edx, edx;       // ANIM_stand
		xor  ebx, ebx;       // delay (unused)
		lea  eax, [edx + 1]; // RB_UNRESERVED
		call fo::funcoffs::register_begin_;
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];
		call fo::funcoffs::register_object_animate_;
		mov  ecx, dword ptr ds:[FO_VAR_combat_highlight]; // backup setting
		mov  dword ptr ds:[FO_VAR_combat_highlight], eax; // prevent calling combat_outline_off_ (eax is never 2)
		call fo::funcoffs::register_end_;
		mov  dword ptr ds:[FO_VAR_combat_highlight], ecx; // restore setting
		pop  eax;
		jmp  fo::funcoffs::gsound_play_sfx_file_;
	}
}

static __declspec(naked) void automap_hack() {
	static const DWORD ScannerHookRet  = 0x41BC1D;
	static const DWORD ScannerHookFail = 0x41BC65;
	using fo::PID_MOTION_SENSOR;
	__asm {
		mov  eax, ds:[FO_VAR_obj_dude];
		mov  edx, PID_MOTION_SENSOR;
		call fo::funcoffs::inven_pid_is_carried_ptr_;
		test eax, eax;
		jz   fail;
		mov  edx, eax;
		jmp  ScannerHookRet;
fail:
		jmp  ScannerHookFail;
	}
}

static bool __fastcall SeeIsFront(fo::GameObject* source, fo::GameObject* target) {
	long dir = source->rotation - fo::func::tile_dir(source->tile, target->tile);
	if (dir < 0) dir = -dir;
	if (dir == 1 || dir == 5) { // peripheral/side vision, reduce the range for seeing through (3x instead of 5x)
		return (fo::func::obj_dist(source, target) <= (fo::func::stat_level(source, fo::STAT_pe) * 3));
	}
	return (dir == 0); // is directly in front
}

static __declspec(naked) void op_obj_can_see_obj_hook() {
	using namespace fo;
	using namespace Fields;
	__asm {
		mov  edi, [esp + 4]; // buf **outObject
		test ebp, ebp; // check only once
		jz   checkSee;
		xor  ebp, ebp; // for only once
		push edx;
		push eax;
		mov  edx, [edi - 8]; // target
		mov  ecx, eax;       // source
		call SeeIsFront;
		xor  ecx, ecx;
		test al, al;
		pop  eax;
		pop  edx;
		jnz  checkSee; // can see
		// vanilla behavior
		push 0x10;
		push edi;
		call fo::funcoffs::make_straight_path_;
		retn 8;
checkSee:
		push eax;                                  // keep source
		push fo::funcoffs::obj_shoot_blocking_at_; // check hex objects func pointer
		push 0x20;                                 // flags, 0x20 = check ShootThru
		push edi;
		call fo::funcoffs::make_straight_path_func_; // overlapping if len(eax) == 0
		pop  ecx;            // source
		mov  edx, [edi - 8]; // target
		mov  ebx, [edi];     // blocking object
		test ebx, ebx;
		jz   isSee;          // no blocking object
		cmp  ebx, edx;
		jne  checkObj;       // object is not equal to target
		retn 8;
isSee:
		mov  [edi], edx;     // fix for target with ShootThru flag
		retn 8;
checkObj:
		mov  eax, [ebx + protoId];
		shr  eax, 24;
		cmp  eax, OBJ_TYPE_CRITTER;
		je   continue; // see through critter
		retn 8;
continue:
		mov  [edi], ecx;                // outObject - ignore source (for cases of overlapping tiles from multihex critters)
		mov  [edi - 4], ebx;            // replace source with blocking object
		mov  dword ptr [esp], 0x456BAB; // repeat from the blocking object
		retn 8;
	}
}

static DWORD __fastcall GetWeaponSlotMode(DWORD itemPtr, DWORD mode) {
	int slot = (mode > 0) ? 1 : 0;
	fo::ItemButtonItem* itemButton = &fo::var::itemButtonItems[slot];
	if ((DWORD)itemButton->item == itemPtr) {
		int slotMode = itemButton->mode;
		if (slotMode == 3 || slotMode == 4) {
			mode++;
		}
	}
	return mode;
}

static __declspec(naked) void display_stats_hook() {
	__asm {
		push eax;
		push ecx;
		mov  ecx, ds:[esp + edi + 0xA8 + 0xC]; // get itemPtr
		call GetWeaponSlotMode;                // ecx - itemPtr, edx - mode;
		mov  edx, eax;
		pop  ecx;
		pop  eax;
		jmp  fo::funcoffs::item_w_range_;
	}
}

static void __fastcall SwapHandSlots(fo::GameObject* item, fo::GameObject* &toSlot) {
	if (toSlot && fo::util::GetItemType(item) != fo::item_type_weapon && fo::util::GetItemType(toSlot) != fo::item_type_weapon) {
		return;
	}
	fo::ItemButtonItem* leftSlot  = &fo::var::itemButtonItems[fo::HandSlot::Left];
	fo::ItemButtonItem* rightSlot = &fo::var::itemButtonItems[fo::HandSlot::Right];

	if (toSlot == nullptr) { // copy to empty slot
		fo::ItemButtonItem* dstSlot;
		fo::ItemButtonItem item;
		if ((int)&toSlot == FO_VAR_i_lhand) {
			std::memcpy(&item, rightSlot, 0x14);
			item.primaryAttack   = fo::AttackType::ATKTYPE_LWEAPON_PRIMARY;
			item.secondaryAttack = fo::AttackType::ATKTYPE_LWEAPON_SECONDARY;
			dstSlot = leftSlot; // Rslot > Lslot
		} else {
			std::memcpy(&item, leftSlot, 0x14);
			item.primaryAttack   = fo::AttackType::ATKTYPE_RWEAPON_PRIMARY;
			item.secondaryAttack = fo::AttackType::ATKTYPE_RWEAPON_SECONDARY;
			dstSlot = rightSlot; // Lslot > Rslot;
		}
		std::memcpy(dstSlot, &item, 0x14);
	} else { // swap slots
		auto& hands = fo::var::itemButtonItems;
		hands[fo::HandSlot::Left].primaryAttack    = fo::AttackType::ATKTYPE_RWEAPON_PRIMARY;
		hands[fo::HandSlot::Left].secondaryAttack  = fo::AttackType::ATKTYPE_RWEAPON_SECONDARY;
		hands[fo::HandSlot::Right].primaryAttack   = fo::AttackType::ATKTYPE_LWEAPON_PRIMARY;
		hands[fo::HandSlot::Right].secondaryAttack = fo::AttackType::ATKTYPE_LWEAPON_SECONDARY;

		std::memcpy(leftSlot,  &hands[fo::HandSlot::Right], 0x14); // Rslot > Lslot
		std::memcpy(rightSlot, &hands[fo::HandSlot::Left],  0x14); // Lslot > Rslot
	}
}

static __declspec(naked) void switch_hand_hack() {
	__asm {
		pushfd;
		test ebx, ebx;
		jz   skip;
		cmp  ebx, edx;
		jz   skip;
		push ecx;
		mov  ecx, eax;
		call SwapHandSlots;
		pop  ecx;
skip:
		popfd;
		jz   end;
		retn;
end:
		mov  dword ptr [esp], 0x4715B7;
		retn;
	}
}

static long pHitL, sHitL, modeL = -2;
static long pHitR, sHitR, modeR = -2;

static long intface_update_items_hack_begin() {
	if (!fo::var::itemButtonItems[fo::HandSlot::Left].item && !fo::func::inven_left_hand(fo::var::obj_dude)) {
		modeL = fo::var::itemButtonItems[fo::HandSlot::Left].mode;
		pHitL = fo::var::itemButtonItems[fo::HandSlot::Left].primaryAttack;
		sHitL = fo::var::itemButtonItems[fo::HandSlot::Left].secondaryAttack;
	}
	if (!fo::var::itemButtonItems[fo::HandSlot::Right].item && !fo::func::inven_right_hand(fo::var::obj_dude)) {
		modeR = fo::var::itemButtonItems[fo::HandSlot::Right].mode;
		pHitR = fo::var::itemButtonItems[fo::HandSlot::Right].primaryAttack;
		sHitR = fo::var::itemButtonItems[fo::HandSlot::Right].secondaryAttack;
	}
	return fo::var::itemCurrentItem;
}

static void intface_update_restore() {
	if (modeL != -2 && pHitL == fo::var::itemButtonItems[fo::HandSlot::Left].primaryAttack &&
	    sHitL == fo::var::itemButtonItems[fo::HandSlot::Left].secondaryAttack)
	{
		fo::var::itemButtonItems[fo::HandSlot::Left].mode = modeL;
	}
	if (modeR != -2 && pHitR == fo::var::itemButtonItems[fo::HandSlot::Right].primaryAttack &&
	    sHitR == fo::var::itemButtonItems[fo::HandSlot::Right].secondaryAttack)
	{
		fo::var::itemButtonItems[fo::HandSlot::Right].mode = modeR;
	}
	modeL = -2;
	modeR = -2;
}

static __declspec(naked) void intface_update_items_hack_end() {
	__asm {
		call intface_update_restore;
		cmp  dword ptr [esp + 0x1C - 0x18 + 4], 0; // animate
		retn;
	}
}

static __declspec(naked) void action_use_skill_on_hook() {
	__asm { // eax = dude_obj, edx = target, ebp = party_member
		cmp  eax, edx;
		jnz  end;                     // jump if target != dude_obj
		mov  edx, ebp;
		call fo::funcoffs::obj_dist_; // check distance between dude_obj and party_member
		cmp  eax, 1;                  // if the distance is greater than 1, then reset the register
		jg   skip;
		inc  eax;
		retn;
skip:
		xor  eax, eax;
		retn;
end:
		jmp  fo::funcoffs::obj_dist_;
	}
}

static __declspec(naked) void endgame_movie_hook() {
	__asm {
		cmp  dword ptr [esp + 16], 0x45C563; // call from op_endgame_movie_
		je   playWalkMovie;
		retn;
playWalkMovie:
		call fo::funcoffs::stat_level_;
		xor  edx, edx;
		add  eax, 10;
		mov  ecx, eax;
		mov  eax, 1500;
		call fo::funcoffs::pause_for_tocks_;
		mov  eax, ecx;
		jmp  fo::funcoffs::gmovie_play_;
	}
}

static long __fastcall GetRadHighlightColor(bool selected) {
	if (fo::util::IsRadInfluence()) {
		return (selected) ? fo::var::LightRedColor : fo::var::RedColor;
	}
	if (fo::var::obj_dude->critter.rads == 0 && fo::util::GetRadiationEvent(0)) {
		return (selected) ? fo::var::WhiteColor : fo::var::NearWhiteColor;
	}
	return 0;
}

static __declspec(naked) void ListDrvdStats_hook() {
	__asm {
		cmp  dword ptr [esp], 0x4354BE + 5; // from called
		sete cl;
		call GetRadHighlightColor;
		test eax, eax;
		jnz  skip;
		mov  eax, ds:[FO_VAR_obj_dude];
		jmp  fo::funcoffs::critter_get_rads_;
skip:
		mov  ecx, eax;
		mov  dword ptr [esp], 0x4354D9; // ListDrvdStats_
		retn;
	}
}

static __declspec(naked) void obj_render_outline_hack() {
	__asm {
		test eax, 0xFF00;
		jnz  palColor;
		mov  al, ds:[FO_VAR_GoodColor];
		retn;
palColor:
		mov  al, ah;
		retn;
	}
}

static void __fastcall RemoveAllFloatTextObjects() {
	long textCount = fo::var::text_object_index;
	if (textCount > 0) {
		for (long i = 0; i < textCount; i++) {
			fo::func::mem_free(fo::var::text_object_list[i]->buffer);
			fo::func::mem_free(fo::var::text_object_list[i]);
		}
		fo::var::text_object_index = 0;
	}
}

static __declspec(naked) void obj_move_to_tile_hook() {
	__asm {
		push eax;
		push edx;
		call RemoveAllFloatTextObjects;
		pop  edx;
		pop  eax;
		jmp  fo::funcoffs::map_set_elevation_;
	}
}

static __declspec(naked) void map_check_state_hook() {
	__asm {
		push eax;
		call RemoveAllFloatTextObjects;
		pop  eax;
		jmp  fo::funcoffs::map_load_idx_;
	}
}

// Frees up space in the array to create a text object
static void __fastcall RemoveFloatTextObject(fo::GameObject* source) {
	size_t index = 0;
	size_t textCount = fo::var::text_object_index;
	long minTime = fo::var::text_object_list[0]->time;

	for (size_t i = 1; i < textCount; i++) {
		if (fo::var::text_object_list[i]->owner == source) {
			index = i;
			break;
		}
		if (fo::var::text_object_list[i]->time < minTime) {
			minTime = fo::var::text_object_list[i]->time;
			index = i;
		}
	}
	fo::FloatText* tObj = fo::var::text_object_list[index];

	fo::func::tile_coord(tObj->tile_num, &tObj->rect.x, &tObj->rect.y);
	tObj->rect.y += tObj->y_off;
	tObj->rect.x += tObj->x_off;

	fo::BoundRect rect;
	rect.x = tObj->rect.x;
	rect.y = tObj->rect.y;
	rect.offx = tObj->rect.width + tObj->rect.x - 1;
	rect.offy = tObj->rect.height + tObj->rect.y - 1;

	fo::func::mem_free(tObj->buffer);
	fo::func::mem_free(tObj);

	// copy the last element of the array to the place of the removed one
	if (--textCount > index) fo::var::text_object_list[index] = fo::var::text_object_list[textCount];
	fo::var::text_object_index--;

	fo::func::tile_refresh_rect(&rect, fo::var::map_elevation);
}

static __declspec(naked) void text_object_create_hack() {
	__asm {
		mov  ecx, eax;
		push 0x4B03A6; // ret addr
		jmp  RemoveFloatTextObject;
	}
}

static void AdditionalWeaponAnimsPatch() {
	//if (IniReader::GetConfigInt("Misc", "AdditionalWeaponAnims", 1)) {
		dlogr("Applying additional weapon animations patch.", DL_INIT);
		SafeWrite8(0x419320, 18); // art_get_code_
		HookCalls(WeaponAnimHook, {
			0x451648, 0x451671, // gsnd_build_character_sfx_name_
			0x4194CC            // art_get_name_
		});
	//}
}

static void SkilldexImagesPatch() {
	dlogr("Checking for changed skilldex images.", DL_INIT);
	long tmp = IniReader::GetConfigInt("Misc", "Lockpick", 293);
	if (tmp != 293) SafeWrite32(0x518D54, tmp);
	tmp = IniReader::GetConfigInt("Misc", "Steal", 293);
	if (tmp != 293) SafeWrite32(0x518D58, tmp);
	tmp = IniReader::GetConfigInt("Misc", "Traps", 293);
	if (tmp != 293) SafeWrite32(0x518D5C, tmp);
	tmp = IniReader::GetConfigInt("Misc", "FirstAid", 293);
	if (tmp != 293) SafeWrite32(0x518D4C, tmp);
	tmp = IniReader::GetConfigInt("Misc", "Doctor", 293);
	if (tmp != 293) SafeWrite32(0x518D50, tmp);
	tmp = IniReader::GetConfigInt("Misc", "Science", 293);
	if (tmp != 293) SafeWrite32(0x518D60, tmp);
	tmp = IniReader::GetConfigInt("Misc", "Repair", 293);
	if (tmp != 293) SafeWrite32(0x518D64, tmp);
}

static void ScienceOnCrittersPatch() {
	switch (IniReader::GetConfigInt("Misc", "ScienceOnCritters", 0)) {
	case 1:
		HookCall(0x41276E, action_use_skill_on_hook_science);
		break;
	case 2:
		SafeWrite8(0x41276A, CodeType::JumpShort);
		break;
	}
}

static void BoostScriptDialogLimitPatch() {
	const DWORD script_dialog_msgs[] = {
		0x4A50C2, 0x4A5169, 0x4A52FA, 0x4A5302, 0x4A6B86, 0x4A6BE0, 0x4A6C37,
	};

	if (IniReader::GetConfigInt("Misc", "BoostScriptDialogLimit", 0)) {
		const int scriptDialogCount = 10000;
		dlogr("Applying script dialog limit patch.", DL_INIT);
		scriptDialog = new int[scriptDialogCount * 2]; // Because the msg structure is 8 bytes, not 4.
		SafeWrite32(0x4A50E3, scriptDialogCount); // scr_init
		SafeWrite32(0x4A519F, scriptDialogCount); // scr_game_init
		SafeWrite32(0x4A534F, scriptDialogCount * 8); // scr_message_free
		SafeWriteBatch<DWORD>((DWORD)scriptDialog, script_dialog_msgs); // scr_get_dialog_msg_file
	}
}

static void NumbersInDialoguePatch() {
	if (IniReader::GetConfigInt("Misc", "NumbersInDialogue", 0)) {
		dlogr("Applying numbers in dialogue patch.", DL_INIT);
		SafeWrite32(0x502C32, 0x2000202E);        // '%c ' > '%c. '
		SafeWrite8(0x446F3B, 0x35);
		SafeWrite32(0x5029E2, 0x7325202E);        // '%c %s' > '%c. %s'
		SafeWrite32(0x446F03, 0x2424448B);        // mov  eax, [esp+0x24]
		SafeWrite8(0x446F07, 0x50);               // push eax
		SafeWrite32(0x446FE0, 0x2824448B);        // mov  eax, [esp+0x28]
		SafeWrite8(0x446FE4, 0x50);               // push eax
		MakeJump(0x4458F5, gdAddOptionStr_hack);
	}
}

static void InstantWeaponEquipPatch() {
	const DWORD PutAwayWeapon[] = {
		0x411EA2, // action_climb_ladder_
		0x412046, // action_use_an_item_on_object_
		0x41224A, // action_get_an_object_
		0x4606A5, // intface_change_fid_animate_
		0x472996, // invenWieldFunc_
	};

	if (IniReader::GetConfigInt("Misc", "InstantWeaponEquip", 0)) {
		//Skip weapon equip/unequip animations
		dlogr("Applying instant weapon equip patch.", DL_INIT);
		SafeWriteBatch<BYTE>(CodeType::JumpShort, PutAwayWeapon); // jmps
		BlockCall(0x472AD5); //
		BlockCall(0x472AE0); // invenUnwieldFunc_
		BlockCall(0x472AF0); //
		MakeJump(0x415238, register_object_take_out_hack);
	}
}

static void DontTurnOffSneakIfYouRunPatch() {
	if (IniReader::GetConfigInt("Misc", "DontTurnOffSneakIfYouRun", 0)) {
		dlogr("Applying DontTurnOffSneakIfYouRun patch.", DL_INIT);
		SafeWrite8(0x418135, CodeType::JumpShort);
	}
}

static void PlayIdleAnimOnReloadPatch() {
	if (IniReader::GetConfigInt("Misc", "PlayIdleAnimOnReload", 0)) {
		dlogr("Applying idle anim on reload patch.", DL_INIT);
		HookCall(0x460B8C, intface_item_reload_hook);
	}
}

static void MotionScannerFlagsPatch() {
	if (long flags = IniReader::GetConfigInt("Misc", "MotionScannerFlags", 1)) {
		dlogr("Applying MotionScannerFlags patch.", DL_INIT);
		if (flags & 1) MakeJump(0x41BBE9, automap_hack);
		if (flags & 2) {
			// automap_
			SafeWrite16(0x41BC24, 0x9090);
			BlockCall(0x41BC3C);
			// item_m_use_charged_item_
			SafeWrite8(0x4794B3, 0x5E); // jbe short 0x479512
		}
	}
}

static __declspec(naked) void ResizeEncounterMessagesTable() {
	__asm {
		add  eax, eax; // double the increment
		add  eax, 3000;
		retn;
	}
}

static void EncounterTableSizePatch() {
	const DWORD EncounterTableSize[] = {
		0x4BD1A3, 0x4BD1D9, 0x4BD270, 0x4BD604, 0x4BDA14, 0x4BDA44, 0x4BE707,
		0x4C0815, 0x4C0D4A, 0x4C0FD4,
	};

	int tableSize = IniReader::GetConfigInt("Misc", "EncounterTableSize", 0);
	if (tableSize > 40) {
		dlogr("Applying EncounterTableSize patch.", DL_INIT);
		if (tableSize > 50) {
			if (tableSize > 100) tableSize = 100;
			// Increase the count of message lines from 50 to 100 for the encounter tables in worldmap.msg
			MakeCalls(ResizeEncounterMessagesTable, {0x4C102C, 0x4C0B57});
		}
		SafeWrite8(0x4BDB17, (BYTE)tableSize);
		DWORD nsize = (tableSize + 1) * 180 + 0x50;
		SafeWriteBatch<DWORD>(nsize, EncounterTableSize);
	}
}

static void DisablePipboyAlarmPatch() {
	if (IniReader::GetConfigInt("Misc", "DisablePipboyAlarm", 0)) {
		dlogr("Applying Disable Pip-Boy alarm button patch.", DL_INIT);
		SafeWrite8(0x499518, CodeType::Ret);
		SafeWrite8(0x443601, 0);
	}
}

static void ObjCanSeeShootThroughPatch() {
	if (IniReader::GetConfigInt("Misc", "ObjCanSeeObj_ShootThru_Fix", 0)) {
		dlogr("Applying obj_can_see_obj fix for seeing through critters and ShootThru objects.", DL_INIT);
		HookCall(0x456BC6, op_obj_can_see_obj_hook);
	}
}

static void OverrideMusicDirPatch() {
	static const char* musicOverridePath = "data\\sound\\music\\";

	if (long overrideMode = IniReader::GetConfigInt("Sound", "OverrideMusicDir", 0)) {
		SafeWriteBatch<DWORD>((DWORD)musicOverridePath, {0x4449C2, 0x4449DB}); // set paths if not present in the cfg
		if (overrideMode == 2) {
			SafeWriteBatch<DWORD>((DWORD)musicOverridePath, {0x518E78, 0x518E7C});
			SafeWrite16(0x44FCF3, 0x40EB); // jmp 0x44FD35 (skip paths initialization)
		}
	}
}

static void DialogueFix() {
	if (IniReader::GetConfigInt("Misc", "DialogueFix", 1)) {
		dlogr("Applying dialogue patch.", DL_INIT);
		SafeWrite8(0x446848, 0x31);
	}
}

static void AlwaysReloadMsgs() {
	if (IniReader::GetConfigInt("Misc", "AlwaysReloadMsgs", 0)) {
		dlogr("Applying always reload messages patch.", DL_INIT);
		SafeWrite8(0x4A6B8D, 0); // jnz $+6
	}
}

static void MusicInDialoguePatch() {
	if (IniReader::GetConfigInt("Misc", "EnableMusicInDialogue", 0)) {
		dlogr("Applying music in dialogue patch.", DL_INIT);
		SafeWrite16(0x44525A, 0x9090);
		//BlockCall(0x450627);
	}
}

static void PipboyAvailableAtStartPatch() {
	switch (IniReader::GetConfigInt("Misc", "PipBoyAvailableAtGameStart", 0)) {
	case 1:
		LoadGameHook::OnBeforeGameStart() += []() {
			fo::var::gmovie_played_list[3] = true; // PipBoy aquiring video
		};
		break;
	case 2:
		SafeWrite8(0x497011, CodeType::JumpShort); // skip the vault suit movie check
		break;
	}
}

static void DisableHorriganPatch() {
	if (IniReader::GetConfigInt("Misc", "DisableHorrigan", 0)) {
		LoadGameHook::OnAfterGameStarted() += []() {
			fo::var::Meet_Frank_Horrigan = true;
		};
	}
}

static void DisplaySecondWeaponRangePatch() {
	// Display the range of the secondary attack mode in the inventory when you switch weapon modes in active item slots
	//if (IniReader::GetConfigInt("Misc", "DisplaySecondWeaponRange", 1)) {
		dlogr("Applying display second weapon range patch.", DL_INIT);
		HookCall(0x472201, display_stats_hook);
	//}
}

static void KeepSelectModePatch() {
	//if (IniReader::GetConfigInt("Misc", "KeepWeaponSelectMode", 1)) {
		dlogr("Applying keep selected attack mode patch.", DL_INIT);
		MakeCall(0x4714EC, switch_hand_hack, 1);
		// Keep unarmed mode
		MakeCall(0x45F019, intface_update_items_hack_begin);
		MakeCall(0x45F380, intface_update_items_hack_end);
	//}
}

static void PartyMemberSkillPatch() {
	// Fix getting distance from source to target when using skills
	// Note: this will cause the party member to perform First Aid/Doctor skills when you use them on the player, but only if
	// the player is standing next to the party member. Because the related engine function is not fully implemented, enabling
	// this option without a global script that overrides First Aid/Doctor functions has very limited usefulness
	if (IniReader::GetConfigInt("Misc", "PartyMemberSkillFix", 0)) {
		dlogr("Applying First Aid/Doctor skill use patch for party members.", DL_INIT);
		HookCall(0x412836, action_use_skill_on_hook);
	}
	// Small code patch for HOOK_USESKILLON (change obj_dude to source)
	SafeWrite32(0x4128F3, 0x90909090);
	SafeWrite16(0x4128F7, 0xFE39); // cmp esi, _obj_dude -> cmp esi, edi
}

#pragma pack(push, 1)
struct CodeData {
	DWORD dd = 0x0024548D;
	BYTE  db = 0x90;
} patchData;
#pragma pack(pop)

static void SkipLoadingGameSettingsPatch() {
	if (int skipLoading = IniReader::GetConfigInt("Misc", "SkipLoadingGameSettings", 0)) {
		dlogr("Applying skip loading game settings from a saved game patch.", DL_INIT);
		BlockCall(0x493421);
		SafeWrite8(0x4935A8, 0x1F);
		SafeWrite32(0x4935AB, 0x90901B75);

		if (skipLoading == 2) SafeWriteBatch<CodeData>(patchData, {0x49341C, 0x49343B});
		SafeWriteBatch<CodeData>(patchData, {
			0x493450, 0x493465, 0x49347A, 0x49348F, 0x4934A4, 0x4934B9, 0x4934CE,
			0x4934E3, 0x4934F8, 0x49350D, 0x493522, 0x493547, 0x493558, 0x493569,
			0x49357A
		});
	}
}

static void UseWalkDistancePatch() {
	int distance = IniReader::GetConfigInt("Misc", "UseWalkDistance", 3) + 2;
	if (distance > 1 && distance < 5) {
		dlogr("Applying walk distance for using objects patch.", DL_INIT);
		SafeWriteBatch<BYTE>(distance, {0x411E41, 0x411FF0, 0x4121C4, 0x412475, 0x412906}); // default is 5
	}
}

static void F1EngineBehaviorPatch() {
	if (IniReader::GetConfigInt("Misc", "Fallout1Behavior", 0)) {
		dlogr("Applying Fallout 1 engine behavior patch.", DL_INIT);
		BlockCall(0x4A4343); // disable playing the final movie/credits after the endgame slideshow
		SafeWrite8(0x477C71, CodeType::JumpShort); // disable halving the weight for power armor items
		HookCall(0x43F872, endgame_movie_hook); // play movie 10 or 11 based on the player's gender before the credits
	}
}

static long cMusicArea = -1;

static __declspec(naked) void wmMapMusicStart_hook() {
	__asm {
		push edx;
		push eax;
		call fo::funcoffs::map_target_load_area_; // returns the area ID of the loaded map
		cmp  eax, cMusicArea;
		mov  cMusicArea, eax;
		jne  default;
		mov  eax, [esp];
		lea  edx, ds:[FO_VAR_background_fname_requested];
		call fo::funcoffs::stricmp_; // compare music file name
		test eax, eax;
		jz   continuePlay;
default:
		pop  eax;
		pop  edx;
		jmp  fo::funcoffs::gsound_background_play_level_music_;
continuePlay:
		pop  eax;
		pop  edx;
		xor  eax, eax;
		retn;
	}
}

static __declspec(naked) void map_load_file_hook() {
	__asm {
		push eax;
		call InWorldMap;
		test eax, eax;
		jnz  playWind;
		lea  eax, LoadGameHook::mapLoadingName;
		call fo::funcoffs::wmMapMatchNameToIdx_;
		test eax, eax;
		js   default; // -1
		push edx;
		sub  esp, 4;
		mov  edx, esp;
		call fo::funcoffs::wmMatchAreaContainingMapIdx_;
		pop  eax;
		pop  edx;
		cmp  eax, cMusicArea;
		jne  playWind;
		add  esp, 4;
		retn;
playWind:
		mov  eax, -1;
default:
		mov  cMusicArea, eax;
		pop  eax;
		jmp  fo::funcoffs::gsound_background_play_;
	}
}

static __declspec(naked) void wmSetMapMusic_hook() {
	__asm {
		mov  ds:[FO_VAR_background_fname_requested], 0;
		jmp  fo::funcoffs::wmMapMusicStart_;
	}
}

// When moving to another map that uses the same music, the music playback will not restart from the beginning
static void PlayingMusicPatch() {
	HookCall(0x4C58FB, wmMapMusicStart_hook);
	HookCall(0x482BA0, map_load_file_hook);

	HookCall(0x4C5999, wmSetMapMusic_hook); // related fix

	LoadGameHook::OnGameReset() += []() {
		cMusicArea = -1;
	};
}

static __declspec(naked) void main_death_scene_hook() {
	__asm {
		mov  eax, 101;
		call fo::funcoffs::text_font_;
		jmp  fo::funcoffs::debug_printf_;
	}
}

static __declspec(naked) void op_display_msg_hook() {
	__asm {
		cmp  dword ptr ds:[FO_VAR_debug_func], 0;
		jne  debug;
		retn;
debug:
		jmp  fo::funcoffs::config_get_value_;
	}
}

static void EngineOptimizationPatches() {
	// Speed up display_msg script function
	HookCall(0x455404, op_display_msg_hook);

	// Remove redundant/duplicate code
	BlockCall(0x45EBBF); // intface_redraw_
	BlockCall(0x4A4859); // exec_script_proc_
	SafeMemSet(0x455189, CodeType::Nop, 11); // op_create_object_sid_

	// Improve performance of the data conversion of script interpreter
	// mov eax, [edx+eax]; bswap eax; ret;
	SafeWrite32(0x4672A4, 0x0F02048B);
	SafeWrite16(0x4672A8, 0xC3C8);
	// mov eax, [edx+eax]; bswap eax;
	SafeWrite32(0x4673E5, 0x0F02048B);
	SafeWrite8(0x4673E9, 0xC8);
	// mov ax, [eax]; rol ax, 8; ret;
	SafeWrite32(0x467292, 0x66008B66);
	SafeWrite32(0x467296, 0xC308C0C1);

	// Disable unused code for the RandX and RandY window structure fields (these fields can now be used for other purposes)
	SafeWrite32(0x4D630C, 0x9090C031); // xor eax, eax
	SafeWrite8(0x4D6310, 0x90);
	BlockCall(0x4D6319);

	// Reduce excessive delays in the save/load game screens
	SafeWriteBatch<BYTE>(16, { // 41 to 16 ms
		0x47D00D, // LoadGame_
		0x47C1FD  // SaveGame_
	});
	// LoadGame_
	SafeWrite8(0x47CF0D, 195 + 10); // jz 0x47CFDE
	// SaveGame_
	SafeWrite8(0x47C135, 140 + 10); // jz 0x47C1CF
}

void MiscPatches::SetIdle(int value) {
	idle = (value > 50) ? 50 : value;
}

void MiscPatches::init() {
	EngineOptimizationPatches();

	if (IniReader::GetConfigString("Misc", "StartingMap", "", mapName, 16)) {
		dlogr("Applying starting map patch.", DL_INIT);
		SafeWrite32(0x480AAA, (DWORD)&mapName);
	}

	if (IniReader::GetConfigString("Misc", "VersionString", "", versionString, 65)) {
		dlogr("Applying version string patch.", DL_INIT);
		SafeWrite32(0x4B4588, (DWORD)&versionString);
	}

	if (IniReader::GetConfigString("Misc", "PatchFile", "", patchName, 65)) {
		dlogr("Applying patch file patch.", DL_INIT);
		SafeWrite32(0x444323, (DWORD)&patchName);
	}

	if (IniReader::GetConfigInt("Misc", "SingleCore", 1)) {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		if (sysInfo.dwNumberOfProcessors > 1) {
			dlogr("Applying single core patch.", DL_INIT);
			HANDLE process = GetCurrentProcess();
			SetProcessAffinityMask(process, 2); // use only CPU 1
			CloseHandle(process);
		}
	}

	if (IniReader::GetConfigInt("Misc", "OverrideArtCacheSize", 0)) {
		dlogr("Applying override art cache size patch.", DL_INIT);
		SafeWrite32(0x418867, 0x90909090);
		SafeWrite32(0x418872, 261); // default for 512 MB system memory from offical installer
	}

	int time = IniReader::GetConfigInt("Misc", "CorpseDeleteTime", 6); // time in days
	if (time != 6) {
		dlogr("Applying corpse deletion time patch.", DL_INIT);
		if (time <= 0) {
			time = 12; // hours
		} else if (time > 13) {
			time = 13 * 24;
		} else {
			time *= 24;
		}
		SafeWrite32(0x483348, time);
	}

	// Set idle function
	fo::var::idle_func = reinterpret_cast<void*>(Sleep);
	SafeWrite16(0x4C9F12, 0x7D6A); // push 125 (ms)

	int ms = IniReader::GetConfigInt("Misc", "ProcessorIdle", -1);
	if (ms > idle) SetIdle(ms);
	if (idle >= 0) MakeCall(0x4C9CF8, GNW95_process_message_hack, 2);

	BlockCall(0x4425E6); // Patch out ereg call

	SafeWrite8(0x4810AB, CodeType::JumpShort); // Disable selfrun

	SimplePatch<DWORD>(0x440C2A, "Misc", "SpecialDeathGVAR", fo::GVAR_MODOC_SHITTY_DEATH);

	// Remove hardcoding for maps with IDs 19 and 37
	if (IniReader::GetConfigInt("Misc", "DisableSpecialMapIDs", 0)) {
		dlogr("Applying disable special maps handling patch.", DL_INIT);
		SafeWriteBatch<BYTE>(0, {0x4836D6, 0x4836DB});
	}

	// Remove hardcoding for city areas 45 and 46 (AREA_FAKE_VAULT_13)
	if (IniReader::GetConfigInt("Misc", "DisableSpecialAreas", 0)) {
		dlogr("Applying disable special areas handling patch.", DL_INIT);
		SafeWrite8(0x4C0576, CodeType::JumpShort);
	}

	// Set the normal font for death screen subtitles
	if (IniReader::GetConfigInt("Misc", "DeathScreenFontPatch", 0)) {
		dlogr("Applying death screen font patch.", DL_INIT);
		HookCall(0x4812DF, main_death_scene_hook);
	}

	// Highlight "Radiated" in red when the player is under the influence of negative effects of radiation
	// also highlight in gray when the player still has an impending radiation effect
	HookCalls(ListDrvdStats_hook, {0x43549C, 0x4354BE});

	// Allow setting custom colors from the game palette for object outlines
	MakeCall(0x48EE00, obj_render_outline_hack);

	// Remove floating text messages after moving to another map or elevation
	HookCall(0x48A94B, obj_move_to_tile_hook);
	HookCall(0x4836BB, map_check_state_hook);

	// Remove an old floating message when creating a new one if the maximum number of floating messages has been reached
	HookCall(0x4B03A1, text_object_create_hack); // jge hack

	// Increase the maximum value of the combat speed slider from 50 to 100
	SafeWriteBatch<BYTE>(100, {
		0x492120, 0x49212A, // UpdateThing_
		0x493787, 0x493790  // RestoreSettings_
	});
	SafeWrite8(0x519B82, 0x59); // 100.0

	if (!HRP::Setting::IsEnabled()) {
		// Corrects the height of the black background for death screen subtitles
		if (!HRP::Setting::ExternalEnabled()) SafeWrite32(0x48134D, 38 - (640 * 3)); // main_death_scene_ (shift y-offset 2px up, w/o HRP)
		if (!HRP::Setting::ExternalEnabled() || HRP::Setting::VersionIsValid) SafeWrite8(0x481345, 4); // main_death_scene_
		if (HRP::Setting::VersionIsValid) SafeWrite8(HRP::Setting::GetAddress(0x10011738), 10);
	}

	F1EngineBehaviorPatch();
	DialogueFix();
	AdditionalWeaponAnimsPatch();
	AlwaysReloadMsgs();
	PlayIdleAnimOnReloadPatch();

	SkilldexImagesPatch();
	ScienceOnCrittersPatch();

	OverrideMusicDirPatch();
	BoostScriptDialogLimitPatch();
	MotionScannerFlagsPatch();
	EncounterTableSizePatch();

	DisablePipboyAlarmPatch();

	ObjCanSeeShootThroughPatch();
	MusicInDialoguePatch();
	DontTurnOffSneakIfYouRunPatch();

	InstantWeaponEquipPatch();
	NumbersInDialoguePatch();
	PipboyAvailableAtStartPatch();
	DisableHorriganPatch();

	DisplaySecondWeaponRangePatch();
	KeepSelectModePatch();

	PartyMemberSkillPatch();

	SkipLoadingGameSettingsPatch();

	UseWalkDistancePatch();
	PlayingMusicPatch();
}

void MiscPatches::exit() {
	if (scriptDialog) delete[] scriptDialog;
}

}
