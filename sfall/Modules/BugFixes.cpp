#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "ScriptExtender.h"

#include "BugFixes.h"

namespace sfall
{
using namespace fo;
using namespace Fields;

DWORD WeightOnBody = 0;

static void __declspec(naked) SharpShooterFix() {
	__asm {
		call fo::funcoffs::stat_level_                          // Perception
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

static void __declspec(naked) item_d_check_addict_hack() {
	__asm {
		mov  edx, 2                               // type = addiction
		cmp  eax, -1                              // Has drug_pid?
		je   skip                                 // No
		xchg ebx, eax                             // ebx = drug_pid
		mov  eax, esi                             // eax = who
		call fo::funcoffs::queue_find_first_
loopQueue:
		test eax, eax                             // Has something in the list?
		jz   end                                  // No
		cmp  ebx, dword ptr [eax+0x4]             // drug_pid == queue_addict.drug_pid?
		je   end                                  // Has specific addiction
		mov  eax, esi                             // eax = who
		mov  edx, 2                               // type = addiction
		call fo::funcoffs::queue_find_next_
		jmp  loopQueue
skip:
		mov  eax, dword ptr ds:[FO_VAR_obj_dude]
		call fo::funcoffs::queue_find_first_
end:
		push 0x47A6A1
		retn
	}
}

static void __declspec(naked) remove_jet_addict() {
	__asm {
		cmp  eax, dword ptr ds:[FO_VAR_wd_obj]
		jne  end
		cmp  dword ptr [edx+0x4], PID_JET         // queue_addict.drug_pid == PID_JET?
		jne  end
		xor  eax, eax
		inc  eax                                  // Delete from queue
		retn
end:
		xor  eax, eax                             // Don't touch
		retn
	}
}

static void __declspec(naked) item_d_take_drug_hack() {
	__asm {
		cmp  dword ptr [eax], 0                   // queue_addict.init
		jne  skip                                 // Addiction is not active yet
		mov  edx, PERK_add_jet
		mov  eax, esi
		call fo::funcoffs::perform_withdrawal_end_
skip:
		mov  dword ptr ds:[FO_VAR_wd_obj], esi
		mov  eax, 2                               // type = addiction
		mov  edx, offset remove_jet_addict
		call fo::funcoffs::queue_clear_type_
		push 0x479FD1
		retn
	}
}

static void __declspec(naked) item_d_load_hack() {
	__asm {
		sub  esp, 4
		mov  [ebp], edi                           // edi->queue_drug
		mov  ecx, 7
		mov  esi, FO_VAR_drugInfoList + 12
loopDrug:
		cmp  dword ptr [esi+8], 0                 // drugInfoList.numeffects
		je   nextDrug
		mov  edx, esp
		mov  eax, [esi]                           // drugInfoList.pid
		call fo::funcoffs::proto_ptr_
		mov  edx, [esp]
		mov  eax, [edx+0x24]                      // drug.stat0
		cmp  eax, [edi+0x4]                       // drug.stat0 == queue_drug.stat0?
		jne  nextDrug                             // No
		mov  eax, [edx+0x28]                      // drug.stat1
		cmp  eax, [edi+0x8]                       // drug.stat1 == queue_drug.stat1?
		jne  nextDrug                             // No
		mov  eax, [edx+0x2C]                      // drug.stat2
		cmp  eax, [edi+0xC]                       // drug.stat2 == queue_drug.stat2?
		je   foundPid                             // Yes
nextDrug:
		add  esi, 12
		loop loopDrug
foundPid:
		jecxz end
		mov  eax, [esi]                           // drugInfoList.pid
		mov  [edi], eax                           // queue_drug.drug_pid
end:
		xor  eax, eax
		add  esp, 4
		retn
	}
}

static void __declspec(naked) queue_clear_type_mem_free_hook() {
	__asm {
		mov  ebx, [esi]
		jmp  fo::funcoffs::mem_free_
	}
}

static void __declspec(naked) partyMemberCopyLevelInfo_stat_level_hook() {
	__asm {
nextArmor:
		mov  eax, esi
		call fo::funcoffs::inven_worn_
		test eax, eax
		jz   noArmor
		and  byte ptr [eax+0x27], 0xFB            // Unset the flag of equipped armor
		jmp  nextArmor
noArmor:
		mov  eax, esi
		jmp  fo::funcoffs::stat_level_
	}
}

static void __declspec(naked) correctFidForRemovedItem_adjust_ac_hook() {
	__asm {
		call fo::funcoffs::adjust_ac_
nextArmor:
		mov  eax, esi
		call fo::funcoffs::inven_worn_
		test eax, eax
		jz   end
		and  byte ptr [eax+0x27], 0xFB            // Unset flag of equipped armor
		jmp  nextArmor
end:
		retn
	}
}

static void __declspec(naked) partyMemberCopyLevelInfo_hook() {
	__asm {
		push eax
		call fo::funcoffs::partyMemberCopyLevelInfo_
		pop  ebx
		cmp  eax, -1
		je   end
		pushad
		mov  dword ptr ds:[FO_VAR_critterClearObj], ebx
		mov  edx, fo::funcoffs::critterClearObjDrugs_
		call fo::funcoffs::queue_clear_type_
		mov  ecx, 8
		mov  edi, FO_VAR_drugInfoList
		mov  esi, ebx
loopAddict:
		mov  eax, dword ptr [edi]                 // eax = drug pid
		call fo::funcoffs::item_d_check_addict_
		test eax, eax                             // Has addiction?
		jz   noAddict                             // No
		cmp  dword ptr [eax], 0                   // queue_addict.init
		jne  noAddict                             // Addiction is not active yet
		mov  edx, dword ptr [eax+0x8]             // queue_addict.perk
		mov  eax, ebx
		call fo::funcoffs::perk_add_effect_
noAddict:
		add  edi, 12
		loop loopAddict
		popad
end:
		retn
	}
}


static void __declspec(naked) gdProcessUpdate_hack() {
	__asm {
		add  eax, esi
		cmp  eax, dword ptr ds:[FO_VAR_optionRect + 0xC]         // _optionRect.offy
		jge  skip
		add  eax, 2
		push 0x44702D
		retn
skip:
		push 0x4470DB
		retn
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
		pop  eax                                  // Destroy the return address
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
		pop  ebx                                  // Destroy the return address
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
		pop  eax                                  // Destroy the return address
		xor  eax, eax
		inc  eax
		mov  [esp+0xC], eax
		push 0x437B41
end:
		retn
	}
}

static void __declspec(naked) loot_container_hack() {
	__asm {
		mov  eax, [esp+0x114+0x4]
		test eax, eax
		jz   noLeftWeapon
		call fo::funcoffs::item_weight_
noLeftWeapon:
		mov  WeightOnBody, eax
		mov  eax, [esp+0x118+0x4]
		test eax, eax
		jz   noRightWeapon
		call fo::funcoffs::item_weight_
noRightWeapon:
		add  WeightOnBody, eax
		mov  eax, [esp+0x11C+0x4]
		test eax, eax
		jz   noArmor
		call fo::funcoffs::item_weight_
noArmor:
		add  WeightOnBody, eax
		xor  eax, eax
		inc  eax
		inc  eax
		retn
	}
}

static void __declspec(naked) barter_inventory_hack() {
	__asm {
		mov  eax, [esp+0x20+0x4]
		test eax, eax
		jz   noArmor
		call fo::funcoffs::item_weight_
noArmor:
		mov  WeightOnBody, eax
		mov  eax, [esp+0x1C+0x4]
		test eax, eax
		jnz  haveWeapon
		cmp  dword ptr ds:[FO_VAR_dialog_target_is_party], eax
		jne  end                                  // This is a party member
		mov  eax, [esp+0x18+0x4]
		test eax, eax
		jz   end
haveWeapon:
		call fo::funcoffs::item_weight_
		add  WeightOnBody, eax
end:
		mov  ebx, PID_JESSE_CONTAINER
		retn
	}
}

static void __declspec(naked) barter_attempt_transaction_hook() {
	__asm {
		call fo::funcoffs::stat_level_                          // eax = Max weight
		sub  eax, WeightOnBody                    // Accounting for weight of target's equipped armor and weapon
		retn
	}
}

static DWORD Looting = 0;
static void __declspec(naked) move_inventory_hook() {
	__asm {
		inc  Looting
		call fo::funcoffs::move_inventory_
		dec  Looting
		retn
	}
}

static void __declspec(naked) item_add_mult_hook() {
	__asm {
		call fo::funcoffs::stat_level_                          // eax = Max weight
		cmp  Looting, 0
		je   end
		sub  eax, WeightOnBody                    // Accounting for weight of target's equipped armor and weapon
end:
		retn
	}
}

static void __declspec(naked) inven_pickup_hack() {
	__asm {
		mov  edx, ds:[FO_VAR_pud]
		mov  edx, [edx]                           // itemsCount
		dec  edx
		sub  edx, eax
		lea  edx, ds:0[edx*8]
		push 0x470EC9
		retn
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
		jecxz skip
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
		jcxz skip                                 // inventory is empty (another excess check, but leave it)
		mov  edx, [edx+8]                         // FirstItem
nextItem:
		cmp  ebp, [edx]                           // Our weapon?
		je   foundItem                            // Yes
		add  edx, 8                               // Go to the next
		loop nextItem
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
		test eax, eax
		jz   end
		jmp  fo::funcoffs::display_print_
end:
		retn
	}
}

static void __declspec(naked) op_wield_obj_critter_adjust_ac_hook() {
	__asm {
		call fo::funcoffs::adjust_ac_
		xor  eax, eax                       // not animated
		jmp  fo::funcoffs::intface_update_ac_
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
		test [edi + flags + 1], 0x08; // target is multihex?
		jnz  multiHex;
		test [esi + flags + 1], 0x08; // source is multihex?
		jz   moveTile;
multiHex:
		mov  edx, [esp + 4];          // source goto tile
		cmp  [edi + tile], edx;       // target tile
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
		test [edi + flags + 1], 0x08; // target is multihex?
		jnz  multiHex;
		test [esi + flags + 1], 0x08; // source is multihex?
		jz   runTile;
multiHex:
		mov  edx, [esp + 4];          // source goto tile
		cmp  [edi + tile], edx;       // target tile
		jnz  runTile;
		add  esp, 4;
		jmp  ai_move_steps_closer_run_object_ret;
runTile:
		retn;
	}
}

static void __declspec(naked) MultiHexAIMissHitFix() {
	__asm {
		push ebx;                           // var: weaponRange
		call fo::funcoffs::tile_num_beyond_;
		mov  ebx, [esi];                    // source
		test [ebx + flags + 1], 0x08;       // is MultiHex?
		jz   skip;
		//
		push eax;
		mov  edx, [ebx + tile];             // source tile
		call fo::funcoffs::tile_dist_;
		cmp  eax, 2;
		pop  eax;
		jge  skip;
		mov  eax, [ebx + tile];             // source tile
		mov  edx, [ebx + rotation];
		mov  ebx, dword ptr [esp];          // distance weaponRange
		call fo::funcoffs::tile_num_in_direction_;  // return new tile for miss projectile
skip:
		add  esp, 4;
		retn;
	}
}

//checks if an attacked object is a critter before attempting dodge animation
static void __declspec(naked) action_melee_hack() {
	__asm {
		mov  edx, 0x4113DC
		mov  ebx, [eax + artFid]                  // objStruct->FID
		and  ebx, 0x0F000000
		sar  ebx, 0x18
		cmp  ebx, OBJ_TYPE_CRITTER                // check if object FID type flag is set to critter
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
		sar  ebx, 0x18
		cmp  ebx, OBJ_TYPE_CRITTER                // check if object FID type flag is set to critter
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
		test ah, DAM_KNOCKED_OUT                  // DAM_KNOCKED_OUT?
		jz   end                                  // No
		mov  eax, esi
		xor  edx, edx
		inc  edx                                  // type = knockout
		jmp  fo::funcoffs::queue_remove_this_     // Remove knockout from queue (if there is one)
end:
		pop  eax                                  // Destroy the return address
		push 0x424FC6
		retn
	}
}

static void __declspec(naked) critter_wake_clear_hack() {
	__asm {
		jne  end                                  // This is not a critter
		mov  dl, [esi + damageFlags]
		test dl, DAM_DEAD                         // DAM_DEAD?
		jnz  end                                  // This is a corpse
		and  dl, ~DAM_KNOCKED_OUT                 // 0xFE Unset DAM_KNOCKED_OUT
		or   dl, DAM_KNOCKED_DOWN                 // Set DAM_KNOCKED_DOWN
		mov  [esi + damageFlags], dl
end:
		xor  eax, eax
		inc  eax
		pop  esi
		pop  ecx
		pop  ebx
		retn
	}
}

static void __declspec(naked) obj_load_func_hack() {
	__asm {
		test byte ptr [eax+0x25], 0x4             // Temp_
		jnz  end
		mov  edi, [eax + protoId]
		shr  edi, 0x18
		cmp  edi, OBJ_TYPE_CRITTER
		jne  skip
		test byte ptr [eax + damageFlags], DAM_KNOCKED_DOWN
		jz   clear                                // No
		pushad
		xor  ecx, ecx
		inc  ecx
		xor  ebx, ebx
		xor  edx, edx
		xchg edx, eax
		call fo::funcoffs::queue_add_
		popad
clear:
		and  word ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKED_DOWN) // 0x7FFD
skip:
		push 0x488F14
		retn
end:
		push 0x488EF9
		retn
	}
}

static void __declspec(naked) partyMemberPrepLoadInstance_hook() {
	__asm {
		and  word ptr [eax + damageFlags], ~(DAM_LOSE_TURN or DAM_KNOCKED_DOWN) // 0x7FFD
		jmp  fo::funcoffs::dude_stand_
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
	using fo::ScriptProc::destroy_p_proc;
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
		inc  ecx
		mov  edx, [eax]                           // Inventory.inv_size
		sub  edx, ecx
		jge  end
		mov  edx, [eax]                           // Inventory.inv_size
end:
		retn
	}
}

static void __declspec(naked) Save_as_ASCII_hack() {
	__asm {
		mov  edx, STAT_sequence;
		mov  ebx, 626; // line index in EDITOR.MSG
		push 0x4396FC; // call stat_level_
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
		push 0x495FFC;
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

static void __declspec(naked) obj_move_to_tile_hack() {
	__asm {
		cmp  ds:[FO_VAR_map_state], 0;
		jz   map_leave;
		pop  eax;
		push 0x48A74E;
map_leave:
		mov  ebx, 16;
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
		// overwritted engine code
		add  esp, 0x34;
		pop  ebp;
		pop  edi;
		jmp  ComputeDamageRet;
	}
}

static int  currDescLen = 0;
static char textBuf[355];
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
		pushad;
		push esi;
		push eax;
		call AppendText;
		popad;
		retn;
skip:
		jmp  dword ptr [esp + 0x1AC - 0x14 + 4];
	}
}

static void __declspec(naked) obj_examine_func_hack_ammo1() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14 + 4], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		pushad;
		push 0;
		push eax;
		call AppendText;
		mov  currDescLen, 0;
		popad;
		lea  eax, [textBuf];
		jmp  fo::funcoffs::gdialogDisplayMsg_;
skip:
		jmp  dword ptr [esp + 0x1AC - 0x14 + 4];
	}
}

static void __declspec(naked) obj_examine_func_hack_weapon() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14], 0x445448; // gdialogDisplayMsg_
		jnz  skip;
		pushad;
		push esi;
		push eax;
		call AppendText;
		mov  eax, currDescLen;
		sub  eax, 2;
		mov  byte ptr textBuf[eax], 0; // cutoff last character
		mov  currDescLen, 0;
		popad;
		lea  eax, [textBuf];
skip:
		mov  ecx, 0x49B63C;
		jmp  ecx;
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
		mov eax, 0x4745E3;
		jmp eax;
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
		mov eax, [esp + 0x28 - 0x28 + 4];  // target
		mov edx, [esp + 0x28 - 0x24 + 4];  // source
		retn;
	}
}

static void __declspec(naked) ai_best_weapon_hook() {
	__asm {
		mov eax, [esp + 0xF4 - 0x10 + 4]; // prev.item
		jmp fo::funcoffs::item_w_perk_;
	}
}

void BugFixes::init()
{
	#ifndef NDEBUG
		if (isDebug && (GetConfigInt("Debugging", "BugFixes", 1) == 0)) return;
	#endif

	//if (GetConfigInt("Misc", "SharpshooterFix", 1)) {
		dlog("Applying Sharpshooter patch.", DL_INIT);
		// http://www.nma-fallout.com/threads/fo2-engine-tweaks-sfall.178390/page-119#post-4050162
		// by Slider2k
		HookCall(0x4244AB, &SharpShooterFix); // hooks stat_level_() call in detemine_to_hit_func_()
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
		HookCall(0x4A596A, &scr_write_ScriptNode_hook);
		HookCall(0x4A59C1, &scr_write_ScriptNode_hook);
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
	MakeCall(0x47A243, item_d_load_hack);
	dlogr(" Done", DL_INIT);

	// Fix crash when leaving the map while waiting for someone to die of a super stimpak overdose
	dlog("Applying fix for \"leaving the map while waiting for a super stimpak overdose death\" crash.", DL_INIT);
	HookCall(0x4A27E7, &queue_clear_type_mem_free_hook); // hooks mem_free_()
	dlogr(" Done", DL_INIT);

	// Evil bug! If party member has the same armor type in inventory as currently equipped, then
	// on level up he loses Armor Class equal to the one received from this armor.
	// The same happens if you just order NPC to remove the armor through dialogue.
	//if (GetConfigInt("Misc", "ArmorCorruptsNPCStatsFix", 1)) {
		dlog("Applying fix for armor reducing NPC original stats when removed.", DL_INIT);
		HookCall(0x495F3B, &partyMemberCopyLevelInfo_stat_level_hook);
		HookCall(0x45419B, &correctFidForRemovedItem_adjust_ac_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix of invalid stats when party member gains a level while being on drugs
	dlog("Applying fix for addicted party member level up bug.", DL_INIT);
	HookCall(0x495D5C, &partyMemberCopyLevelInfo_hook);
	dlogr(" Done", DL_INIT);

	// Allow 9 options (lines of text) to be displayed correctly in a dialog window
	if (GetConfigInt("Misc", "DialogOptions9Lines", 1)) {
		dlog("Applying 9 dialog options patch.", DL_INIT);
		MakeJump(0x44701C, gdProcessUpdate_hack);
		dlogr(" Done", DL_INIT);
	}

	// Fix for "Unlimited Ammo" exploit
	dlog("Applying fix for Unlimited Ammo exploit.", DL_INIT);
	HookCall(0x472957, &invenWieldFunc_item_get_type_hook); // hooks item_get_type_()
	dlogr(" Done", DL_INIT);

	// Fix for negative values in Skilldex window ("S")
	dlog("Applying fix for negative values in Skilldex window.", DL_INIT);
	SafeWrite8(0x4AC377, 0x7F);                // jg
	dlogr(" Done", DL_INIT);

	// Fix for negative SPECIAL values in character creation
	dlog("Applying fix for negative SPECIAL values in character creation.", DL_INIT);
	MakeCall(0x43DF6F, is_supper_bonus_hack);
	MakeCall(0x434BFF, PrintBasicStat_hack);
	HookCall(0x437AB4, &StatButtonUp_hook);
	HookCall(0x437B26, &StatButtonDown_hook);
	dlogr(" Done", DL_INIT);

	// Fix for not counting in the weight of equipped items on NPC when stealing or bartering
	//if (GetConfigInt("Misc", "NPCWeightFix", 1)) {
		dlog("Applying fix for not counting in weight of equipped items on NPC.", DL_INIT);
		MakeCall(0x473B4E, loot_container_hack);
		MakeCall(0x47588A, barter_inventory_hack);
		HookCall(0x474CB8, &barter_attempt_transaction_hook);
		HookCall(0x4742AD, &move_inventory_hook);
		HookCall(0x4771B5, &item_add_mult_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Corrects "Weight of items" text element width to be 64 (and not 80), which matches container element width
	SafeWrite8(0x475541, 64);
	SafeWrite8(0x475789, 64);

	//if (GetConfigInt("Misc", "InventoryDragIssuesFix", 1)) {
		dlog("Applying inventory reverse order issues fix.", DL_INIT);
		// Fix for minor visual glitch when picking up solo item from the top of inventory
		// and there is multiple item stack at the bottom of inventory
		MakeJump(0x470EC2, inven_pickup_hack);
		// Fix for error in player's inventory, related to IFACE_BAR_MODE=1 in f2_res.ini, and
		// also for reverse order error
		MakeJump(0x47114A, inven_pickup_hack2);
		// Fix for using only one box of ammo when a weapon is above the ammo in the inventory list
		HookCall(0x476598, &drop_ammo_into_weapon_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "NPCLevelFix", 1)) {
		dlog("Applying NPC level fix.", DL_INIT);
		HookCall(0x495BC9, (void*)0x495E51);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "BlackSkilldexFix", 1)) {
		dlog("Applying black Skilldex patch.", DL_INIT);
		HookCall(0x497D0F, &PipStatus_AddHotLines_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "FixWithdrawalPerkDescCrash", 1)) {
		dlog("Applying withdrawal perk description crash fix.", DL_INIT);
		HookCall(0x47A501, &perform_withdrawal_start_display_print_hook);
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

	//if (GetConfigInt("Misc", "WieldObjCritterFix", 1)) {
		dlog("Applying wield_obj_critter fix.", DL_INIT);
		SafeWrite8(0x456912, 0x1E);
		HookCall(0x45697F, &op_wield_obj_critter_adjust_ac_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetConfigInt("Misc", "MultiHexPathingFix", 0)) {
		dlog("Applying MultiHex Pathing Fix.", DL_INIT);
		MakeCalls(MultiHexFix, { 0x42901F, 0x429170 });
		// Fix for multihex critters moving too close and overlapping their targets in combat
		MakeCall(0x42A14F, MultiHexCombatRunFix, 1);
		MakeCall(0x42A178, MultiHexCombatMoveFix, 1);
		dlogr(" Done", DL_INIT);
	//}

	// Fix the impact in itself in case of a miss hit for multihex critters when using throwing weapon
	// Note: in fact, the bug is in tile_num_beyond_ and related functions, in case of fix, this crutch will need to be removed
	if (GetConfigInt("Misc", "MultiHexSelfHitFix", 0) != 0) {
		dlog("Applying multihex critter miss hit fix.", DL_INIT);
		HookCalls(MultiHexAIMissHitFix, { 0x423B44, 0x42315D });
		dlogr(" Done", DL_INIT);
	}

	//if (GetConfigInt("Misc", "DodgyDoorsFix", 1)) {
		dlog("Applying Dodgy Door Fix.", DL_INIT);
		MakeJump(0x4113D6, action_melee_hack);
		MakeJump(0x411BCC, action_ranged_hack);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for "NPC turns into a container" bug
	//if (GetConfigInt("Misc", "NPCTurnsIntoContainerFix", 1)) {
		dlog("Applying fix for \"NPC turns into a container\" bug.", DL_INIT);
		MakeCall(0x424F8E, set_new_results_hack);
		MakeJump(0x42E46E, critter_wake_clear_hack);
		MakeJump(0x488EF3, obj_load_func_hack);
		HookCall(0x4949B2, &partyMemberPrepLoadInstance_hook);
		dlogr(" Done", DL_INIT);
	//}

	dlog("Applying fix for explosives bugs.", DL_INIT);
	// Fix crashes when killing critters with explosives
	MakeJump(0x422F05, combat_ctd_init_hack);
	MakeJump(0x489413, obj_save_hack);
	// Fix for destroy_p_proc not being called if the critter is killed by explosives when you leave the map
	MakeCall(0x4130C3, action_explode_hack);
	MakeCall(0x4130E5, action_explode_hack1);
	dlogr(" Done", DL_INIT);

	// Fix for unable to sell used geiger counters or stealth boys
	if (GetConfigInt("Misc", "CanSellUsedGeiger", 1)) {
		dlog("Applying fix for unable to sell used geiger counters or stealth boys.", DL_INIT);
		SafeWrite8(0x478115, 0xBA);
		SafeWrite8(0x478138, 0xBA);
		MakeJump(0x474D22, barter_attempt_transaction_hack);
		HookCall(0x4798B1, &item_m_turn_off_hook);
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
		HookCall(0x448250, &gdActivateBarter_hook);
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
		MakeCall(0x471B7F, inven_item_wearing); // inven_right_hand_
		SafeWrite8(0x471B84, 0x90); // nop
		MakeCall(0x471BCB, inven_item_wearing); // inven_left_hand_
		SafeWrite8(0x471BD0, 0x90); // nop
		MakeCall(0x471C17, inven_item_wearing); // inven_worn_
		SafeWrite8(0x471C1C, 0x90); // nop
		// Fix crash when trying to open bag/backpack on the table in the bartering interface
		MakeCall(0x473191, inven_action_cursor_hack);
		dlogr(" Done", DL_INIT);
	//}

	// Fix crash when clicking on empty space in the inventory list opened by "Use Inventory Item On" (backpack) action icon
	MakeCall(0x471A94, use_inventory_on_hack);

	// Fix for Sequence stat value not being printed correctly when using "print to file" option
	MakeJump(0x4396F5, Save_as_ASCII_hack);

	// Fix for Bonus Move APs being replenished when you save and load the game in combat
	//if (GetConfigInt("Misc", "BonusMoveFix", 1)) {
		dlog("Applying fix for Bonus Move exploit.", DL_INIT);
		HookCall(0x420E93, &combat_load_hook);
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
		HookCall(0x423A99, &compute_attack_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix crash when calling partyMemberGetCurLevel_ on a critter that has no data in party.txt
	MakeJump(0x495FF6, partyMemberGetCurLevel_hack);

	// Fix for player's base EMP DR not being properly initialized when creating a new character and then starting the game
	HookCall(0x4A22DF, &ResetPlayer_hook);

	// Fix for add_mult_objs_to_inven only adding 500 of an object when the value of "count" argument is over 99999
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

	// Fix missing AC/DR mod stats when examining ammo in barter screen
	dlog("Applying fix for displaying ammo stats in barter screen.", DL_INIT);
	MakeCalls(obj_examine_func_hack_ammo0, {0x49B4AD, 0x49B504});
	SafeWrite16(0x49B4B2, 0x9090);
	SafeWrite16(0x49B509, 0x9090);
	MakeCall(0x49B563, obj_examine_func_hack_ammo1, 2);
	dlogr(" Done", DL_INIT);

	// Display full item description for weapon/ammo in barter screen
	showItemDescription = (GetConfigInt("Misc", "FullItemDescInBarter", 0) != 0);
	if (showItemDescription) {
		dlog("Applying full item description in barter patch.", DL_INIT);
		HookCall(0x49B452, obj_examine_func_hack_weapon); // it's jump
		dlogr(" Done", DL_INIT);
	}

	// Display experience points with the bonus from Swift Learner perk when gained from non-scripted situations
	if (GetConfigInt("Misc", "DisplaySwiftLearnerExp", 1) != 0) {
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
	if (GetConfigInt("Misc", "ObjCanHearObjFix", 0) != 0) {
		dlog("Applying obj_can_hear_obj fix.", DL_INIT);
		SafeWrite8(0x4583D8, 0x3B); // jz loc_458414
		SafeWrite8(0x4583DE, 0x74); // jz loc_458414
		MakeCall(0x4583E0, op_obj_can_hear_obj_hack);
		SafeWrite8(0x4583E5, 0x90);
		dlogr(" Done", DL_INIT);
	}

	// Fix: The wrong item was passed to the function to check the presence of perk at the weapon
	int bestWeaponPerkMod = GetConfigInt("Misc", "AIBestWeaponPerkFix", 0);
	if (bestWeaponPerkMod > 0) {
		dlog("Applying AI best weapon choose fix.", DL_INIT);
		HookCall(0x42954B, ai_best_weapon_hook);
		// also corrected calculate weapon perk modifier: multiply by 2 + modificator (default modifier should be set to 5)
		if (bestWeaponPerkMod > 1) {
			if (bestWeaponPerkMod > 100) bestWeaponPerkMod = 100;
			SafeWriteBatch<DWORD>(bestWeaponPerkMod, {0x42955F, 0x4296E8});
			SafeWrite16(0x42955C, 0xD201);  // add edx, edx
			SafeWrite8(0x42955E,  0xB8);    // mov eax, imm
			SafeWrite16(0x4296E5, 0xD201);  // add edx, edx
			SafeWrite8(0x4296E7,  0xB8);    // mov eax, imm
		}
		dlogr(" Done", DL_INIT);
	}
}

}
