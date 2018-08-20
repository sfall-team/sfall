/*
 *    sfall
 *    Copyright (C) 2012  The sfall team
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

#include <unordered_map>

#include "..\main.h"

#include "AI.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SafeWrite.h"

namespace sfall 
{
using namespace fo;
using namespace Fields;

typedef std::unordered_map<DWORD, DWORD> :: const_iterator iter;

static std::unordered_map<DWORD,DWORD> targets;
static std::unordered_map<DWORD,DWORD> sources;

static const DWORD ai_search_environ_ret = 0x429D3E;
static void __declspec(naked) ai_search_environ_hook() {
	__asm {
		call fo::funcoffs::obj_dist_;
		cmp  [esp + 0x28 + 0x1C + 4], item_type_ammo;
		jz   end;
		//
		push ecx;
		push edx;
		push eax;
		mov  eax, esi;
		mov  edx, STAT_max_move_points;
		call fo::funcoffs::stat_level_;
		mov  ecx, [esi + movePoints];    // source current ap
		cmp  ecx, eax;                   // npc already used their ap?
		pop  eax;
		pop  edx;
		jge  skip;
		// distance & AP check
		sub  ecx, 3;                     // pickup cost ap
		cmp  ecx, eax;                   // eax - distance to the object
		jl   continue;
skip:
		pop  ecx;
end:
		retn;
continue:
		pop  ecx;
		add  esp, 4;                     // destroy return
		jmp  ai_search_environ_ret;      // next object
	}
}

static bool GoToTile = false;
static DWORD __cdecl sf_ai_move_steps_closer(fo::GameObject* source, fo::GameObject* target, DWORD* distPtr) {
	DWORD distance, shotTile = 0;

	char rotationData[256];
	long pathLength = fo::func::make_path_func(source, source->tile, target->tile, rotationData, 0, (void*)fo::funcoffs::obj_blocking_at_);

	long checkTile = source->tile;
	for (int i = 0; i < pathLength; i++)
	{
		checkTile = fo::func::tile_num_in_direction(checkTile, rotationData[i], 1);

		DWORD object = 0;
		fo::func::make_straight_path_func(target, target->tile, checkTile, 0, &object, 32, (void*)fo::funcoffs::obj_shoot_blocking_at_);
		if (!object) {
			shotTile = checkTile;
			distance = i + 1;
			break;
		}
	}

	if (shotTile) {
		int needAP = distance + fo::func::item_w_primary_mp_cost(fo::func::inven_right_hand(source));
		if (source->critter.movePoints < needAP) {
			shotTile = 0;
		} else {
			*distPtr = distance;
			GoToTile = true;
		}
	}
	return shotTile;
}

static void __declspec(naked) ai_move_steps_closer_hook() {
	__asm {
		cmp  dword ptr [esp + 0x1C + 4], 0x42AC5A;  // calls from try attack: shot blocked
		jnz  end;
		push ecx;
		push ebp;  // dist
		push esp;  // distPtr
		push edx;
		push eax;
		call sf_ai_move_steps_closer;
		test eax, eax;
		jz   skip;
		mov [ebx], eax;
skip:
		pop  eax;
		pop  edx;
		add  esp, 4;
		pop  ebp;
		pop  ecx;
end:
		jmp  fo::funcoffs::cai_retargetTileFromFriendlyFire_;
	}
}

static const DWORD ai_move_to_object_ret = 0x42A192;
static void __declspec(naked) ai_move_steps_closer_hack_move() {
	__asm {
		cmp  GoToTile, 1;
		jnz  skip;
		mov  GoToTile, 0;
		retn;
skip:
		test [edi + flags + 1], 0x08; // target is multihex?
		jnz  moveObject;
		test [esi + flags + 1], 0x08; // source is multihex?
		jnz  moveObject;
		retn;
moveObject:
		add  esp, 4;
		jmp  ai_move_to_object_ret;
	}
}

static const DWORD ai_run_to_object_ret = 0x42A169;
static void __declspec(naked) ai_move_steps_closer_hack_run() {
	__asm {
		cmp  GoToTile, 1;
		jnz  skip;
		mov  GoToTile, 0;
		retn;
skip:
		test [edi + flags + 1], 0x08; // target is multihex?
		jnz  runObject;
		test [esi + flags + 1], 0x08; // source is multihex?
		jnz  runObject;
		retn;
runObject:
		add  esp, 4;
		jmp  ai_run_to_object_ret;
	}
}

static fo::GameObject* __stdcall sf_ai_search_weapon_environ(fo::GameObject* source, fo::GameObject* item, fo::GameObject* target) {

	long* objectsList = nullptr;

	long numObjects = fo::func::obj_create_list(-1, source->elevation, fo::ObjType::OBJ_TYPE_ITEM, &objectsList);
	if (numObjects > 0) {
		fo::var::combat_obj = source;
		fo::func::qsort(objectsList, numObjects, 4, fo::funcoffs::compare_nearer_);

		for (int i = 0; i < numObjects; i++)
		{
			fo::GameObject* itemGround = (fo::GameObject*)objectsList[i];
			if (fo::func::item_get_type(itemGround) == fo::item_type_weapon) {
				if (fo::func::obj_dist(source, itemGround) > source->critter.movePoints + 1) break;

				if (fo::func::ai_can_use_weapon(source, itemGround, fo::AttackType::ATKTYPE_RWEAPON_PRIMARY)) {
					if (fo::func::ai_best_weapon(source, item, itemGround, target) == itemGround) {
						item = itemGround;
					}
				}
			}
		}
		fo::func::obj_delete_list(objectsList);
	}
	return item;
}

bool LookupOnGround = false;
static void __fastcall sf_ai_search_weapon(fo::GameObject* source, fo::GameObject* target, DWORD* weapon, DWORD* hitMode) {

	fo::GameObject* itemHand   = fo::func::inven_right_hand(source); // current item
	fo::GameObject* bestWeapon = itemHand;

	DWORD slotNum = -1;
	while (true)
	{
		fo::GameObject* item = fo::func::inven_find_type(source, fo::item_type_weapon, &slotNum);
		if (!item) break;

		if ((source->critter.movePoints >= fo::func::item_w_primary_mp_cost(item))
			&& fo::func::ai_can_use_weapon(source, item, fo::AttackType::ATKTYPE_RWEAPON_PRIMARY)
			&& (fo::func::item_w_subtype(item, fo::AttackType::ATKTYPE_RWEAPON_PRIMARY) != fo::AttackSubType::GUNS
			|| fo::func::item_w_curr_ammo(item) || fo::func::ai_have_ammo(source, item, 0)))
		{
			bestWeapon = fo::func::ai_best_weapon(source, bestWeapon, item, target);
		}
	}

	if ((LookupOnGround || !itemHand) && source->critter.movePoints >= 3 && fo::func::critter_body_type(source) == fo::BodyType::Biped) {
		fo::GameObject* itemGround = sf_ai_search_weapon_environ(source, target, bestWeapon);
		if (itemGround && (!bestWeapon || itemGround->protoId != bestWeapon->protoId)) {
			itemGround = fo::func::ai_retrieve_object(source, itemGround);
			if (itemGround) bestWeapon = itemGround;
		}
	}

	if (bestWeapon && (!itemHand || itemHand->protoId != bestWeapon->protoId)) {
		*weapon = (DWORD)bestWeapon;
		*hitMode = fo::func::ai_pick_hit_mode(source, bestWeapon, target);
		fo::func::inven_wield(source, bestWeapon, fo::InvenType::INVEN_TYPE_RIGHT_HAND);
		_asm call fo::funcoffs::combat_turn_run_;
	}
}

static void __declspec(naked) ai_try_attack_hook() {
	__asm {
		test edi, edi;                        // begin turn?
		jnz  end;
		//
		lea  eax, [esp + 0x364 - 0x38 + 4];   // hit_mode
		push eax;
		lea  eax, [esp + 0x364 - 0x3C + 8];   // right_weapon
		push eax;
		mov  ecx, esi;                        // source
		call sf_ai_search_weapon;             // edx - target
		// restore value reg.
		mov  eax, esi;
		mov  edx, ebp;
		xor  ecx, ecx;
end:
		jmp  fo::funcoffs::combat_check_bad_shot_;
	}
}

static DWORD RetryCombatLastAP;
static DWORD RetryCombatMinAP;
static void __declspec(naked) RetryCombatHook() {
	__asm {
		mov  RetryCombatLastAP, 0;
retry:
		call fo::funcoffs::combat_ai_;
process:
		cmp  dword ptr ds:[FO_VAR_combat_turn_running], 0;
		jle  next;
		call fo::funcoffs::process_bk_;
		jmp  process;
next:
		mov  eax, [esi + movePoints];
		cmp  eax, RetryCombatMinAP;
		jl   end;
		cmp  eax, RetryCombatLastAP;
		je   end;
		mov  RetryCombatLastAP, eax;
		mov  eax, esi;
		xor  edx, edx;
		jmp  retry;
end:
		retn;
	}
}
//----------------------------------

static void _cdecl CombatAttackHook(DWORD source, DWORD target) {
	sources[target] = source;
	targets[source] = target;
}

static void __declspec(naked) combat_attack_hook() {
	_asm {
		push ecx;
		push edx;
		push eax;
		call CombatAttackHook;
		pop  eax;
		pop  edx;
		pop  ecx;
		jmp  fo::funcoffs::combat_attack_;
	}
}

static DWORD combatDisabled;
void _stdcall AIBlockCombat(DWORD i) {
	combatDisabled = i ? 1 : 0;
}

static std::string combatBlockedMessage;
static void _stdcall CombatBlocked() {
	fo::func::display_print(combatBlockedMessage.c_str());
}

static const DWORD BlockCombatHook1Ret1=0x45F6B4;
static const DWORD BlockCombatHook1Ret2=0x45F6D7;
static void __declspec(naked) BlockCombatHook1() {
	__asm {
		mov  eax, combatDisabled;
		test eax, eax;
		jz   end;
		call CombatBlocked;
		jmp  BlockCombatHook1Ret2;
end:
		mov  eax, 0x14;
		jmp  BlockCombatHook1Ret1;
	}
}

static void __declspec(naked) BlockCombatHook2() {
	__asm {
		mov  eax, dword ptr ds:[FO_VAR_intfaceEnabled];
		test eax, eax;
		jz   end;
		mov  eax, combatDisabled;
		test eax, eax;
		jz   succeed;
		push ecx;
		push edx;
		call CombatBlocked;
		pop  edx;
		pop  ecx;
		xor  eax, eax;
		retn;
succeed:
		inc  eax;
end:
		retn;
	}
}

void AI::init() {

	//HookCall(0x42AE1D, ai_attack_hook);
	//HookCall(0x42AE5C, ai_attack_hook);
	HookCall(0x426A95, combat_attack_hook);
	HookCall(0x42A796, combat_attack_hook);

	MakeJump(0x45F6AF, BlockCombatHook1);
	HookCall(0x4432A6, BlockCombatHook2);
	combatBlockedMessage = Translate("sfall", "BlockedCombat", "You cannot enter combat at this time.");

	// Combat AI improve and fixes

	// When npc does not have enough AP to use the weapon, it begin looking in the inventory another weapon to use,
	// if no suitable weapon is found, then are search the nearby objects(weapons) on the ground to pick-up them
	// This fix prevents pick-up of the object located on the ground, if npc does not have the full amount of AP (ie, the action does occur at not the beginning of its turn)
	// or if there is not enough AP to pick up the object on the ground. Npc will not spend its AP for inappropriate use
	if (GetConfigInt("CombatAI", "ItemPickUpFix", 0) != 0) {
		HookCall(0x429CAF, ai_search_environ_hook);
	}

	// Before starting his turn npc will always check if it has better weapons in inventory, than there is a current weapon
	int BetterWeapons = GetConfigInt("CombatAI", "TakeBetterWeapons", 0);
	if (BetterWeapons) {
		HookCall(0x42A92F, ai_try_attack_hook);
		LookupOnGround = (BetterWeapons > 1);    // always check the items available on the ground
	}

	// Checks the movement path for the possibility à shot, if the shot to the target is blocked
	if (GetConfigInt("CombatAI", "CheckShotOnMove", 0) != 0) {
		HookCall(0x42A125, ai_move_steps_closer_hook);
		MakeCall(0x42A178, ai_move_steps_closer_hack_move, 1);
		MakeCall(0x42A14F, ai_move_steps_closer_hack_run, 1);
	}

	RetryCombatMinAP = GetConfigInt("CombatAI", "NPCsTryToSpendExtraAP", -1);
	if (RetryCombatMinAP == -1) RetryCombatMinAP = GetConfigInt("Misc", "NPCsTryToSpendExtraAP", 0); // compatibility older versions
	if (RetryCombatMinAP > 0) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, RetryCombatHook); // combat_turn_
		dlogr(" Done", DL_INIT);
	}
}

DWORD _stdcall AIGetLastAttacker(DWORD target) {
	iter itr = sources.find(target);
	if(itr == sources.end()) {
		return 0;
	} else {
		return itr->second;
	}
}

DWORD _stdcall AIGetLastTarget(DWORD source) {
	iter itr = targets.find(source);
	if(itr == targets.end()) {
		return 0;
	} else {
		return itr->second;
	}
}

void _stdcall AICombatStart() {
	targets.clear();
	sources.clear();
}

void _stdcall AICombatEnd() {
	targets.clear();
	sources.clear();
}

}
