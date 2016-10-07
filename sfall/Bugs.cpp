#include "main.h"

#include "Bugs.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "ScriptExtender.h"

DWORD WeightOnBody = 0;

static void __declspec(naked) SharpShooterFix() {
	__asm {
		call stat_level_                          // Perception
		cmp  edi, dword ptr ds:[_obj_dude]
		jne  end
		xchg ecx, eax
		mov  eax, edi                             // _obj_dude
		mov  edx, PERK_sharpshooter
		call perk_level_
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
		cmp  byte ptr ds:[_holo_flag], 0
		jne  end
		xor  ebx, ebx                             // No man, no problem (c) :-p
end:
		mov  eax, ds:[_crnt_func]
		retn
	}
}

static void __declspec(naked) PipAlarm_hack() {
	__asm {
		mov  ds:[_crnt_func], eax
		mov  eax, 0x400
		call PipStatus_
		mov  eax, 0x50CC04                        // 'iisxxxx1'
		retn
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
		call scr_write_ScriptNode_
		xchg dword ptr [ebp+0xE04], ecx           // NextBlock
		pop  dword ptr [ebp+0xE00]                // num
		retn
	}
}

static void __declspec(naked) protinst_default_use_item_hack() {
	__asm {
		mov  eax, dword ptr [edx+0x64]            // eax = target pid
		cmp  eax, PID_DRIVABLE_CAR
		je   isCar
		cmp  eax, PID_CAR_TRUNK
		jne  notCar
isCar:
		mov  eax, ebx
		call obj_use_power_on_car_
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
		call queue_find_first_
loopQueue:
		test eax, eax                             // Has something in the list?
		jz   end                                  // No
		cmp  ebx, dword ptr [eax+0x4]             // drug_pid == queue_addict.drug_pid?
		je   end                                  // Has specific addiction
		mov  eax, esi                             // eax = who
		mov  edx, 2                               // type = addiction
		call queue_find_next_
		jmp  loopQueue
skip:
		mov  eax, dword ptr ds:[_obj_dude]
		call queue_find_first_
end:
		push 0x47A6A1
		retn
	}
}

static void __declspec(naked) remove_jet_addict() {
	__asm {
		cmp  eax, dword ptr ds:[_wd_obj]
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
		call perform_withdrawal_end_
skip:
		mov  dword ptr ds:[_wd_obj], esi
		mov  eax, 2                               // type = addiction
		mov  edx, offset remove_jet_addict
		call queue_clear_type_
		push 0x479FD1
		retn
	}
}

static void __declspec(naked) item_d_load_hack() {
	__asm {
		sub  esp, 4
		mov  [ebp], edi                           // edi->queue_drug
		mov  ecx, 7
		mov  esi, _drugInfoList+12
loopDrug:
		cmp  dword ptr [esi+8], 0                 // drugInfoList.numeffects
		je   nextDrug
		mov  edx, esp
		mov  eax, [esi]                           // drugInfoList.pid
		call proto_ptr_
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
		jmp  mem_free_
	}
}

static void __declspec(naked) partyMemberCopyLevelInfo_stat_level_hook() {
	__asm {
nextArmor:
		mov  eax, esi
		call inven_worn_
		test eax, eax
		jz   noArmor
		and  byte ptr [eax+0x27], 0xFB            // Unset the flag of equipped armor
		jmp  nextArmor
noArmor:
		mov  eax, esi
		jmp  stat_level_
	}
}

static void __declspec(naked) correctFidForRemovedItem_adjust_ac_hook() {
	__asm {
		call adjust_ac_
nextArmor:
		mov  eax, esi
		call inven_worn_
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
		call partyMemberCopyLevelInfo_
		pop  ebx
		cmp  eax, -1
		je   end
		pushad
		mov  dword ptr ds:[_critterClearObj], ebx
		mov  edx, critterClearObjDrugs_
		call queue_clear_type_
		mov  ecx, 8
		mov  edi, _drugInfoList
		mov  esi, ebx
loopAddict:
		mov  eax, dword ptr [edi]                 // eax = drug pid
		call item_d_check_addict_
		test eax, eax                             // Has addiction?
		jz   noAddict                             // No
		cmp  dword ptr [eax], 0                   // queue_addict.init
		jne  noAddict                             // Addiction is not active yet
		mov  edx, dword ptr [eax+0x8]             // queue_addict.perk
		mov  eax, ebx
		call perk_add_effect_
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
		cmp  eax, dword ptr ds:[_optionRect + 0xC]         // _optionRect.offy
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
		call item_remove_mult_
		xchg ebx, eax
		mov  eax, esi
		test cl, 0x2                              // Right hand?
		jz   leftHand                             // No
		call inven_right_hand_
		jmp  removeFlag
leftHand:
		call inven_left_hand_
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
		call item_add_force_
skip:
		mov  eax, edi
		jmp  item_get_type_
	}
}

static void __declspec(naked) loot_container_hack() {
	__asm {
		mov  eax, [esp+0x114+0x4]
		test eax, eax
		jz   noLeftWeapon
		call item_weight_
noLeftWeapon:
		mov  WeightOnBody, eax
		mov  eax, [esp+0x118+0x4]
		test eax, eax
		jz   noRightWeapon
		call item_weight_
noRightWeapon:
		add  WeightOnBody, eax
		mov  eax, [esp+0x11C+0x4]
		test eax, eax
		jz   noArmor
		call item_weight_
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
		call item_weight_
noArmor:
		mov  WeightOnBody, eax
		mov  eax, [esp+0x1C+0x4]
		test eax, eax
		jnz  haveWeapon
		cmp  dword ptr ds:[_dialog_target_is_party], eax
		jne  end                                  // This is a party member
		mov  eax, [esp+0x18+0x4]
		test eax, eax
		jz   end
haveWeapon:
		call item_weight_
		add  WeightOnBody, eax
end:
		mov  ebx, PID_JESSE_CONTAINER
		retn
	}
}

static void __declspec(naked) barter_attempt_transaction_hook() {
	__asm {
		call stat_level_                          // eax = Max weight
		sub  eax, WeightOnBody                    // Accounting for weight of target's equipped armor and weapon
		retn
	}
}

static DWORD Looting = 0;
static void __declspec(naked) move_inventory_hook() {
	__asm {
		inc  Looting
		call move_inventory_
		dec  Looting
		retn
	}
}

static void __declspec(naked) item_add_mult_hook() {
	__asm {
		call stat_level_                          // eax = Max weight
		cmp  Looting, 0
		je   end
		sub  eax, WeightOnBody                    // Accounting for weight of target's equipped armor and weapon
end:
		retn
	}
}

static void __declspec(naked) inven_pickup_hack() {
	__asm {
		mov  edx, ds:[_pud]
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
		mov  eax, ds:[_i_wid]
		call GNW_find_
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
		call mouse_click_in_
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
		mov  eax, ds:[_pud]
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
		lea  edx, [eax+0x2C]                      // Inventory
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
		call item_remove_mult_
		test eax, eax                             // Have weapon been deleted from inventory?
		jnz  end                                  // No
		sub  [esp+0x24+4], esi                    // Yes, correct from_slot
end:
		retn
	}
}

static void __declspec(naked) PipStatus_AddHotLines_hook() {
	__asm {
		call AddHotLines_
		xor  eax, eax
		mov  dword ptr ds:[_hot_line_count], eax
		retn
	}
}

static void __declspec(naked) perform_withdrawal_start_display_print_hook() {
	__asm {
		test eax, eax
		jz   end
		jmp  display_print_
end:
		retn
	}
}

static void __declspec(naked) item_d_take_drug_hack1() {
	__asm {
		push 0x47A168
		retn
	}
}

static void __declspec(naked) op_wield_obj_critter_adjust_ac_hook() {
	__asm {
		call adjust_ac_
		xor  eax, eax                       // not animated
		jmp  intface_update_ac_
	}
}

// Haenlomal
static void __declspec(naked) MultiHexFix() {
	__asm {
		xor  ecx, ecx;                      // argument value for make_path_func: ecx=0 (unknown arg)
		test byte ptr ds:[ebx+0x25], 0x08;  // is target multihex?
		mov  ebx, dword ptr ds:[ebx+0x4];   // argument value for make_path_func: target's tilenum (end_tile)
		je   end;                           // skip if not multihex
		inc  ebx;                           // otherwise, increase tilenum by 1
end:
		retn;                               // call make_path_func (at 0x429024, 0x429175)
	}
}

//checks if an attacked object is a critter before attempting dodge animation
static void __declspec(naked) action_melee_hack() {
	__asm {
		mov  edx, 0x4113DC
		mov  ebx, [eax+0x20]                      // objStruct->FID
		and  ebx, 0x0F000000
		sar  ebx, 0x18
		cmp  ebx, OBJ_TYPE_CRITTER                // check if object FID type flag is set to critter
		jne  end                                  // if object not a critter leave jump condition flags
		// set to skip dodge animation
		test byte ptr [eax+0x44], 0x3             // (original code) DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN
		jnz  end
		mov  edx, 0x4113FE
end:
		jmp  edx
	}
}

static void __declspec(naked) action_ranged_hack() {
	__asm {
		mov  edx, 0x411B6D
		mov  ebx, [eax+0x20]                      // objStruct->FID
		and  ebx, 0x0F000000
		sar  ebx, 0x18
		cmp  ebx, OBJ_TYPE_CRITTER                // check if object FID type flag is set to critter
		jne  end                                  // if object not a critter leave jump condition flags
		// set to skip dodge animation
		test byte ptr [eax+0x44], 0x3             // (original code) DAM_KNOCKED_OUT or DAM_KNOCKED_DOWN
		jnz  end
		mov  edx, 0x411BD2
end:
		jmp  edx
	}
}

static void __declspec(naked) set_new_results_hack() {
	__asm {
		test ah, 0x1                              // DAM_KNOCKED_OUT?
		jz   end                                  // No
		mov  eax, esi
		xor  edx, edx
		inc  edx                                  // type = knockout
		jmp  queue_remove_this_                   // Remove knockout from queue (if there is one)
end:
		pop  eax                                  // Destroying return address
		push 0x424FC6
		retn
	}
}

static void __declspec(naked) critter_wake_clear_hack() {
	__asm {
		jne  end                                  // This is not a critter
		mov  dl, [esi+0x44]
		test dl, 0x80                             // DAM_DEAD?
		jnz  end                                  // This is a corpse
		and  dl, 0xFE                             // Unset DAM_KNOCKED_OUT
		or   dl, 0x2                              // Set DAM_KNOCKED_DOWN
		mov  [esi+0x44], dl
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
		mov  edi, [eax+0x64]
		shr  edi, 0x18
		cmp  edi, OBJ_TYPE_CRITTER
		jne  skip
		test byte ptr [eax+0x44], 0x2             // DAM_KNOCKED_DOWN?
		jz   clear                                // No
		pushad
		xor  ecx, ecx
		inc  ecx
		xor  ebx, ebx
		xor  edx, edx
		xchg edx, eax
		call queue_add_
		popad
clear:
		and  word ptr [eax+0x44], 0x7FFD          // not (DAM_LOSE_TURN or DAM_KNOCKED_DOWN)
skip:
		push 0x488F14
		retn
end:
		push 0x488EF9
		retn
	}
}

static void __declspec(naked) partyMemberPrepLoadInstance_hack() {
	__asm {
		and  word ptr [eax+0x44], 0x7FFD          // not (DAM_LOSE_TURN or DAM_KNOCKED_DOWN)
		jmp  dude_stand_
	}
}

static void __declspec(naked) combat_ctd_init_hack() {
	__asm {
		mov  [esi+0x24], eax                      // ctd.targetTile
		mov  eax, [ebx+0x54]                      // pobj.who_hit_me
		inc  eax
		jnz  end
		mov  [ebx+0x54], eax                      // pobj.who_hit_me
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
		mov  eax, [eax+0x68]                      // pobj.who_hit_me.cid
		test byte ptr ds:[_combat_state], 1       // in combat?
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
	__asm {
		mov  edx, destroy_p_proc
		mov  eax, [esi+0x78]                      // pobj.sid
		call exec_script_proc_
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
		cmp  dword ptr [eax+0x64], PID_ACTIVE_GEIGER_COUNTER
		je   found
		cmp  dword ptr [eax+0x64], PID_ACTIVE_STEALTH_BOY
		je   found
		mov  eax, 0x474D34
		jmp  eax
found:
		call item_m_turn_off_
		mov  eax, 0x474D17
		jmp  eax                                  // Is there any other activated items among the ones being sold?
	}
}

static void __declspec(naked) item_m_turn_off_hook() {
	__asm {
		and  byte ptr [eax+0x25], 0xDF            // Rest flag of used items
		jmp  queue_remove_this_
	}
}

static void __declspec(naked) combat_hack() {
	__asm {
		mov  eax, [ecx+eax]                       // eax = source
		test eax, eax
		jz   end
		push eax
		mov  edx, STAT_max_move_points
		call stat_level_
		mov  edx, ds:[_gcsd]
		test edx, edx
		jz   skip
		add  eax, [edx+0x8]                       // gcsd.free_move
skip:
		pop  edx
		xchg edx, eax                             // eax = source, edx = Max action points
		mov  [eax+0x40], edx                      // pobj.curr_mp
		test byte ptr ds:[_combat_state], 1       // in combat?
		jz   end                                  // No
		mov  edx, [eax+0x68]                      // pobj.cid
		cmp  edx, -1
		je   end
		push eax
		mov  eax, ds:[_aiInfoList]
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
		cmp  ebx, ds:[_WorldMapCurrArea]
		je   end
		mov  ds:[_WorldMapCurrArea], ebx
		sub  eax, edx
		add  eax, ds:[_wmAreaInfoList]
		mov  edx, [eax+0x30]                      // wmAreaInfoList.world_posy
		mov  ds:[_world_ypos], edx
		mov  edx, [eax+0x2C]                      // wmAreaInfoList.world_posx
		mov  ds:[_world_xpos], edx
end:
		xor  eax, eax
		mov  ds:[_target_xpos], eax
		mov  ds:[_target_ypos], eax
		mov  ds:[_In_WorldMap], eax
		push 0x4C5A77
		retn
	}
}


void BugsInit()
{
	//if (GetPrivateProfileIntA("Misc", "SharpshooterFix", 1, ini)) {
		dlog("Applying sharpshooter patch.", DL_INIT);
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
	dlog("Applying fix for Pip-Boy rest exploit.", DL_INIT);
	MakeCall(0x4971C7, &pipboy_hack, false);
	MakeCall(0x499530, &PipAlarm_hack, false);
	dlogr(" Done", DL_INIT);

	// Fix for "Too Many Items" bug
	//if (GetPrivateProfileIntA("Misc", "TooManyItemsBugFix", 1, ini)) {
		dlog("Applying preventive patch for \"Too Many Items\" bug.", DL_INIT);
		HookCall(0x4A596A, &scr_write_ScriptNode_hook);
		HookCall(0x4A59C1, &scr_write_ScriptNode_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for cells getting consumed even when the car is already fully charged
	MakeCall(0x49BE70, &obj_use_power_on_car_hack, false);

	// Fix for being able to charge the car by using cells on other scenary/critters
	if (GetPrivateProfileIntA("Misc", "CarChargingFix", 1, ini)) {
		dlog("Applying car charging fix.", DL_INIT);
		MakeCall(0x49C36D, &protinst_default_use_item_hack, true);
		dlogr(" Done", DL_INIT);
	}

	// Fix for gaining stats from more than two doses of a specific chem after save-load
	dlog("Applying fix for save-load unlimited drug use exploit.", DL_INIT);
	MakeCall(0x47A243, &item_d_load_hack, false);
	dlogr(" Done", DL_INIT);

	// Fix crash when leaving the map while waiting for someone to die of a super stimpak overdose
	dlog("Applying fix for \"leaving the map while waiting for a super stimpak overdose death\" crash.", DL_INIT);
	HookCall(0x4A27E7, &queue_clear_type_mem_free_hook); // hooks mem_free_()
	dlogr(" Done", DL_INIT);

	// Evil bug! If party member has the same armor type in inventory as currently equipped, then
	// on level up he loses Armor Class equal to the one received from this armor.
	// The same happens if you just order NPC to remove the armor through dialogue.
	//if (GetPrivateProfileIntA("Misc", "ArmorCorruptsNPCStatsFix", 1, ini)) {
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
	if (GetPrivateProfileIntA("Misc", "DialogOptions9Lines", 1, ini)) {
		dlog("Applying 9 dialog options patch.", DL_INIT);
		MakeCall(0x44701C, &gdProcessUpdate_hack, true);
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

	// Fix for not counting in the weight of equipped items on NPC when stealing or bartering
	//if (GetPrivateProfileIntA("Misc", "NPCWeightFix", 1, ini)) {
		dlog("Applying fix for not counting in weight of equipped items on NPC.", DL_INIT);
		MakeCall(0x473B4E, &loot_container_hack, false);
		MakeCall(0x47588A, &barter_inventory_hack, false);
		HookCall(0x474CB8, &barter_attempt_transaction_hook);
		HookCall(0x4742AD, &move_inventory_hook);
		HookCall(0x4771B5, &item_add_mult_hook);
		dlogr(" Done", DL_INIT);
	//}

	// Corrects "Weight of items" text element width to be 64 (and not 80), which matches container element width
	SafeWrite8(0x475541, 64);
	SafeWrite8(0x475789, 64);

	//if (GetPrivateProfileIntA("Misc", "InventoryDragIssuesFix", 1, ini)) {
		dlog("Applying inventory reverse order issues fix.", DL_INIT);
		// Fix for minor visual glitch when picking up solo item from the top of inventory
		// and there is multiple item stack at the bottom of inventory
		MakeCall(0x470EC2, &inven_pickup_hack, true);
		// Fix for error in player's inventory, related to IFACE_BAR_MODE=1 in f2_res.ini, and
		// also for reverse order error
		MakeCall(0x47114A, &inven_pickup_hack2, true);
		// Fix for using only one box of ammo when a weapon is above the ammo in the inventory list
		HookCall(0x476598, &drop_ammo_into_weapon_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "NPCLevelFix", 1, ini)) {
		dlog("Applying NPC level fix.", DL_INIT);
		HookCall(0x495BC9, (void*)0x495E51);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "BlackSkilldexFix", 1, ini)) {
		dlog("Applying black skilldex patch.", DL_INIT);
		HookCall(0x497D0F, &PipStatus_AddHotLines_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "FixWithdrawalPerkDescCrash", 1, ini)) {
		dlog("Applying withdrawal perk description crash fix.", DL_INIT);
		HookCall(0x47A501, &perform_withdrawal_start_display_print_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "JetAntidoteFix", 1, ini)) {
		dlog("Applying Jet Antidote fix.", DL_INIT);
		// the original jet antidote fix
		MakeCall(0x47A013, &item_d_take_drug_hack1, true);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "NPCDrugAddictionFix", 1, ini)) {
		dlog("Applying NPC's drug addiction fix.", DL_INIT);
		// proper checks for NPC's addiction instead of always using global vars
		MakeCall(0x47A644, &item_d_check_addict_hack, true);
		MakeCall(0x479FC5, &item_d_take_drug_hack, true);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileInt("Misc", "ShivPatch", 1, ini)) {
		dlog("Applying shiv patch.", DL_INIT);
		SafeWrite8(0x477B2B, 0xEB);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileInt("Misc", "ImportedProcedureFix", 0, ini)) {
		dlog("Applying imported procedure patch.", DL_INIT);
		// http://teamx.ru/site_arc/smf/index.php-topic=398.0.htm
		SafeWrite16(0x46B35B, 0x1C60); // Fix problems with the temporary stack
		SafeWrite32(0x46B35D, 0x90909090);
		SafeWrite8(0x46DBF1, 0xEB); // Disable warnings
		SafeWrite8(0x46DDC4, 0xEB); // Disable warnings
		SafeWrite8(0x4415CC, 0x00); // Prevent crashes when re-exporting
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "WieldObjCritterFix", 1, ini)) {
		dlog("Applying wield_obj_critter fix.", DL_INIT);
		SafeWrite8(0x456912, 0x1E);
		HookCall(0x45697F, &op_wield_obj_critter_adjust_ac_hook);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "MultiHexPathingFix", 1, ini)) {
		dlog("Applying MultiHex Pathing Fix.", DL_INIT);
		MakeCall(0x42901F, &MultiHexFix, false);
		MakeCall(0x429170, &MultiHexFix, false);
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "DodgyDoorsFix", 1, ini)) {
		dlog("Applying Dodgy Door Fix.", DL_INIT);
		MakeCall(0x4113D6, &action_melee_hack, true);
		MakeCall(0x411BCC, &action_ranged_hack, true);
		dlogr(" Done", DL_INIT);
	//}

	// Fix for "NPC turns into a container" bug
	//if (GetPrivateProfileIntA("Misc", "NPCTurnsIntoContainerFix", 1, ini)) {
		dlog("Applying fix for \"NPC turns into a container\" bug.", DL_INIT);
		MakeCall(0x424F8E, &set_new_results_hack, false);
		MakeCall(0x42E46E, &critter_wake_clear_hack, true);
		MakeCall(0x488EF3, &obj_load_func_hack, true);
		HookCall(0x4949B2, &partyMemberPrepLoadInstance_hack);
		dlogr(" Done", DL_INIT);
	//}

	dlog("Applying fix for explosives bugs.", DL_INIT);
	// Fix crashes when killing critters with explosives
	MakeCall(0x422F05, &combat_ctd_init_hack, true);
	MakeCall(0x489413, &obj_save_hack, true);
	// Fix for destroy_p_proc not being called if the critter is killed by explosives when you leave the map
	MakeCall(0x4130C3, &action_explode_hack, false);
	MakeCall(0x4130E5, &action_explode_hack1, false);
	dlogr(" Done", DL_INIT);

	// Fix for unable to sell used geiger counters or stealth boys
	if (GetPrivateProfileIntA("Misc", "CanSellUsedGeiger", 1, ini)) {
		dlog("Applying fix for unable to sell used geiger counters or stealth boys.", DL_INIT);
		SafeWrite8(0x478115, 0xBA);
		SafeWrite8(0x478138, 0xBA);
		MakeCall(0x474D22, &barter_attempt_transaction_hack, true);
		HookCall(0x4798B1, &item_m_turn_off_hook);
		dlogr(" Done", DL_INIT);
	}

	// Fix for incorrect initialization of action points at the beginning of each turn
	dlog("Applying Action Points initialization fix.", DL_INIT);
	BlockCall(0x422E02);
	MakeCall(0x422E1B, &combat_hack, false);
	dlogr(" Done", DL_INIT);

	// Fix for incorrect death animations being used when killing critters with kill_critter_type function
	dlog("Applying kill_critter_type fix.", DL_INIT);
	SafeWrite16(0x457E22, 0xDB31); // xor ebx, ebx
	SafeWrite32(0x457C99, 0x30BE0075); // jnz loc_457C9B; mov esi, 48
	dlogr(" Done", DL_INIT);

	// Fix for checking the horizontal position on the y-axis instead of x when setting coordinates on the world map
	SafeWrite8(0x4C4743, 0xC6); // cmp esi, eax

	// Partial fix for incorrect positioning after exiting small locations (e.g. Ghost Farm)
	//if (GetPrivateProfileIntA("Misc", "SmallLocExitFix", 1, ini)) {
		dlog("Applying fix for incorrect positioning after exiting small locations.", DL_INIT);
		MakeCall(0x4C5A41, &wmTeleportToArea_hack, true);
		dlogr(" Done", DL_INIT);
	//}
}
