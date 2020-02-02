#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "HookScripts\InventoryHs.h"
#include "Drugs.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"

#include "BugFixes.h"

namespace sfall
{
using namespace fo;
using namespace Fields;
using namespace ObjectFlag;

static DWORD critterBody = 0;
static DWORD sizeOnBody = 0;
static DWORD weightOnBody = 0;

static char textBuf[355];

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
	for (auto it = drugsPid.begin(); it != drugsPid.end(); it++) {
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
///////////////////////////////////////////////////////////////////////////////

void ResetBodyState() {
	__asm mov critterBody, 0;
	__asm mov sizeOnBody, 0;
	__asm mov weightOnBody, 0;
}

static void Initialization() {
	*(DWORD*)FO_VAR_gDialogMusicVol = *(DWORD*)FO_VAR_background_volume; // fix dialog music

	// Restore calling original engine functions from HRP hacks (there is no difference in HRP functions)
	long long data = 0xC189565153;
	SafeWriteBytes(0x4D78CC, (BYTE*)&data, 5); // win_get_top_win_
	data = 0xC389565153;
	SafeWriteBytes(0x4CA9DC, (BYTE*)&data, 5); // mouse_get_position_
}

// fix for vanilla negate operator not working on floats
static const DWORD NegateFixHack_Back = 0x46AB77;
static void __declspec(naked) NegateFixHack() {
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

static const DWORD UnarmedAttacksFixEnd = 0x423A0D;
static void __declspec(naked) compute_attack_hack() {
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

static void __declspec(naked) SharpShooterFix() {
	__asm {
		call fo::funcoffs::stat_level_            // Perception
		cmp  edi, dword ptr ds:[FO_VAR_obj_dude]
		jne  end
		xchg ecx, eax
		mov  eax, edi                             // _obj_dude
		mov  edx, PERK_sharpshooter
		call fo::funcoffs::perk_level_
		shl  eax, 1
		add  eax, ecx
end:
		retn
	}
}

static void __declspec(naked) pipboy_hack() {
	__asm {
		cmp  ebx, 0x210                           // Back button?
		je   end
		cmp  byte ptr ds:[FO_VAR_holo_flag], 0
		jne  end
		xor  ebx, ebx                             // No man, no problem (c) :-p
end:
		mov  eax, ds:[FO_VAR_crnt_func]
		retn
	}
}

static void __declspec(naked) PipAlarm_hack() {
	__asm {
		mov  ds:[FO_VAR_crnt_func], eax
		mov  eax, 0x400
		call fo::funcoffs::PipStatus_
		mov  eax, 0x50CC04                        // 'iisxxxx1'
		retn
	}
}

static void __declspec(naked) PipStatus_hook() {
	__asm {
		call fo::funcoffs::ListHoloDiskTitles_;
		mov  dword ptr ds:[FO_VAR_holodisk], ebx;
		retn;
	}
}

// corrects saving script blocks (to *.sav file) by properly accounting for actual number of scripts to be saved
static void __declspec(naked) scr_write_ScriptNode_hook() {
	__asm {
		mov  ecx, 16
		cmp  dword ptr [esp+0xEC+4], ecx          // number_of_scripts
		jg   skip
		mov  ecx, dword ptr [esp+0xEC+4]
		cmp  ecx, 0
		jg   skip
		xor  eax, eax
		retn
skip:
		sub  dword ptr [esp+0xEC+4], ecx          // number_of_scripts
		push dword ptr [ebp+0xE00]                // num
		mov  dword ptr [ebp+0xE00], ecx           // num
		xor  ecx, ecx
		xchg dword ptr [ebp+0xE04], ecx           // NextBlock
		call fo::funcoffs::scr_write_ScriptNode_
		xchg dword ptr [ebp+0xE04], ecx           // NextBlock
		pop  dword ptr [ebp+0xE00]                // num
		retn
	}
}

static void __declspec(naked) protinst_default_use_item_hack() {
	__asm {
		mov  eax, dword ptr [edx + protoId]       // eax = target pid
		cmp  eax, PID_DRIVABLE_CAR
		je   isCar
		cmp  eax, PID_CAR_TRUNK
		jne  notCar
isCar:
		mov  eax, ebx
		call fo::funcoffs::obj_use_power_on_car_
		cmp  eax, -1
		jne  skip
notCar:
		push 0x49C38B
		retn                                      // "That does nothing."
skip:
		test eax, eax
		jnz  end
		dec  eax
end:
		push 0x49C3C5
		retn
	}
}

static void __declspec(naked) obj_use_power_on_car_hack() {
	__asm {
		xor  eax, eax
		cmp  ebx, 596                             // "The car is already full of power."?
		je   skip                                 // Yes
		inc  eax                                  // "You charge the car with more power."
skip:
		retn
	}
}

static void __declspec(naked) item_d_check_addict_hack() { // replace engine function
	__asm {
		push 0x47A6A1;                            // return addr
		mov  edx, 2;                              // type = addiction
		cmp  eax, -1;                             // Has drug_pid?
		jne  skip;                                // No
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];
		jmp  fo::funcoffs::queue_find_first_;     // return player addiction
skip:
		mov  ebx, eax;                            // ebx = drug_pid
		mov  eax, esi;                            // eax = who
		call fo::funcoffs::queue_find_first_;
loopQueue:
		test eax, eax;                            // Has something in the list?
		jz   end;                                 // No
		cmp  ebx, dword ptr [eax + 0x4];          // drug_pid == queue_addict.drug_pid?
		je   end;                                 // Has specific addiction
		mov  eax, esi;                            // eax = who
		mov  edx, 2;                              // type = addiction
		call fo::funcoffs::queue_find_next_;
		jmp  loopQueue;
end:
		retn;
	}
}

static void __declspec(naked) RemoveJetAddictFunc() {
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

static void __declspec(naked) item_d_take_drug_hack() {
	__asm {
		cmp  dword ptr [eax], 0;                  // queue_addict.init
		jne  skip;                                // Addiction is not active yet
		mov  edx, PERK_add_jet;
		mov  eax, esi;
		call fo::funcoffs::perform_withdrawal_end_;
skip:
		mov  dword ptr ds:[FO_VAR_wd_obj], esi;
		mov  eax, 2;                              // type = addiction
		mov  edx, offset RemoveJetAddictFunc;
		call fo::funcoffs::queue_clear_type_;
		push 0x479FD1;
		retn;
	}
}

static void __declspec(naked) item_d_load_subfix() {
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
static void __declspec(naked) item_d_load_hack() {
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
static void __declspec(naked) item_d_save_hack() {
	__asm {
		pushadc;
		mov  ecx, [edx];                          // drug pid
		call DrugPidPush;
		popadc;
		mov  ebx, 3;
		retn;
	}
}

static void __declspec(naked) queue_clear_type_mem_free_hook() {
	__asm {
		mov  ebx, [esi]
		jmp  fo::funcoffs::mem_free_
	}
}

static void __declspec(naked) partyMemberCopyLevelInfo_hack() {
	__asm {
		mov  eax, esi; // source
		xor  ecx, ecx; // animation
		mov  ebx, 1;   // right slot
		retn;
	}
}

static void __declspec(naked) partyMemberCopyLevelInfo_hook_stat_level() {
	__asm {
nextArmor:
		mov  eax, esi;
		call fo::funcoffs::inven_worn_;
		test eax, eax;
		jz   noArmor;
		and  byte ptr [eax][flags + 3], ~Worn >> 24; // Unset flag of equipped armor
		jmp  nextArmor;
noArmor:
		mov  eax, esi;
		jmp  fo::funcoffs::stat_level_;
	}
}

static void __declspec(naked) correctFidForRemovedItem_hook_adjust_ac() {
	__asm {
		call fo::funcoffs::adjust_ac_;
nextArmor:
		mov  eax, esi;
		call fo::funcoffs::inven_worn_;
		test eax, eax;
		jz   end;
		and  byte ptr [eax][flags + 3], ~Worn >> 24; // Unset flag of equipped armor
		jmp  nextArmor;
end:
		retn;
	}
}

static void __declspec(naked) op_move_obj_inven_to_obj_hook() {
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

static void __declspec(naked) obj_drop_hook() {
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

static void __declspec(naked) partyMemberIncLevels_hook() {
	__asm {
		mov  ebx, eax;                            // party member pointer
		call fo::funcoffs::partyMemberCopyLevelInfo_;
		cmp  eax, -1;
		je   end;
		xor  edx, edx;                            // queue type (0)
		mov  eax, ebx;                            // source
		call fo::funcoffs::queue_remove_this_;
		push ecx;
		push edi;
		push esi;
		mov  ecx, 8;
		mov  edi, FO_VAR_drugInfoList;
		mov  esi, ebx;                            // pointer for fixed item_d_check_addict_
loopAddict:
		mov  eax, dword ptr [edi];                // eax = drug pid
		call fo::funcoffs::item_d_check_addict_;
		test eax, eax;                            // Has addiction?
		jz   noAddict;                            // No
		cmp  dword ptr [eax], 0;                  // queue_addict.init
		jne  noAddict;                            // Addiction is not active yet
		mov  edx, dword ptr [eax + 0x8];          // queue_addict.perk
		mov  eax, ebx;
		call fo::funcoffs::perk_add_effect_;
noAddict:
		lea  edi, [edi + 12];
		dec  ecx;
		jnz  loopAddict;
		pop  esi;
		pop  edi;
		pop  ecx;
end:
		retn;
	}
}

static void __declspec(naked) gdProcessUpdate_hack() {
	__asm {
		lea  edx, [eax - 2];
		cmp  edx, dword ptr ds:[FO_VAR_optionRect + 0xC]; // _optionRect.offy
		jl   skip;
		mov  eax, edx;
skip:
		retn;
	}
}

static void __declspec(naked) invenWieldFunc_item_get_type_hook() {
	__asm {
		mov  edx, esi
		xor  ebx, ebx
		inc  ebx
		push ebx
		mov  cl, byte ptr [edi+0x27]
		and  cl, 0x3
		xchg edx, eax                             // eax = who, edx = item
		call fo::funcoffs::item_remove_mult_
		xchg ebx, eax
		mov  eax, esi
		test cl, INVEN_TYPE_LEFT_HAND             // Right hand?
		jz   leftHand                             // No
		call fo::funcoffs::inven_right_hand_
		jmp  removeFlag
leftHand:
		call fo::funcoffs::inven_left_hand_
removeFlag:
		test eax, eax
		jz   noWeapon
		and  byte ptr [eax+0x27], 0xFC            // Unset flag of a weapon in hand
noWeapon:
		or   byte ptr [edi+0x27], cl              // Set flag of a weapon in hand
		inc  ebx
		pop  ebx
		jz   skip
		mov  eax, esi
		mov  edx, edi
		call fo::funcoffs::item_add_force_
skip:
		mov  eax, edi
		jmp  fo::funcoffs::item_get_type_
	}
}

static void __declspec(naked) is_supper_bonus_hack() {
	__asm {
		add  eax, ecx
		test eax, eax
		jle  skip
		cmp  eax, 10
		jle  end
skip:
		add  esp, 4                               // Destroy the return address
		xor  eax, eax
		inc  eax
		pop  edx
		pop  ecx
		pop  ebx
end:
		retn
	}
}

static void __declspec(naked) PrintBasicStat_hack() {
	__asm {
		test eax, eax
		jle  skip
		cmp  eax, 10
		jg   end
		add  esp, 4                               // Destroy the return address
		push 0x434C21
		retn
skip:
		xor  eax, eax
end:
		retn
	}
}

static void __declspec(naked) StatButtonUp_hook() {
	__asm {
		call fo::funcoffs::inc_stat_
		test eax, eax
		jl   end
		test ebx, ebx
		jge  end
		sub  ds:[FO_VAR_character_points], esi
		dec  esi
		mov  [esp+0xC+0x4], esi
end:
		retn
	}
}

static void __declspec(naked) StatButtonDown_hook() {
	__asm {
		call fo::funcoffs::stat_level_
		cmp  eax, 1
		jg   end
		add  esp, 4                               // Destroy the return address
		xor  eax, eax
		inc  eax
		mov  [esp+0xC], eax
		push 0x437B41
end:
		retn
	}
}

// Calculate weight & size of equipped items
static void __declspec(naked) loot_container_hack() {
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

static void __declspec(naked) barter_inventory_hook() {
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
static void __declspec(naked) item_total_weight_hack() {
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
static void __declspec(naked) item_c_curr_size_hack() {
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

static void __declspec(naked) inven_pickup_hack() {
	__asm {
		mov  edx, ds:[FO_VAR_pud];
		mov  edx, [edx];                          // itemsCount
		dec  edx;
		sub  edx, eax;
		lea  edx, ds:0[edx * 8];
		retn;
	}
}

static void __declspec(naked) inven_pickup_hack2() {
	__asm {
		test eax, eax
		jz   end
		mov  eax, ds:[FO_VAR_i_wid]
		call fo::funcoffs::GNW_find_
		mov  ecx, [eax+0x8+0x4]                   // ecx = _i_wid.rect.y
		mov  eax, [eax+0x8+0x0]                   // eax = _i_wid.rect.x
		add  eax, 44                              // x_start
		mov  ebx, 64
		add  ebx, eax                             // x_end
		xor  edx, edx
next:
		push eax
		push edx
		push ecx
		push ebx
		imul edx, edx, 48
		add  edx, 35
		add  edx, ecx                             // y_start
		mov  ecx, edx
		add  ecx, 48                              // y_end
		call fo::funcoffs::mouse_click_in_
		pop  ebx
		pop  ecx
		pop  edx
		test eax, eax
		pop  eax
		jnz  found
		inc  edx
		cmp  edx, 6
		jb   next
end:
		push 0x47125C
		retn
found:
		mov  ebx, 0x4711DF
		add  edx, [esp+0x40]                      // inventory_offset
		mov  eax, ds:[FO_VAR_pud]
		mov  ecx, [eax]                           // itemsCount
		test ecx, ecx
		jz   skip
		dec  ecx
		cmp  edx, ecx
		ja   skip
		sub  ecx, edx
		mov  edx, ecx
		mov  ebx, 0x471181
skip:
		jmp  ebx
	}
}

static void __declspec(naked) drop_ammo_into_weapon_hook() {
	__asm {
		dec  esi
		test esi, esi                             // One box of ammo?
		jz   skip                                 // Yes
		xor  esi, esi
		// Excess check for from_slot, but leave it be
		mov  edx, [esp+0x24+4]                    // from_slot
		cmp  edx, 1006                            // Hands?
		jge  skip                                 // Yes
		lea  edx, [eax + inventory]               // Inventory
		mov  ecx, [edx]                           // itemsCount
		test ecx, ecx
		jz   skip                                 // inventory is empty (another excess check, but leave it)
		mov  edx, [edx+8]                         // FirstItem
nextItem:
		cmp  ebp, [edx]                           // Our weapon?
		je   foundItem                            // Yes
		add  edx, 8                               // Go to the next
		dec  ecx
		jnz  nextItem
		jmp  skip                                 // Our weapon is not in inventory
foundItem:
		cmp  dword ptr [edx+4], 1                 // Only one weapon?
		jg   skip                                 // No
		mov  edx, [esp+0x24+4]                    // from_slot
		lea  edx, [edx-1000]
		add  edx, [esp+0x40+4+0x24+4]             // edx = ordinal number of slot with ammo
		cmp  ecx, edx                             // Weapon is after the ammo?
		jg   skip                                 // Yes
		inc  esi                                  // No, need to change from_slot
skip:
		mov  edx, ebp
		call fo::funcoffs::item_remove_mult_
		test eax, eax                             // Have weapon been deleted from inventory?
		jnz  end                                  // No
		sub  [esp+0x24+4], esi                    // Yes, correct from_slot
end:
		retn
	}
}

static void __declspec(naked) PipStatus_AddHotLines_hook() {
	__asm {
		call fo::funcoffs::AddHotLines_
		xor  eax, eax
		mov  dword ptr ds:[FO_VAR_hot_line_count], eax
		retn
	}
}

static void __declspec(naked) perform_withdrawal_start_display_print_hook() {
	__asm {
		test eax, eax;
		jz   end;
		jmp  fo::funcoffs::display_print_;
end:
		retn;
	}
}

static void __declspec(naked) op_wield_obj_critter_adjust_ac_hook() {
	__asm {
		call fo::funcoffs::adjust_ac_;
		xor  eax, eax;                      // not animated
		jmp  fo::funcoffs::intface_update_ac_;
	}
}

static const DWORD partyMember_init_End = 0x493D16;
static void __declspec(naked) NPCStage6Fix1() {
	__asm {
		imul eax, edx, 204;                 // multiply record size 204 bytes by number of NPC records in party.txt
		mov  ebx, eax;                      // copy total record size for later memset
		call fo::funcoffs::mem_malloc_;     // malloc the necessary memory
		jmp  partyMember_init_End;          // call memset to set all malloc'ed memory to 0
	}
}

static const DWORD partyMemberGetAIOptions_End = 0x49423A;
static void __declspec(naked) NPCStage6Fix2() {
	__asm {
		imul edx, 204;                      // multiply record size 204 bytes by NPC number as listed in party.txt
		mov  eax, dword ptr ds:[FO_VAR_partyMemberAIOptions]; // get starting offset of internal NPC table
		jmp  partyMemberGetAIOptions_End;   // eax + edx = offset of specific NPC record
	}
}

// Haenlomal: Check path to critter for attack
static void __declspec(naked) MultiHexFix() {
	__asm {
		xor  ecx, ecx;                      // argument value for make_path_func: ecx=0 (rotation data arg)
		test [ebx + flags + 1], 0x08;       // is target multihex?
		mov  ebx, [ebx + tile];             // argument value for make_path_func: target's tilenum (end_tile)
		je   end;                           // skip if not multihex
		inc  ebx;                           // otherwise, increase tilenum by 1
end:
		retn;                               // call make_path_func (at 0x429024, 0x429175)
	}
}

static const DWORD ai_move_steps_closer_move_object_ret = 0x42A192;
static void __declspec(naked) MultiHexCombatMoveFix() {
	__asm {
		test [edi + flags + 1], 0x08; // is target multihex?
		jnz  multiHex;
		test [esi + flags + 1], 0x08; // is source multihex?
		jz   moveTile;
multiHex:
		mov  edx, [esp + 4];          // source's destination tilenum
		cmp  [edi + tile], edx;       // target's tilenum
		jnz  moveTile;
		add  esp, 4;
		jmp  ai_move_steps_closer_move_object_ret;
moveTile:
		retn;
	}
}

static const DWORD ai_move_steps_closer_run_object_ret = 0x42A169;
static void __declspec(naked) MultiHexCombatRunFix() {
	__asm {
		test [edi + flags + 1], 0x08; // is target multihex?
		jnz  multiHex;
		test [esi + flags + 1], 0x08; // is source multihex?
		jz   runTile;
multiHex:
		mov  edx, [esp + 4];          // source's destination tilenum
		cmp  [edi + tile], edx;       // target's tilenum
		jnz  runTile;
		add  esp, 4;
		jmp  ai_move_steps_closer_run_object_ret;
runTile:
		retn;
	}
}

//checks if an attacked object is a critter before attempting dodge animation
static void __declspec(naked) action_melee_hack() {
	__asm {
		mov  edx, 0x4113DC
		mov  ebx, [eax + artFid]                  // objStruct->FID
		and  ebx, 0x0F000000
		cmp  ebx, OBJ_TYPE_CRITTER << 24          // check if object FID type flag is set to critter
		jne  end                                  // if object not a critter leave jump condition flags
		// set to skip dodge animation
		test byte ptr [eax + damageFlags], DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN    // (original code)
		jnz  end
		mov  edx, 0x4113FE
end:
		jmp  edx
	}
}

static void __declspec(naked) action_ranged_hack() {
	__asm {
		mov  edx, 0x411B6D
		mov  ebx, [eax + artFid]                  // objStruct->FID
		and  ebx, 0x0F000000
		cmp  ebx, OBJ_TYPE_CRITTER << 24          // check if object FID type flag is set to critter
		jne  end                                  // if object not a critter leave jump condition flags
		// set to skip dodge animation
		test byte ptr [eax + damageFlags], DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN    // (original code)
		jnz  end
		mov  edx, 0x411BD2
end:
		jmp  edx
	}
}

static void __declspec(naked) set_new_results_hack() {
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

static void __declspec(naked) critter_wake_clear_hack() {
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

static const DWORD obj_load_func_Ret = 0x488F14;
static void __declspec(naked) obj_load_func_hack() {
	__asm {
		test byte ptr [eax + (flags + 1)], 0x4; // Temp_
		jz   fix;
		retn;
fix:
		mov  edi, [eax + protoId];
		and  edi, 0x0F000000;
		cmp  edi, OBJ_TYPE_CRITTER << 24;
		jne  skip;
		test byte ptr [eax + damageFlags], DAM_KNOCKED_OUT;
		jnz  clear;    // Yes
		test byte ptr [eax + damageFlags], DAM_KNOCKED_DOWN;
		jz   clear;    // No
		push eax;
		xor  ecx, ecx;
		mov  edx, eax; // object
		mov  ebx, ecx; // extramem null
		mov  eax, ecx; // time = 0
		inc  ecx;      // type = 1
		call fo::funcoffs::queue_add_; // run stand up anim
		pop  eax;
clear:
		and  word ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKED_DOWN);
skip:
		add  esp, 4;
		jmp  obj_load_func_Ret;
	}
}

static void __declspec(naked) partyMemberPrepLoadInstance_hook() {
	__asm {
		and  word ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKED_DOWN);
		jmp  fo::funcoffs::dude_stand_;
	}
}

static void __declspec(naked) combat_over_hack() {
	__asm {
		mov  [eax + combatState], edx;
		and  word ptr [eax + damageFlags], ~DAM_LOSE_TURN;
		retn;
	}
}

static void __declspec(naked) combat_over_hook() {
	__asm {
		test byte ptr [eax + damageFlags], DAM_DEAD;
		jz   fix;
		retn; // the dead cannot reload their weapons
fix:
		test byte ptr [eax + damageFlags], DAM_KNOCKED_DOWN;
		jz   skip;
		push eax;
		call fo::funcoffs::dude_standup_;
		pop  eax;
		xor  edx, edx;
skip:
		jmp  fo::funcoffs::cai_attempt_w_reload_;
	}
}

static void __declspec(naked) combat_ctd_init_hack() {
	__asm {
		mov  [esi+0x24], eax                      // ctd.targetTile
		mov  eax, [ebx + whoHitMe]                // pobj.who_hit_me
		inc  eax
		jnz  end
		mov  [ebx + whoHitMe], eax                // pobj.who_hit_me
end:
		push 0x422F11
		retn
	}
}

static void __declspec(naked) obj_save_hack() {
	__asm {
		inc  eax
		jz   end
		dec  eax
		mov  edx, [esp+0x1C]                      // combat_data
		mov  eax, [eax + cid]                     // pobj.who_hit_me.cid
		test byte ptr ds:[FO_VAR_combat_state], 1 // in combat?
		jz   clear                                // No
		cmp  dword ptr [edx], 0                   // in combat?
		jne  skip                                 // Yes
clear:
		xor  eax, eax
		dec  eax
skip:
		mov  [edx+0x18], eax                      // combat_data.who_hit_me
end:
		push 0x489422
		retn
	}
}

static void __declspec(naked) action_explode_hack() {
	using namespace fo::Scripts;
	__asm {
		mov  edx, destroy_p_proc
		mov  eax, [esi + scriptId]                // pobj.sid
		call fo::funcoffs::exec_script_proc_
		xor  edx, edx
		dec  edx
		retn
	}
}

static void __declspec(naked) action_explode_hack1() {
	__asm {
		push esi
		mov  esi, [esi+0x40]                      // ctd.target#
		call action_explode_hack
		pop  esi
		retn
	}
}

static void __declspec(naked) barter_attempt_transaction_hack() {
	__asm {
		cmp  dword ptr [eax + protoId], PID_ACTIVE_GEIGER_COUNTER
		je   found
		cmp  dword ptr [eax + protoId], PID_ACTIVE_STEALTH_BOY
		je   found
		mov  eax, 0x474D34
		jmp  eax
found:
		call fo::funcoffs::item_m_turn_off_
		mov  eax, 0x474D17
		jmp  eax                                  // Is there any other activated items among the ones being sold?
	}
}

static void __declspec(naked) barter_attempt_transaction_hook_weight() {
	__asm {
		call fo::funcoffs::item_total_weight_;
		test eax, eax;
		jnz  skip;
		xor  edx, edx;
skip:
		retn;
	}
}

static void __declspec(naked) item_m_turn_off_hook() {
	__asm {
		and  byte ptr [eax+0x25], 0xDF            // Rest flag of used items
		jmp  fo::funcoffs::queue_remove_this_
	}
}

static void __declspec(naked) combat_hack() {
	__asm {
		mov  eax, [ecx+eax]                       // eax = source
		test eax, eax
		jz   end
		push eax
		mov  edx, STAT_max_move_points
		call fo::funcoffs::stat_level_
		mov  edx, ds:[FO_VAR_gcsd]
		test edx, edx
		jz   skip
		add  eax, [edx+0x8]                       // gcsd.free_move
skip:
		pop  edx
		xchg edx, eax                             // eax = source, edx = Max action points
		mov  [eax + movePoints], edx              // pobj.curr_mp
		test byte ptr ds:[FO_VAR_combat_state], 1 // in combat?
		jz   end                                  // No
		mov  edx, [eax + cid]                     // pobj.cid
		cmp  edx, -1
		je   end
		push eax
		mov  eax, ds:[FO_VAR_aiInfoList]
		shl  edx, 4
		mov  dword ptr [edx+eax+0xC], 0           // aiInfo.lastMove
		pop  eax
end:
		mov  edx, edi                             // dude_turn
		retn
	}
}

static void __declspec(naked) wmTeleportToArea_hack() {
	__asm {
		cmp  ebx, ds:[FO_VAR_WorldMapCurrArea]
		je   end
		mov  ds:[FO_VAR_WorldMapCurrArea], ebx
		sub  eax, edx
		add  eax, ds:[FO_VAR_wmAreaInfoList]
		mov  edx, [eax+0x30]                      // wmAreaInfoList.world_posy
		mov  ds:[FO_VAR_world_ypos], edx
		mov  edx, [eax+0x2C]                      // wmAreaInfoList.world_posx
		mov  ds:[FO_VAR_world_xpos], edx
end:
		xor  eax, eax
		mov  ds:[FO_VAR_target_xpos], eax
		mov  ds:[FO_VAR_target_ypos], eax
		mov  ds:[FO_VAR_In_WorldMap], eax
		push 0x4C5A77
		retn
	}
}

static void __declspec(naked) db_get_file_list_hack() {
	__asm {
		push edi
		push edx
		xchg edi, eax                             // edi = *filename
		mov  eax, [eax+4]                         // file_lists.filenames
		lea  esi, [eax+edx]
		cld
		push es
		push ds
		pop  es
		xor  ecx, ecx
		dec  ecx
		mov  edx, ecx
		mov  ebx, ecx
		xor  eax, eax                             // searching for end of line
		repne scasb
		not  ecx
		dec  ecx
		xchg ebx, ecx                             // ebx = filename length
		lea  edi, [esp+0x200+4*6]
		repne scasb
		not  ecx
		xchg edx, ecx                             // edx = extension length +1 for "end of line"
		mov  edi, [esi]
		repne scasb
		not  ecx                                  // ecx = buffer line length +1 for "end of line"
		pop  es
		lea  eax, [ebx+edx]                       // eax = new line length
		cmp  eax, ecx                             // new line length <= buffer line length?
		jbe  end                                  // Yes
		mov  edx, [esi]
		xchg edx, eax
		call fo::funcoffs::nrealloc_                  // eax = mem, edx = size
		test eax, eax
		jnz  skip
		push 0x50B2F0                             // "Error: Ran out of memory!"
		call fo::funcoffs::debug_printf_
		add  esp, 4
		jmp  end
skip:
		mov  [esi], eax
end:
		xchg esi, eax
		pop  edx
		pop  edi
		retn
	}
}

static void __declspec(naked) gdActivateBarter_hook() {
	__asm {
		call fo::funcoffs::gdialog_barter_pressed_
		cmp  ds:[FO_VAR_dialogue_state], ecx
		jne  skip
		cmp  ds:[FO_VAR_dialogue_switch_mode], esi
		je   end
skip:
		push ecx
		push esi
		push edi
		push ebp
		sub  esp, 0x18
		push 0x44A5CC
end:
		retn
	}
}

static void __declspec(naked) switch_hand_hack() {
	__asm {
		mov  eax, ds:[FO_VAR_inven_dude]
		push eax
		mov  [edi], ebp
		inc  ecx                                   // if ecx == -1
		jz   skip
		xor  ebx, ebx
		inc  ebx
		mov  edx, ebp
		call fo::funcoffs::item_remove_mult_
skip:
		pop  edx                                  // _inven_dude
		mov  eax, ebp
		call fo::funcoffs::item_get_type_
		cmp  eax, item_type_container
		jne  end
		mov  [ebp + owner], edx                   // iobj.owner = _inven_dude
end:
		pop  ebp
		pop  edi
		pop  esi
		retn
	}
}

static void __declspec(naked) inven_item_wearing() {
	__asm {
		mov  esi, ds:[FO_VAR_inven_dude]
		xchg ebx, eax                             // ebx = source
		mov  eax, [esi + artFid]
		and  eax, 0xF000000
		sar  eax, 0x18
		test eax, eax                             // check if object FID type flag is set to item
		jnz  skip                                 // No
		mov  eax, esi
		call fo::funcoffs::item_get_type_
		cmp  eax, item_type_container             // Bag/Backpack?
		jne  skip                                 // No
		mov  eax, esi
		call fo::funcoffs::obj_top_environment_
		test eax, eax                             // has an owner?
		jz   skip                                 // No
		mov  ecx, [eax + artFid]
		and  ecx, 0xF000000
		sar  ecx, 0x18
		cmp  ecx, OBJ_TYPE_CRITTER                // check if object FID type flag is set to critter
		jne  skip                                 // No
		cmp  eax, ebx                             // the owner of the bag == source?
		je   end                                  // Yes
skip:
		xchg ebx, eax
		cmp  eax, esi
end:
		retn
	}
}

static void __declspec(naked) inven_action_cursor_hack() {
	__asm {
		cmp  dword ptr [esp+0x44+0x4], item_type_container
		jne  end
		cmp  eax, ds:[FO_VAR_stack]
		je   end
		cmp  eax, ds:[FO_VAR_target_stack]
end:
		retn
	}
}

static void __declspec(naked) use_inventory_on_hack() {
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

static int __stdcall ItemCountFix(fo::GameObject* who, fo::GameObject* item) {
	int count = 0;
	for (int i = 0; i < who->invenSize; i++) {
		auto tableItem = &who->invenTable[i];
		if (tableItem->object == item) {
			count += tableItem->count;
		} else if (fo::func::item_get_type(tableItem->object) == fo::item_type_container) {
			count += ItemCountFix(tableItem->object, item);
		}
	}
	return count;
}

static void __declspec(naked) item_count_hack() {
	__asm {
		push ecx;
		push edx; // save state
		push edx; // item
		push eax; // container-object
		call ItemCountFix;
		pop  edx;
		pop  ecx; // restore
		retn;
	}
}

static void __declspec(naked) Save_as_ASCII_hack() {
	__asm {
		mov  edx, STAT_sequence;
		mov  ebx, 626; // line index in EDITOR.MSG
		retn;
	}
}

static DWORD combatFreeMoveTmp = 0xFFFFFFFF;
static void __declspec(naked) combat_load_hook() {
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

static void __declspec(naked) combat_turn_hack() {
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

static void __declspec(naked) combat_display_hack() {
	__asm {
		mov  ebx, 0x42536B;
		je   end;                                 // This is a critter
		cmp  dword ptr [ecx + scriptId], -1;      // Does the target have a script?
		jne  end;                                 // Yes
		mov  ebx, 0x425413;
end:
		jmp  ebx;
	}
}

static void __declspec(naked) apply_damage_hack() {
	__asm {
		xchg edx, eax;
		test [esi+0x15], dl;                      // ctd.flags2Source & DAM_HIT_?
		jz   end;                                 // No
		inc  ebx;
end:
		retn;
	}
}

static void __declspec(naked) compute_attack_hook() {
	__asm {
		call fo::funcoffs::attack_crit_success_;
		test [esi+0x15], 2;                       // ctd.flags2Source & DAM_CRITICAL_?
		jz   end;                                 // No
		cmp  dword ptr [esp+0x4+0x20], 4;         // Has Silent Death perk?
		jne  end;                                 // No
		shl  eax, 1;                              // Multiply by 2 for the perk effect
end:
		retn;
	}
}

static void __declspec(naked) partyMemberGetCurLevel_hack() {
	__asm {
		mov  esi, 0xFFFFFFFF; // initialize party member index
		mov  edi, dword ptr ds:[FO_VAR_partyMemberMaxCount];
		retn;
	}
}

static void __declspec(naked) ResetPlayer_hook() {
	__asm {
		mov  edx, eax;
		call fo::funcoffs::stat_set_defaults_;
		mov  dword ptr [edx + 0x78], 100; // critter_data.base_dr_emp
		retn;
	}
}

static const DWORD obj_move_to_tile_Ret = 0x48A74E;
static void __declspec(naked) obj_move_to_tile_hack() {
	__asm {
		cmp  dword ptr ds:[FO_VAR_map_state], 0; // map number, -1 exit to worldmap
		jz   mapLeave;
		add  esp, 4;
		jmp  obj_move_to_tile_Ret;
mapLeave:
		mov  ebx, 16;
		retn;
	}
}

static void __declspec(naked) obj_move_to_tile_hack_seen() {
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

static void __declspec(naked) ai_move_steps_closer_hook() {
	__asm {
		call  fo::funcoffs::combat_turn_run_;
		movzx dx, word ptr [esi + damageFlags]; // combat_data.results
		test  dx, DAM_DEAD or DAM_KNOCKED_OUT or DAM_LOSE_TURN;
		jz    end;
		mov   [esi + movePoints], 0;            // pobj.curr_mp (source reset ap)
end:
		retn;
	}
}

//zero damage insta death criticals fix (moved from compute_damage hook)
static void __fastcall InstantDeathFix(fo::ComputeAttackResult &ctd) {
	if (ctd.targetDamage == 0 && (ctd.targetFlags & fo::DamageFlag::DAM_DEAD)) {
		ctd.targetDamage++; // set 1 hp damage
	}
}

static const DWORD ComputeDamageRet = 0x424BA7;
static void __declspec(naked) compute_damage_hack() {
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
		strncpy_s(textBuf, desc, 161);
		int len = strlen(textBuf);
		if (len > 160) {
			len = 158;
			textBuf[len++] = '.';
			textBuf[len++] = '.';
			textBuf[len++] = '.';
		}
		textBuf[len++] = ' ';
		textBuf[len] = 0;
		currDescLen  = len;
	} else if (currDescLen == 0) {
		textBuf[0] = 0;
	}

	strncat(textBuf, text, 64);
	currDescLen += strlen(text);
	if (currDescLen < 300) {
		textBuf[currDescLen++] = '.';
		textBuf[currDescLen++] = ' ';
		textBuf[currDescLen] = 0;
	}
}

static void __declspec(naked) obj_examine_func_hack_ammo0() {
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

static void __declspec(naked) obj_examine_func_hack_ammo1() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14 + 4], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		push 0;
		push eax;
		call AppendText;
		mov  currDescLen, 0;
		lea  eax, [textBuf];
		jmp  fo::funcoffs::gdialogDisplayMsg_;
skip:
		jmp  dword ptr [esp + 0x1AC - 0x14 + 4];
	}
}

static const DWORD ObjExamineFuncWeapon_Ret = 0x49B63C;
static void __declspec(naked) obj_examine_func_hack_weapon() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		push esi;
		push eax;
		call AppendText;
		mov  eax, currDescLen;
		sub  eax, 2;
		mov  byte ptr textBuf[eax], 0; // cutoff last character
		mov  currDescLen, 0;
		lea  eax, [textBuf];
skip:
		jmp  ObjExamineFuncWeapon_Ret;
	}
}

static DWORD expSwiftLearner; // experience points for print
static void __declspec(naked) statPCAddExperienceCheckPMs_hack() {
	__asm {
		mov  expSwiftLearner, edi;
		mov  eax, dword ptr ds:[FO_VAR_Experience_];
		retn;
	}
}

static void __declspec(naked) combat_give_exps_hook() {
	__asm {
		call fo::funcoffs::stat_pc_add_experience_;
		mov  ebx, expSwiftLearner;
		retn;
	}
}

static const DWORD LootContainerExp_Ret = 0x4745E3;
static void __declspec(naked) loot_container_exp_hack() {
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

static void __declspec(naked) wmRndEncounterOccurred_hook() {
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

static void __declspec(naked) op_obj_can_see_obj_hack() {
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

static void __declspec(naked) op_obj_can_hear_obj_hack() {
	__asm {
		mov  eax, [esp + 0x28 - 0x28 + 4];  // target
		mov  edx, [esp + 0x28 - 0x24 + 4];  // source
		retn;
	}
}

static void __declspec(naked) ai_best_weapon_hook() {
	__asm {
		mov  eax, [esp + 0xF4 - 0x10 + 4];  // prev.item
		jmp  fo::funcoffs::item_w_perk_;
	}
}

static void __declspec(naked) wmSetupRandomEncounter_hook() {
	__asm {
		push eax;                  // text 2
		push edi;                  // text 1
		push 0x500B64;             // fmt '%s %s'
		lea  edi, textBuf;
		push edi;                  // buf
		call fo::funcoffs::sprintf_;
		add  esp, 16;
		mov  eax, edi;
		jmp  fo::funcoffs::display_print_;
	}
}

static void __declspec(naked) inven_obj_examine_func_hack() {
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
static void __declspec(naked) SliderBtn_hook_down() {
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

static void __declspec(naked) Add4thTagSkill_hook() {
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
static void __declspec(naked) ai_retrieve_object_hook() {
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

static void __declspec(naked) inven_find_id_hack() {
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

static DWORD op_start_gdialog_ret = 0x456F4B;
static void __declspec(naked) op_start_gdialog_hack() {
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

static void __declspec(naked) item_w_range_hook() {
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

static void __declspec(naked) determine_to_hit_func_hook() {
	__asm {
		test [esp + 0x38 + 0x8 + 4], 1; // isRange
		jz   noRange;
		jmp  fo::funcoffs::obj_dist_with_tile_;
noRange:
		mov  eax, 1;                    // set distance
		retn;
	}
}

static void __declspec(naked) process_rads_hook() {
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

static DWORD firstItemDrug = -1;
static const DWORD ai_check_drugs_hack_Ret = 0x42878B;
static void __declspec(naked) ai_check_drugs_hack_break() {
	__asm {
		mov  eax, -1;
		cmp  firstItemDrug, eax;
		jnz  firstDrugs;
		add  esp, 4;
		jmp  ai_check_drugs_hack_Ret;      // break loop
firstDrugs:
		mov  dword ptr [esp + 4], eax;     // buf
		mov  edi, firstItemDrug;
		mov  ebx, edi;
		mov  firstItemDrug, eax;
		retn;                              // use drug
	}
}

static void __declspec(naked) ai_check_drugs_hack_check() {
	__asm {
		test [esp + 0x34 - 0x30 + 4], 1;   // check NoInvenItem flag
		jnz  skip;
		cmp  dword ptr [edx + 0xAC], -1;   // Chemical Preference Number (cap.chem_primary_desire)
		jnz  checkDrugs;
skip:
		xor  ebx, ebx;                     // set zero flag for skipping preference list check
		retn;
checkDrugs:
		cmp  ebx, [edx + 0xAC];            // Chemical Preference Number
		retn;
	}
}

static const DWORD ai_check_drugs_hack_Loop = 0x428675;
static void __declspec(naked) ai_check_drugs_hack_use() {
	__asm {
		cmp  eax, 3;
		jge  beginLoop;
		retn;                              // use drug
beginLoop:
		cmp  firstItemDrug, -1;
		jnz  skip;
		mov  firstItemDrug, edi;           // keep drug item
skip:
		add  esp, 4;
		jmp  ai_check_drugs_hack_Loop;     // goto begin loop
	}
}

static const DWORD config_get_values_hack_Get = 0x42C13F;
static const DWORD config_get_values_hack_OK = 0x42C14D;
static const DWORD config_get_values_hack_Fail = 0x42C131;
static void __declspec(naked) config_get_values_hack() {
	__asm {
		cmp ebp, 1;                        // counter value
		jl  getOK;
		jz  getLast;
		// if ebp > 1
		mov eax, [esp + 0x100];
		cmp [eax], 0;                      // check char
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

static void __declspec(naked) db_freadInt_hook() {
	__asm {
		call fo::funcoffs::xfread_;
		test eax, eax;
		jnz  skip;
		dec  eax;
skip:
		retn;
	}
}

static void __declspec(naked) op_attack_hook() {
	__asm {
		mov  esi, dword ptr [esp + 0x3C + 4];   // free_move
		mov  ebx, dword ptr [esp + 0x40 + 4];   // add amount damage to target
		jmp  fo::funcoffs::gdialogActive_;
	}
}

static void __declspec(naked) combat_attack_hack() {
	__asm {
		mov  ebx, ds:[FO_VAR_main_ctd + 0x2C]; // amountTarget
		test ebx, ebx;
		jz   end;
		retn;
end:
		add  esp, 4;
		mov  ebx, 0x423039;
		jmp  ebx;
	}
}

static void __declspec(naked) op_use_obj_on_obj_hack() {
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

static void __declspec(naked) op_use_obj_hack() {
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

static const DWORD combat_End = 0x422E45;
static const DWORD combat_Load = 0x422E91;
static void __declspec(naked) combat_hack_load() {
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

static void __declspec(naked) JesseContainerFid() {
	__asm {
		dec  edx; // set fid to -1
		jmp  fo::funcoffs::obj_new_;
	}
}

static void __declspec(naked) ai_search_inven_weap_hook() {
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
		mov  eax, GUNS; // set GUNS if has ammo pid
skip:
		retn;
	}
}

static void __declspec(naked) map_age_dead_critters_hack() {
	__asm {
		test ecx, ecx; // dead_bodies_age
		jz   skip;     // if (dead_bodies_age == No) exit func
		cmp  dword ptr [esp + 0x3C - 0x30 + 4], 0;
skip:
		retn;
	}
}

static void __declspec(naked) partyFixMultipleMembers_hack() {
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

static void __declspec(naked) PrintAutoMapList() {
	__asm {
		mov  eax, ds:[FO_VAR_wmMaxMapNum];
		cmp  eax, AUTOMAP_MAX;
		jb   skip;
		mov  eax, AUTOMAP_MAX;
skip:
		retn;
	}
}

static void __declspec(naked) automap_pip_save_hook() {
	__asm {
		mov  eax, ds:[FO_VAR_map_number];
		cmp  eax, AUTOMAP_MAX;
		jb   skip;
		xor  eax, eax;
skip:
		retn;
	}
}

static DWORD dudeScriptID;
static void __declspec(naked) obj_load_dude_hook0() {
	__asm {
		mov  eax, ds:[FO_VAR_obj_dude];
		mov  eax, [eax + scriptId];
		mov  dudeScriptID, eax;
		retn;
	}
}

static void __declspec(naked) obj_load_dude_hook1() {
	__asm {
		mov  ebx, dudeScriptID;
		mov  [eax + scriptId], ebx;
		retn;
	}
}

static void __declspec(naked) PrintAMList_hook() {
	__asm {
		cmp  ebp, 20; // max line count
		jle  skip;
		mov  ebp, 20;
skip:
		jmp  fo::funcoffs::qsort_;
	}
}

static void __declspec(naked) exec_script_proc_hack() {
	__asm {
		mov  eax, [esi + 0x58];
		test eax, eax;
		ja   end;
		inc  eax; // start proc
end:
		retn;
	}
}

static void __declspec(naked) exec_script_proc_hack1() {
	__asm {
		mov  esi, [edi + 0x58];
		test esi, esi;
		ja   end;
		inc  esi; // start proc
end:
		retn;
	}
}

static void __declspec(naked) op_dialogue_reaction_hook() {
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

static void __declspec(naked) obj_pickup_hook() {
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

static char pickupMessageBuf[65] = {0};
static const char* __fastcall GetPickupMessage(const char* name) {
	if (pickupMessageBuf[0] == 0) {
		Translate("sfall", "NPCPickupFail", "%s cannot pick up the item.", pickupMessageBuf, 64);
	}
	sprintf(textBuf, pickupMessageBuf, name);
	return textBuf;
}

static void __declspec(naked) obj_pickup_hook_message() {
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
static const DWORD anim_move_to_tile_jmp = 0x416D91;
static void __declspec(naked) anim_move_to_tile_hook() {
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

static void __declspec(naked) anim_move_to_tile_hook_tile() {
	__asm {
		cmp  blockingTileObj, 0;
		jne  getTile;
		retn;
getTile:
		jmp  fo::funcoffs::tile_num_in_direction_;
	}
}

static void __declspec(naked) action_use_an_item_on_object_hack() {
	__asm {
		add  ebx, ds:[FO_VAR_combat_free_move];
		mov  eax, 2; // RB_RESERVED
		retn;
	}
}

static void __declspec(naked) action_climb_ladder_hack() {
	__asm {
		add  ecx, ds:[FO_VAR_combat_free_move];
		mov  eax, 2; // RB_RESERVED
		retn;
	}
}

//static const DWORD wmAreaMarkVisitedState_Error = 0x4C4698;
static const DWORD wmAreaMarkVisitedState_Ret = 0x4C46A2;
static void __declspec(naked) wmAreaMarkVisitedState_hack() {
	__asm {
		mov  [ecx + 0x40], esi; // wmAreaInfoList.visited
		test esi, esi;          // mark "unknown" state
		jz   skip;
		mov  eax, [ecx + 0x2C]; // wmAreaInfoList.world_posx
		mov  edx, [ecx + 0x30]; // wmAreaInfoList.world_posy
		// fix loc coordinates
		cmp  [ecx + 0x34], 1; // wmAreaInfoList.size
		jg   largeLoc;
		je   mediumLoc;
//smallLoc:
		sub eax, 5;
		lea edx, [edx - 5];
mediumLoc:
		sub eax, 10;
		lea edx, [edx - 10];
largeLoc:
		mov  ebx, esp; // ppSubTile out
		push edx;
		push eax;
		call fo::funcoffs::wmFindCurSubTileFromPos_;
//		cmp  eax, -1; // always return 0
//		jz   error;
		pop  eax;
		pop  edx;
		mov  ebx, [esp];
		mov  ebx, [ebx + 0x18]; // sub-tile state
		test ebx, ebx;
		jnz  skip;
		inc  ebx; // 1
skip:
		cmp  [ecx + 0x38], 1;   // wmAreaInfoList.start_state
		jne  hideLoc;
		cmp  esi, 2; // mark visited state
		jne  fix;
		call fo::funcoffs::wmMarkSubTileRadiusVisited_;
hideLoc:
		jmp  wmAreaMarkVisitedState_Ret;
fix:
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

static void __declspec(naked) wmWorldMap_hack() {
	__asm {
		mov  ebx, [ebx + 0x34]; // wmAreaInfoList.size
		cmp  ebx, 1;
		jg   largeLoc;
		je   mediumLoc;
//smallLoc:
		sub  eax, 5;
		lea  edx, [edx - 5];
mediumLoc:
		sub  eax, 10;
		lea  edx, [edx - 10];
largeLoc:
		xor  ebx, ebx;
		jmp  fo::funcoffs::wmPartyInitWalking_;
	}
}

static void __declspec(naked) wmTownMapFunc_hack() {
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

static const DWORD combat_should_end_break = 0x422D00;
static void __declspec(naked) combat_should_end_hack() {
	__asm { // ecx = dude.team_num
		cmp  ecx, [ebp + 0x50]; // npc who_hit_me.team_num
		je   break;
		test byte ptr [edx], 1; // npc combat_data.combat_state
		jnz  break;
		retn; // check next critter
break:
		add  esp, 4;
		jmp  combat_should_end_break;
	}
}

static void __declspec(naked) wmInterfaceInit_hack() {
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

static void __declspec(naked) map_check_state_hook() {
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

static void __declspec(naked) op_critter_rm_trait_hook() {
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

static void __declspec(naked) op_critter_add_trait_hook() {
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
static void __declspec(naked) obj_shoot_blocking_at_hack0() {
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
static void __declspec(naked) obj_shoot_blocking_at_hack1() {
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

static void __declspec(naked) main_death_scene_hook() {
	__asm {
		mov  eax, 100;
		call fo::funcoffs::block_for_tocks_;
		jmp  fo::funcoffs::get_time_;
	}
}

void BugFixes::init()
{
	#ifndef NDEBUG
		LoadGameHook::OnBeforeGameClose() += PrintAddrList;
		if (isDebug && (iniGetInt("Debugging", "BugFixes", 1, ::sfall::ddrawIni) == 0)) return;
	#endif

	// Missing game initialization
	LoadGameHook::OnBeforeGameInit() += Initialization;

	// Fix vanilla negate operator for float values
	MakeCall(0x46AB68, NegateFixHack);
	// Fix incorrect int-to-float conversion
	// op_mult:
	SafeWrite16(0x46A3F4, 0x04DB); // replace operator to "fild 32bit"
	SafeWrite16(0x46A3A8, 0x04DB);
	// op_div:
	SafeWrite16(0x46A566, 0x04DB);
	SafeWrite16(0x46A4E7, 0x04DB);
	// Fix for vanilla division operator treating negative integers as unsigned
	if (GetConfigInt("Misc", "DivisionOperatorFix", 1)) {
		dlog("Applying division operator fix.", DL_INIT);
		SafeWrite32(0x46A51D, 0xFBF79990); // xor edx, edx; div ebx > cdq; idiv ebx
		dlogr(" Done", DL_INIT);
	}

	//if (GetConfigInt("Misc", "SpecialUnarmedAttacksFix", 1)) {
		dlog("Applying Special Unarmed Attacks fix.", DL_INIT);
		MakeJump(0x42394D, compute_attack_hack);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "SharpshooterFix", 1)) {
		dlog("Applying Sharpshooter patch.", DL_INIT);
		// http://www.nma-fallout.com/threads/fo2-engine-tweaks-sfall.178390/page-119#post-4050162
		// by Slider2k
		HookCall(0x4244AB, SharpShooterFix); // hooks stat_level_() call in detemine_to_hit_func_()
		// // removes this line by making unconditional jump:
		// if ( who == obj_dude )
		//     dist -= 2 * perk_level_(obj_dude, PERK_sharpshooter);
		SafeWrite8(0x424527, 0xEB);  // in detemine_to_hit_func_()
		dlogr(" Done", DL_INIT);
	//}

	// Fixes for clickability issue in Pip-Boy and exploit that allows to rest in places where you shouldn't be able to rest
	dlog("Applying fix for Pip-Boy clickability issues and rest exploit.", DL_INIT);
	MakeCall(0x4971C7, pipboy_hack);
	MakeCall(0x499530, PipAlarm_hack);
	// Fix for clickability issue of holodisk list
	HookCall(0x497E9F, PipStatus_hook);
	SafeWrite16(0x497E8C, 0xD389); // mov ebx, edx
	SafeWrite32(0x497E8E, 0x90909090);
	dlogr(" Done", DL_INIT);

	// Fix for "Too Many Items" bug
	//if (GetConfigInt("Misc", "TooManyItemsBugFix", 1)) {
		dlog("Applying preventive patch for \"Too Many Items\" bug.", DL_INIT);
		HookCall(0x4A596A, scr_write_ScriptNode_hook);
		HookCall(0x4A59C1, scr_write_ScriptNode_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for cells getting consumed even when the car is already fully charged
	MakeCall(0x49BE70, obj_use_power_on_car_hack);

	// Fix for being able to charge the car by using cells on other scenery/critters
	if (GetConfigInt("Misc", "CarChargingFix", 1)) {
		dlog("Applying car charging fix.", DL_INIT);
		MakeJump(0x49C36D, protinst_default_use_item_hack);
		dlogr(" Done", DL_INIT);
	}

	// Fix for gaining stats from more than two doses of a specific chem after save-load
	dlog("Applying fix for save-load unlimited drug use exploit.", DL_INIT);
	MakeCall(0x47A25B, item_d_save_hack);
	MakeCall(0x47A243, item_d_load_hack);
	dlogr(" Done", DL_INIT);

	// Fix crash when leaving the map while waiting for someone to die of a super stimpak overdose
	dlog("Applying fix for \"leaving the map while waiting for a super stimpak overdose death\" crash.", DL_INIT);
	HookCall(0x4A27E7, queue_clear_type_mem_free_hook); // hooks mem_free_()
	dlogr(" Done", DL_INIT);

	// Evil bug! If party member has the same armor type in inventory as currently equipped, then
	// on level up he loses Armor Class equal to the one received from this armor.
	// The same happens if you just order NPC to remove the armor through dialogue.
	//if (GetConfigInt("Misc", "ArmorCorruptsNPCStatsFix", 1)) {
		dlog("Applying fix for armor reducing NPC original stats when removed.", DL_INIT);
		HookCall(0x495F3B, partyMemberCopyLevelInfo_hook_stat_level);
		HookCall(0x45419B, correctFidForRemovedItem_hook_adjust_ac);
		// Fix for move_obj_inven_to_obj function
		HookCall(0x45C49A, op_move_obj_inven_to_obj_hook);
		SafeWrite16(0x45C496, 0x9090);
		SafeWrite8(0x45C4A3, 0x75); // jmp > jnz
		// Fix for drop_obj function
		HookCall(0x49B965, obj_drop_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix of invalid stats when party member gains a level while being on drugs
	dlog("Applying fix for addicted party member level up bug.", DL_INIT);
	HookCall(0x495D5C, partyMemberIncLevels_hook);
	dlogr(" Done", DL_INIT);

	// Allow 9 options (lines of text) to be displayed correctly in a dialog window
	//if (GetConfigInt("Misc", "DialogOptions9Lines", 1)) {
		dlog("Applying 9 dialog options patch.", DL_INIT);
		MakeCall(0x447021, gdProcessUpdate_hack, 1);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for "Unlimited Ammo" exploit
	dlog("Applying fix for Unlimited Ammo exploit.", DL_INIT);
	HookCall(0x472957, invenWieldFunc_item_get_type_hook); // hooks item_get_type_()
	dlogr(" Done", DL_INIT);

	// Fix for negative values in Skilldex window ("S")
	dlog("Applying fix for negative values in Skilldex window.", DL_INIT);
	SafeWrite8(0x4AC377, 0x7F);                // jg
	dlogr(" Done", DL_INIT);

	// Fix for negative SPECIAL values in character creation
	dlog("Applying fix for negative SPECIAL values in character creation.", DL_INIT);
	MakeCall(0x43DF6F, is_supper_bonus_hack);
	MakeCall(0x434BFF, PrintBasicStat_hack);
	HookCall(0x437AB4, StatButtonUp_hook);
	HookCall(0x437B26, StatButtonDown_hook);
	dlogr(" Done", DL_INIT);

	// Fix for not counting in the weight/size of equipped items on NPC when stealing or bartering
	//if (GetConfigInt("Misc", "NPCWeightFix", 1)) {
		dlog("Applying fix for not counting in weight of equipped items on NPC.", DL_INIT);
		MakeCall(0x473B4E, loot_container_hack);
		HookCall(0x4758AB, barter_inventory_hook);
		MakeCall(0x477EAB, item_total_weight_hack, 1);
		MakeCall(0x479A2F, item_c_curr_size_hack, 1);
		dlogr(" Done", DL_INIT);
	//}

	// Corrects the max text width of the item weight in trading interface to be 64 (was 80), which matches the table width
	SafeWrite8(0x475541, 64);
	SafeWrite8(0x475789, 64);

	// Corrects the max text width of the player name in inventory to be 140 (was 80), which matches the width for item name
	SafeWrite32(0x471E48, 140);

	//if (GetConfigInt("Misc", "InventoryDragIssuesFix", 1)) {
		dlog("Applying inventory reverse order issues fix.", DL_INIT);
		// Fix for minor visual glitch when picking up solo item from the top of inventory
		// and there is multiple item stack at the bottom of inventory
		MakeCall(0x470EC2, inven_pickup_hack, 2);
		// Fix for error in player's inventory, related to IFACE_BAR_MODE=1 in f2_res.ini, and
		// also for reverse order error
		MakeJump(0x47114A, inven_pickup_hack2);
		// Fix for using only one box of ammo when a weapon is above the ammo in the inventory list
		HookCall(0x476598, drop_ammo_into_weapon_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "NPCLevelFix", 1)) {
		dlog("Applying NPC level fix.", DL_INIT);
		HookCall(0x495BC9, (void*)0x495E51); // jz 0x495E7F > jz 0x495E51
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "BlackSkilldexFix", 1)) {
		dlog("Applying black Skilldex patch.", DL_INIT);
		HookCall(0x497D0F, PipStatus_AddHotLines_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "FixWithdrawalPerkDescCrash", 1)) {
		dlog("Applying withdrawal perk description crash fix.", DL_INIT);
		HookCall(0x47A501, perform_withdrawal_start_display_print_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "JetAntidoteFix", 1)) {
		dlog("Applying Jet Antidote fix.", DL_INIT);
		// the original jet antidote fix
		MakeJump(0x47A013, (void*)0x47A168);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "NPCDrugAddictionFix", 1)) {
		dlog("Applying NPC's drug addiction fix.", DL_INIT);
		// proper checks for NPC's addiction instead of always using global vars
		MakeJump(0x47A644, item_d_check_addict_hack);
		MakeJump(0x479FC5, item_d_take_drug_hack);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "ShivPatch", 1)) {
		dlog("Applying shiv patch.", DL_INIT);
		SafeWrite8(0x477B2B, 0xEB);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "ImportedProcedureFix", 0)) {
		dlog("Applying imported procedure patch.", DL_INIT);
		// http://teamx.ru/site_arc/smf/index.php-topic=398.0.htm
		SafeWrite16(0x46B35B, 0x1C60); // Fix problems with the temporary stack
		SafeWrite32(0x46B35D, 0x90909090);
		SafeWrite8(0x46DBF1, 0xEB); // Disable warnings
		SafeWrite8(0x46DDC4, 0xEB); // Disable warnings
		SafeWrite8(0x4415CC, 0x00); // Prevent crashes when re-exporting
		dlogr(" Done", DL_INIT);
	//}
	// Fix for op_lookup_string_proc_ engine function not searching the last procedure in a script
	SafeWrite8(0x46C7AC, 0x76); // jb > jbe

	// Update the AC counter
	//if (GetConfigInt("Misc", "WieldObjCritterFix", 1)) {
		dlog("Applying wield_obj_critter fix.", DL_INIT);
		SafeWrite8(0x456912, 0x1E); // jnz 0x456931
		HookCall(0x45697F, op_wield_obj_critter_adjust_ac_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Enable party members with level 6 protos to reach level 6
	//if (GetConfigInt("Misc", "NPCStage6Fix", 1)) {
		dlog("Applying NPC Stage 6 Fix.", DL_INIT);
		MakeJump(0x493CE9, NPCStage6Fix1); // partyMember_init_
		MakeJump(0x494224, NPCStage6Fix2); // partyMemberGetAIOptions_
		SafeWrite8(0x494063, 6);   // loop should look for a potential 6th stage (partyMember_init_)
		SafeWrite8(0x4940BB, 204); // move pointer by 204 bytes instead of 200
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "MultiHexPathingFix", 1)) {
		dlog("Applying MultiHex Pathing Fix.", DL_INIT);
		MakeCalls(MultiHexFix, {0x42901F, 0x429170});
		// Fix for multihex critters moving too close and overlapping their targets in combat
		MakeCall(0x42A14F, MultiHexCombatRunFix, 1);
		MakeCall(0x42A178, MultiHexCombatMoveFix, 1);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "DodgyDoorsFix", 1)) {
		dlog("Applying Dodgy Door Fix.", DL_INIT);
		MakeJump(0x4113D6, action_melee_hack);
		MakeJump(0x411BCC, action_ranged_hack);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for "NPC turns into a container" bug
	//if (GetConfigInt("Misc", "NPCTurnsIntoContainerFix", 1)) {
		dlog("Applying fix for \"NPC turns into a container\" bug.", DL_INIT);
		MakeJump(0x42E46E, critter_wake_clear_hack);
		MakeCall(0x488EF3, obj_load_func_hack, 1);
		HookCall(0x4949B2, partyMemberPrepLoadInstance_hook);
		MakeCall(0x421F64, combat_over_hack, 1);
		dlogr(" Done", DL_INIT);
	//}
	// Fix for multiple knockout events being added to the queue
	HookCall(0x424F9A, set_new_results_hack);
	// Fix for knocked down critters not playing stand up animation when the combat ends (when DAM_LOSE_TURN and DAM_KNOCKED_DOWN
	// flags are set) and prevent dead NPCs from reloading their weapons
	HookCall(0x421F30, combat_over_hook);

	dlog("Applying fix for explosives bugs.", DL_INIT);
	// Fix crashes when killing critters with explosives
	MakeJump(0x422F05, combat_ctd_init_hack);
	MakeJump(0x489413, obj_save_hack);
	// Fix for destroy_p_proc not being called if the critter is killed by explosives when you leave the map
	MakeCall(0x4130C3, action_explode_hack);
	MakeCall(0x4130E5, action_explode_hack1);
	dlogr(" Done", DL_INIT);

	// Fix for being unable to sell used geiger counters or stealth boys
	if (GetConfigInt("Misc", "CanSellUsedGeiger", 1)) {
		dlog("Applying fix for being unable to sell used geiger counters or stealth boys.", DL_INIT);
		SafeWrite8(0x478115, 0xBA);
		SafeWrite8(0x478138, 0xBA);
		MakeJump(0x474D22, barter_attempt_transaction_hack);
		HookCall(0x4798B1, item_m_turn_off_hook);
		dlogr(" Done", DL_INIT);
	}

	// Fix for incorrect initialization of action points at the beginning of each turn
	dlog("Applying Action Points initialization fix.", DL_INIT);
	BlockCall(0x422E02);
	MakeCall(0x422E1B, combat_hack);
	dlogr(" Done", DL_INIT);

	// Fix for incorrect death animations being used when killing critters with kill_critter_type function
	dlog("Applying kill_critter_type fix.", DL_INIT);
	SafeWrite16(0x457E22, 0xDB31); // xor ebx, ebx
	SafeWrite32(0x457C99, 0x30BE0075); // jnz loc_457C9B; mov esi, 48
	dlogr(" Done", DL_INIT);

	// Fix for checking the horizontal position on the y-axis instead of x when setting coordinates on the world map
	SafeWrite8(0x4C4743, 0xC6); // cmp esi, eax

	// Partial fix for incorrect positioning after exiting small locations (e.g. Ghost Farm)
	//if (GetConfigInt("Misc", "SmallLocExitFix", 1)) {
		dlog("Applying fix for incorrect positioning after exiting small locations.", DL_INIT);
		MakeJump(0x4C5A41, wmTeleportToArea_hack);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "PrintToFileFix", 1)) {
		dlog("Applying print to file fix.", DL_INIT);
		MakeCall(0x4C67D4, db_get_file_list_hack);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for display issues when calling gdialog_mod_barter with critters with no "Barter" flag set
	//if (GetConfigInt("Misc", "gdBarterDispFix", 1)) {
		dlog("Applying gdialog_mod_barter display fix.", DL_INIT);
		HookCall(0x448250, gdActivateBarter_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "BagBackpackFix", 1)) {
		dlog("Applying fix for bag/backpack bugs.", DL_INIT);
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
		dlogr(" Done", DL_INIT);
	//}

	// Fix crash when clicking on empty space in the inventory list opened by "Use Inventory Item On" (backpack) action icon
	MakeCall(0x471A94, use_inventory_on_hack);

	// Fix item_count function returning incorrect value when there is a container-item inside
	MakeJump(0x47808C, item_count_hack); // replacing item_count_ function

	// Fix for Sequence stat value not being printed correctly when using "print to file" option
	MakeCall(0x4396F5, Save_as_ASCII_hack, 2);

	// Fix for Bonus Move APs being replenished when you save and load the game in combat
	//if (GetConfigInt("Misc", "BonusMoveFix", 1)) {
		dlog("Applying fix for Bonus Move exploit.", DL_INIT);
		HookCall(0x420E93, combat_load_hook);
		MakeCall(0x422A06, combat_turn_hack);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for the displayed message when the attack randomly hits a target that is not a critter and has a script attached
	MakeJump(0x425365, combat_display_hack);

	// Fix for damage_p_proc being called for misses if the target is not a critter
	MakeCall(0x424CD2, apply_damage_hack);

	// Fix for the double damage effect of Silent Death perk not being applied to critical hits
	//if (GetConfigInt("Misc", "SilentDeathFix", 1)) {
		dlog("Applying Silent Death patch.", DL_INIT);
		SafeWrite8(0x4238DF, 0x8C); // jl loc_423A0D
		HookCall(0x423A99, compute_attack_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix crash when calling partyMemberGetCurLevel_ on a critter that has no data in party.txt
	MakeCall(0x495FF6, partyMemberGetCurLevel_hack, 1);

	// Fix for player's base EMP DR not being properly initialized when creating a new character and then starting the game
	HookCall(0x4A22DF, ResetPlayer_hook);

	// Fix for add_mult_objs_to_inven only adding 500 of an object when the value of the "count" argument is over 99999
	SafeWrite32(0x45A2A0, 0x1869F); // 99999

	// Fix for being at incorrect hex after map change when the exit hex in source map is at the same position as
	// some exit hex in destination map
	MakeCall(0x48A704, obj_move_to_tile_hack);

	// Fix for critters killed in combat by scripting still being able to move in their combat turn if the distance parameter
	// in their AI packages is set to stay_close/charge, or NPCsTryToSpendExtraAP is enabled
	HookCall(0x42A1A8, ai_move_steps_closer_hook); // 0x42B24D

	// Fix instant death critical
	dlog("Applying instant death fix.", DL_INIT);
	MakeJump(0x424BA2, compute_damage_hack);
	dlogr(" Done", DL_INIT);

	// Fix missing AC/DR mod stats when examining ammo in the barter screen
	dlog("Applying fix for displaying ammo stats in barter screen.", DL_INIT);
	MakeCalls(obj_examine_func_hack_ammo0, {0x49B4AD, 0x49B504});
	SafeWrite16(0x49B4B2, 0x9090);
	SafeWrite16(0x49B509, 0x9090);
	MakeCall(0x49B563, obj_examine_func_hack_ammo1, 2);
	dlogr(" Done", DL_INIT);

	// Display full item description for weapon/ammo in the barter screen
	showItemDescription = (GetConfigInt("Misc", "FullItemDescInBarter", 0) != 0);
	if (showItemDescription) {
		dlog("Applying full item description in barter patch.", DL_INIT);
		HookCall(0x49B452, obj_examine_func_hack_weapon); // it's jump
		dlogr(" Done", DL_INIT);
	}

	// Display experience points with the bonus from Swift Learner perk when gained from non-scripted situations
	if (GetConfigInt("Misc", "DisplaySwiftLearnerExp", 1)) {
		dlog("Applying Swift Learner exp display patch.", DL_INIT);
		MakeCall(0x4AFAEF, statPCAddExperienceCheckPMs_hack);
		HookCall(0x4221E2, combat_give_exps_hook);
		MakeJump(0x4745AE, loot_container_exp_hack);
		SafeWrite16(0x4C0AB1, 0x23EB); // jmps 0x4C0AD6
		HookCall(0x4C0AEB, wmRndEncounterOccurred_hook);
		dlogr(" Done", DL_INIT);
	}

	// Fix for obj_can_see_obj not checking if source and target objects are on the same elevation before calling
	// is_within_perception_
	MakeCall(0x456B63, op_obj_can_see_obj_hack);
	SafeWrite16(0x456B76, 0x23EB); // jmp loc_456B9B (skip unused engine code)

	// Fix broken op_obj_can_hear_obj_ function
	if (GetConfigInt("Misc", "ObjCanHearObjFix", 0)) {
		dlog("Applying obj_can_hear_obj fix.", DL_INIT);
		SafeWrite8(0x4583D8, 0x3B); // jz loc_458414
		SafeWrite8(0x4583DE, 0x74); // jz loc_458414
		MakeCall(0x4583E0, op_obj_can_hear_obj_hack, 1);
		dlogr(" Done", DL_INIT);
	}

	// Fix for AI not checking weapon perks properly when searching for the best weapon
	int bestWeaponPerkFix = GetConfigInt("Misc", "AIBestWeaponFix", 0);
	if (bestWeaponPerkFix > 0) {
		dlog("Applying AI best weapon choice fix.", DL_INIT);
		HookCall(0x42954B, ai_best_weapon_hook);
		// also change the priority multiplier for having weapon perk to 3x (the original is 5x)
		if (bestWeaponPerkFix > 1) {
			SafeWriteBatch<BYTE>(0x55, {0x42955E, 0x4296E7});
		}
		dlogr(" Done", DL_INIT);
	}

	// Fix for the encounter description being displayed in two lines instead of one
	SafeWrite32(0x4C1011, 0x9090C789); // mov edi, eax;
	SafeWrite8(0x4C1015, 0x90);
	HookCall(0x4C1042, wmSetupRandomEncounter_hook);

	// Fix for being unable to sell/give items in the barter screen when the player/party member is overloaded
	HookCalls(barter_attempt_transaction_hook_weight, {0x474C73, 0x474CCA});

	// Fix for the underline position in the inventory display window when the item name is longer than one line
	MakeCall(0x472F5F, inven_obj_examine_func_hack, 1);

	// Fix for the exploit that allows you to gain excessive skill points from Tag! perk before leaving the character screen
	//if (GetConfigInt("Misc", "TagPerkFix", 1)) {
		dlog("Applying fix for Tag! exploit.", DL_INIT);
		HookCall(0x43B463, SliderBtn_hook_down);
		HookCall(0x43D7DD, Add4thTagSkill_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for ai_retrieve_object_ engine function not returning the requested object when there are different objects
	// with the same ID
	dlog("Applying ai_retrieve_object engine function fix.", DL_INIT);
	HookCall(0x429D7B, ai_retrieve_object_hook);
	MakeCall(0x472708, inven_find_id_hack);
	dlogr(" Done", DL_INIT);

	// Fix for the "mood" argument of start_gdialog function being ignored for talking heads
	if (GetConfigInt("Misc", "StartGDialogFix", 0)) {
		dlog("Applying start_gdialog argument fix.", DL_INIT);
		MakeCall(0x456F08, op_start_gdialog_hack);
		dlogr(" Done", DL_INIT);
	}

	// Fix for Heave Ho! perk increasing Strength stat above 10 when determining the maximum range of thrown weapons
	dlog("Applying Heave Ho! perk fix.", DL_INIT);
	HookCall(0x478AD9, item_w_range_hook);
	dlogr(" Done", DL_INIT);

	// Fix for determine_to_hit_func_ engine function taking distance into account when called from determine_to_hit_no_range_
	HookCall(0x4244C3, determine_to_hit_func_hook);

	// Display a pop-up messages box about death from radiation
	HookCall(0x42D733, process_rads_hook);

	// Fix for AI not taking chem_primary_desire in AI.txt as drug use preference when using drugs in their inventory
	if (GetConfigInt("Misc", "AIDrugUsePerfFix", 0)) {
		dlog("Applying AI drug use preference fix.", DL_INIT);
		MakeCall(0x42869D, ai_check_drugs_hack_break);
		MakeCall(0x4286AB, ai_check_drugs_hack_check);
		SafeWrite16(0x4286B0, 0x7490); // jnz > jz
		SafeWrite8(0x4286C5, 0x75);    // jz  > jnz
		MakeCall(0x4286C7, ai_check_drugs_hack_use);
		dlogr(" Done", DL_INIT);
	}

	// Fix for config_get_values_ engine function not getting the last value in a list if the list has less than the requested
	// number of values (for chem_primary_desire)
	MakeJump(0x42C12C, config_get_values_hack);

	// Fix returned result value when the file is missing
	HookCall(0x4C6162, db_freadInt_hook);

	// Fix and repurpose the unused called_shot/num_attack arguments of attack_complex function
	// also change the behavior of the result flags arguments
	// called_shot - additional damage, when the damage received by the target is above the specified minimum
	// num_attacks - the number of free action points on the first turn only
	// attacker_results - unused, must be 0 or not equal to the target_results argument when specifying result flags for the target
	if (GetConfigInt("Misc", "AttackComplexFix", 0)) {
		dlog("Applying attack_complex fix.", DL_INIT);
		HookCall(0x456D4A, op_attack_hook);
		SafeWrite8(0x456D61, 0x74); // mov [esp+x], esi
		SafeWrite8(0x456D92, 0x5C); // mov [esp+x], ebx
		SafeWrite8(0x456D98, 0x94); // setnz > setz (fix setting result flags)
		dlogr(" Done", DL_INIT);
	}

	// Fix for attack_complex still causing minimum damage to the target when the attacker misses
	MakeCall(0x422FE5, combat_attack_hack, 1);

	// Fix for critter_mod_skill taking a negative amount value as a positive
	dlog("Applying critter_mod_skill fix.", DL_INIT);
	SafeWrite8(0x45B910, 0x7E); // jbe > jle
	dlogr(" Done", DL_INIT);

	// Fix crash when calling use_obj/use_obj_on_obj without using set_self in global scripts
	// also change the behavior of use_obj_on_obj function
	// if the object uses the item on itself, then another function is called (not a bug fix)
	MakeCall(0x45C376, op_use_obj_on_obj_hack, 1);
	MakeCall(0x456A92, op_use_obj_hack, 1);

	// Fix pickup_obj/drop_obj/use_obj functions, change them to get pointer from script.self instead of script.target
	// script.target contains an incorrect pointer, which may vary depending on the situations in the game
	dlog("Applying pickup_obj/drop_obj/use_obj fix.", DL_INIT);
	SafeWriteBatch<BYTE>(0x34, { // script.target > script.self
		0x456554, // op_pickup_obj_
		0x456600, // op_drop_obj_
		0x456A6D, // op_use_obj_
		0x456AA4  // op_use_obj_
	});
	dlogr(" Done", DL_INIT);

	// Fix for critters not attacking the player in combat when loading a game saved in combat mode
	BlockCall(0x48D6F0); // obj_fix_combat_cid_for_dude_

	// Fix for the player's turn being skipped when loading a game saved in combat mode
	MakeCall(0x422E25, combat_hack_load);

	// Fix for the reserved item FRM being displayed in the top-left corner when in the loot/barter screens
	HookCalls(JesseContainerFid, {0x473AC9, 0x475895});

	// Fix the return value of has_skill function for incorrect skill numbers
	SafeWrite32(0x4AA56B, 0);

	// Fix for NPC stuck in a loop of reloading melee/unarmed weapons when out of ammo
	dlog("Applying fix for NPC stuck in a loop of reloading melee/unarmed weapons.", DL_INIT);
	HookCall(0x429A2B, ai_search_inven_weap_hook);
	dlogr(" Done", DL_INIT);

	// Fix for critters not being healed over time when entering the map if 'dead_bodies_age=No' is set in maps.txt
	// also fix the zero initialization of a local variable to correct time for removing corpses and blood
	dlog("Applying fix for the self-healing of critters when entering the map.", DL_INIT);
	MakeCall(0x483356, map_age_dead_critters_hack);
	SafeWrite32(0x4832A0, 0x9090C189); // mov ecx, eax (keep dead_bodies_age flag)
	SafeWrite32(0x4832A4, 0x0C245489); // mov [esp + var_30], edx
	dlogr(" Done", DL_INIT);

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

	// Fix for anim_move_to_tile_ engine function ignoring the distance argument for the player
	HookCall(0x416D44, anim_move_to_tile_hook);
	HookCall(0x416DD2, anim_move_to_tile_hook_tile);

	// Fix for the player's movement in combat being interrupted when trying to use objects with Bonus Move APs available
	MakeCall(0x411FD6, action_use_an_item_on_object_hack);
	MakeCall(0x411DF7, action_climb_ladder_hack); // bug caused by anim_move_to_tile_ fix

	// Fix for Scout perk being taken into account when setting the visibility of locations with mark_area_known function
	// also fix the incorrect coordinates for small/medium location circles that the engine uses to highlight their sub-tiles
	// and fix visited tiles on the world map being darkened again when a location is added next to them
	MakeJump(0x4C466F, wmAreaMarkVisitedState_hack);
	SafeWrite8(0x4C46AB, 0x58); // esi > ebx

	// Fix the position of the target marker for small/medium location circles
	MakeCall(0x4C03AA, wmWorldMap_hack, 2);

	// Fix to prevent using number keys to enter unvisited areas on a town map
	//if (GetConfigInt("Misc", "TownMapHotkeysFix", 1)) {
		dlog("Applying town map hotkeys patch.", DL_INIT);
		MakeCall(0x4C495A, wmTownMapFunc_hack, 1);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for combat not ending automatically when there are no hostile critters
	MakeCall(0x422CF3, combat_should_end_hack);
	SafeWrite16(0x422CEA, 0x0C74); // jz 0x422CF8 (skip party members)

	// Fix for the car being lost when entering a location via the Town/World button and then leaving on foot
	// (sets GVAR_CAR_PLACED_TILE (633) to -1 on exit to the world map)
	if (GetConfigInt("Misc", "CarPlacedTileFix", 1)) {
		dlog("Applying car placed tile fix.", DL_INIT);
		MakeCall(0x4C2367,  wmInterfaceInit_hack);
		dlogr(" Done", DL_INIT);
	}

	// Place the player on a nearby empty tile if the entrance tile is blocked by another object when entering a map
	HookCall(0x4836F8, map_check_state_hook);

	// Remove duplicate code from intface_redraw_ engine function
	BlockCall(0x45EBBF);

	// Fix for critter_add/rm_trait functions ignoring the value of the "amount" argument
	// Note: pass negative amount values to critter_rm_trait to remove all ranks of the perk (vanilla behavior)
	HookCall(0x458CDB, op_critter_rm_trait_hook);
	HookCall(0x458B3D, op_critter_add_trait_hook);

	// Fix to prevent corpses from blocking line of fire
	//if (GetConfigInt("Misc", "CorpseLineOfFireFix", 1)) {
		dlog("Applying fix for corpses blocking line of fire.", DL_INIT);
		MakeJump(0x48B994, obj_shoot_blocking_at_hack0);
		MakeJump(0x48BA04, obj_shoot_blocking_at_hack1);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for party member's equipped weapon being placed in the incorrect item slot after leveling up
	MakeCall(0x495FD9, partyMemberCopyLevelInfo_hack, 1);

	// Fix the playback of the speech sound file for the death screen
	HookCall(0x481409, main_death_scene_hook);
}

}
