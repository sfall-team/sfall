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
#include "..\Translate.h"

#include "HookScripts\InventoryHs.h"

#include "..\Game\objects.h"

#include "Drugs.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"
#include "Worldmap.h"

#include "BugFixes.h"

namespace sfall
{

using namespace fo;
using namespace Fields;
using namespace ObjectFlag;

static DWORD critterBody = 0;
static DWORD sizeOnBody = 0;
static DWORD weightOnBody = 0;

static char messageBuffer[355];

/*
	Saving a list of PIDs for saved drug effects
	Note: the sequence of saving and loading the list is very important!
*/
std::list<int> drugsPid;

static void __fastcall DrugPidPush(int pid) {
	drugsPid.push_back(pid);
}

static int __fastcall DrugPidPop() {
	if (drugsPid.empty()) return 0;
	int pid = drugsPid.front();
	drugsPid.pop_front();
	return pid;
}

void BugFixes::DrugsSaveFix(HANDLE file) {
	DWORD sizeWrite, count = drugsPid.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	if (!count) return;
	for (auto it = drugsPid.begin(); it != drugsPid.end(); ++it) {
		int pid = *it;
		WriteFile(file, &pid, 4, &sizeWrite, 0);
	}
	drugsPid.clear();
	return;
}

bool BugFixes::DrugsLoadFix(HANDLE file) {
	DWORD count, sizeRead;
	ReadFile(file, &count, 4, &sizeRead, 0);
	if (sizeRead != 4) return false;
	for (DWORD i = 0; i < count; i++) {
		DWORD pid;
		ReadFile(file, &pid, 4, &sizeRead, 0);
		if (sizeRead != 4) return true;
		drugsPid.push_back(pid);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void ResetBodyState() {
	__asm mov critterBody, 0;
	__asm mov sizeOnBody, 0;
	__asm mov weightOnBody, 0;
}

static void Initialization() {
	// Restore calling original engine functions from HRP hacks (there is no difference in HRP functions)
	__int64 data = 0xC189565153;
	SafeWriteBytes(0x4D78CC, (BYTE*)&data, 5); // win_get_top_win_
	data = 0xC389565153;
	SafeWriteBytes(0x4CA9DC, (BYTE*)&data, 5); // mouse_get_position_
}

static std::vector<fo::AIcap> aiCapsBackup;

static void combat_ai_init_backup() {
	long num_caps = fo::var::num_caps;
	fo::AIcap* caps = fo::var::cap;

	aiCapsBackup.resize(num_caps);
	std::memcpy(&aiCapsBackup[0], caps, num_caps * sizeof(fo::AIcap));
}

static void combat_ai_reset() {
	long num_caps = fo::var::num_caps;
	fo::AIcap* caps = fo::var::cap;
	std::memcpy(caps, &aiCapsBackup[0], num_caps * sizeof(fo::AIcap));
}

static __declspec(naked) void GNW95_init_window_hack() {
	__asm {
		pop  eax;
		push 0x50FA0C; // "GNW95 Class"
		push (WS_EX_TOPMOST | WS_EX_APPWINDOW); // was WS_EX_TOPMOST
		jmp  eax;
	}
}

static __declspec(naked) void GNW95_init_window_hack_HRP() {
	__asm {
		pop  eax;
		push 0x50FA0C; // "GNW95 Class"
		push WS_EX_APPWINDOW; // was 0
		jmp  eax;
	}
}

// fix for vanilla negate operator not working on floats
static __declspec(naked) void NegateFixHack() {
	static const DWORD NegateFixHack_Back = 0x46AB77;
	__asm {
		mov  eax, [ecx + 0x1C];
		cmp  si, VAR_TYPE_FLOAT;
		je   isFloat;
		neg  ebx;
		retn;
isFloat:
		push ebx;
		fld  [esp];
		fchs;
		fstp [esp];
		pop  ebx;
		call fo::funcoffs::pushLongStack_;
		mov  edx, VAR_TYPE_FLOAT;
		add  esp, 4;                              // Destroy the return address
		jmp  NegateFixHack_Back;
	}
}
/*
static __declspec(naked) void compute_attack_hack() {
	static const DWORD UnarmedAttacksFixEnd = 0x423A0D;
	__asm {
		mov  ecx, 5;                        // 5% chance of critical hit
		cmp  edx, ATKTYPE_POWERKICK;        // Power Kick
		je   RollCheck;
		cmp  edx, ATKTYPE_HAMMERPUNCH;      // Hammer Punch
		je   RollCheck;
		add  ecx, 5;                        // 10% chance of critical hit
		cmp  edx, ATKTYPE_HOOKKICK;         // Hook Kick
		je   RollCheck;
		cmp  edx, ATKTYPE_JAB;              // Jab
		je   RollCheck;
		add  ecx, 5;                        // 15% chance of critical hit
		cmp  edx, ATKTYPE_HAYMAKER;         // Haymaker
		je   RollCheck;
		add  ecx, 5;                        // 20% chance of critical hit
		cmp  edx, ATKTYPE_PALMSTRIKE;       // Palm Strike
		je   RollCheck;
		add  ecx, 20;                       // 40% chance of critical hit
		cmp  edx, ATKTYPE_PIERCINGSTRIKE;   // Piercing Strike
		je   RollCheck;
		cmp  edx, ATKTYPE_PIERCINGKICK;     // Piercing Kick
		jne  end;
		add  ecx, 10;                       // 50% chance of critical hit
RollCheck:
		mov  edx, 100;
		mov  eax, 1;
		call fo::funcoffs::roll_random_;
		cmp  eax, ecx;                      // Check chance
		jg   end;
		mov  ebx, ROLL_CRITICAL_SUCCESS;    // Upgrade to critical hit
end:
		jmp  UnarmedAttacksFixEnd;
	}
}
*/
static __declspec(naked) void SharpShooterFix() {
	__asm {
		call fo::funcoffs::stat_level_;           // Perception
		cmp  edi, dword ptr ds:[FO_VAR_obj_dude];
		jne  end;
		xchg ecx, eax;
		mov  eax, edi;                            // _obj_dude
		mov  edx, PERK_sharpshooter;
		call fo::funcoffs::perk_level_;
		shl  eax, 1;
		add  eax, ecx;
end:
		retn;
	}
}

static __declspec(naked) void pipboy_hack() {
	__asm {
		cmp  ebx, 0x210;                          // Back button?
		je   end;
		cmp  byte ptr ds:[FO_VAR_holo_flag], 0;
		jne  end;
		xor  ebx, ebx;                            // No man, no problem (c) :-p
end:
		mov  eax, ds:[FO_VAR_crnt_func];
		retn;
	}
}

static __declspec(naked) void PipAlarm_hack() {
	__asm {
		mov  ds:[FO_VAR_crnt_func], eax;
		mov  eax, 0x400;
		call fo::funcoffs::PipStatus_;
		mov  eax, 0x50CC04;                       // 'iisxxxx1'
		retn;
	}
}

static __declspec(naked) void PipStatus_hook() {
	__asm {
		call fo::funcoffs::ListHoloDiskTitles_;
		mov  dword ptr ds:[FO_VAR_holodisk], ebx;
		retn;
	}
}

// corrects saving script blocks (to *.sav file) by properly accounting for actual number of scripts to be saved
static __declspec(naked) void scr_write_ScriptNode_hook() {
	__asm {
		mov  ecx, 16;                             // maximum number of scripts in block
		cmp  dword ptr [esp + 0xEC + 4], ecx;     // number_of_scripts (total scripts)
		jg   writeBlock;
		mov  ecx, dword ptr [esp + 0xEC + 4];
		test ecx, ecx;
		jg   writeBlock; // > 0
		xor  eax, eax;
		retn;                                     // don't save the current ScriptBlock
writeBlock:
		sub  dword ptr [esp + 0xEC + 4], ecx;     // number_of_scripts (reduce number [e.g. 24-16=8] or set it to 0)
		xchg dword ptr [ebp + 0xE00], ecx;        // ScriptBlocks.num (keep and set correct value: 16 or previous value of number_of_scripts)
		xor  esi, esi;
		xchg dword ptr [ebp + 0xE04], esi;        // ScriptBlocks.NextBlock (keep pointer and set it to 0)
		call fo::funcoffs::scr_write_ScriptNode_;
		mov  dword ptr [ebp + 0xE04], esi;        // restore ScriptBlocks.NextBlock
		mov  dword ptr [ebp + 0xE00], ecx;        // restore ScriptBlocks.num
		retn;
	}
}

static __declspec(naked) void protinst_default_use_item_hack() {
	__asm {
		mov  eax, dword ptr [edx + protoId];      // eax = target pid
		cmp  eax, PID_DRIVABLE_CAR;
		je   isCar;
		cmp  eax, PID_CAR_TRUNK;
		jne  notCar;
isCar:
		mov  eax, ebx;
		call fo::funcoffs::obj_use_power_on_car_;
		cmp  eax, -1;
		jne  skip;
notCar:
		push 0x49C38B;
		retn;                                     // "That does nothing."
skip:
		test eax, eax;
		jnz  end;
		dec  eax;
end:
		push 0x49C3C5;
		retn;
	}
}

static __declspec(naked) void obj_use_power_on_car_hack() {
	__asm {
		xor  eax, eax;
		cmp  ebx, 596;                            // "The car is already full of power."?
		je   skip;                                // Yes
		inc  eax;                                 // "You charge the car with more power."
skip:
		retn;
	}
}

static __declspec(naked) void item_wd_process_hack() {
	__asm {
		cmp  Drugs::JetWithdrawal, 1;
		je   skip;
		cmp  esi, PERK_add_jet; // esi - queue_addict.perk
		je   jetAddict;
skip:
		retn;
jetAddict:
		push ecx;
		xor  edx, edx;    // wait (0 - effects of addiction have already been applied)
		mov  eax, ecx;    // critter (any party members or dude)
		push [ebx + 0x4]; // queue_addict.drugPid (PID_JET)
		mov  ebx, 10080;  // time (7 days)
		mov  ecx, esi;    // addict perk (PERK_add_jet)
		call fo::funcoffs::insert_withdrawal_;
		pop  ecx;
		mov  dword ptr [esp], 0x47A3FB; // ret addr
		retn;
	}
}

static __declspec(naked) void item_d_take_drug_hook() {
	__asm {
		push edx;
		push ecx;
		mov  edx, eax; // item pid
		mov  ecx, esi; // critter
		call fo::util::CheckAddictByPid;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static __declspec(naked) void RemoveJetAddictFunc() {
	__asm {
		cmp  eax, dword ptr ds:[FO_VAR_wd_obj];
		jne  end;
		cmp  dword ptr [edx + 0x4], PID_JET;      // queue_addict.drug_pid == PID_JET?
end:
		sete al;                                  // 1 = Delete from queue, 0 = Don't touch
		and  eax, 0xFF;
		retn;
	}
}

static __declspec(naked) void item_d_take_drug_hack() {
	__asm {
		cmp  dword ptr [eax], 0;                  // queue_addict.wait (queue returned from item_d_take_drug_hook)
		jne  skip;                                // Addiction is not active yet
		mov  eax, esi;
		call fo::funcoffs::perform_withdrawal_end_;
skip:	// remove event from queue
		mov  dword ptr ds:[FO_VAR_wd_obj], esi;
		mov  eax, addict_event;                   // type = addiction
		mov  edx, offset RemoveJetAddictFunc;
		jmp  fo::funcoffs::queue_clear_type_;
	}
}

static __declspec(naked) void item_d_load_subfix() {
	__asm {
		sub  esp, 4;                              // proto buf
//		mov  [ebp], edi;                          // edi->queue_drug
		xor  ebp, ebp;                            // set drug_pid = 0
		mov  ecx, 9;                              // vanilla count
		mov  esi, FO_VAR_drugInfoList;
		mov  ebx, 12;
loopDrug:
		cmp  dword ptr [esi + 8], 0;              // drugInfoList.numeffects
		je   nextDrug;
		mov  edx, esp;
		mov  eax, [esi];                          // drugInfoList.pid
		call fo::funcoffs::proto_ptr_;
		test eax, eax;
		js   nextDrug;                            // -1 - can't open proto
		mov  edx, [esp];
		mov  eax, [edx + 0x24];                   // drug.stat0
		cmp  eax, [edi + 0x4];                    // drug.stat0 == queue_drug.stat0?
		jne  nextDrug;                            // No
		mov  eax, [edx + 0x28];                   // drug.stat1
		cmp  eax, [edi + 0x8];                    // drug.stat1 == queue_drug.stat1?
		jne  nextDrug;                            // No
		mov  eax, [edx + 0x2C];                   // drug.stat2
		cmp  eax, [edi + 0xC];                    // drug.stat2 == queue_drug.stat2?
		jne  nextDrug;                            // No
		mov  eax, [edx + 0x30];                   // drug.amount0
		cmp  eax, [edi + 0x10];                   // drug.amount0 == queue_drug.amount0?
		jne  nextDrug;                            // No
		mov  eax, [edx + 0x34];                   // drug.amount1
		cmp  eax, [edi + 0x14];                   // drug.amount1 == queue_drug.amount1?
		jne  nextDrug;                            // No
		mov  eax, [edx + 0x38];                   // drug.amount2
		cmp  eax, [edi + 0x18];                   // drug.amount2 == queue_drug.amount2?
		je   foundPid;                            // Yes
nextDrug:
		lea  esi, [esi + ebx];
		dec  ecx;
		jnz  loopDrug;
		cmp  ebx, 12;
		jnz  end;                                 // failed, this drug effect was not found
		// try find in new drugs
		call Drugs::GetDrugCount;
		test eax, eax;
		jz   end;
		mov  ebx, SIZE_S_DRUGS;                   // sizeof structure
		mov  ecx, eax;
		lea  edx, drugs;
		mov  esi, [edx];
		jmp  loopDrug;
foundPid:
		mov  ebp, [esi];                          // drugInfoList.pid
end:
		mov  [edi], ebp;                          // queue_drug.drug_pid
		xor  eax, eax;
		add  esp, 4;
		retn;
	}
}

// take the drug pid from the list after loading sfallgv.sav
static __declspec(naked) void item_d_load_hack() {
	__asm {
		mov  [ebp], edi;                          // edi->queue_drug
		call DrugPidPop;
		test eax, eax;
		jnz  skip;
		jmp  item_d_load_subfix;                  // if the pid was not saved, then try to find it
skip:
		mov  [edi], eax;                          // queue_drug.drug_pid
		xor  eax, eax;
		retn;
	}
}

// add drug pid to the list to save to sfallgv.sav
static __declspec(naked) void item_d_save_hack() {
	__asm {
		pushadc;
		mov  ecx, [edx];                          // drug pid
		call DrugPidPush;
		popadc;
		mov  ebx, 3;
		retn;
	}
}

static __declspec(naked) void queue_clear_type_mem_free_hook() {
	__asm {
		mov  ebx, [esi];
		jmp  fo::funcoffs::mem_free_;
	}
}

static __declspec(naked) void partyMemberCopyLevelInfo_hack() {
	__asm {
		mov  eax, esi; // source
		xor  ecx, ecx; // animation
		mov  ebx, 1;   // right slot
		retn;
	}
}

static __declspec(naked) void partyMemberCopyLevelInfo_hook_stat_level() {
	__asm {
nextArmor:
		mov  eax, esi;
		call fo::funcoffs::inven_worn_;
		test eax, eax;
		jz   noArmor;
		and  byte ptr [eax][flags + 3], ~(Worn >> 24); // Unset flag of equipped armor
		jmp  nextArmor;
noArmor:
		mov  eax, esi;
		jmp  fo::funcoffs::stat_level_;
	}
}

static __declspec(naked) void correctFidForRemovedItem_hook_adjust_ac() {
	__asm {
		call fo::funcoffs::adjust_ac_;
nextArmor:
		mov  eax, esi;
		call fo::funcoffs::inven_worn_;
		test eax, eax;
		jz   end;
		and  byte ptr [eax][flags + 3], ~(Worn >> 24); // Unset flag of equipped armor
		jmp  nextArmor;
end:
		retn;
	}
}

static __declspec(naked) void op_move_obj_inven_to_obj_hook() {
	__asm {
		jz   skip;                     // source == dude
		mov  eax, edx;
		call fo::funcoffs::isPartyMember_;
		test eax, eax;                 // is source a party member?
		jnz  skip;
		retn;                          // must be eax = 0
skip:
		mov  eax, edx;
		call fo::funcoffs::inven_worn_;
		cmp  edx, ecx;                 // source(edx) == dude(ecx)
		jz   dudeFix;
		test eax, eax;
		jz   end;
		// fix for party member
		call InvenUnwield_HookMove;    // run HOOK_INVENWIELD before moving item
		push ebx;
		mov  ecx, edx;
		xor  ebx, ebx;                 // new armor
		xchg eax, edx;                 // set: eax - source, edx - removed armor
		call fo::funcoffs::adjust_ac_;
		mov  edx, ecx;
		pop  ebx;
		xor  eax, eax;
end:
		retn;                          // must be eax = 0
dudeFix:
		test eax, eax;
		jz   equipped;                 // no armor
		// additionally check flag of equipped armor for dude
		test byte ptr [eax][flags + 3], Worn >> 24;
		jnz  equipped;
		xor  eax, eax;
equipped:
		or   cl, 1;                    // reset ZF
		retn;
	}
}

static __declspec(naked) void obj_drop_hook() {
	__asm {
		test byte ptr [edx][flags + 3], (Worn | Right_Hand | Left_Hand) >> 24;
		jz   skipHook;
		call InvenUnwield_HookDrop;    // run HOOK_INVENWIELD before dropping item
skipHook:
		test byte ptr [edx][flags + 3], Worn >> 24;
		jnz  fixArmorStat;
		jmp  fo::funcoffs::obj_remove_from_inven_;
fixArmorStat:
		call fo::funcoffs::isPartyMember_; // and dude
		test eax, eax;
		jz   skip;
		mov  eax, ecx;
		xor  ebx, ebx;                 // new armor
		call fo::funcoffs::adjust_ac_; // eax - source, edx - removed armor
		mov  edx, esi;
skip:
		mov  eax, ecx;
		jmp  fo::funcoffs::obj_remove_from_inven_;
	}
}

static void __fastcall CheckAddiction(fo::GameObject* critter) {
	fo::func::queue_remove_this(critter, fo::QueueType::drug_effect_event);

	for (size_t i = 0; i < 9; i++) {
		long pid = fo::var::drugInfoList[i].itemPid;

		fo::QueueAddictData* queue = fo::util::CheckAddictByPid(critter, pid);
		if (queue && queue->wait == 0) { // Has addiction and it is active
			fo::func::perk_add_effect(critter, queue->perkId);
		}
	}
}

static __declspec(naked) void partyMemberIncLevels_hook() {
	__asm {
		mov  ebx, eax; // party member pointer
		call fo::funcoffs::partyMemberCopyLevelInfo_;
		cmp  eax, -1;
		je   end;
		xchg ecx, ebx; // ecx <> ebx
		call CheckAddiction;
		mov  ecx, ebx;
end:
		retn;
	}
}

static __declspec(naked) void gdProcessUpdate_hack() {
	__asm {
		lea  edx, [eax - 2];
		cmp  edx, dword ptr ds:[FO_VAR_optionRect + 0xC]; // _optionRect.offy
		jl   skip;
		mov  eax, edx;
skip:
		retn;
	}
}

static __declspec(naked) void invenWieldFunc_item_get_type_hook() {
	__asm {
		mov  edx, esi;
		xor  ebx, ebx;
		inc  ebx;
		push ebx;
		mov  cl, byte ptr [edi + 0x27];
		and  cl, 0x3;
		xchg edx, eax;                            // eax = who, edx = item
		push eax;
		push -2;
		call SetRemoveObjectType;                 // call addr for HOOK_REMOVEINVENOBJ
		pop  eax;
		call fo::funcoffs::item_remove_mult_;
		xchg ebx, eax;
		mov  eax, esi;
		test cl, INVEN_TYPE_LEFT_HAND;            // Right hand?
		jz   leftHand;                            // No
		call fo::funcoffs::inven_right_hand_;
		jmp  removeFlag;
leftHand:
		call fo::funcoffs::inven_left_hand_;
removeFlag:
		test eax, eax;
		jz   noWeapon;
		and  byte ptr [eax + 0x27], 0xFC;         // Unset flag of a weapon in hand
noWeapon:
		or   byte ptr [edi + 0x27], cl;           // Set flag of a weapon in hand
		inc  ebx;
		pop  ebx;
		jz   skip;
		mov  eax, esi;
		mov  edx, edi;
		call fo::funcoffs::item_add_force_;
skip:
		mov  eax, edi;
		jmp  fo::funcoffs::item_get_type_;
	}
}

static __declspec(naked) void is_supper_bonus_hack() {
	__asm {
		add  eax, ecx;
		test eax, eax;
		jle  skip;
		cmp  eax, 10;
		jle  end;
skip:
		add  esp, 4;                              // Destroy the return address
		xor  eax, eax;
		inc  eax;
		pop  edx;
		pop  ecx;
		pop  ebx;
end:
		retn;
	}
}

static __declspec(naked) void PrintBasicStat_hack() {
	__asm {
		test eax, eax;
		jle  skip;
		cmp  eax, 10;
		jg   end;
		add  esp, 4;                              // Destroy the return address
		push 0x434C21;
		retn;
skip:
		xor  eax, eax;
end:
		retn;
	}
}

static __declspec(naked) void StatButtonUp_hook() {
	__asm {
		call fo::funcoffs::inc_stat_;
		test eax, eax;
		jl   end;
		test ebx, ebx;
		jge  end;
		sub  ds:[FO_VAR_character_points], esi;
		dec  esi;
		mov  [esp + 0xC + 0x4], esi;
end:
		retn;
	}
}

static __declspec(naked) void StatButtonDown_hook() {
	__asm {
		call fo::funcoffs::stat_level_;
		cmp  eax, 1;
		jg   end;
		add  esp, 4;                              // Destroy the return address
		xor  eax, eax;
		inc  eax;
		mov  [esp + 0xC], eax;
		push 0x437B41;
end:
		retn;
	}
}

// Calculate weight & size of equipped items
static __declspec(naked) void loot_container_hack() {
	__asm {
		mov  critterBody, ebp;                    // target
		push esi;
		mov  esi, [esp + 0x114 + 8];              // item lhand
		mov  sizeOnBody, esi;
		mov  eax, esi;
		test esi, esi;
		jz   noLeftWeapon;
		call fo::funcoffs::item_size_;
		mov  sizeOnBody, eax;
		mov  eax, esi;
		call fo::funcoffs::item_weight_;
noLeftWeapon:
		mov  weightOnBody, eax;
		mov  esi, [esp + 0x118 + 8];              // item rhand
		test esi, esi;
		jz   noRightWeapon;
		mov  eax, esi;
		call fo::funcoffs::item_size_;
		add  sizeOnBody, eax;
		mov  eax, esi;
		call fo::funcoffs::item_weight_;
		add  weightOnBody, eax;
noRightWeapon:
		mov  esi, [esp + 0x11C + 8];              // item armor
		test esi, esi;
		jz   noArmor;
		mov  eax, esi;
		call fo::funcoffs::item_size_;
		add  sizeOnBody, eax;
		mov  eax, esi;
		call fo::funcoffs::item_weight_;
		add  weightOnBody, eax;
noArmor:
		mov  esi, [esp + 0xF8 + 8]; // check JESSE_CONTAINER
		mov  eax, esi;
		call fo::funcoffs::item_c_curr_size_;
		add  sizeOnBody, eax;
		mov  eax, esi;
		call fo::funcoffs::item_total_weight_;
		add  weightOnBody, eax;
		pop  esi;
		mov  eax, 2; // overwritten code
		retn;
	}
}

static __declspec(naked) void barter_inventory_hook() {
	__asm {
		mov  critterBody, eax;                         // target
		call fo::funcoffs::item_move_all_hidden_;
		push esi;
		mov  esi, [esp + 0x20 + 8];                    // armor
		mov  sizeOnBody, esi;
		mov  eax, esi;
		test eax, eax;
		jz   noArmor;
		call fo::funcoffs::item_size_;
		mov  sizeOnBody, eax
		mov  eax, esi;
		call fo::funcoffs::item_weight_;
noArmor:
		mov  weightOnBody, eax;
		mov  esi, [esp + 0x1C + 8];                    // weapon
		test esi, esi;
		jnz  haveWeapon;
		cmp  ds:[FO_VAR_dialog_target_is_party], esi;  // esi = 0
		jne  skip;                                     // This is a party member
		mov  eax, [esp + 0x18 + 8];                    // item_weapon
		test eax, eax;
		jz   skip;
haveWeapon:
		mov  eax, esi;
		call fo::funcoffs::item_size_;
		add  sizeOnBody, eax;
		mov  eax, esi;
		call fo::funcoffs::item_weight_;
		add  weightOnBody, eax;
skip:
		mov  esi, [esp + 0x10 + 8]; // check JESSE_CONTAINER
		mov  eax, esi;
		call fo::funcoffs::item_c_curr_size_;
		add  sizeOnBody, eax
		mov  eax, esi;
		call fo::funcoffs::item_total_weight_;
		add  weightOnBody, eax
		pop  esi;
		retn;
	}
}

// Add weightOnBody to item_total_weight_ function
static __declspec(naked) void item_total_weight_hack() {
	__asm {
		xor edx, edx;
		mov ebx, [edi];                          // Inventory.inv_size
		xor esi, esi;
		//------
		cmp eax, dword ptr ds:[FO_VAR_obj_dude]; // eax - source
		jz  skip;
		cmp critterBody, eax;                    // if condition is true, then now it's exchanging/bartering with source object
		cmovz esi, weightOnBody;                 // take the weight of target's equipped items into account
skip:
		retn;
	}
}

// Add sizeOnBody to item_c_curr_size_ function
static __declspec(naked) void item_c_curr_size_hack() {
	__asm {
		xor esi, esi;
		mov edx, [ecx];                          // Inventory.inv_size
		xor edi, edi;
		//------
		cmp eax, dword ptr ds:[FO_VAR_obj_dude]; // eax - source
		jz  skip;
		cmp critterBody, eax;                    // if condition is true, then now it's exchanging/bartering with source object
		cmovz edi, sizeOnBody;                   // take the size of target's equipped items into account
skip:
		retn;
	}
}

static __declspec(naked) void inven_pickup_hack() {
	__asm {
		mov  edx, ds:[FO_VAR_pud];
		mov  edx, [edx];                          // itemsCount
		dec  edx;
		sub  edx, eax;
		lea  edx, ds:0[edx * 8];
		retn;
	}
}

static __declspec(naked) void inven_pickup_hack2() {
	__asm {
		test eax, eax;
		jz   end;
		mov  eax, ds:[FO_VAR_i_wid];
		call fo::funcoffs::GNW_find_;
		mov  ecx, [eax + 0x8 + 0x4];              // ecx = _i_wid.rect.y
		mov  eax, [eax + 0x8 + 0x0];              // eax = _i_wid.rect.x
		add  eax, 44;                             // x_start
		mov  ebx, 64;
		add  ebx, eax;                            // x_end
		xor  edx, edx;
next:
		push eax;
		push edx;
		push ecx;
		push ebx;
		imul edx, edx, 48;
		add  edx, 35;
		add  edx, ecx;                            // y_start
		mov  ecx, edx;
		add  ecx, 48;                             // y_end
		call fo::funcoffs::mouse_click_in_;
		pop  ebx;
		pop  ecx;
		pop  edx;
		test eax, eax;
		pop  eax;
		jnz  found;
		inc  edx;
		cmp  edx, ds:[FO_VAR_inven_cur_disp];
		jb   next;
end:
		push 0x47125C;
		retn;
found:
		mov  ebx, 0x4711DF;
		add  edx, [esp + 0x40];                   // inventory_offset
		mov  eax, ds:[FO_VAR_pud];
		mov  ecx, [eax];                          // itemsCount
		test ecx, ecx;
		jz   skip;
		dec  ecx;
		cmp  edx, ecx;
		ja   skip;
		sub  ecx, edx;
		mov  edx, ecx;
		mov  ebx, 0x471181;
skip:
		jmp  ebx;
	}
}

static __declspec(naked) void drop_ammo_into_weapon_hook() {
	__asm {
		dec  esi;
		test esi, esi;                            // One box of ammo?
		jz   skip;                                // Yes
		xor  esi, esi;
		// Excess check for from_slot, but leave it be
		mov  edx, [esp + 0x24 + 4];               // from_slot
		cmp  edx, 1006;                           // Hands?
		jge  skip;                                // Yes
		lea  edx, [eax + inventory];              // Inventory
		mov  ecx, [edx];                          // itemsCount
		test ecx, ecx;
		jz   skip;                                // inventory is empty (another excess check, but leave it)
		mov  edx, [edx + 8];                      // FirstItem
nextItem:
		cmp  ebp, [edx];                          // Our weapon?
		je   foundItem;                           // Yes
		add  edx, 8;                              // Go to the next
		dec  ecx;
		jnz  nextItem;
		jmp  skip;                                // Our weapon is not in inventory
foundItem:
		cmp  dword ptr [edx + 4], 1;              // Only one weapon?
		jg   skip;                                // No
		mov  edx, [esp + 0x24 + 4];               // from_slot
		lea  edx, [edx - 1000];
		add  edx, [esp + 0x40 + 4 + 0x24 + 4];    // edx = ordinal number of slot with ammo
		cmp  ecx, edx;                            // Weapon is after the ammo?
		jg   skip;                                // Yes
		inc  esi;                                 // No, need to change from_slot
skip:
		mov  edx, ebp;
		push eax;
		push 0x47659D;
		call SetRemoveObjectType;                 // call addr for HOOK_REMOVEINVENOBJ
		pop  eax;
		call fo::funcoffs::item_remove_mult_;
		test eax, eax;                            // Have weapon been deleted from inventory?
		jnz  end;                                 // No
		sub  [esp + 0x24 + 4], esi;               // Yes, correct from_slot
end:
		retn;
	}
}

static __declspec(naked) void PipStatus_AddHotLines_hook() {
	__asm {
		call fo::funcoffs::AddHotLines_;
		xor  eax, eax;
		mov  dword ptr ds:[FO_VAR_hot_line_count], eax;
		retn;
	}
}

static __declspec(naked) void perform_withdrawal_start_display_print_hook() {
	__asm {
		test eax, eax;
		jz   end;
		jmp  fo::funcoffs::display_print_;
end:
		retn;
	}
}

static __declspec(naked) void op_wield_obj_critter_adjust_ac_hook() {
	__asm {
		call fo::funcoffs::adjust_ac_;
		xor  eax, eax;                      // not animated
		jmp  fo::funcoffs::intface_update_ac_;
	}
}

static __declspec(naked) void NPCStage6Fix1() {
	static const DWORD partyMember_init_End = 0x493D16;
	__asm {
		imul eax, edx, 204;                 // multiply record size 204 bytes by number of NPC records in party.txt
		mov  ebx, eax;                      // copy total record size for later memset
		call fo::funcoffs::mem_malloc_;     // malloc the necessary memory
		jmp  partyMember_init_End;          // call memset to set all malloc'ed memory to 0
	}
}

static __declspec(naked) void NPCStage6Fix2() {
	static const DWORD partyMemberGetAIOptions_End = 0x49423A;
	__asm {
		imul edx, 204;                      // multiply record size 204 bytes by NPC number as listed in party.txt
		mov  eax, dword ptr ds:[FO_VAR_partyMemberAIOptions]; // get starting offset of internal NPC table
		jmp  partyMemberGetAIOptions_End;   // eax + edx = offset of specific NPC record
	}
}

static __declspec(naked) void make_path_func_hook() {
	__asm {
		mov  ebx, [edx + tile];            // object tile
		cmp  ebx, [esp + 0x5C - 0x1C + 4]; // target tile
		je   fix;
		jmp  fo::funcoffs::anim_can_use_door_;
fix:	// replace the target tile (where the multihex object is located) with the current tile
		mov  ebx, [esp + 0x5C - 0x14 + 4]; // current tile
		mov  [esp + 0x5C - 0x1C + 4], ebx; // target tile
		retn;
	}
}

// Haenlomal: Check path to critter for attack
//static __declspec(naked) void MultiHexFix() {
//	__asm {
//		xor  ecx, ecx;                      // argument value for make_path_func: ecx=0 (rotation data arg)
//		test [ebx + flags + 1], 0x08;       // is target multihex?
//		mov  ebx, [ebx + tile];             // argument value for make_path_func: target's tilenum (end_tile)
//		je   end;                           // skip if not multihex
//		inc  ebx;                           // otherwise, increase tilenum by 1
//end:
//		retn;                               // call make_path_func (at 0x429024, 0x429175)
//	}
//}

static __declspec(naked) void MultiHexRetargetTileFix() {
	__asm {
		push edx;                     // retargeted tile
		call fo::funcoffs::obj_blocking_at_;
		pop  edx;
		test eax, eax;
		jz   isFreeTile;
		retn;
isFreeTile:
		test [ebp + flags + 1], 0x08; // is source multihex?
		jnz  isMultiHex;
		retn;
isMultiHex:
		push ecx;
		mov  ecx, ebp;
		call fo::util::MultiHexMoveIsBlocking;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void MultiHexCombatMoveFix() {
	static const DWORD ai_move_steps_closer_move_object_ret = 0x42A192;
	__asm {
		test [edi + flags + 1], 0x08; // is target multihex?
		jnz  multiHex;
		test [esi + flags + 1], 0x08; // is source multihex?
		jnz  multiHex;
		retn;                         // move to tile
multiHex:
		mov  edx, [esp + 4];          // source's destination tilenum
		cmp  [edi + tile], edx;       // target's tilenum
		je   moveToObject;
		retn;                         // tilenums are not equal, always move to tile
moveToObject:
		add  esp, 4;
		jmp  ai_move_steps_closer_move_object_ret; // move to object
	}
}

static __declspec(naked) void MultiHexCombatRunFix() {
	static const DWORD ai_move_steps_closer_run_object_ret = 0x42A169;
	__asm {
		test [edi + flags + 1], 0x08; // is target multihex?
		jnz  multiHex;
		test [esi + flags + 1], 0x08; // is source multihex?
		jnz  multiHex;
		retn;                         // run to tile
multiHex:
		mov  edx, [esp + 4];          // source's destination tilenum
		cmp  [edi + tile], edx;       // target's tilenum
		je   runToObject;
		retn;                         // tilenums are not equal, always run to tile
runToObject:
		add  esp, 4;
		jmp  ai_move_steps_closer_run_object_ret; // run to object
	}
}

// checks if an attacked object is a critter before attempting dodge animation
static __declspec(naked) void action_melee_hack() {
	__asm {
		mov  eax, [ebp + ctdTarget];
		mov  ebx, [eax + artFid];         // objStruct->FID
		and  ebx, 0x0F000000;
		cmp  ebx, OBJ_TYPE_CRITTER << 24; // check if object FID type flag is set to critter
		jne  end;                         // if object not a critter leave jump condition flags
		test byte ptr [eax + damageFlags], DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN; // (original code)
end:
		retn; // JNZ to skip dodge animation
	}
}

static __declspec(naked) void action_ranged_hack() {
	__asm {
		mov  eax, [eax + ctdTarget];
		mov  ebx, [eax + artFid];         // objStruct->FID
		and  ebx, 0x0F000000;
		cmp  ebx, OBJ_TYPE_CRITTER << 24; // check if object FID type flag is set to critter
		jne  end;                         // if object not a critter leave jump condition flags
		test byte ptr [eax + damageFlags], DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN; // (original code)
end:
		retn; // JNZ to skip dodge animation
	}
}

////////// "NPC turns into a container" bug and knockout/down issues ///////////

static __declspec(naked) void set_new_results_hack() {
	__asm {
		call fo::funcoffs::stat_level_;
		push eax;
		mov  eax, esi;
		xor  edx, edx;
		inc  edx;                                 // type = knockout
		call fo::funcoffs::queue_remove_this_;    // Remove knockout from queue (if there is one)
		pop  eax;
		retn;
	}
}

static __declspec(naked) void op_critter_state_hack() {
	__asm {
		mov  ebx, 2;
		test eax, DAM_KNOCKOUT_WOKEN;
		cmovnz esi, ebx;
		and  eax, (DAM_CRIP_LEG_LEFT or DAM_CRIP_LEG_RIGHT or DAM_CRIP_ARM_LEFT or DAM_CRIP_ARM_RIGHT or DAM_BLIND);
		retn;
	}
}

// if the combat ends before the turn passes to the critter, the DAM_KNOCKOUT_WOKEN flag will remain set
// therefore the flag for the critter is removed before the combat_turn_ execution, as well as when the combat ends in combat_over_
static __declspec(naked) void critter_wake_up_hack() {
	using fo::InCombat;
	__asm {
		or   byte ptr [eax], InCombat; // combat_data.combat_state
		test dl, DAM_KNOCKED_OUT;      // is critter knocked out?
		jz   skip;                     // No
		or   dword ptr [ebx + damageFlags], DAM_KNOCKOUT_WOKEN; // game is in combat, set the flag
skip:
		xor  eax, eax;
		retn;
	}
}

// used when the player leaves the map and the time has been advanced in the map_exit_p_proc procedure
// in that case, the critter_wake_up_ function will not work properly and the critter will remain in the prone position
static void __fastcall sf_critter_wake_clear(fo::GameObject* critter) {
	if (critter->IsCritter()) {
		critter->critter.damageFlags &= ~(fo::DAM_KNOCKED_DOWN | fo::DAM_KNOCKED_OUT);
		critter->artFid = fo::func::art_id(fo::ObjType::OBJ_TYPE_CRITTER, critter->artFid & 0xFFF, 0, (critter->artFid & 0xF000) >> 12, critter->rotation + 1);
		//fo::func::obj_change_fid(critter, artID, 0);
	}
}

static __declspec(naked) void critter_wake_up_hook() {
	__asm {
		mov  ecx, eax;
		test ScriptExtender::OnMapLeave, 1;
		jnz  sf_critter_wake_clear;
		jmp  fo::funcoffs::dude_standup_;
	}
}

// called when leaving the map if the critter has a "knockout_event" event in the queue
static __declspec(naked) void critter_wake_clear_hack() {
	__asm {
		jne  end;                                 // This is not a critter
		mov  dl, [esi + damageFlags];
		test dl, DAM_DEAD;                        // DAM_DEAD?
		jnz  end;                                 // This is a corpse
		and  dl, ~DAM_KNOCKED_OUT;                // Unset DAM_KNOCKED_OUT
		or   dl, DAM_KNOCKED_DOWN;                // Set DAM_KNOCKED_DOWN
		mov  [esi + damageFlags], dl;
end:
		xor  eax, eax;
		inc  eax;
		pop  esi;
		pop  ecx;
		pop  ebx;
		retn; // exit from func
	}
}

// when loading the map
static __declspec(naked) void obj_load_func_hack() {
	static const DWORD obj_load_func_Ret = 0x488F14;
	using fo::InCombat;
	__asm {
		test word ptr [eax + flags], NoRemove; // engine code
		jz   fix;
		retn;
fix:
		mov  edi, [eax + protoId];
		and  edi, 0x0F000000;
		cmp  edi, OBJ_TYPE_CRITTER << 24;
		jne  skip;
		test byte ptr [eax + combatState], InCombat;
		jnz  skip;     // do nothing if the critter has the flag set
		test byte ptr [eax + damageFlags], DAM_DEAD;
		jnz  skip;     // is dead
		and  dword ptr [eax + damageFlags], ~(DAM_KNOCKOUT_WOKEN or DAM_LOSE_TURN); // clear DAM_LOSE_TURN for "NPC turns into a container" bug
		test byte ptr [eax + damageFlags], DAM_KNOCKED_OUT;
		jnz  clear;    // flag is set, so the "knockout_event" event is already in the queue (if there is no event, then there is a bug somewhere)
		test byte ptr [eax + damageFlags], DAM_KNOCKED_DOWN;
		jz   clear;    // No
		push eax;
		call fo::funcoffs::critter_wake_up_; // (stand up anim)
		pop  eax;
clear:
		//and  word ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKED_DOWN);
skip:
		add  esp, 4;
		jmp  obj_load_func_Ret;
	}
}

static __declspec(naked) void partyMemberPrepLoadInstance_hook() {
	__asm {
		and  word ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKED_DOWN); // clear DAM_LOSE_TURN for "NPC turns into a container" bug
		jmp  fo::funcoffs::dude_stand_;
	}
}

// after combat_is_over_p_proc
static __declspec(naked) void combat_over_hack() {
	__asm {
		mov  eax, esi; // critter
		mov  esi, ds:[FO_VAR_list_total];
		mov  edx, [eax + damageFlags];
		and  dword ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKOUT_WOKEN); // clear DAM_LOSE_TURN for "NPC turns into a container" bug
		test dl, DAM_DEAD or DAM_KNOCKED_OUT;
		jnz  end;
		test dl, DAM_KNOCKED_DOWN;
		jnz  standup;
end:
		retn;
standup:
		test edx, DAM_KNOCKOUT_WOKEN;
		jnz  delay;
		jmp  fo::funcoffs::dude_standup_;
delay:
		xor  ecx, ecx;
		mov  edx, eax; // object
		xor  ebx, ebx; // extramem
		inc  ecx;      // type = 1 (knockout_event)
		mov  eax, ecx; // time
		jmp  fo::funcoffs::queue_add_; // critter_wake_up_ will be called (stand up anim)
	}
}

static __declspec(naked) void dude_standup_hook() {
	__asm {
		mov  edx, [ecx + artFid];
		and  edx, 0xFF0000;
		cmp  edx, ANIM_fall_back << 16;
		je   standup;
		cmp  edx, ANIM_fall_front << 16;
		jne  skip;
standup:
		jmp  fo::funcoffs::register_begin_;
skip:
		add  esp, 4;
		mov  edx, 0x4185AD;
		jmp  edx;
	}
}

////////////////////////////////////////////////////////////////////////////////

static __declspec(naked) void combat_over_hook() {
	__asm {
		test byte ptr [eax + damageFlags], DAM_DEAD;
		jnz  fix;
		jmp  fo::funcoffs::cai_attempt_w_reload_;
fix:
		retn; // the dead cannot reload their weapons
	}
}

static __declspec(naked) void combat_ctd_init_hack() {
	static const DWORD combat_ctd_init_Ret = 0x422F11;
	__asm {
		mov  [esi + 0x24], eax;                   // ctd.targetTile
		mov  eax, [ebx + whoHitMe];               // pobj.who_hit_me
		inc  eax;
		jnz  end;                                 // jump if whoHitMe != -1
		mov  [ebx + whoHitMe], eax;               // pobj.who_hit_me = 0
end:
		jmp  combat_ctd_init_Ret;
	}
}

static __declspec(naked) void obj_save_hack() {
	__asm { // edx - combat_data
		test byte ptr ds:[FO_VAR_combat_state], 1; // in combat?
		jnz  inCombat;                             // Yes
clear:
		mov  dword ptr [edx + 0x18], -1;           // combat_data.who_hit_me
		retn;
inCombat:
		cmp  dword ptr [edx], 0;                   // critter in combat?
		je   clear;                                // No
		// default
		mov  eax, [eax + cid];                     // pobj.who_hit_me.cid
		mov  [edx + 0x18], eax;                    // combat_data.who_hit_me
		retn;
	}
}

static __declspec(naked) void action_explode_hack() {
	using namespace fo::Scripts;
	__asm {
		mov  edx, destroy_p_proc;
		mov  eax, [esi + scriptId];               // pobj.sid
		call fo::funcoffs::exec_script_proc_;
		xor  edx, edx;
		dec  edx;
		retn;
	}
}

static __declspec(naked) void action_explode_hack1() {
	__asm {
		push esi;
		mov  esi, [esi + 0x40];                   // ctd.target#
		call action_explode_hack;
		pop  esi;
		retn;
	}
}

static __declspec(naked) void barter_attempt_transaction_hook_weight() {
	__asm {
		call fo::funcoffs::item_total_weight_;
		test eax, eax;
		jnz  skip;
		xor  edx, edx;
skip:
		retn;
	}
}

static __declspec(naked) void barter_attempt_transaction_hack() {
	__asm {
		mov  edx, [eax + protoId];
		cmp  edx, PID_ACTIVE_GEIGER_COUNTER;
		je   found;
		cmp  edx, PID_ACTIVE_STEALTH_BOY;
		je   found;
		mov  eax, 0x474D34;                       // Can't sell
		jmp  eax;
found:
		push 0x474D17;                            // Is there any other activated items among the ones being sold?
		jmp  fo::funcoffs::item_m_turn_off_;
	}
}

static __declspec(naked) void item_m_turn_off_hook() {
	__asm {
		and  byte ptr [eax + 0x25], ~0x20;        // Unset flag of used items
		jmp  fo::funcoffs::queue_remove_this_;
	}
}

static __declspec(naked) void combat_hack() {
	__asm {
		mov  eax, [ecx + eax];                     // eax = source combat turn
		push eax;
		mov  edx, STAT_max_move_points;
		call fo::funcoffs::stat_level_;
		mov  edx, ds:[FO_VAR_gcsd];
		test edx, edx;
		jz   skip;
		add  eax, [edx + 0x8];                     // gcsd.free_move
skip:
		pop  edx;                                  // source
		xchg edx, eax;                             // eax = source, edx = Max action points
		mov  [eax + movePoints], edx;              // pobj.curr_mp
		test byte ptr ds:[FO_VAR_combat_state], 1; // in combat?
		jz   end;                                  // No
		mov  edx, [eax + cid];                     // pobj.cid
		cmp  edx, -1;
		je   end;
		mov  esi, ds:[FO_VAR_aiInfoList];
		shl  edx, 4;
		mov  dword ptr [edx + esi + 0xC], 0;       // aiInfo.lastMove
end:
		// remove the flag set by critter_wake_up_ function if critter was knocked out
		and  dword ptr [eax + damageFlags], ~DAM_KNOCKOUT_WOKEN;
		mov  edx, edi;                             // dude_turn
		retn;
	}
}

static __declspec(naked) void db_get_file_list_hack() {
	__asm {
		push edi;
		push edx;
		xchg edi, eax;                            // edi = *filename
		mov  eax, [eax + 4];                      // file_lists.filenames
		lea  esi, [eax + edx];
		cld;
		push es;
		push ds;
		pop  es;
		xor  ecx, ecx;
		dec  ecx;
		mov  edx, ecx;
		mov  ebx, ecx;
		xor  eax, eax;                            // searching for end of line
		repne scasb;
		not  ecx;
		dec  ecx;
		xchg ebx, ecx;                            // ebx = filename length
		lea  edi, [esp + 0x200 + 6*4];
		repne scasb;
		not  ecx;
		xchg edx, ecx;                            // edx = extension length +1 for "end of line"
		mov  edi, [esi];
		repne scasb;
		not  ecx;                                 // ecx = buffer line length +1 for "end of line"
		pop  es;
		lea  eax, [ebx + edx];                    // eax = new line length
		cmp  eax, ecx;                            // new line length <= buffer line length?
		jbe  end;                                 // Yes
		mov  edx, [esi];
		xchg edx, eax;
		call fo::funcoffs::nrealloc_;             // eax = mem, edx = size
		test eax, eax;
		jnz  skip;
		push 0x50B2F0;                            // "Error: Ran out of memory!"
		call fo::funcoffs::debug_printf_;
		add  esp, 4;
		jmp  end;
skip:
		mov  [esi], eax;
end:
		xchg esi, eax;
		pop  edx;
		pop  edi;
		retn;
	}
}

static __declspec(naked) void gdActivateBarter_hook() {
	__asm {
		call fo::funcoffs::gdialog_barter_pressed_;
		cmp  ds:[FO_VAR_dialogue_state], ecx;
		jne  skip;
		cmp  ds:[FO_VAR_dialogue_switch_mode], esi;
		je   end;
skip:
		push ecx;
		push esi;
		push edi;
		push ebp;
		sub  esp, 0x18;
		push 0x44A5CC;
end:
		retn;
	}
}

static __declspec(naked) void switch_hand_hack() {
	__asm {
		mov  eax, ds:[FO_VAR_inven_dude];
		push eax;
		mov  [edi], ebp;
		inc  ecx;                                  // if ecx == -1
		jz   skip;
		xor  ebx, ebx;
		inc  ebx;
		mov  edx, ebp;
		push eax;
		push 0x4715F3;
		call SetRemoveObjectType;                 // call addr for HOOK_REMOVEINVENOBJ
		pop  eax;
		call fo::funcoffs::item_remove_mult_;
skip:
		pop  edx;                                 // _inven_dude
		mov  eax, ebp;
		call fo::funcoffs::item_get_type_;
		cmp  eax, item_type_container;
		jne  end;
		mov  [ebp + owner], edx;                  // iobj.owner = _inven_dude
end:
		pop  ebp;
		pop  edi;
		pop  esi;
		retn;
	}
}

static __declspec(naked) void inven_item_wearing() {
	__asm {
		mov  esi, ds:[FO_VAR_inven_dude];
		xchg ebx, eax;                            // ebx = source
		mov  eax, [esi + artFid];
		and  eax, 0xF000000;
		sar  eax, 0x18;
		test eax, eax;                            // check if object FID type flag is set to item
		jnz  skip;                                // No
		mov  eax, esi;
		call fo::funcoffs::item_get_type_;
		cmp  eax, item_type_container;            // Bag/Backpack?
		jne  skip;                                // No
		mov  eax, esi;
		call fo::funcoffs::obj_top_environment_;
		test eax, eax;                            // has an owner?
		jz   skip;                                // No
		mov  ecx, [eax + artFid];
		and  ecx, 0xF000000;
		sar  ecx, 0x18;
		cmp  ecx, OBJ_TYPE_CRITTER;               // check if object FID type flag is set to critter
		jne  skip;                                // No
		cmp  eax, ebx;                            // the owner of the bag == source?
		je   end;                                 // Yes
skip:
		xchg ebx, eax;
		cmp  eax, esi;
end:
		retn;
	}
}

static __declspec(naked) void inven_action_cursor_hack() {
	__asm {
		cmp  dword ptr [esp + 0x44 + 4], item_type_container;
		jne  end;
		cmp  eax, ds:[FO_VAR_stack];
		je   end;
		cmp  eax, ds:[FO_VAR_target_stack];
end:
		retn;
	}
}

static __declspec(naked) void use_inventory_on_hack() {
	__asm {
		inc  ecx;
		mov  edx, [eax];                          // Inventory.inv_size
		sub  edx, ecx;
		jge  end;
		mov  edx, [eax];                          // Inventory.inv_size
end:
		retn;
	}
}

static __declspec(naked) void Save_as_ASCII_hack() {
	__asm {
		mov  edx, STAT_sequence;
		mov  ebx, 626; // line index in EDITOR.MSG
		retn;
	}
}

static __declspec(naked) void combat_load_hook_critter() {
	__asm {
		push ecx;
		mov  edx, OBJ_TYPE_CRITTER;
		mov  ecx, eax;
		call game::Objects::FindObjectFromID;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void combat_load_hook_item() {
	__asm {
		push ecx;
		mov  edx, OBJ_TYPE_ITEM;
		mov  ecx, eax;
		call game::Objects::FindObjectFromID;
		pop  ecx;
		retn;
	}
}

static DWORD combatFreeMoveTmp = 0xFFFFFFFF;

static __declspec(naked) void combat_load_hook() {
	__asm {
		call fo::funcoffs::db_freadInt_;
		test eax, eax;                            // Successful?
		jnz  end;                                 // No
		push PERK_bonus_move;
		pop  edx;
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];
		call fo::funcoffs::perk_level_;
		test eax, eax;                            // Have the perk?
		jz   end;                                 // No
		mov  eax, ds:[FO_VAR_combat_free_move];   // eax = real value
		mov  combatFreeMoveTmp, eax;              // Save to the temp
		xor  eax, eax;
end:
		retn;
	}
}

static __declspec(naked) void combat_turn_hack() {
	__asm {
		mov  edx, combatFreeMoveTmp;
		cmp  edx, 0xFFFFFFFF;                     // Is there a real value?
		je   end;                                 // No
		mov  combatFreeMoveTmp, 0xFFFFFFFF;
		xchg edx, eax;
end:
		mov  ds:[FO_VAR_combat_free_move], eax;
		retn;
	}
}

static __declspec(naked) void combat_display_hack() {
	__asm {
		test [esi + flags], Flat;                 // ctd.mainTarget
		jnz  end;                                 // Main target is flat (engine jump)
		cmp  eax, OBJ_TYPE_CRITTER << 24;         // Is this a critter?
		je   end;                                 // Yes (engine no jump)
		cmp  dword ptr [ecx + scriptId], -1;      // Does the target have a script?
//		jne  hasScript;                           // Yes (engine no jump)
		lahf;
		xor  ah, 0x40; // invert ZF (01000000b)
		sahf;          // if there is a script, do not jump (ZF is set)
end:
		retn;
	}
}

static __declspec(naked) void apply_damage_hack() {
	__asm {
		xchg edx, eax;
		test [esi + 0x15], dl;                    // ctd.flags2Source & DAM_HIT_?
		jz   end;                                 // No
		inc  ebx;
end:
		retn;
	}
}

static __declspec(naked) void compute_attack_hook() {
	__asm {
		call fo::funcoffs::attack_crit_success_;
		test [esi + 0x15], 2;                     // ctd.flags2Source & DAM_CRITICAL_?
		jz   end;                                 // No
		cmp  dword ptr [esp + 0x20 + 4], 4;       // Has Silent Death perk?
		jne  end;                                 // No
		shl  eax, 1;                              // Multiply by 2 for the perk effect
end:
		retn;
	}
}

static __declspec(naked) void partyMemberGetCurLevel_hack() {
	__asm {
		mov  esi, 0xFFFFFFFF; // initialize party member index
		mov  edi, dword ptr ds:[FO_VAR_partyMemberMaxCount];
		retn;
	}
}

static __declspec(naked) void ResetPlayer_hook() {
	__asm {
		mov  edx, eax;
		call fo::funcoffs::stat_set_defaults_;
		mov  dword ptr [edx + 0x78], 100; // critter_data.base_dr_emp
		retn;
	}
}

static __declspec(naked) void obj_move_to_tile_hack() {
	static const DWORD obj_move_to_tile_Ret = 0x48A74E;
	__asm {
		cmp  ds:[FO_VAR_loadingGame], 0; // prevents leaving the map after loading a saved game if the player died
		jnz  skip;                       // on the world map from radiation (or in some cases on another map)
		cmp  dword ptr ds:[FO_VAR_map_state], 0; // map number, -1 exit to worldmap
		jz   mapLeave;
skip:
		add  esp, 4;
		jmp  obj_move_to_tile_Ret;
mapLeave:
		mov  ebx, 16;
		retn;
	}
}

static __declspec(naked) void obj_move_to_tile_hack_seen() {
	__asm {
		cmp  ds:[FO_VAR_loadingGame], 0;         // loading saved game
		jnz  end; // fix
		// if (map_state <= 0 && mapEntranceTileNum != -1) then fix
		cmp  dword ptr ds:[FO_VAR_map_state], 0; // map number, -1 exit to worldmap
		jle  skip;
		cmp  dword ptr ds:[FO_VAR_mapEntranceTileNum], -1;
		jne  end; // fix
skip:
		or   byte ptr ds:[FO_VAR_obj_seen][eax], dl;
end:
		retn;
	}
}

static __declspec(naked) void ai_combat_turn_run_hook() {
	__asm {
		call fo::funcoffs::combat_turn_run_;
		mov  edx, [esi + damageFlags]; // combat_data.results
		test edx, DAM_DEAD or DAM_KNOCKED_OUT or DAM_LOSE_TURN;
		jz   end;
		mov  dword ptr [esi + movePoints], 0; // pobj.curr_mp (source reset ap)
end:
		retn;
	}
}

// fix for zero damage instadeath criticals (moved from compute_damage hook)
static void __fastcall InstantDeathFix(fo::ComputeAttackResult &ctd) {
	if ((ctd.targetFlags & fo::DamageFlag::DAM_DEAD) && ctd.targetDamage <= 0) {
		ctd.targetDamage = 1; // set 1 hp damage
	}
}

static __declspec(naked) void compute_damage_hack() {
	static const DWORD ComputeDamageRet = 0x424BA7;
	__asm {
		mov  ecx, esi; // ctd
		call InstantDeathFix;
		// overwritten engine code
		add  esp, 0x34;
		pop  ebp;
		pop  edi;
		jmp  ComputeDamageRet;
	}
}

static int currDescLen = 0;
static bool showItemDescription = false;

static void __stdcall AppendText(const char* text, const char* desc) {
	if (showItemDescription && currDescLen == 0) {
		if (desc == nullptr) {
			desc = fo::util::MessageSearch(&fo::var::proto_main_msg_file, 493);
		}
		strncpy_s(messageBuffer, desc, 161);
		int len = strlen(messageBuffer);
		if (len > 160) {
			len = 158;
			messageBuffer[len++] = '.';
			messageBuffer[len++] = '.';
			messageBuffer[len++] = '.';
		}
		messageBuffer[len++] = ' ';
		messageBuffer[len] = 0;
		currDescLen = len;
	} else if (currDescLen == 0) {
		messageBuffer[0] = 0;
	}

	strncat(messageBuffer, text, 64);
	currDescLen += strlen(text);
	if (currDescLen < 300) {
		messageBuffer[currDescLen++] = '.';
		messageBuffer[currDescLen++] = ' ';
		messageBuffer[currDescLen] = 0;
	}
}

static __declspec(naked) void obj_examine_func_hack_ammo0() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14 + 4], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		push esi;
		push eax;
		call AppendText;
		retn;
skip:
		jmp  dword ptr [esp + 0x1AC - 0x14 + 4];
	}
}

static __declspec(naked) void obj_examine_func_hack_ammo1() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14 + 4], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		push 0;
		push eax;
		call AppendText;
		mov  currDescLen, 0;
		lea  eax, messageBuffer;
		jmp  fo::funcoffs::gdialogDisplayMsg_;
skip:
		jmp  dword ptr [esp + 0x1AC - 0x14 + 4];
	}
}

static __declspec(naked) void obj_examine_func_hack_weapon() {
	static const DWORD ObjExamineFuncWeapon_Ret = 0x49B63C;
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		push esi;
		push eax;
		call AppendText;
		mov  eax, currDescLen;
		sub  eax, 2;
		mov  byte ptr messageBuffer[eax], 0; // cutoff last character
		mov  currDescLen, 0;
		lea  eax, messageBuffer;
skip:
		jmp  ObjExamineFuncWeapon_Ret;
	}
}

static DWORD expSwiftLearner; // experience points for print

static __declspec(naked) void statPCAddExperienceCheckPMs_hack() {
	__asm {
		mov  expSwiftLearner, edi;
		cmp  dword ptr [esp + 0x24 + 4], 0x496CAA + 5; // called from perk_add_effect_ (PERK_here_and_now)
		jne  notHereAndNow;
		mov  dword ptr ds:[FO_VAR_hereAndNowExps], edi;
notHereAndNow:
		mov  eax, dword ptr ds:[FO_VAR_Experience_pc];
		retn;
	}
}

static __declspec(naked) void combat_give_exps_hook() {
	__asm {
		call fo::funcoffs::stat_pc_add_experience_;
		mov  ebx, expSwiftLearner;
		retn;
	}
}

static __declspec(naked) void loot_container_exp_hack() {
	static const DWORD LootContainerExp_Ret = 0x4745E3;
	__asm {
		mov  edx, [esp + 0x150 - 0x18];  // experience
		xchg edx, eax;
		call fo::funcoffs::stat_pc_add_experience_;
		// engine code
		cmp  edx, 1;                     // from eax
		jnz  skip;
		push expSwiftLearner;
		mov  ebx, [esp + 0x154 - 0x78 + 0x0C]; // msgfile.message
		push ebx;
		lea  eax, [esp + 0x158 - 0x150]; // buf
		push eax;
		call fo::funcoffs::sprintf_;
		add  esp, 0x0C;
		mov  eax, esp;
		call fo::funcoffs::display_print_;
		// end code
skip:
		jmp  LootContainerExp_Ret;
	}
}

static __declspec(naked) void wmRndEncounterOccurred_hook() {
	__asm {
		call fo::funcoffs::stat_pc_add_experience_;
		cmp  ecx, 110;
		jnb  skip;
		push expSwiftLearner;
		// engine code
		push edx;
		lea  eax, [esp + 0x08 + 4];
		push eax;
		call fo::funcoffs::sprintf_;
		add  esp, 0x0C;
		lea  eax, [esp + 4];
		call fo::funcoffs::display_print_;
		// end code
skip:
		retn;
	}
}

static __declspec(naked) void op_obj_can_see_obj_hack() {
	__asm {
		mov  eax, [esp + 0x2C - 0x28 + 4];  // source
		mov  ecx, [eax + tile];             // source.tile
		cmp  ecx, -1;
		jz   end;
		mov  eax, [eax + elevation];    // source.elev
		mov  edi, [edx + elevation];    // target.elev
		cmp  eax, edi;                  // check source.elev == target.elev
		retn;
end:
		dec  ecx;  // reset ZF
		retn;
	}
}

static __declspec(naked) void op_obj_can_hear_obj_hack() {
	__asm {
		mov  eax, [esp + 0x28 - 0x28 + 4];  // target
		mov  edx, [esp + 0x28 - 0x24 + 4];  // source
		retn;
	}
}

// correct signed division by 4
static __declspec(naked) void ai_best_weapon_hack() {
	__asm {
		add  edx, 3;
		test eax, eax;
		cmovs eax, edx;
		sar  eax, 1;
		retn;
	}
}

static __declspec(naked) void ai_best_weapon_hook() {
	__asm {
		mov  eax, [esp + 0xF4 - 0x10 + 4];  // prev.item
		jmp  fo::funcoffs::item_w_perk_;
	}
}

static __declspec(naked) void ai_search_inven_armor_hook() {
	__asm {
		call fo::funcoffs::item_ar_dt_;
		cmp  ebx, DMG_electrical;
		jne  skip;
		inc  ebx; // skip DMG_emp later
skip:
		retn;
	}
}

static __declspec(naked) void wmSetupRandomEncounter_hook() {
	__asm {
		push eax;                  // text 2
		push edi;                  // text 1
		push 0x500B64;             // fmt '%s %s'
		lea  edi, messageBuffer;
		push edi;                  // buf
		call fo::funcoffs::sprintf_;
		add  esp, 16;
		mov  eax, edi;
		jmp  fo::funcoffs::display_print_;
	}
}

static __declspec(naked) void inven_obj_examine_func_hack() {
	__asm {
		mov edx, dword ptr ds:[0x519064]; // inven_display_msg_line
		cmp edx, 2; // >2
		ja  fix;
		retn;
fix:
		cmp edx, 5; // 4 lines
		ja  limit;
		dec edx;
		sub eax, 3;
		mul edx;
		add eax, 3;
		retn;
limit:
		mov eax, 30;
		retn;
	}
}

int tagSkill4LevelBase = -1;
static __declspec(naked) void SliderBtn_hook_down() {
	__asm {
		call fo::funcoffs::skill_level_;
		cmp  tagSkill4LevelBase, -1;
		jnz  fix;
		retn;
fix:
		cmp  ds:[FO_VAR_tag_skill + 3 * 4], ebx;  // _tag_skill4, ebx = _skill_cursor
		jnz  skip;
		cmp  eax, tagSkill4LevelBase;             // curr > x2
		jg   skip;
		xor  eax, eax;
skip:
		retn;
	}
}

static __declspec(naked) void Add4thTagSkill_hook() {
	__asm {
		mov  edi, eax;
		call fo::funcoffs::skill_set_tags_;
		mov  eax, ds:[FO_VAR_obj_dude];
		mov  edx, dword ptr ds:[edi + 3 * 4];    // _temp_tag_skill4
		call fo::funcoffs::skill_level_;
		mov  tagSkill4LevelBase, eax;            // x2
		retn;
	}
}

static BYTE retrievePtr = 0;
static __declspec(naked) void ai_retrieve_object_hook() {
	__asm {
		mov  retrievePtr, 1;
		mov  edx, ebx;                          // object ptr
		call fo::funcoffs::inven_find_id_;      // check prt (fix behavior)
		mov  retrievePtr, 0;
		test eax, eax;
		jz   tryFindId;
		retn;
tryFindId:
		mov  eax, ecx;                          // source
		mov  edx, [ebx];                        // obj.id
		jmp  fo::funcoffs::inven_find_id_;      // vanilla behavior
	}
}

static __declspec(naked) void inven_find_id_hack() {
	__asm {
		mov  ebx, [edi + ebx];                 // inv.item
		test retrievePtr, 0xFF;
		jnz  fix;
		cmp  ecx, [ebx];                       // obj.id == obj.id
		retn;
fix:
		cmp  ecx, ebx;                         // object ptr == object ptr
		retn;
	}
}

static __declspec(naked) void op_start_gdialog_hack() {
	static const DWORD op_start_gdialog_ret = 0x456F4B;
	__asm {
		cmp  eax, -1;                                 // check mood arg
		jnz  useMood;
		mov  eax, dword ptr [esp + 0x3C - 0x30 + 4];  // fix dialog_target (overwritten engine code)
		retn;
useMood:
		add  esp, 4;                                  // Destroy the return address
		jmp  op_start_gdialog_ret;
	}
}

static __declspec(naked) void item_w_range_hook() {
	__asm {
		call fo::funcoffs::stat_level_;  // get ST
		lea  ecx, [eax + ebx];           // ebx - bonus from "Heave Ho!"
		sub  ecx, 10;                    // compare ST + bonus <= 10
		jle  skip;
		sub  ebx, ecx;                   // cutoff
skip:
		retn;
	}
}

static __declspec(naked) void determine_to_hit_func_hook() {
	__asm {
		test [esp + 0x38 + 0x8 + 4], 1; // isRange
		jz   noRange;
		jmp  fo::funcoffs::obj_dist_with_tile_;
noRange:
		mov  eax, 1;                    // set distance
		retn;
	}
}

static __declspec(naked) void process_rads_hack() {
	static const DWORD process_rads_Ret = 0x42D708;
	__asm {
		test ebp, ebp; // modifier: -1 - recovery, 1 - downgrade level
		jl   fix;
		test byte ptr [ecx + damageFlags], DAM_DEAD;
		jnz  fix;
		retn;
fix:
		add  esp, 4;
		jmp  process_rads_Ret;
	}
}

static long radEffectsMsgNum = 3003; // "You feel better" for removing effects

static __declspec(naked) void process_rads_hook() {
	__asm {
		test ebp, ebp;
		jg   skip;
		mov  esi, radEffectsMsgNum;
		mov  [esp + 0x20 - 0x20 + 4], esi;
skip:
		jmp  fo::funcoffs::message_search_;
	}
}

static __declspec(naked) void process_rads_hook_msg() {
	__asm {
		push eax; // death message for DialogOut
		call fo::funcoffs::display_print_;
		call GetLoopFlags;
		test eax, PIPBOY;
		jz   skip;
		mov  eax, 1;
		call fo::funcoffs::gmouse_set_cursor_;
skip:
		mov  ebx, dword ptr ds:[FO_VAR_game_user_wants_to_quit];
		mov  dword ptr ds:[FO_VAR_game_user_wants_to_quit], 0;
		call fo::func::DialogOut;
		mov  dword ptr ds:[FO_VAR_game_user_wants_to_quit], ebx;
		retn;
	}
}

/*static DWORD firstItemDrug = -1;

// when there are no more items in the inventory
static __declspec(naked) void ai_check_drugs_hack_break() {
	static const DWORD ai_check_drugs_hack_Ret = 0x42878B;
	__asm {
		mov  eax, -1;
		cmp  firstItemDrug, eax;
		jne  useDrugs;                     // pid != -1
		add  esp, 4;
		jmp  ai_check_drugs_hack_Ret;      // break loop
useDrugs: // use the first found item
		mov  dword ptr [esp + 4], eax;     // slot set -1
		mov  edi, firstItemDrug;
		mov  ebx, edi;
		mov  firstItemDrug, eax;           // set -1
		retn;                              // goto check (use drug)
	}
}

static __declspec(naked) void ai_check_drugs_hack_check() {
	__asm {
		test [esp + 0x34 - 0x30 + 4], 1;   // check NoInvenItem != 0
		jnz  skip;                         // there are no more drugs in inventory
		cmp  dword ptr [edx + 0xAC], -1;   // cap.chem_primary_desire (Chemical Preference Number)
		jnz  checkDrugs;
skip:
		xor  ebx, ebx;                     // set ZF for skipping preference list check
		retn;
checkDrugs:
		cmp  ebx, [edx + 0xAC];            // item.pid == cap.chem_primary_desire?
		retn;                              // if yes, use it (jz 0x4286C7); otherwise, check the other values of chem_primary_desire
	}
}

static __declspec(naked) void ai_check_drugs_hack_use() {
	static const DWORD ai_check_drugs_hack_Loop = 0x428675;
	__asm {
		cmp  eax, 3;                       // counter
		jge  beginLoop;
		retn;                              // use drug
beginLoop:
		cmp  firstItemDrug, -1;
		jne  skip;
		mov  firstItemDrug, edi;           // keep first found drug item
skip:
		add  esp, 4;
		jmp  ai_check_drugs_hack_Loop;     // goto begin loop, search next item
	}
}*/

static __declspec(naked) void cai_cap_save_hook() {
	__asm {
		add  esi, 4;
		mov  edx, [edx];
		jmp  fo::funcoffs::db_fwriteInt_;
	}
}

static __declspec(naked) void config_get_values_hack() {
	static const DWORD config_get_values_hack_Get = 0x42C13F;
	static const DWORD config_get_values_hack_OK = 0x42C14D;
	static const DWORD config_get_values_hack_Fail = 0x42C131;
	__asm {
		cmp ebp, 1;                        // counter value
		jl  getOK;
		jz  getLast;
		// if ebp > 1
		mov eax, [esp + 0x100];
		cmp byte ptr [eax], 0;             // check char
		jz  getFail;
		mov eax, dword ptr [esp + 0x114];  // total num of values
		sub eax, ebp;
		cmp eax, 1;
		ja  getFail;
getLast:
		jmp config_get_values_hack_Get;    // get last value
getOK:
		jmp config_get_values_hack_OK;
getFail:
		jmp config_get_values_hack_Fail;
	}
}

static __declspec(naked) void db_freadInt_hook() {
	__asm {
		call fo::funcoffs::xfread_;
		test eax, eax;
		jnz  skip;
		dec  eax;
skip:
		retn;
	}
}

static __declspec(naked) void op_attack_hook() {
	__asm {
		mov  esi, dword ptr [esp + 0x3C + 4]; // free_move
		mov  ebx, dword ptr [esp + 0x40 + 4]; // add amount damage to target
		jmp  fo::funcoffs::gdialogActive_;
	}
}

static __declspec(naked) void op_attack_hook_flags() {
	__asm {
		and  eax, 0xFF;
		shl  al, 1; // shift to bit 2
		test ebp, ebp;
		jz   skip;
		or   al, 1;
skip:
		// EAX: gcsd.changeFlags contains the attribute value for setting the flags
		// bit 1 - set result flags for target, bit 2 - set result flags for attacker
		test eax, eax;
		retn;
	}
}

static void __stdcall combat_attack_gcsd() {
	if (fo::var::gcsd->changeFlags & 2) { // only for AttackComplexFix
		long flags = fo::var::gcsd->flagsSource;
		if (flags & fo::DamageFlag::DAM_PRESERVE_FLAGS) {
			flags &= ~fo::DamageFlag::DAM_PRESERVE_FLAGS;
			flags |= fo::var::main_ctd.attackerFlags;
		} else {
			flags |= fo::var::main_ctd.attackerFlags & (fo::DamageFlag::DAM_HIT | fo::DamageFlag::DAM_DEAD); // don't unset DAM_HIT and DAM_DEAD flags
		}
		fo::var::main_ctd.attackerFlags = flags;
	}
	if (fo::var::gcsd->changeFlags & 1) {
		long flags = fo::var::gcsd->flagsTarget;
		if (flags & fo::DamageFlag::DAM_PRESERVE_FLAGS) {
			flags &= ~fo::DamageFlag::DAM_PRESERVE_FLAGS;
			flags |= fo::var::main_ctd.targetFlags;
		} else {
			flags |= fo::var::main_ctd.targetFlags & fo::DamageFlag::DAM_DEAD; // don't unset DAM_DEAD flag (fix death animation)
		}
		fo::var::main_ctd.targetFlags = flags;
	}

	if (fo::var::main_ctd.attackerFlags & fo::DamageFlag::DAM_HIT) {
		long damage = fo::var::main_ctd.targetDamage;
		fo::var::main_ctd.targetDamage += fo::var::gcsd->bonusDamage;
		if (fo::var::main_ctd.targetDamage < fo::var::gcsd->minDamage) {
			fo::var::main_ctd.targetDamage = fo::var::gcsd->minDamage;
		}
		if (damage < fo::var::main_ctd.targetDamage) { // check the hit points and set the DAM_DEAD flag
			fo::func::check_for_death(fo::var::main_ctd.target, fo::var::main_ctd.targetDamage, &fo::var::main_ctd.targetFlags);
		}

		if (fo::var::main_ctd.targetDamage > fo::var::gcsd->maxDamage) {
			fo::var::main_ctd.targetDamage = fo::var::gcsd->maxDamage;
		}
		if (damage > fo::var::main_ctd.targetDamage && fo::var::main_ctd.target->IsCritter()) {
			long cHP = fo::var::main_ctd.target->critter.health;
			if (cHP > fo::var::gcsd->maxDamage && cHP <= damage) {
				fo::var::main_ctd.targetFlags &= ~fo::DamageFlag::DAM_DEAD; // unset
			}
		}
	}
}

static __declspec(naked) void combat_attack_hack() {
	__asm {
		push 0x423039; // return addr
		jmp  combat_attack_gcsd;
	}
}

static __declspec(naked) void op_use_obj_on_obj_hack() {
	__asm {
		test eax, eax; // source
		jz   fail;
		mov  edx, [eax + protoId];
		shr  edx, 24;
		cmp  dword ptr [esp + 4], eax; // target != source
		jne  skip;
		xor  edx, edx; // for calling obj_use_item_on_ instead of action_use_an_item_on_object_
skip:
		retn;
fail:
		add  esp, 4;
		mov  edx, 0x45C3A3; // exit func
		jmp  edx;
	}
}

static __declspec(naked) void op_use_obj_hack() {
	__asm {
		test eax, eax; // source
		jz   fail;
		mov  edx, [eax + protoId];
		shr  edx, 24;
		retn;
fail:
		add  esp, 4;
		mov  edx, 0x456ABA; // exit func
		jmp  edx;
	}
}

static __declspec(naked) void combat_hack_load() {
	static const DWORD combat_End = 0x422E45;
	static const DWORD combat_Load = 0x422E91;
	__asm {
		cmp  eax, -1;
		je   skip;
		retn;
skip:
		add  esp, 4; // destroy addr
		cmp  ds:[FO_VAR_combat_end_due_to_load], 0;
		jnz  isLoad;
		jmp  combat_End;
isLoad:
		jmp  combat_Load;
	}
}

static __declspec(naked) void JesseContainerFid() {
	__asm {
		dec  edx; // set fid to -1
		jmp  fo::funcoffs::obj_new_;
	}
}

static __declspec(naked) void ai_search_inven_weap_hook0() {
	__asm {
		call fo::funcoffs::item_w_subtype_;
		cmp  eax, THROWING;
		jne  fix;
		retn;
fix:
		xor  eax, eax;
		mov  edx, [esi + ammoPid];
		test edx, edx;
		js   skip;
		mov  eax, RANGED; // set RANGED if has ammo pid
skip:
		retn;
	}
}

static __declspec(naked) void ai_search_inven_weap_hook1() {
	static const DWORD ai_search_inven_weap_next = 0x4299E3;
	__asm {
		call fo::funcoffs::inven_find_type_;
		test eax, eax; // found weapon object
		jz   skip;
		cmp  dword ptr [eax + protoId], PID_SOLAR_SCORCHER;
		jne  skip;
		cmp  dword ptr [eax + charges], 0;
		jne  skip;
		push eax;
		call fo::funcoffs::light_get_ambient_;
		cmp  eax, 62259;
		pop  eax;
		jg   skip;
		add  esp, 4;
		jmp  ai_search_inven_weap_next; // skip empty solar scorcher when it's dark
skip:
		retn;
	}
}

static __declspec(naked) void map_age_dead_critters_hack() {
	__asm {
		test ecx, ecx; // dead_bodies_age
		jz   skip;     // if (dead_bodies_age == No) exit func
		cmp  dword ptr [esp + 0x3C - 0x30 + 4], 0;
skip:
		retn;
	}
}

static __declspec(naked) void partyFixMultipleMembers_hack() {
	__asm {
		cmp  esi, edx;
		je   skip;
		mov  eax, [esi + protoId];
		sar  eax, 24;
		cmp  eax, OBJ_TYPE_CRITTER;
		jne  skip;
		test [esi + damageFlags], DAM_DEAD;
		cmovnz edx, esi;
skip:
		retn;
	}
}

static __declspec(naked) void PrintAutoMapList() {
	__asm {
		mov  eax, ds:[FO_VAR_wmMaxMapNum];
		cmp  eax, AUTOMAP_MAX;
		jb   skip;
		mov  eax, AUTOMAP_MAX;
skip:
		retn;
	}
}

static __declspec(naked) void automap_pip_save_hook() {
	__asm {
		mov  eax, ds:[FO_VAR_map_number];
		cmp  eax, AUTOMAP_MAX;
		jb   skip;
		xor  eax, eax;
skip:
		retn;
	}
}

static __declspec(naked) void map_load_file_hook() {
	__asm {
		mov  eax, 0x04004650; // dude script ID
		call fo::funcoffs::scr_remove_;
		jmp  fo::funcoffs::partyMemberRecoverLoad_;
	}
}

static DWORD dudeScriptID; // usually equal to 0x04004650

static __declspec(naked) void obj_load_dude_hook0() {
	__asm {
		mov  eax, ds:[FO_VAR_obj_dude];
		mov  eax, [eax + scriptId];
		mov  dudeScriptID, eax;
		retn;
	}
}

static __declspec(naked) void obj_load_dude_hook1() {
	__asm {
		call fo::funcoffs::scr_clear_dude_script_;
		mov  ebx, ds:[FO_VAR_obj_dude];
		mov  eax, dudeScriptID;
		mov  [ebx + scriptId], eax;
		retn;
	}
}

static __declspec(naked) void PrintAMList_hook() {
	__asm {
		cmp  ebp, 20; // max line count
		jle  skip;
		mov  ebp, 20;
skip:
		jmp  fo::funcoffs::qsort_;
	}
}

static __declspec(naked) void exec_script_proc_hack() {
	__asm {
		test edi, edi; // loading?
		jnz  end;
		mov  eax, [esi + 0x58]; // script.procedure_table.start
		test eax, eax;
		ja   end; // != 0
		dec  eax; // is bad - set to -1 for skipping execution
end:
		retn;
	}
}

static __declspec(naked) void exec_script_proc_hack1() {
	__asm {
		mov  esi, [edi + 0x58]; // script.procedure_table.start
		test esi, esi;
		ja   end; // != 0
		inc  esi; // 1 - default position
end:
		retn;
	}
}

static __declspec(naked) void op_dialogue_reaction_hook() {
	__asm {
		cmp  eax, 4; // neutral fidget
		mov  eax, 1;
		jb   good;
		je   neutral;
		jmp  bad;
good:
		dec  eax;
neutral:
		dec  eax;
bad:
		jmp  fo::funcoffs::talk_to_critter_reacts_; // -1 - good, 0 - neutral, 1 - bad
	}
}

static __declspec(naked) void obj_pickup_hook() {
	__asm {
		cmp  edi, dword ptr ds:[FO_VAR_obj_dude];
		je   dude;
		test byte ptr ds:[FO_VAR_combat_state], 1; // in combat?
		jz   dude;
		jmp  fo::funcoffs::item_add_force_;
dude:
		jmp  fo::funcoffs::item_add_mult_;
	}
}

static char pickupMessage[65] = {0};

static const char* __fastcall GetPickupMessage(const char* name) {
	std::sprintf(messageBuffer, pickupMessage, name);
	return messageBuffer;
}

static __declspec(naked) void obj_pickup_hook_message() {
	__asm {
		cmp  edi, dword ptr ds:[FO_VAR_obj_dude];
		je   dude;
		mov  eax, edi;
		call fo::funcoffs::critter_name_;
		mov  ecx, eax;
		call GetPickupMessage;
		mov  [esp + 0x34 - 0x28 + 4], eax;
		mov  eax, 1;
		retn;
dude:
		jmp  fo::funcoffs::message_search_;
	}
}

static long blockingTileObj = 0;
static __declspec(naked) void anim_move_to_tile_hook() {
	static const DWORD anim_move_to_tile_jmp = 0x416D91;
	__asm {
		call fo::funcoffs::obj_blocking_at_;
		mov  blockingTileObj, eax;
		cmp  edi, ds:[FO_VAR_obj_dude];
		je   isDude;
		retn;
isDude:
		test eax, eax;
		jnz  skip; // tile is blocked
		mov  ebx, dword ptr [esp + 0x18 - 0x10 + 4]; // distance
		test ebx, ebx;
		jl   skip; // dist < 0
		sub  ebx, ds:[FO_VAR_combat_free_move];
		cmp  ebx, [edi + movePoints];
		jge  skip; // dist >= source.curr_mp
		test eax, eax;
		jnz  skip;
		add  esp, 4;
		jmp  anim_move_to_tile_jmp;
skip:
		retn;
	}
}

static __declspec(naked) void anim_move_to_tile_hook_tile() {
	__asm {
		cmp  blockingTileObj, 0;
		jne  getTile;
		retn;
getTile:
		jmp  fo::funcoffs::tile_num_in_direction_;
	}
}

static __declspec(naked) void action_use_an_item_on_object_hack() {
	__asm {
		add  ebx, ds:[FO_VAR_combat_free_move];
		mov  eax, 2; // RB_RESERVED
		retn;
	}
}

static __declspec(naked) void action_climb_ladder_hack() {
	__asm {
		add  ecx, ds:[FO_VAR_combat_free_move];
		mov  eax, 2; // RB_RESERVED
		retn;
	}
}

static __declspec(naked) void wmTeleportToArea_hack() {
	static const DWORD wmTeleportToArea_Ret = 0x4C5A77;
	__asm {
		xor  ecx, ecx;
		cmp  ebx, ds:[FO_VAR_WorldMapCurrArea];
		je   end;
		mov  ds:[FO_VAR_WorldMapCurrArea], ebx;
		sub  eax, edx;
		add  eax, ds:[FO_VAR_wmAreaInfoList];
		cmp  dword ptr [eax + 0x34], 1;           // wmAreaInfoList.size
		mov  edx, [eax + 0x30];                   // wmAreaInfoList.world_posy
		mov  eax, [eax + 0x2C];                   // wmAreaInfoList.world_posx
		jg   largeLoc;
		je   mediumLoc;
//smallLoc:
		sub  eax, 5;
		lea  edx, [edx - 5];
mediumLoc:
		sub  eax, 10
		lea  edx, [edx - 10];
		// check negative values
		test  eax, eax;
		cmovl eax, ecx;
		test  edx, edx;
		cmovl edx, ecx;
largeLoc:
		mov  ds:[FO_VAR_world_ypos], edx;
		mov  ds:[FO_VAR_world_xpos], eax;
end:
		mov  ds:[FO_VAR_target_xpos], ecx;
		mov  ds:[FO_VAR_target_ypos], ecx;
		mov  ds:[FO_VAR_In_WorldMap], ecx;
		jmp  wmTeleportToArea_Ret;
	}
}

static __declspec(naked) void wmAreaMarkVisitedState_hack() {
	static const DWORD wmAreaMarkVisitedState_Ret = 0x4C46A2;
	//static const DWORD wmAreaMarkVisitedState_Error = 0x4C4698;
	static long isNoRadius;

	isNoRadius = Worldmap::AreaMarkStateIsNoRadius(); // F1 behavior radius
	__asm {
		mov  [ecx + 0x40], esi; // wmAreaInfoList.visited
		test esi, esi;          // mark "unknown" state
		jz   noRadius;
		mov  eax, [ecx + 0x2C]; // wmAreaInfoList.world_posx
		mov  edx, [ecx + 0x30]; // wmAreaInfoList.world_posy
		// fix loc coordinates
		cmp  dword ptr [ecx + 0x34], 1; // wmAreaInfoList.size
		jg   largeLoc;
		je   mediumLoc;
//smallLoc:
		sub  eax, 5;
		lea  edx, [edx - 5];
mediumLoc:
		sub  eax, 10;
		lea  edx, [edx - 10];
		// check negative values
		push ecx;
		xor  ecx, ecx;
		test eax, eax;
		cmovl eax, ecx;
		test edx, edx;
		cmovl edx, ecx;
		pop  ecx;
largeLoc:
		lea  ebx, [esp]; // ppSubTile out
		push edx;
		push eax;
		call fo::funcoffs::wmFindCurSubTileFromPos_;
//		cmp  eax, -1; // always return 0
//		jz   error;
		pop  eax;
		pop  edx;
		mov  ebx, [esp];
		mov  ebx, [ebx + 0x18]; // sub-tile state: 0 - black, 1 - uncovered, 2 - visited
		test ebx, ebx;
		jnz  skip;
		inc  ebx; // set 1
skip:
		///////// check F1 behavior radius result /////////
		cmp isNoRadius, 1;
		je  noRadius;
		///////////////////////////////////////////////////
		cmp  dword ptr [ecx + 0x38], 1; // wmAreaInfoList.start_state
		jne  noRadius;        // hidden location
		cmp  esi, 2;          // mark visited state
		jne  fixRadius;
		call fo::funcoffs::wmMarkSubTileRadiusVisited_;
noRadius:
		jmp  wmAreaMarkVisitedState_Ret;
fixRadius:
		push ebx;
		mov  ebx, 1; // radius (fix w/o PERK_scout)
		call fo::funcoffs::wmSubTileMarkRadiusVisited_;
		pop  ebx;
		jmp  wmAreaMarkVisitedState_Ret;
//error:
//		add  esp, 8;
//		jmp  wmAreaMarkVisitedState_Error;
	}
}

static __declspec(naked) void wmWorldMap_hack() {
	__asm {
		cmp  dword ptr [ebx + 0x34], 1; // wmAreaInfoList.size
		mov  ebx, 0;
		jg   largeLoc;
		je   mediumLoc;
//smallLoc:
		sub  eax, 5;
		lea  edx, [edx - 5];
mediumLoc:
		sub  eax, 10;
		lea  edx, [edx - 10];
		// check negative values
		test eax, eax;
		cmovl eax, ebx;
		test edx, edx;
		cmovl edx, ebx;
largeLoc:
		jmp  fo::funcoffs::wmPartyInitWalking_;
	}
}

static __declspec(naked) void wmTownMapFunc_hack() {
	__asm {
		cmp  dword ptr [edi][eax * 4 + 0], 0;  // Visited
		je   end;
		cmp  dword ptr [edi][eax * 4 + 4], -1; // Xpos
		je   end;
		cmp  dword ptr [edi][eax * 4 + 8], -1; // Ypos
		je   end;
		// engine code
		mov  edx, [edi][eax * 4 + 0xC];
		mov  [esi], edx
		retn;
end:
		add  esp, 4; // destroy the return address
		mov  eax, 0x4C4976;
		jmp  eax;
	}
}

static bool __fastcall combat_should_end_check_fix(long dudeTeam, fo::GameObject* critter, fo::GameObject* target) {
	// target: the current target of the critter (does not need to be checked for null)

	if (critter->critter.teamNum == dudeTeam) { // critter is in the player's team
		// EnemyOutOfRange - set when the critter does not want (can't continue) combat or has left the combat due to exceeding the max distance
		if ((target->critter.combatState & fo::CombatStateFlag::EnemyOutOfRange) == false && target->critter.IsNotDead()) return false;
	} else {
		if (target->critter.teamNum == dudeTeam) return false; // don't end combat: target is from the player's team
		// for other targets
		if (target->critter.IsNotDead()) return false; // don't end combat: target is still alive
		//if (critter->critter.combatState & fo::CombatStateFlag::InCombat) return false; // critter is in combat
	}
	return true; // check next critter
}

static __declspec(naked) void combat_should_end_hack() {
	static const DWORD combat_should_end_break = 0x422D00;
	__asm { // ecx = dude.team_num; ebp = critter->who_hit_me
		push eax;
		push ecx;
		sub  edx, 0x3C; // critter
		push ebp;       // target
		call combat_should_end_check_fix;
		test al, al;
		jz   break;
		pop  ecx;
		pop  eax;
		retn; // check next critter
break:
		add  esp, 8+4;
		jmp  combat_should_end_break; // break check loop (don't end combat)
	}
}

static __declspec(naked) void wmInterfaceInit_hack() {
	__asm {
		mov  eax, GVAR_CAR_PLACED_TILE;
		cmp  eax, dword ptr ds:[FO_VAR_num_game_global_vars];
		jge  skip;
		mov  edx, ds:[FO_VAR_game_global_vars];
		lea  edx, [edx + eax * 4];
		mov  dword ptr [edx], -1; // set gvar
skip:
		mov  edx, 12;
		retn;
	}
}

static long __fastcall GetFreeTilePlacement(long elev, long tile) {
	long count = 0, dist = 1;
	long checkTile = tile;
	long rotation = fo::var::rotation;
	while (fo::func::obj_blocking_at(0, checkTile, elev)) {
		checkTile = fo::func::tile_num_in_direction(checkTile, rotation, dist);
		if (++count > 5 && ++dist > 5) return tile;
		if (++rotation > 5) rotation = 0;
	}
	return checkTile; // free tile
}

static __declspec(naked) void map_check_state_hook() {
	__asm {
		mov  ecx, esi; // elev
		call GetFreeTilePlacement; // edx - tile
		mov  edx, eax; // tile
		// restore
		mov  ebx, esi; // elev
		mov  eax, ds:[FO_VAR_obj_dude];
		xor  ecx, ecx;
		jmp  fo::funcoffs::obj_move_to_tile_;
	}
}

static long __fastcall MultiHexPlacement(fo::GameObject* source) {
	//if (!(source->flags & fo::ObjectFlag::MultiHex)) return 0;
	long elevation = fo::var::obj_dude->elevation;
	long dudeTile = fo::var::obj_dude->tile;
	long dudeRot = (fo::var::obj_dude->rotation + 3) % 6; // invert rotation

	long count = 2;
	do {
		for (long distance = 2; distance < 8; distance++) {
			long tilePlace = fo::func::tile_num_in_direction(dudeTile, dudeRot, distance);
			if (fo::func::wmEvalTileNumForPlacement(tilePlace)) {
				for (long dir = 5; dir >= 0; dir--) {
					if (fo::func::obj_blocking_at(source, fo::func::tile_num_in_direction(tilePlace, dir, 1), elevation)) {
						tilePlace = 0;
						break;
					}
				}
				if (tilePlace) {
					__asm {
						xor  edx, edx;
						mov  eax, source;
						call fo::funcoffs::obj_turn_on_;
						xor  ecx, ecx;
						mov  ebx, elevation;
						mov  edx, tilePlace;
						mov  eax, source;
						call fo::funcoffs::obj_move_to_tile_;
					}
					return 1; // next PM
				}
			}
		}
		dudeRot = fo::var::obj_dude->rotation;
	} while (--count);
	return 0; // default placement
}

static __declspec(naked) void partyMemberSyncPosition_hack() {
	__asm {
		and  eax, 0x0F000000;
		cmp  eax, OBJ_TYPE_CRITTER << 24;
		jne  default;
		test [edx + flags + 1], MultiHex >> 8;
		jnz  isMultiHex;
		xor  eax, eax;
default:
		retn;
isMultiHex:
		mov  ecx, edx;
		call MultiHexPlacement;
		test eax, eax;
		retn;
	}
}

static __declspec(naked) void op_critter_rm_trait_hook() {
	__asm {
		mov  ebx, [esp + 0x34 - 0x34 + 4]; // amount
		test ebx, ebx;
		jz   skip;
		call fo::funcoffs::perk_level_;
		test eax, eax;
		jz   end;
		dec  dword ptr [esp + 0x34 - 0x34 + 4];
		test ebx, ebx;
		jnz  end; // continue if amount != 0
skip:
		xor  eax, eax; // exit loop
end:
		retn;
	}
}

static __declspec(naked) void op_critter_add_trait_hook() {
	__asm {
		mov  edi, edx; // perk id
		mov  ebp, eax; // source
		mov  ebx, [esp + 0x34 - 0x34 + 4]; // amount
addLoop:
		call fo::funcoffs::perk_add_force_;
		test eax, eax;
		jnz  end; // can't add
		mov  edx, edi;
		mov  eax, ebp;
		dec  ebx;
		jnz  addLoop;
		xor  eax, eax;
end:
		retn;
	}
}

static const DWORD CorpseShotBlockFix_continue_loop[] = {0x48B99B, 0x48BA0B};
static __declspec(naked) void obj_shoot_blocking_at_hack0() {
	__asm {
		mov  edx, eax;
		mov  eax, [eax];
		call fo::funcoffs::critter_is_dead_; // found some object, check if it's a dead critter
		test eax, eax;
		jz   endLoop; // if not, allow breaking the loop (will return this object)
		mov  eax, edx;
		jmp  CorpseShotBlockFix_continue_loop[0]; // otherwise continue searching
endLoop:
		mov  eax, [edx];
		pop  ebp;
		pop  edi;
		pop  esi;
		pop  ecx;
		retn;
	}
}

// same logic as above, for different loop
static __declspec(naked) void obj_shoot_blocking_at_hack1() {
	__asm {
		mov  eax, [edx];
		call fo::funcoffs::critter_is_dead_;
		test eax, eax;
		jz   endLoop;
		jmp  CorpseShotBlockFix_continue_loop[4];
endLoop:
		mov  eax, [edx];
		pop  ebp;
		pop  edi;
		pop  esi;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void main_death_scene_hook() {
	__asm {
		mov  eax, 100;
		call fo::funcoffs::block_for_tocks_;
		jmp  fo::funcoffs::get_time_;
	}
}

static __declspec(naked) void action_loot_container_hack() {
	__asm {
		cmp dword ptr [esp + 0x10 + 4], 0x44C1D9 + 5;
		je  fix;
		xor eax, eax; // set ZF
		retn;
fix:
		sub  esp, 4;
		mov  edx, esp;
		call fo::funcoffs::proto_ptr_;
		mov  edx, [esp];
		add  esp, 4;
		test [edx + 0x20], NoSteal; // critter flags
		jnz  look;
		retn;
look:
		mov  eax, esi;
		mov  edx, edi;
		call fo::funcoffs::obj_examine_;
		or   eax, 1; // unset ZF
		retn;
	}
}

static void FixCreateBarterButton() {
	const long artID = fo::OBJ_TYPE_INTRFACE << 24;
	*(BYTE**)FO_VAR_dialog_red_button_up_buf = fo::func::art_ptr_lock_data(artID | 96, 0 ,0, (DWORD*)FO_VAR_dialog_red_button_up_key);
	*(BYTE**)FO_VAR_dialog_red_button_down_buf = fo::func::art_ptr_lock_data(artID | 95, 0 ,0, (DWORD*)FO_VAR_dialog_red_button_down_key);
}

static __declspec(naked) void gdialog_window_create_hook() {
	__asm {
		call fo::funcoffs::art_ptr_unlock_;
		cmp  dword ptr ds:[FO_VAR_dialog_red_button_down_buf], 0;
		jz   FixCreateBarterButton;
		retn;
	}
}

static __declspec(naked) void gdialog_bk_hook() {
	__asm {
		xor  ebp, ebp;
		mov  eax, ds:[FO_VAR_curr_font_num];
		cmp  eax, 101;
		je   skip0;
		mov  ebp, eax;
		mov  eax, 101; // set font
		call fo::funcoffs::text_font_;
skip0:
		mov  eax, ds:[FO_VAR_obj_dude];
		call fo::funcoffs::item_caps_total_;
		push eax;      // caps
		push 0x502B1C; // fmt: $%d
		lea  eax, messageBuffer;
		push eax;
		call fo::funcoffs::sprintf_;
		add  esp, 3 * 4;
		lea  eax, messageBuffer;
		call ds:[FO_VAR_text_width];
		mov  edx, 60;  // max width
		mov  ebx, eax; // ebx - textWidth
		cmp  eax, edx;
		cmova ebx, edx;
		movzx eax, ds:[FO_VAR_GreenColor];
		or   eax, 0x7000000; // print flags
		push eax;
		mov  eax, ebx;
		mov  ecx, 38;  // x
		push 36;       // y
		sar  eax, 1;
		sub  ecx, eax; // x shift
		lea  edx, messageBuffer;
		mov  eax, ds:[FO_VAR_dialogueWindow];
		call fo::funcoffs::win_print_;
		test ebp, ebp;
		jz   skip1;
		mov  eax, ebp;
		call fo::funcoffs::text_font_;
skip1:
		mov  eax, edi;
		jmp  fo::funcoffs::win_show_;
	}
}

static __declspec(naked) void gdReviewDisplay_hack() {
	__asm {
		mov  ebx, eax;
		mov  eax, -3;
		cmp  ebx, 407; // vertical position limit
		cmovge ecx, eax;
		cmp  ecx, eax;
		retn;
	}
}

static __declspec(naked) void combat_hook() {
	__asm {
		call fo::funcoffs::combat_should_end_;
		test eax, eax;
		jnz  skip;
		cmp  ds:[FO_VAR_game_user_wants_to_quit], 2;
		je   skip;
		//cmp  ds:[FO_VAR_script_engine_running], 1;
		//jne  skip;
		call fo::funcoffs::GNW_do_bk_process_;
		call fo::funcoffs::combat_turn_run_;
		xor  eax, eax;
skip:
		retn;
	}
}

static __declspec(naked) void wmSubTileMarkRadiusVisited_hack() {
	static const DWORD wmSubTileMarkRadiusVisited_Ret = 0x4C3730;
	__asm {
		call fo::funcoffs::wmMarkSubTileOffsetVisitedFunc_;
		cmp  ebp, 7; // count of horizontal sub-tiles
		je   fix;
		jmp  wmSubTileMarkRadiusVisited_Ret;
fix:
		test esi, esi; // if this is zero, then need to apply the fix
		jz   checkTiles;
jback:
		xor  ecx, ecx;
		dec  esi;
		cmovnz ebp, ecx; // ebp=0 to continue uncovering sub-tiles
		dec  dword ptr [esp + 0x1C - 0x14]; // subtract one tile from the left
		jmp  wmSubTileMarkRadiusVisited_Ret;
checkTiles:
		mov  eax, ds:[FO_VAR_world_xpos]; // player's X position
		mov  ecx, 350;
		xor  edx, edx;
		div  ecx; // eax: count of tiles on the left of the player's position
		mov  esi, eax;
		cmp  eax, 1;
		jg   jback;
		jmp  wmSubTileMarkRadiusVisited_Ret;
	}
}

static __declspec(naked) void doBkProcesses_hook() {
	__asm {
		call fo::funcoffs::gdialogActive_;
		test eax, eax;
		jz   skip;
		retn;
skip:
		jmp  fo::funcoffs::gmovieIsPlaying_;
	}
}

static bool dudeIsAnimDeath = false;

static __declspec(naked) void show_damage_to_object_hack() {
	static const DWORD show_damage_to_object_Ret = 0x410B90;
	__asm {
		jnz  isDeath;
		add  esp, 4;
		jmp  show_damage_to_object_Ret;
isDeath:
		cmp  esi, ds:[FO_VAR_obj_dude];
		sete dudeIsAnimDeath;
		retn;
	}
}

static __declspec(naked) void obj_move_to_tile_hack_ondeath() {
	static const DWORD obj_move_to_tile_Ret = 0x48A759;
	__asm {
		jz   skip;
		cmp  dudeIsAnimDeath, 0;
		jnz  skip;
		retn;
skip:
		add  esp, 4;
		jmp  obj_move_to_tile_Ret;
	}
}

static __declspec(naked) void action_knockback_hack() {
	__asm {
		mov  ecx, 20; // cap knockback distance
		cmp  ebp, ecx;
		cmovg ebp, ecx;
		mov  ecx, 1;
		retn;
	}
}

static __declspec(naked) void check_door_state_hack_close() {
	__asm {
		mov  eax, esi;
		call fo::funcoffs::obj_is_a_portal_;
		test eax, eax;
		jz   skip;
		and  dword ptr [esi + flags], ~(NoBlock | LightThru | ShootThru);
skip:
		retn;
	}
}

static __declspec(naked) void check_door_state_hack_open() {
	__asm {
		mov  eax, esi;
		call fo::funcoffs::obj_is_a_portal_;
		test eax, eax;
		jz   skip;
		or   ecx, (NoBlock | LightThru | ShootThru);
skip:
		retn;
	}
}

static BYTE fixRegion = 0;

static __declspec(naked) void checkAllRegions_hack() {
	static const DWORD checkAllRegions_BackRet = 0x4B6C40;
	static const DWORD checkAllRegions_FixRet = 0x4B6AAB;
	__asm {
		test eax, eax;
		jnz  skip; // != 0
		cmp  dword ptr ds:[FO_VAR_lastWin], -1;
		je   skip;
		mov  fixRegion, 1;
		jmp  checkAllRegions_FixRet; // trigger the leave event for _lastWin
skip:
		add  esp, 0x10;
		pop  ebp;
		pop  edi;
		jmp  checkAllRegions_BackRet;
	}
}

static __declspec(naked) void checkAllRegions_hook() {
	__asm {
		test byte ptr fixRegion, 1;
		jnz  skip; // == 1
		jmp  fo::funcoffs::windowCheckRegion_;
skip:
		mov  fixRegion, 0;
		mov  dword ptr ds:[FO_VAR_lastWin], -1;
		retn;
	}
}

static __declspec(naked) void win_show_hack() {
	__asm {
		xor  ebx, ebx;
		lea  edx, [esi + 0x8]; // window.rect
		mov  eax, esi;
		call fo::funcoffs::GNW_win_refresh_;
		pop  esi;
		pop  edx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static bool createObjectSidStartFix = false;

static __declspec(naked) void op_create_object_sid_hack() {
	static const char* proDbgMsg = "\nError: attempt to create object with PID of %d: %s!";
	using fo::Scripts::start;
	__asm {
		mov  ebx, [esp + 0x50 - 0x20 + 4]; // createObj
		test ebx, ebx;
		jz   noObject;
		mov  ecx, [ebx + scriptId];
		cmp  ecx, -1;
		jne  setScrIdx;
		mov  edx, ebx;
		mov  eax, esi;
		retn;
setScrIdx:
		mov  eax, [esp + 0x50 - 0x40 + 4]; // scriptIndex
		cmp  eax, -1;
		je   init;
		dec  eax;
		mov  [ebx + scriptIndex], eax;
init:
		call LoadGameHook::IsMapLoading;
		test al, al;
		jnz  end; // yes - initialization will be performed by engine functions
		cmp  createObjectSidStartFix, 0;
		jne  runStart;
		call ScriptExtender::InitScript;
		mov  edx, ebx;
		mov  eax, esi;
		retn;
runStart:
		mov  edx, start; // procedure
		mov  eax, ecx;
		call fo::funcoffs::exec_script_proc_;
end:
		mov  edx, ebx;
		mov  eax, esi;
		retn;
noObject:
		push [esi];
		push [esp + 0x50 - 0x34 + 8]; // object pid
		push proDbgMsg;
		call fo::funcoffs::debug_printf_;
		add  esp, 3*4;
		jmp  end;
	}
}

static __declspec(naked) void op_create_object_sid_hack1() {
	static const DWORD create_object_sid_Ret = 0x4551C0;
	__asm {
		cmp  dword ptr [esp + 0x50 - 0x40 + 4], 0; // scriptIndex
		jne  end;
		mov  ebx, [esp + 0x50 - 0x20 + 4]; // createObj
		mov  eax, [ebx + scriptId];
		call fo::funcoffs::scr_remove_;
		mov  dword ptr [ebx + scriptId], -1;
		add  esp, 4;
		jmp  create_object_sid_Ret;
end:
		cmp  dword ptr [esp + 0x50 - 0x40 + 4], -1; // overwritten engine code
		retn;
	}
}

// returns 0 (allows adding) if the critter has the "barter" flag set or its body type is "biped"
static long __fastcall CheckBarterAndBodyType(fo::GameObject* critter) {
	fo::Proto* proto;
	return (fo::util::GetProto(critter->protoId, &proto) &&
	        !(proto->critter.critterFlags & fo::CritterFlags::Barter) && proto->critter.bodyType);
}

static __declspec(naked) void item_add_mult_hook() {
	__asm {
		push edx;
		push ecx;
		call CheckBarterAndBodyType; // ecx - critter
		pop  ecx;
		pop  edx;
		retn;
	}
}

// similar to critter_is_active_ function, with additional check for being knocked down in combat
static long __fastcall TargetIsActiveForPush(fo::GameObject* source, fo::GameObject* target) {
	if (target->critter.IsNotActive()) return 0;
	if (target->critter.damageFlags & fo::DamageFlag::DAM_KNOCKED_DOWN && fo::var::combat_state & fo::CombatStateFlag::InCombat) return 0;
	return (target->critter.IsDead()) ? 0 : 1;
}

static __declspec(naked) void action_can_be_pushed_hook() {
	__asm {
		push ecx;
		call TargetIsActiveForPush; // ecx - source, edx - target
		pop  ecx;
		mov  edx, ebx; // restore target
		retn;
	}
}

static __declspec(naked) void op_float_msg_hack() {
	__asm {
		cmp  eax, 7; // 3 - black, 7 - purple, 12 - grey
		je   purple;
		jl   black;
		movzx eax, byte ptr ds:[FO_VAR_GreyColor];
		retn;
purple:
		movzx eax, byte ptr ds:[FO_VAR_GoodColor];
		retn;
black:
		movzx eax, byte ptr ds:[FO_VAR_NearBlackColor];
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static __declspec(naked) void item_add_force_hack0() {
	__asm { // eax - invenTable, edx - index
		cmp  dword ptr [eax + edx + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [eax + edx + 4], 1; // fix stack count
end: // overwritten engine code
		mov  eax, edi;                     // item
		jmp  fo::funcoffs::item_get_type_;
	}
}

static __declspec(naked) void item_add_force_hack1() {
	__asm { // eax - invenTable, ebx - index
		cmp  dword ptr [eax + ebx * 8 + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [eax + ebx * 8 + 4], INT_MAX; // set max (overflow after changing)
end: // overwritten engine code
		mov  [eax + ebx * 8], edi;             // item
		xor  eax, eax;
		retn;
	}
}

static __declspec(naked) void item_remove_mult_hack0() {
	__asm { // eax - invenItem
		cmp  dword ptr [eax + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [eax + 4], 1; // fix stack count
end: // overwritten engine code
		mov  edx, [eax + 4];         // count
		cmp  edx, ebx;
		retn;
	}
}

static __declspec(naked) void item_remove_mult_hack1() {
	__asm { // eax - invenTable, ecx - index
		sub  [ecx + eax + 4], ebx;         // calculate new item count
		cmp  dword ptr [ecx + eax + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [ecx + eax + 4], INT_MAX; // set max (overflow after changing)
end: // overwritten engine code
		mov  eax, ebp;                     // item
		retn;
	}
}

static __declspec(naked) void item_count_hack() {
	static const DWORD item_count_Ret = 0x4780DB;
	__asm { // eax - invenItem
		cmp  dword ptr [eax + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [eax + 4], 1; // fix stack count
end: // overwritten engine code
		mov  edx, [eax + 4];         // count
		// Fix item_count_ function returning incorrect value when there is a container item inside
		jmp  item_count_Ret;
	}
}

static __declspec(naked) void item_caps_adjust_hack() {
	__asm {
		test edi, edi;     // new caps count
		jg   end;
		mov  edi, INT_MAX; // set max (overflow after changing)
end: // overwritten engine code
		xor  ecx, ecx;
		mov  [edx + esi * 8 + 4], edi; // count
		retn;
	}
}

static __declspec(naked) void item_caps_total_hack() {
	static const DWORD item_caps_total_Ret = 0x47A6E5;
	__asm { // edi - invenItem (caps)
		cmp  dword ptr [edi + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [edi + 4], 1; // fix stack count
end: // overwritten engine code
		add  edx, [edi + 4];         // accumulate quantity
		jmp  item_caps_total_Ret;
	}
}

static __declspec(naked) void inven_pid_quantity_carried_hack() {
	__asm {
		mov  eax, [esp + 0x18 - 0x18 + 4]; // invenItem
		cmp  dword ptr [eax + 4], 0;       // invenItem.count
		jg   end;
		mov  dword ptr [eax + 4], 1;       // fix stack count
end: // overwritten engine code
		add  ebx, [eax + 4];               // accumulate quantity
		retn;
	}
}

static __declspec(naked) void display_inventory_hack_info() {
	__asm { // eax - invenItem
		cmp  dword ptr [eax + 4], 0; // invenItem.count
		jg   end;
		mov  dword ptr [eax + 4], 1; // fix stack count
end: // overwritten engine code
		mov  edx, [eax + 4];         // count
		mov  eax, [eax];             // item
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static bool useInvenOn = false;

static __declspec(naked) void gmouse_handle_event_hack_use_inv() {
	__asm {
		cmp  useInvenOn, 0;
		je   end;
		xor  eax, eax;       // set ZF for skipping to the end
		mov  useInvenOn, al; // reset
		retn;
end:
		cmp  dword ptr ds:[FO_VAR_gmouse_initialized], 0; // overwritten engine code
		retn;
	}
}

static __declspec(naked) void gmouse_handle_event_hook_use_inv() {
	__asm {
		mov  useInvenOn, 1;
		jmp  fo::funcoffs::use_inventory_on_;
	}
}

static __declspec(naked) void op_using_skill_hack() {
	static const DWORD op_using_skill_Ret = 0x4546C4;
	__asm {
		mov  dword ptr [esp + 0x28 - 0x1C + 4], 0; // initialize the value
		cmp  dword ptr [esp + 0x28 - 0x28 + 4], SKILL_SNEAK; // overwritten engine code
		jne  skip;
		retn;
skip:
		add  esp, 4;
		jmp  op_using_skill_Ret;
	}
}

static bool pickedTag = false;
static bool pickedMutate = false;

static __declspec(naked) void perks_dialog_hook_tag() {
	static const DWORD perks_dialog_tag_Ret = 0x43C9C5;
	__asm {
		cmp  pickedTag, 0;
		jne  skip;
		call fo::funcoffs::Add4thTagSkill_;
		mov  pickedTag, al;
		retn;
skip:
		add  esp, 4;
		jmp  perks_dialog_tag_Ret;
	}
}

static __declspec(naked) void perks_dialog_hook_mutate() {
	static const DWORD perks_dialog_mutate_Ret = 0x43CA04;
	__asm {
		cmp  pickedMutate, 0;
		jne  skip;
		call fo::funcoffs::GetMutateTrait_;
		mov  pickedMutate, al;
		retn;
skip:
		add  esp, 4;
		jmp  perks_dialog_mutate_Ret;
	}
}

static __declspec(naked) void editor_design_hook() {
	__asm { // eax = 0
		mov  pickedTag, al;
		mov  pickedMutate, al;
		jmp  fo::funcoffs::intface_update_hit_points_;
	}
}

void BugFixes::init() {
	#ifndef NDEBUG
	LoadGameHook::OnBeforeGameClose() += PrintAddrList;
	if (IniReader::GetIntDefaultConfig("Debugging", "BugFixes", 1) == 0) return;
	#endif

	// Missing game initialization
	LoadGameHook::OnBeforeGameInit() += Initialization;
	LoadGameHook::OnAfterGameInit() += combat_ai_init_backup;
	LoadGameHook::OnGameReset() += []() {
		dudeIsAnimDeath = false;
		combat_ai_reset();
	};

	// Fix for the game disappearing from the taskbar after using Alt+Tab
	MakeCall(0x4CAF14, GNW95_init_window_hack, 2);
	if (HRP::Setting::VersionIsValid) { // for HRP 4.1.8 in DD7/DX9 mode
		MakeCall(HRP::Setting::GetAddress(0x10026504), GNW95_init_window_hack_HRP, 2); // windowed
		MakeCall(HRP::Setting::GetAddress(0x10026605), GNW95_init_window_hack, 2); // fullscreen
	}

	// Fix vanilla negate operator for float values
	MakeCall(0x46AB68, NegateFixHack);

	// Fix incorrect int-to-float conversion
	SafeWriteBatch<WORD>(0x04DB, { // fild 64bit > fild 32bit
		0x46A3A8, 0x46A3F4, // op_mul_
		0x46A4E7, 0x46A566, // op_div_
		0x46A280, 0x46A2CD  // op_sub_
	});

	// Fix for vanilla division and modulo operators treating negative integers as unsigned
	//if (IniReader::GetConfigInt("Misc", "DivisionOperatorFix", 1)) {
		dlogr("Applying division operator fix.", DL_FIX);
		SafeWriteBatch<DWORD>(0x90FBF799, { // xor edx, edx; div ebx > cdq; idiv ebx
			0x46A51D, // op_div_
			0x46A669  // op_mod_
		});
	//}

	//if (IniReader::GetConfigInt("Misc", "SpecialUnarmedAttacksFix", 1)) {
		dlogr("Applying Special Unarmed Attacks fix.", DL_FIX);
		//MakeJump(0x42394D, compute_attack_hack); - implementation moved to Unarmed module
	//}

	//if (IniReader::GetConfigInt("Misc", "SharpshooterFix", 1)) {
		dlogr("Applying Sharpshooter patch.", DL_FIX);
		// https://www.nma-fallout.com/threads/fo2-engine-tweaks-sfall.178390/page-119#post-4050162
		// by Slider2k
		HookCall(0x4244AB, SharpShooterFix); // hooks stat_level_() call in detemine_to_hit_func_()
		// // removes this line by making unconditional jump:
		// if ( who == obj_dude )
		//     dist -= 2 * perk_level_(obj_dude, PERK_sharpshooter);
		SafeWrite8(0x424527, CodeType::JumpShort); // in detemine_to_hit_func_()
	//}

	// Fixes for clickability issue in Pip-Boy and exploit that allows resting in places where you shouldn't be able to
	dlogr("Applying fix for Pip-Boy clickability issues and rest exploit.", DL_FIX);
	MakeCall(0x4971C7, pipboy_hack);
	MakeCall(0x499530, PipAlarm_hack);
	// Fix for clickability issue of the holodisk list
	HookCall(0x497E9F, PipStatus_hook);
	SafeWrite16(0x497E8C, 0xD389); // mov ebx, edx
	SafeWrite32(0x497E8E, 0x90909090);

	// Fix for "Too Many Items" bug
	// http://fforum.kochegarov.com/index.php?showtopic=29288&view=findpost&p=332242
	//if (IniReader::GetConfigInt("Misc", "TooManyItemsBugFix", 1)) {
		dlogr("Applying preventive patch for \"Too Many Items\" bug.", DL_FIX);
		HookCalls(scr_write_ScriptNode_hook, {0x4A596A, 0x4A59C1});
	//}

	// Fix for cells getting consumed even when the car is already fully charged
	MakeCall(0x49BE70, obj_use_power_on_car_hack);

	// Fix for being able to charge the car by using cells on other scenery/critters
	if (IniReader::GetConfigInt("Misc", "CarChargingFix", 1)) {
		dlogr("Applying car charging fix.", DL_FIX);
		MakeJump(0x49C36D, protinst_default_use_item_hack);
	}

	// Fix for gaining stats from more than two doses of a specific chem after save-load
	dlogr("Applying fix for save-load unlimited drug use exploit.", DL_FIX);
	MakeCall(0x47A25B, item_d_save_hack);
	MakeCall(0x47A243, item_d_load_hack);

	// Fix crash when leaving the map while waiting for someone to die of a super stimpak overdose
	dlogr("Applying fix for \"leaving the map while waiting for a super stimpak overdose death\" crash.", DL_FIX);
	HookCall(0x4A27E7, queue_clear_type_mem_free_hook); // hooks mem_free_()

	// Evil bug! If party member has the same armor type in inventory as currently equipped, then
	// on level up he loses Armor Class equal to the one received from this armor.
	// The same happens if you just order NPC to remove the armor through dialogue.
	//if (IniReader::GetConfigInt("Misc", "ArmorCorruptsNPCStatsFix", 1)) {
		dlogr("Applying fix for armor reducing NPC original stats when removed.", DL_FIX);
		HookCall(0x495F3B, partyMemberCopyLevelInfo_hook_stat_level);
		HookCall(0x45419B, correctFidForRemovedItem_hook_adjust_ac);
		// Fix for move_obj_inven_to_obj function
		HookCall(0x45C49A, op_move_obj_inven_to_obj_hook);
		SafeWrite16(0x45C496, 0x9090);
		SafeWrite8(0x45C4A3, CodeType::JumpNZ); // jmp > jnz
		// Fix for drop_obj function
		HookCall(0x49B965, obj_drop_hook);
	//}

	// Fix of invalid stats when party member gains a level while being on drugs
	dlogr("Applying fix for addicted party member level up bug.", DL_FIX);
	HookCall(0x495D5C, partyMemberIncLevels_hook);

	// Allow 9 options (lines of text) to be displayed correctly in a dialog window
	//if (IniReader::GetConfigInt("Misc", "DialogOptions9Lines", 1)) {
		dlogr("Applying 9 dialog options patch.", DL_FIX);
		MakeCall(0x447021, gdProcessUpdate_hack, 1);
	//}

	// Fix for "Unlimited Ammo" exploit
	dlogr("Applying fix for Unlimited Ammo exploit.", DL_FIX);
	HookCall(0x472957, invenWieldFunc_item_get_type_hook); // hooks item_get_type_()

	// Fix for negative values in Skilldex window ("S")
	dlogr("Applying fix for negative values in Skilldex window.", DL_FIX);
	SafeWrite8(0x4AC377, 0x7F); // jg

	// Fix for negative SPECIAL values in character creation
	dlogr("Applying fix for negative SPECIAL values in character creation.", DL_FIX);
	MakeCall(0x43DF6F, is_supper_bonus_hack);
	MakeCall(0x434BFF, PrintBasicStat_hack);
	HookCall(0x437AB4, StatButtonUp_hook);
	HookCall(0x437B26, StatButtonDown_hook);

	// Fix for not counting in the weight/size of equipped items on NPC when stealing or bartering
	//if (IniReader::GetConfigInt("Misc", "NPCWeightFix", 1)) {
		dlogr("Applying fix for not counting in weight of equipped items on NPC.", DL_FIX);
		MakeCall(0x473B4E, loot_container_hack);
		HookCall(0x4758AB, barter_inventory_hook);
		MakeCall(0x477EAB, item_total_weight_hack, 1);
		MakeCall(0x479A2F, item_c_curr_size_hack, 1);
	//}

	// Corrects the max text width of the item weight in trading interface to be 64 (was 80), which matches the table width
	SafeWriteBatch<BYTE>(64, {0x475541, 0x475789});

	// Corrects the max text width of the player name in inventory to be 140 (was 80), which matches the width for item name
	SafeWrite32(0x471E48, 140);

	//if (IniReader::GetConfigInt("Misc", "InventoryDragIssuesFix", 1)) {
		dlogr("Applying inventory reverse order issues fix.", DL_FIX);
		// Fix for minor visual glitch when picking up solo item from the top of inventory
		// and there is multiple item stack at the bottom of inventory
		MakeCall(0x470EC2, inven_pickup_hack, 2);
		// Fix for error in player's inventory, related to IFACE_BAR_MODE=1 in f2_res.ini, and
		// also for reverse order error
		MakeJump(0x47114A, inven_pickup_hack2);
		// Fix for using only one box of ammo when a weapon is above the ammo in the inventory list
		HookCall(0x476598, drop_ammo_into_weapon_hook);
	//}

	// Enable party members with level 6 protos to reach level 6
	//if (IniReader::GetConfigInt("Misc", "NPCStage6Fix", 1)) {
		dlogr("Applying NPC Stage 6 Fix.", DL_FIX);
		MakeJump(0x493CE9, NPCStage6Fix1); // partyMember_init_
		MakeJump(0x494224, NPCStage6Fix2); // partyMemberGetAIOptions_
		SafeWrite8(0x494063, 6);   // loop should look for a potential 6th stage (partyMember_init_)
		SafeWrite8(0x4940BB, 204); // move pointer by 204 bytes instead of 200
	//}

	//if (IniReader::GetConfigInt("Misc", "NPCLevelFix", 1)) {
		dlogr("Applying NPC level fix.", DL_FIX);
		HookCall(0x495BC9, (void*)0x495E51); // jz 0x495E7F > jz 0x495E51
	//}

	//if (IniReader::GetConfigInt("Misc", "BlackSkilldexFix", 1)) {
		dlogr("Applying black Skilldex patch.", DL_FIX);
		HookCall(0x497D0F, PipStatus_AddHotLines_hook);
	//}

	//if (IniReader::GetConfigInt("Misc", "FixWithdrawalPerkDescCrash", 1)) {
		dlogr("Applying withdrawal perk description crash fix.", DL_FIX);
		HookCall(0x47A501, perform_withdrawal_start_display_print_hook);
	//}

	//if (IniReader::GetConfigInt("Misc", "JetAntidoteFix", 1)) {
		dlogr("Applying Jet Antidote fix.", DL_FIX);
		// Fix for Jet antidote not being removed after removing the addiction effect (when using the item)
		MakeJump(0x47A013, (void*)0x47A168); // item_d_take_drug_
	//}

	//if (IniReader::GetConfigInt("Misc", "NPCDrugAddictionFix", 1)) {
		dlogr("Applying NPC's drug addiction fix.", DL_FIX);
		// proper checks for NPC's addiction instead of always using global vars
		HookCalls(item_d_take_drug_hook, {0x479FBC, 0x47A0AE}); // replace item_d_check_addict_ function calls with sfall implementation
		MakeCall(0x479FCA, item_d_take_drug_hack, 2);
		// just add a new "addict" event every 7 days (the previous one is deleted) until the Jet addiction is removed by the antidote
		// Note: for critters who are not party members, any addiction is removed after leaving the map
		MakeCall(0x47A3A4, item_wd_process_hack);
	//}

	//if (IniReader::GetConfigInt("Misc", "ShivPatch", 1)) {
		dlogr("Applying shiv patch.", DL_FIX);
		SafeWrite8(0x477B2B, CodeType::JumpShort);
	//}

	//if (IniReader::GetConfigInt("Misc", "ImportedProcedureFix", 0)) {
		dlogr("Applying imported procedure patch.", DL_FIX);
		// http://teamx.ru/site_arc/smf/index.php-topic=398.0.htm
		SafeWrite16(0x46B35B, 0x1C60); // Fix problems with the temporary stack
		SafeWrite32(0x46B35D, 0x90909090);
		SafeWriteBatch<BYTE>(CodeType::JumpShort, {0x46DBF1, 0x46DDC4}); // Disable warnings
		SafeWrite8(0x4415CC, 0x00); // Prevent crashes when re-exporting
	//}
	// Fix for op_lookup_string_proc_ engine function not searching the last procedure in a script
	SafeWrite8(0x46C7AC, 0x76); // jb > jbe

	// Update the AC counter
	//if (IniReader::GetConfigInt("Misc", "WieldObjCritterFix", 1)) {
		dlogr("Applying wield_obj_critter fix.", DL_FIX);
		SafeWrite8(0x456912, 0x1E); // jnz 0x456931
		HookCall(0x45697F, op_wield_obj_critter_adjust_ac_hook);
	//}

	//if (IniReader::GetConfigInt("Misc", "MultiHexPathingFix", 1)) {
		dlogr("Applying MultiHex Pathing Fix.", DL_FIX);
		HookCall(0x416144, make_path_func_hook); // Fix for building the path to the central hex of a multihex object
		//MakeCalls(MultiHexFix, {0x42901F, 0x429170}); // obsolete fix

		// Fix for multihex critters moving too close and overlapping their targets in combat
		MakeCall(0x42A14F, MultiHexCombatRunFix, 1);
		MakeCall(0x42A178, MultiHexCombatMoveFix, 1);
		// Check neighboring tiles to prevent critters from overlapping other object tiles when moving to the retargeted tile
		SafeWrite16(0x42A3A6, 0xE889); // xor eax, eax > mov eax, ebp (fix retargeting tile for multihex critters)
		HookCall(0x42A3A8, MultiHexRetargetTileFix); // cai_retargetTileFromFriendlyFire_
	//}

	//if (IniReader::GetConfigInt("Misc", "DodgyDoorsFix", 1)) {
		dlogr("Applying Dodgy Door Fix.", DL_FIX);
		MakeCall(0x4113D3, action_melee_hack, 2);
		MakeCall(0x411BC9, action_ranged_hack, 2);
	//}

	// Fix for multiple knockout events being added to the queue
	HookCall(0x424F9A, set_new_results_hack);

	// Fix for critter_state function when a critter in combat recovers from the knockout state outside its turn
	// DAM_KNOCKOUT_WOKEN flag lets the function know that the critter was knocked out before it actually woke up (stand up)
	MakeCall(0x45868E, op_critter_state_hack, 1);
	MakeCall(0x42E44C, critter_wake_up_hack);

	// Fixes for knockout/down issues
	HookCall(0x42E456, critter_wake_up_hook);
	HookCall(0x41857E, dude_standup_hook); // check if the critter is actually in the prone position

	// Fix for "NPC turns into a container" bug
	// https://www.nma-fallout.com/threads/fo2-engine-tweaks-sfall.178390/page-123#post-4065716
	//if (IniReader::GetConfigInt("Misc", "NPCTurnsIntoContainerFix", 1)) {
		dlogr("Applying fix for \"NPC turns into a container\" bug.", DL_FIX);
		MakeJump(0x42E46E, critter_wake_clear_hack);
		MakeCall(0x488EF3, obj_load_func_hack, 1);
		HookCall(0x4949B2, partyMemberPrepLoadInstance_hook);
		// Fix for knocked down critters not playing stand up animation when the combat ends (when DAM_LOSE_TURN and DAM_KNOCKED_DOWN flags are set)
		MakeCall(0x42206F, combat_over_hack, 1);
	//}

	// Fix to prevent dead NPCs from reloading their weapons when the combat ends
	HookCall(0x421F30, combat_over_hook);

	dlogr("Applying fix for explosives bugs.", DL_FIX);
	// Fix crashes when killing critters with explosives
	MakeJump(0x422F05, combat_ctd_init_hack);
	MakeCall(0x48941C, obj_save_hack, 1);
	// Fix for destroy_p_proc not being called if the critter is killed by explosives when you leave the map
	MakeCall(0x4130C3, action_explode_hack);
	MakeCall(0x4130E5, action_explode_hack1);

	// Fix for being unable to sell used geiger counters or stealth boys
	if (IniReader::GetConfigInt("Misc", "CanSellUsedGeiger", 1)) {
		dlogr("Applying fix for being unable to sell used geiger counters or stealth boys.", DL_FIX);
		SafeWriteBatch<BYTE>(0xBA, {0x478115, 0x478138}); // item_queued_ (will return the found item)
		MakeJump(0x474D22, barter_attempt_transaction_hack, 1);
		HookCall(0x4798B1, item_m_turn_off_hook);
	}

	// Fix for incorrect initialization of action points at the beginning of each turn
	dlogr("Applying Action Points initialization fix.", DL_FIX);
	BlockCall(0x422E02);
	MakeCall(0x422E1B, combat_hack);

	// Fix for incorrect death animations being used when killing critters with kill_critter_type function
	dlogr("Applying kill_critter_type fix.", DL_FIX);
	SafeWrite16(0x457E22, 0xDB31); // xor ebx, ebx
	SafeWrite32(0x457C99, 0x30BE0075); // jnz 0x457C9B; mov esi, 48

	// Fix for checking the horizontal position on the y-axis instead of x when setting coordinates on the world map
	SafeWrite8(0x4C4743, 0xC6); // cmp esi, eax

	//if (IniReader::GetConfigInt("Misc", "PrintToFileFix", 1)) {
		dlogr("Applying print to file fix.", DL_FIX);
		MakeCall(0x4C67D4, db_get_file_list_hack);
	//}

	// Fix for display issues when calling gdialog_mod_barter with critters with no "Barter" flag set
	//if (IniReader::GetConfigInt("Misc", "gdBarterDispFix", 1)) {
		dlogr("Applying gdialog_mod_barter display fix.", DL_FIX);
		HookCall(0x448250, gdActivateBarter_hook);
	//}

	//if (IniReader::GetConfigInt("Misc", "BagBackpackFix", 1)) {
		dlogr("Applying fix for bag/backpack bugs.", DL_FIX);
		// Fix for items disappearing from inventory when you try to drag them to bag/backpack in the inventory list
		// and are overloaded
		HookCall(0x4764FC, (void*)fo::funcoffs::item_add_force_);
		// Fix for the engine not checking player's inventory properly when putting items into the bag/backpack in the hands
		MakeJump(0x4715DB, switch_hand_hack);
		// Fix to ignore player's equipped items when opening bag/backpack
		MakeCall(0x471B7F, inven_item_wearing, 1); // inven_right_hand_
		MakeCall(0x471BCB, inven_item_wearing, 1); // inven_left_hand_
		MakeCall(0x471C17, inven_item_wearing, 1); // inven_worn_
		// Fix crash when trying to open bag/backpack on the table in the bartering interface
		MakeCall(0x473191, inven_action_cursor_hack);
	//}

	// Fix crash when clicking on empty space in the inventory list opened by "Use Inventory Item On" (backpack) action icon
	MakeCall(0x471A94, use_inventory_on_hack);

	// Fix item_count_ function returning incorrect value when there is a container-item inside
	//SafeWrite8(0x4780B1, 0x29); // jmp 0x4780DB

	// Fix for Sequence stat value not being printed correctly when using "print to file" option
	MakeCall(0x4396F5, Save_as_ASCII_hack, 2);

	// Fix for the incorrect object type search when loading a game saved in combat mode
	HookCalls(combat_load_hook_critter, {0x42113B, 0x42117D});
	HookCall(0x4211C0, combat_load_hook_item);

	// Fix for Bonus Move APs being replenished when you save and load the game in combat
	//if (IniReader::GetConfigInt("Misc", "BonusMoveFix", 1)) {
		dlogr("Applying fix for Bonus Move exploit.", DL_FIX);
		HookCall(0x420E93, combat_load_hook);
		MakeCall(0x422A06, combat_turn_hack);
	//}

	// Fix for the displayed message when the attack randomly hits a target that is not a critter and has a script attached
	// Tweak: if the main target has Flat flag set, display the "You missed" message instead of the message of hitting another object
	MakeCall(0x42535F, combat_display_hack, 1);

	// Fix for damage_p_proc being called for misses if the target is not a critter
	MakeCall(0x424CD2, apply_damage_hack);

	// Fix for the double damage effect of Silent Death perk not being applied to critical hits
	//if (IniReader::GetConfigInt("Misc", "SilentDeathFix", 1)) {
		dlogr("Applying Silent Death patch.", DL_FIX);
		SafeWrite8(0x4238DF, 0x8C); // jl 0x423A0D
		HookCall(0x423A99, compute_attack_hook);
	//}

	// Fix crash when calling partyMemberGetCurLevel_ on a critter that has no data in party.txt
	MakeCall(0x495FF6, partyMemberGetCurLevel_hack, 1);

	// Fix for player's base EMP DR not being properly initialized when creating a new character and then starting the game
	HookCall(0x4A22DF, ResetPlayer_hook);

	// Fix for add_mult_objs_to_inven adding only 500 instances of an object when the "count" argument is over 99999
	SafeWrite8(0x45A289, 0x1A); // jge 0x45A294 (skip upper limit)

	// Fix for being at incorrect hex after map change when the exit hex in source map is at the same position as some exit hex
	// in destination map
	MakeCall(0x48A704, obj_move_to_tile_hack);

	// Fix for critters killed in combat by scripting still being able to move in their combat turn if the distance parameter
	// in their AI packets is set to stay_close/charge, or NPCsTryToSpendExtraAP is enabled
	HookCalls(ai_combat_turn_run_hook, {
		0x42A1A8, // ai_move_steps_closer_ (old 0x42B24D)
		0x42898D, // ai_run_away_  (potential fix)
		0x428AB3  // ai_move_away_ (potential fix)
	});

	// Fix instant death critical hits
	dlogr("Applying instant death fix.", DL_FIX);
	MakeJump(0x424BA2, compute_damage_hack);

	// Fix missing AC/DR mod stats when examining ammo in the barter screen
	dlogr("Applying fix for displaying ammo stats in barter screen.", DL_FIX);
	MakeCall(0x49B4AD, obj_examine_func_hack_ammo0, 2);
	MakeCall(0x49B504, obj_examine_func_hack_ammo0, 2);
	MakeCall(0x49B563, obj_examine_func_hack_ammo1, 2);

	// Display full item description for weapon/ammo in the barter screen
	showItemDescription = (IniReader::GetConfigInt("Misc", "FullItemDescInBarter", 0) != 0);
	if (showItemDescription) {
		dlogr("Applying full item description in barter patch.", DL_FIX);
		HookCall(0x49B452, obj_examine_func_hack_weapon); // it's jump
	}

	// Display experience points with the bonus from Swift Learner perk when gained from non-scripted situations
	//if (IniReader::GetConfigInt("Misc", "DisplaySwiftLearnerExp", 1)) {
		dlogr("Applying Swift Learner exp display patch.", DL_FIX);
		MakeCall(0x4AFAEF, statPCAddExperienceCheckPMs_hack);
		HookCall(0x4221E2, combat_give_exps_hook);
		MakeJump(0x4745AE, loot_container_exp_hack);
		SafeWrite16(0x4C0AB1, 0x23EB); // jmps 0x4C0AD6
		HookCall(0x4C0AEB, wmRndEncounterOccurred_hook);
	//}

	// Fix for obj_can_see_obj not checking if source and target objects are on the same elevation before calling
	// is_within_perception_
	MakeCall(0x456B63, op_obj_can_see_obj_hack);
	SafeWrite16(0x456B76, 0x23EB); // jmp 0x456B9B (skip unused engine code)

	// Fix broken obj_can_hear_obj function
	if (IniReader::GetConfigInt("Misc", "ObjCanHearObjFix", 0)) {
		dlogr("Applying obj_can_hear_obj fix.", DL_FIX);
		SafeWrite8(0x4583D8, 0x3B);            // jz 0x458414
		SafeWrite8(0x4583DE, CodeType::JumpZ); // jz 0x458414
		MakeCall(0x4583E0, op_obj_can_hear_obj_hack, 1);
	}

	if (IniReader::GetConfigInt("Misc", "AIBestWeaponFix", 1)) {
		dlogr("Applying AI best weapon choice fix.", DL_FIX);
		// Fix for the incorrect item being checked for the presence of weapon perks
		HookCall(0x42954B, ai_best_weapon_hook);
		// Corrects the calculation of the weapon score to: (maxDmg + minDmg) / 4
		SafeWriteBatch<BYTE>(0x01, {0x4294E2, 0x429675}); // sub > add
		MakeCalls(ai_best_weapon_hack, {0x4294E6, 0x429679});
		// Corrects the weapon score multiplier for having perks to 2x (was 5x)
		SafeWriteBatch<BYTE>(0x15, {0x42955E, 0x4296E7}); // lea eax, [edx*4] > lea eax, [edx]
	}

	// Change the calculation of the armor score to exclude the EMP stats (not strictly a bug fix)
	HookCalls(ai_search_inven_armor_hook, {0x429ACC, 0x429B17});

	// Fix for the encounter description being displayed in two lines instead of one
	SafeWrite32(0x4C1011, 0x9090C789); // mov edi, eax;
	SafeWrite8(0x4C1015, CodeType::Nop);
	HookCall(0x4C1042, wmSetupRandomEncounter_hook);

	// Fix for being unable to sell/give items in the barter screen when the player/party member is overloaded
	HookCalls(barter_attempt_transaction_hook_weight, {0x474C73, 0x474CCA});

	// Fix for the underline position in the inventory display window when the item name is longer than one line
	MakeCall(0x472F5F, inven_obj_examine_func_hack, 1);

	// Fix for the exploit that allows you to gain excessive skill points from Tag! perk before leaving the character screen
	dlogr("Applying fix for Tag! exploit.", DL_FIX);
	HookCall(0x43B463, SliderBtn_hook_down);
	HookCall(0x43D7DD, Add4thTagSkill_hook);

	// Fix for ai_retrieve_object_ engine function not returning the requested object when there are different objects
	// with the same ID
	dlogr("Applying ai_retrieve_object engine function fix.", DL_FIX);
	HookCall(0x429D7B, ai_retrieve_object_hook);
	MakeCall(0x472708, inven_find_id_hack);

	// Fix for the "mood" argument of start_gdialog function being ignored for talking heads
	if (IniReader::GetConfigInt("Misc", "StartGDialogFix", 0)) {
		dlogr("Applying start_gdialog argument fix.", DL_FIX);
		MakeCall(0x456F08, op_start_gdialog_hack);
	} else {
		// Fix crash when calling start_gdialog outside of the talk_p_proc procedure for talking heads
		__int64 data = 0x900C24448B; // mov eax, [esp + 0x3C - 0x30] (fix dialog_target)
		SafeWriteBytes(0x456F08, (BYTE*)&data, 5);
	}

	// Fix for Heave Ho! perk increasing Strength stat above 10 when determining the maximum range of thrown weapons
	dlogr("Applying Heave Ho! perk fix.", DL_FIX);
	HookCall(0x478AD9, item_w_range_hook);

	// Fix for determine_to_hit_func_ engine function taking distance into account when called from determine_to_hit_no_range_
	HookCall(0x4244C3, determine_to_hit_func_hook);

	// Radiation fixes
	MakeCall(0x42D6C3, process_rads_hack, 1); // prevents player's death if a stat is less than 1 when removing radiation effects
	HookCall(0x42D67A, process_rads_hook);    // fix for the same effect message being displayed when removing radiation effects
	radEffectsMsgNum = IniReader::GetConfigInt("Misc", "RadEffectsRemovalMsg", radEffectsMsgNum);
	// Display messages about radiation for the active geiger counter
	if (IniReader::GetConfigInt("Misc", "ActiveGeigerMsgs", 1)) {
		dlogr("Applying active geiger counter messages patch.", DL_FIX);
		SafeWriteBatch<BYTE>(CodeType::JumpZ, {0x42D424, 0x42D444}); // jnz > jz
	}
	// Display a pop-up message box about death from radiation
	HookCall(0x42D733, process_rads_hook_msg);

	//int drugUsePerfFix = IniReader::GetConfigInt("Misc", "AIDrugUsePerfFix", 1);
	//if (drugUsePerfFix > 0) {
	//	dlogr("Applying AI drug use preference fix.", DL_FIX);
	//	if (drugUsePerfFix != 2) {
	//		// Fix for AI not taking chem_primary_desire in AI.txt as a preference list when using drugs in the inventory
	//		MakeCall(0x42869D, ai_check_drugs_hack_break);
	//		MakeCall(0x4286AB, ai_check_drugs_hack_check, 1);
	//		MakeCall(0x4286C7, ai_check_drugs_hack_use);
	//	}
	//	// Fix to allow using only the drugs listed in chem_primary_desire and healing drugs (stimpaks and healing powder)
	//	SafeWrite8(0x4286B1, CodeType::JumpZ);  // jnz > jz (ai_check_drugs_)
	//	SafeWrite8(0x4286C5, CodeType::JumpNZ); // jz > jnz (ai_check_drugs_)
	//}

	// Fix for chem_primary_desire values in party member AI packets not being saved correctly
	HookCall(0x42803E, cai_cap_save_hook);

	// Fix for config_get_values_ engine function not getting the last value in a list if the list has less than the requested
	// number of values (for chem_primary_desire)
	MakeJump(0x42C12C, config_get_values_hack);

	// Fix returned result value when the file is missing
	HookCall(0x4C6162, db_freadInt_hook);

	// Fix and repurpose the unused called_shot/num_attack arguments of attack_complex function
	// called_shot - additional damage when hitting the target
	// num_attacks - the number of free action points on the first turn only
	if (IniReader::GetConfigInt("Misc", "AttackComplexFix", 0)) {
		dlogr("Applying attack_complex arguments fix.", DL_FIX);
		HookCall(0x456D4A, op_attack_hook);
		SafeWrite8(0x456D61, 0x74); // mov [gcsd.free_move], esi
		SafeWrite8(0x456D92, 0x5C); // mov [gcsd.amount], ebx

		// Allow setting result flags arguments for the attacker and the target (now work independently of each other)
		SafeWrite16(0x456D95, 0xC085); // cmp eax, ebp > test eax, eax
		MakeCall(0x456D9A, op_attack_hook_flags);
		SafeWrite16(0x456DA7, 0x8489); // mov [gcsd.changeFlags], 1 > mov [gcsd.changeFlags], eax
		SafeWrite8(0x456DAB, 0);
		SafeWrite8(0x456DAE, CodeType::Nop);
	} else {
		// Fix setting result flags argument for the target
		SafeWrite16(0x456D95, 0xED85); // cmp eax, ebp > test ebp, ebp
	}
	SafeWrite8(0x456D9F, CodeType::JumpNZ); // jz > jnz
	// Fix result flags for the attacker and the target when calling attack_complex function
	// also set/unset the DAM_DEAD flag when changing the minimum/maximum damage to the target
	// and fix minimum damage still being applied to the target when the attacker misses
	MakeJump(0x422FE5, combat_attack_hack, 1);

	// Fix for critter_mod_skill taking a negative amount value as a positive
	dlogr("Applying critter_mod_skill fix.", DL_FIX);
	SafeWrite8(0x45B910, 0x7E); // jbe > jle

	// Fix crash when calling use_obj/use_obj_on_obj without using set_self in global scripts
	// also change the behavior of use_obj_on_obj function
	// if the object uses the item on itself, then another function is called (not a bug fix)
	MakeCall(0x45C376, op_use_obj_on_obj_hack, 1);
	MakeCall(0x456A92, op_use_obj_hack, 1);

	// Fix pickup_obj/drop_obj/use_obj functions, change them to get pointer from script.self instead of script.target
	// script.target contains an incorrect pointer, which may vary depending on the situations in the game
	dlogr("Applying pickup_obj/drop_obj/use_obj fix.", DL_FIX);
	SafeWriteBatch<BYTE>(0x34, { // script.target > script.self
		0x456554, // op_pickup_obj_
		0x456600, // op_drop_obj_
		0x456A6D, // op_use_obj_
		0x456AA4  // op_use_obj_
	});

	// Fix for critters not attacking the player in combat when loading a game saved in combat mode
	BlockCall(0x48D6F0); // obj_fix_combat_cid_for_dude_

	// Fix for the player's turn being skipped when loading a game saved in combat mode
	MakeCall(0x422E25, combat_hack_load);

	// Fix for the reserved item FRM being displayed in the top-left corner when in the loot/barter screens
	HookCalls(JesseContainerFid, {0x473AC9, 0x475895});

	// Fix the return value of has_skill function for incorrect skill numbers
	SafeWrite32(0x4AA56B, 0);

	// Fix for NPC stuck in a loop of reloading melee/unarmed weapons or solar scorcher when out of ammo
	dlogr("Applying fix for NPC stuck in a loop of reloading empty weapons.", DL_FIX);
	HookCall(0x429A2B, ai_search_inven_weap_hook0); // for melee/unarmed weapons
	HookCall(0x4299EC, ai_search_inven_weap_hook1); // for the solar scorcher

	// Fix for critters not being healed over time when entering the map if 'dead_bodies_age=No' is set in maps.txt
	// also fix the zero initialization of a local variable to correct time for removing corpses and blood
	dlogr("Applying fix for the self-healing of critters when entering the map.", DL_FIX);
	MakeCall(0x483356, map_age_dead_critters_hack);
	SafeWrite32(0x4832A0, 0x9090C189); // mov ecx, eax (keep dead_bodies_age flag)
	SafeWrite32(0x4832A4, 0x0C245489); // mov [esp + var_30], edx

	// Fix for the removal of party member's corpse when loading the map
	MakeCall(0x495769, partyFixMultipleMembers_hack, 1);

	// Fix for unexplored areas being revealed on the automap when entering a map
	MakeCall(0x48A76B, obj_move_to_tile_hack_seen, 1);

	// Fix for the overflow of the automap tables when the number of maps in maps.txt is more than 160
	HookCall(0x41C0FC, automap_pip_save_hook);
	HookCalls(PrintAutoMapList, {
		0x499212, // PrintAMList_
		0x499013  // PrintAMelevList_
	});

	// Fix "out of bounds" bug when printing the automap list
	HookCall(0x499240, PrintAMList_hook);

	// Fix for a duplicate obj_dude script being created when loading a saved game
	HookCall(0x48306E, map_load_file_hook); // removes the redundant saved dude script data from the scripts block
	HookCall(0x48D63E, obj_load_dude_hook0);
	HookCall(0x48D666, obj_load_dude_hook1);
	BlockCall(0x48D675);
	BlockCall(0x48D69D);

	// Fix for the start procedure not being called correctly if the required standard script procedure is missing
	MakeCall(0x4A4926, exec_script_proc_hack);
	MakeCall(0x4A4979, exec_script_proc_hack1);

	// Fix the argument value of dialogue_reaction function
	HookCall(0x456FFA, op_dialogue_reaction_hook);

	// Fix for NPC stuck in a loop of picking up items in combat and the incorrect message being displayed when the NPC cannot pick
	// up an item due to not enough space in the inventory
	HookCall(0x49B6E7, obj_pickup_hook);
	HookCall(0x49B71C, obj_pickup_hook_message);
	Translate::Get("sfall", "NPCPickupFail", "%s cannot pick up the item.", pickupMessage, 65);

	// Fix for anim_move_to_tile_ engine function ignoring the distance argument for the player
	HookCall(0x416D44, anim_move_to_tile_hook);
	HookCall(0x416DD2, anim_move_to_tile_hook_tile);

	// Fix for the player's movement in combat being interrupted when trying to use objects with Bonus Move APs available
	MakeCall(0x411FD6, action_use_an_item_on_object_hack);
	MakeCall(0x411DF7, action_climb_ladder_hack); // bug caused by anim_move_to_tile_ fix

	// Partial fix for incorrect positioning after exiting small/medium locations (e.g. Ghost Farm)
	//if (IniReader::GetConfigInt("Misc", "SmallLocExitFix", 1)) {
		dlogr("Applying fix for incorrect positioning after exiting small/medium locations.", DL_FIX);
		MakeJump(0x4C5A41, wmTeleportToArea_hack);
	//}

	// Fix for Scout perk being taken into account when setting the visibility of locations with mark_area_known function
	// also fix the incorrect coordinates for small/medium location circles that the engine uses to highlight their sub-tiles
	// and fix visited tiles on the world map being darkened again when a location is added next to them
	MakeJump(0x4C466F, wmAreaMarkVisitedState_hack);
	SafeWrite8(0x4C46AB, 0x58); // esi > ebx

	// Fix the position of the destination marker for small/medium location circles
	MakeCall(0x4C03AA, wmWorldMap_hack, 2);

	// Fix to prevent using number keys to enter unvisited areas on a town map
	if (IniReader::GetConfigInt("Misc", "TownMapHotkeysFix", 1)) {
		dlogr("Applying town map hotkeys patch.", DL_FIX);
		MakeCall(0x4C495A, wmTownMapFunc_hack, 1);
	}

	// Fix for combat not ending automatically when there are no hostile critters
	MakeCall(0x422CF3, combat_should_end_hack);
	SafeWrite16(0x422CEA, 0x9090);

	// Fix for the car being lost when entering a location via the Town/World button and then leaving on foot
	// (sets GVAR_CAR_PLACED_TILE (633) to -1 on exit to the world map)
	if (IniReader::GetConfigInt("Misc", "CarPlacedTileFix", 1)) {
		dlogr("Applying car placed tile fix.", DL_FIX);
		MakeCall(0x4C2367,  wmInterfaceInit_hack);
	}

	// Place the player on a nearby empty tile if the entrance tile is blocked by another object when entering a map
	HookCall(0x4836F8, map_check_state_hook);

	// Fix the placement of multihex critters in the player's party when entering a map or elevation
	MakeCall(0x494E33, partyMemberSyncPosition_hack, 1);

	// Fix for critter_add/rm_trait functions ignoring the "amount" argument
	// Note: pass negative amount values to critter_rm_trait to remove all ranks of the perk (vanilla behavior)
	HookCall(0x458CDB, op_critter_rm_trait_hook);
	HookCall(0x458B3D, op_critter_add_trait_hook);

	// Fix to prevent corpses from blocking line of fire
	//if (IniReader::GetConfigInt("Misc", "CorpseLineOfFireFix", 1)) {
		dlogr("Applying fix for corpses blocking line of fire.", DL_FIX);
		MakeJump(0x48B994, obj_shoot_blocking_at_hack0);
		MakeJump(0x48BA04, obj_shoot_blocking_at_hack1);
	//}

	// Fix for party member's equipped weapon being placed in the incorrect item slot after leveling up
	MakeCall(0x495FD9, partyMemberCopyLevelInfo_hack, 1);

	// Fix the playback of the speech sound file for the death screen
	HookCall(0x481409, main_death_scene_hook);

	// Fix for trying to loot corpses with the "NoSteal" flag
	MakeCall(0x4123F8, action_loot_container_hack, 1);
	SafeWrite8(0x4123F2, CommonObj::protoId);
	BlockCall(0x4123F3);

	// Fix the music volume when entering the dialog
	SafeWrite32(0x44525D, (DWORD)FO_VAR_background_volume);

	// Fix for the barter button on the dialog window not animating until after leaving the barter screen
	HookCall(0x44A77C, gdialog_window_create_hook);

	// Fix for the player's money not being displayed in the dialog window after leaving the barter/combat control interface
	HookCall(0x447ACD, gdialog_bk_hook);

	// Cosmetic fix to the dialog review interface
	// to prevent the player name from being displayed at the bottom of the window when the text is longer than one screen
	MakeCall(0x445ECC, gdReviewDisplay_hack);

	// TODO: If a bug is found that leads to an animation glitch, then this fix can be removed
	// Fix crash or animation glitch of the critter in combat when an explosion from explosives and the AI attack animation are
	// performed simultaneously
	// Note: all events in combat will occur before the AI (party member) attack
	HookCall(0x422E5F, combat_hook); // execute all events after the end of the combat sequence

	// Fix for the "Fill_W" flag in worldmap.txt not uncovering all tiles to the left edge of the world map
	MakeJump(0x4C372B, wmSubTileMarkRadiusVisited_hack);
	SafeWrite16(0x4C3723, 0xC931); // mov ecx, esi > xor ecx, ecx
	SafeWrite8(0x4C3727, 0x51);    // push esi > push ecx

	// Fix the code in combat_is_shot_blocked_ to correctly get the next tile from a multihex object instead of the previous
	// object or source tile
	// Note: this bug does not cause any noticeable error in the function
	BYTE codeData[] = {
		0x8B, 0x70, 0x04,       // mov  esi, [eax + 4]
		0xF6, 0x40, 0x25, 0x08, // test [eax + flags2], MultiHex_
		0x74, 0x1E,             // jz   0x426D83
		0x39, 0xEE,             // cmp  esi, ebp
		0x74, 0x1A,             // jz   0x426D83
		0x90
	};
	SafeWriteBytes(0x426D5C, codeData, 14); // combat_is_shot_blocked_

	// Fix for NPC stuck in an animation loop in combat when trying to move close to a multihex critter
	// this prevents moving to the multihex critter when the critters are close together
	BYTE codeData1[] = {
		0x89, 0xF0,                      // mov  eax, esi
		0x89, 0xFA,                      // mov  edx, edi
		0xE8, 0x00, 0x00, 0x0, 0x0,      // call obj_dist_
		0x83, 0xF8, 0x01,                // cmp  eax, 1
		0x0F, 0x8E, 0xAB, 0x0, 0x0, 0x0, // jle  0x42A1B1 (exit)
	};
	SafeWriteBytes(0x42A0F4, codeData1, 18); // ai_move_steps_closer_
	HookCall(0x42A0F8, (void*)fo::funcoffs::obj_dist_);
	BYTE codeData2[] = {        // swap code
		0x90, 0x90, 0x31, 0xC0, // xor  eax, eax
		0xF6, 0x47, 0x25, 0x08, // test [edi + 0x25], MultiHex_
	};
	SafeWriteBytes(0x42A0E6, codeData2, 8); // ai_move_steps_closer_
	SafeWrite8(0x42A0F2, CodeType::JumpZ);  // jmp > jz

	// Fix to prevent the execution of critter_p_proc and game events when playing movies (same as when the dialog is active)
	HookCall(0x4A3C89, doBkProcesses_hook);

	// Fix to prevent the player from leaving the map when the death animation causes the player to cross an exit grid
	// (e.g. fire dance or knockback animation)
	MakeCall(0x41094B, show_damage_to_object_hack, 1);
	MakeCall(0x48A6CB, obj_move_to_tile_hack_ondeath, 1);

	// Fix to limit the maximum distance for the knockback animation
	MakeCall(0x4104D5, action_knockback_hack);

	// Fix for combat_is_shot_blocked_ engine function not taking the flags of critters in the line of fire into account
	// when calculating the hit chance penalty of ranged attacks in determine_to_hit_func_ engine function
	SafeWriteBatch<BYTE>(0x41, {0x426D46, 0x426D4E}); // edi > ecx (replace target with object critter)
	SafeWrite8(0x426D48, fo::DAM_DEAD | fo::DAM_KNOCKED_DOWN | fo::DAM_KNOCKED_OUT);

	// Fix broken Print() script function
	HookCall(0x461AD4, (void*)fo::funcoffs::windowOutput_);

	// Fix for the flags of non-door objects being set/unset when using obj_close/open functions
	MakeCall(0x49CBF7, check_door_state_hack_close, 2);
	MakeCall(0x49CB30, check_door_state_hack_open, 1);

	// Fix for the "Leave" event procedure of the window region not being triggered when the cursor moves to a non-scripted window
	MakeJump(0x4B6C3B, checkAllRegions_hack);
	HookCall(0x4B6C13, checkAllRegions_hook);

	// Fix for the window with the "DontMoveTop" flag not being redrawn after the show function call if it is not the topmost one
	MakeJump(0x4D6E04, win_show_hack);

	// Fix for the script attached to an object not being initialized properly upon object creation
	createObjectSidStartFix = (IniReader::GetConfigInt("Misc", "CreateObjectSidFix", 0) != 0);
	MakeCall(0x4551C0, op_create_object_sid_hack, 1);
	// Fix the error handling in create_object_sid function to prevent a crash when the proto is missing
	SafeWrite8(0x45507B, 0x51); // jz 0x4550CD
	// Fix to allow creating an object with no script correctly when passing 0 as the script index number (also prevent a crash)
	MakeCall(0x4550C8, op_create_object_sid_hack1);

	// Fix to prevent the main menu music from stopping when entering the load game screen
	BlockCall(0x480B25);

	// Fix incorrect value of the limit number of floating messages
	SafeWrite8(0x4B039F, 20); // text_object_create_ (was 19)

	// Fix for being unable to plant items on non-biped critters with the "Barter" flag set (e.g. Skynet and Goris)
	HookCall(0x477183, item_add_mult_hook);

	// Fix for being able to use the "Push" action on members of the player's team in combat when they are knocked down
	HookCall(0x413718, action_can_be_pushed_hook);

	// Fix for float_msg function not setting the purple or black text color correctly (was always grey)
	MakeCall(0x459415, op_float_msg_hack, 2);

	// Corrects the language path for loading art files
	SafeWriteBatch<DWORD>((DWORD)&"art\\%s%s", {0x419B00, 0x419C06}); // art_data_size_, art_data_load_

	// Fix for incorrect death endings being shown when some endings in the list are not available
	SafeWrite8(0x440C8E, 7); // jnz 0x440C96

	// Fix for gaining two levels at once when leveling up from level 97
	SafeWrite8(0x4AF9AF, 0x7F); // jge > jg (stat_pc_min_exp_)

	// Fix to prevent integer overflow for the number of items in a stack in the inventory
	// If the number of items in a stack is less than 1, it is considered an integer overflow
	MakeCall(0x47732E, item_add_force_hack0, 2);   // before adding items
	MakeCall(0x4773F0, item_add_force_hack1);      // after adding items
	SafeWrite8(0x47739A, 0x77);                    // jg > ja (extra handling for adding ammo)
	MakeCall(0x4774D6, item_remove_mult_hack0);    // before removing items
	MakeCall(0x477503, item_remove_mult_hack1, 1); // after removing items
	MakeJump(0x4780AD, item_count_hack);
	MakeCall(0x47A7C3, item_caps_adjust_hack, 1);  // for op_item_caps_adjust_
	MakeJump(0x47A6C9, item_caps_total_hack);      // for op_item_caps_total_ and money display
	MakeCall(0x471D08, inven_pid_quantity_carried_hack, 1); // for op_obj_is_carrying_obj_pid_
	MakeCalls(display_inventory_hack_info, { // when displaying the inventory list
		0x470253, // display_inventory_
		0x470506, // display_target_inventory_
		//0x475481, // display_table_inventories_
		//0x4756B1  // display_table_inventories_
	});
	SafeWrite8(0x4705E9, 0x76); // jle > jbe (for ammo counter in display_inventory_info_)

	// Fix for clickability issue of the "Use Inventory Item On" action when the selected item overlaps an object
	MakeCall(0x44BFB9, gmouse_handle_event_hack_use_inv, 2);
	HookCall(0x44C6FB, gmouse_handle_event_hook_use_inv);

	// Fix for using_skill function returning garbage values when the arguments are not the player and SKILL_SNEAK
	MakeCall(0x4546A5, op_using_skill_hack, 1);

	// Fix for object_fix_weapon_ammo_ engine function not checking "misc" type items
	SafeWrite8(0x48916B, 0x41); // jnz 0x4891AD
	SafeWrite8(0x4891C8, CodeType::JumpShort); // jmp 0x4891E9 (skip proto data correction)

	// Fix to prevent the windows of Tag! and Mutate! perks from reappearing when there are still unused perks
	HookCall(0x43C9A0, perks_dialog_hook_tag);
	HookCall(0x43C9E2, perks_dialog_hook_mutate);
	HookCall(0x4329D1, editor_design_hook); // reset flags on exiting the character screen
}

}
