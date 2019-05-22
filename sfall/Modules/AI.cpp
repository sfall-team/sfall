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

typedef std::unordered_map<DWORD, DWORD>::const_iterator iter;

static std::unordered_map<DWORD, DWORD> targets;
static std::unordered_map<DWORD, DWORD> sources;

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

static void __declspec(naked) ai_try_attack_hook_FleeFix() {
	__asm {
		or  byte ptr [esi + combatState], 8; // set new flag 'ReTarget'
		jmp fo::funcoffs::ai_run_away_;
	}
}

static const DWORD combat_ai_hook_flee_Ret = 0x42B22F;
static void __declspec(naked) combat_ai_hook_FleeFix() {
	__asm {
		test byte ptr [ebp], 8; // 'ReTarget' flag
		jnz  reTarget;
		test byte ptr [ebp], 4; // flee flag? (critter combat_state)
		jz   tryHeal;
flee:
		jmp  fo::funcoffs::critter_name_;
tryHeal:
		call fo::funcoffs::ai_check_drugs_; // try to heal
		mov  eax, esi;
		mov  edx, STAT_current_hp;
		call fo::funcoffs::stat_level_;
		cmp  eax, [ebx+ 0x10];  // cap minimum hp, below which NPC will run away
		mov  eax, esi;
		jl   flee;
		add  esp, 4;
		jmp  combat_ai_hook_flee_Ret;
reTarget:
		and  byte ptr [ebp], ~(4 | 8); // unset flags Flee/ReTarget
		xor  edi, edi;
		mov  dword ptr [esi + whoHitMe], edi;
		add  esp, 4;
		jmp  combat_ai_hook_flee_Ret;
	}
}

static const DWORD combat_ai_hack_Ret = 0x42B204;
static void __declspec(naked) combat_ai_hack() {
	__asm {
		mov  edx, [ebx + 0x10]; // cap.min_hp
		cmp  [ebx + 0x98], -1;  // cap.run_away_mode (none)
		je   skip;
		push eax;               // current hp
		mov  eax, esi;
		call fo::funcoffs::isPartyMember_;
		test eax, eax;
		pop  eax;
		cmovz edx, [esp + 0x1C - 0x18 + 4]; // calculated min_hp
skip:
		cmp  eax, edx;
		jl   end; // curr < min hp
		add  esp, 4;
		jmp  combat_ai_hack_Ret;
end:
		retn; // flee
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////

static bool __fastcall sf_critter_have_ammo(fo::GameObject* critter, fo::GameObject* weapon) {
	DWORD slotNum = -1;
	while (true) {
		fo::GameObject* ammo = fo::func::inven_find_type(critter, fo::item_type_ammo, &slotNum);
		if (!ammo) break;
		if (fo::func::item_w_can_reload(weapon, ammo)) return true;
	}
	return false;
}

static DWORD __fastcall sf_check_ammo(fo::GameObject* weapon, fo::GameObject* critter) {
	if (sf_critter_have_ammo(critter, weapon)) return 1;

	long result = 0;
	long maxDist = fo::func::stat_level(critter, STAT_pe) + 5;
	long* objectsList = nullptr;
	long numObjects = fo::func::obj_create_list(-1, critter->elevation, fo::ObjType::OBJ_TYPE_ITEM, &objectsList);
	if (numObjects > 0) {
		fo::var::combat_obj = critter;
		fo::func::qsort(objectsList, numObjects, 4, fo::funcoffs::compare_nearer_);
		for (int i = 0; i < numObjects; i++)
		{
			fo::GameObject* itemGround = (fo::GameObject*)objectsList[i];
			if (fo::func::obj_dist(critter, itemGround) > maxDist) break;
			if (fo::func::item_get_type(itemGround) == fo::item_type_ammo) {
				if (fo::func::item_w_can_reload(weapon, itemGround)) {
					result = 1;
					break;
				}
			}
		}
		fo::func::obj_delete_list(objectsList);
	}
	return result; // 0 - no have ammo
}

static void __declspec(naked) ai_search_environ_hook_weapon() {
	__asm {
		call fo::funcoffs::ai_can_use_weapon_;
		test eax, eax;
		jnz  checkAmmo;
		retn;
checkAmmo:
		mov  edx, [esp + 4]; // base
		mov  eax, [edx + ecx];
		cmp  dword ptr [eax + charges], 0; // ammo count
		jnz  end;
		push ecx;
		mov  ecx, eax;       // weapon
		mov  edx, esi;       // source
		call sf_check_ammo;
		pop  ecx;
end:
		retn;
	}
}

static DWORD sf_check_critters_on_fireline(fo::GameObject* object, DWORD checkTile, DWORD team) {
	if (object && object->Type() == ObjType::OBJ_TYPE_CRITTER && object->critter.teamNum != team) { // not friendly fire
		fo::GameObject*	obj = nullptr; // continue check the line_of_fire from object to checkTile
		fo::func::make_straight_path_func(object, object->tile, checkTile, 0, (DWORD*)&obj, 32, (void*)fo::funcoffs::obj_shoot_blocking_at_);
		if (!sf_check_critters_on_fireline(obj, checkTile, team)) return 0;
	}
	return (DWORD)object;
}

static DWORD __fastcall sf_ai_move_steps_closer(fo::GameObject* source, fo::GameObject* target, DWORD &distOut) {
	DWORD distance, shotTile = 0;

	char rotationData[256];
	long pathLength = fo::func::make_path_func(source, source->tile, target->tile, rotationData, 0, (void*)fo::funcoffs::obj_blocking_at_);

	long dist = source->critter.movePoints + 1;
	if (dist < pathLength) pathLength = dist;

	long checkTile = source->tile;
	for (int i = 0; i < pathLength; i++)
	{
		checkTile = fo::func::tile_num_in_direction(checkTile, rotationData[i], 1);

		fo::GameObject* object = nullptr; // check the line_of_fire from target to checkTile
		fo::func::make_straight_path_func(target, target->tile, checkTile, 0, (DWORD*)&object, 32, (void*)fo::funcoffs::obj_shoot_blocking_at_);
		if (!sf_check_critters_on_fireline(object, checkTile, source->critter.teamNum)) { // if there are no friendly critters
			shotTile = checkTile;
			distance = i + 1;
			break;
		}
	}
	if (shotTile) {
		fo::GameObject* itemHand = fo::func::inven_right_hand(source);
		int minCost = 100;
		int cost = fo::func::item_w_primary_mp_cost(itemHand);
		if (cost > 0) minCost = cost;
		cost = fo::func::item_w_secondary_mp_cost(itemHand);
		if (cost > 0 && cost < minCost) minCost = cost;

		int needAP = distance + minCost;
		if (source->critter.movePoints < needAP) {
			shotTile = 0;
		} else {
			distOut = distance;  // change distance in ebp register
		}
	}
	return shotTile;
}

static void __declspec(naked) ai_move_steps_closer_hook() {
	__asm {
		cmp  dword ptr [esp + 0x1C + 4], 0x42AC5A;  // calls from try attack: shot blocked
		jnz  end;
		push ecx;
		push edx;
		push eax;
		push ebp;  // distance
		push esp;                     // distPtr
		mov  ecx, eax;                // source
		call sf_ai_move_steps_closer; // edx - target
		test eax, eax;
		jz   skip;
		mov [ebx], eax; // replace target tile
skip:
		pop  ebp;
		pop  eax;
		pop  edx;
		pop  ecx;
end:
		jmp  fo::funcoffs::cai_retargetTileFromFriendlyFire_;
	}
}

static const DWORD ai_move_to_object_ret = 0x42A192;
static void __declspec(naked) ai_move_steps_closer_hack_move() {
	__asm {
		mov  edx, [esp + 4];          // source goto tile
		cmp  [edi + tile], edx;       // target tile
		jnz  moveTile;

		test [edi + flags + 1], 0x08; // target is multihex?
		jnz  moveObject;
		test [esi + flags + 1], 0x08; // source is multihex?
		jz   moveTile;
moveObject:
		add  esp, 4;
		jmp  ai_move_to_object_ret;
moveTile:
		retn; // move to tile
	}
}

static const DWORD ai_run_to_object_ret = 0x42A169;
static void __declspec(naked) ai_move_steps_closer_hack_run() {
	__asm {
		mov  edx, [esp + 4];          // source goto tile
		cmp  [edi + tile], edx;       // target tile
		jnz  runTile;

		test [edi + flags + 1], 0x08; // target is multihex?
		jnz  runObject;
		test [esi + flags + 1], 0x08; // source is multihex?
		jz   runTile;
runObject:
		add  esp, 4;
		jmp  ai_run_to_object_ret;
runTile:
		retn; // run to tile
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
			if (item && item->protoId == itemGround->protoId) continue;

			if (fo::func::obj_dist(source, itemGround) > source->critter.movePoints + 1) break;
			// check real path distance
			int toDistObject = fo::func::make_path_func(source, source->tile, itemGround->tile, 0, 0, (void*)fo::funcoffs::obj_blocking_at_);
			if (toDistObject > source->critter.movePoints + 1) continue;

			if (fo::func::item_get_type(itemGround) == fo::item_type_weapon) {
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

static fo::GameObject* sf_ai_skill_weapon(fo::GameObject* source, fo::GameObject* hWeapon, fo::GameObject* sWeapon) {
	if (!hWeapon) return sWeapon;
	if (!sWeapon) return hWeapon;

	int hSkill = fo::func::item_w_skill(hWeapon, fo::AttackType::ATKTYPE_RWEAPON_PRIMARY);
	int sSkill = fo::func::item_w_skill(sWeapon, fo::AttackType::ATKTYPE_RWEAPON_PRIMARY);

	if (hSkill == sSkill) return sWeapon;

	int hLevel = fo::func::skill_level(source, hSkill);
	int sLevel = fo::func::skill_level(source, sSkill) + 10;

	return (hLevel > sLevel) ? hWeapon : sWeapon;
}

bool LookupOnGround = false;
static void __fastcall sf_ai_search_weapon(fo::GameObject* source, fo::GameObject* target, DWORD &weapon, DWORD &hitMode) {

	fo::GameObject* itemHand   = fo::func::inven_right_hand(source); // current item
	fo::GameObject* bestWeapon = itemHand;
	#ifndef NDEBUG
		if (itemHand) fo::func::debug_printf("\n[AI] HandPid: %d", itemHand->protoId);
	#endif

	DWORD slotNum = -1;
	while (true)
	{
		fo::GameObject* item = fo::func::inven_find_type(source, fo::item_type_weapon, &slotNum);
		if (!item) break;
		if (itemHand && itemHand->protoId == item->protoId) continue;

		if ((source->critter.movePoints >= fo::func::item_w_primary_mp_cost(item))
			&& fo::func::ai_can_use_weapon(source, item, AttackType::ATKTYPE_RWEAPON_PRIMARY))
		{
			if (item->item.ammoPid == -1 || fo::func::item_w_subtype(item, AttackType::ATKTYPE_RWEAPON_PRIMARY) == fo::AttackSubType::THROWING
				|| (fo::func::item_w_curr_ammo(item) || sf_critter_have_ammo(source, item)))
			{
				if (!fo::func::combat_safety_invalidate_weapon_func(source, item, AttackType::ATKTYPE_RWEAPON_PRIMARY, target, 0, 0)) { // weapon safety
					bestWeapon = fo::func::ai_best_weapon(source, bestWeapon, item, target);
				}
			}
		}
	}
	#ifndef NDEBUG
		if (bestWeapon) fo::func::debug_printf("\n[AI] BestWeaponPid: %d", bestWeapon->protoId);
	#endif

	if (itemHand != bestWeapon)	bestWeapon = sf_ai_skill_weapon(source, itemHand, bestWeapon);

	if ((LookupOnGround && !fo::func::critterIsOverloaded(source)) && source->critter.movePoints >= 3 && fo::func::critter_body_type(source) == fo::BodyType::Biped) {
		int toDistTarget = fo::func::make_path_func(source, source->tile, target->tile, 0, 0, (void*)fo::funcoffs::obj_blocking_at_);
		if ((source->critter.movePoints - 3) >= toDistTarget) goto notRetrieve; // ???

		fo::GameObject* itemGround = sf_ai_search_weapon_environ(source, bestWeapon, target);
		#ifndef NDEBUG
			if (itemGround) fo::func::debug_printf("\n[AI] OnGroundPid: %d", itemGround->protoId);
		#endif

		if (itemGround != bestWeapon) {
			if (itemGround && (!bestWeapon || itemGround->protoId != bestWeapon->protoId)) {
				if (bestWeapon && fo::func::item_cost(itemGround) < fo::func::item_cost(bestWeapon) + 50) goto notRetrieve;
				fo::GameObject* item = sf_ai_skill_weapon(source, bestWeapon, itemGround);
				if (item != itemGround) goto notRetrieve;

				#ifndef NDEBUG
					fo::func::debug_printf("\n[AI] TryRetrievePid: %d MP: %d", itemGround->protoId, source->critter.movePoints);
				#endif
				fo::GameObject* itemRetrieve = fo::func::ai_retrieve_object(source, itemGround);
				#ifndef NDEBUG
					int pid = (itemRetrieve) ? itemRetrieve->protoId : 0;
					fo::func::debug_printf("\n[AI] PickupPid: %d MP: %d", pid, source->critter.movePoints);
				#endif
				if (itemRetrieve && itemRetrieve->protoId == itemGround->protoId) {
					// if there is not enough action points to use the weapon, then just pick up this item
					bestWeapon = (source->critter.movePoints >= fo::func::item_w_primary_mp_cost(itemRetrieve)) ? itemRetrieve : nullptr;
				}
			}
		}
	}
notRetrieve:
	#ifndef NDEBUG
		fo::func::debug_printf("\n[AI] BestWeaponPid: %d MP: %d", ((bestWeapon) ? bestWeapon->protoId : 0), source->critter.movePoints);
	#endif

	if (bestWeapon && (!itemHand || itemHand->protoId != bestWeapon->protoId)) {
		weapon = (DWORD)bestWeapon;
		hitMode = fo::func::ai_pick_hit_mode(source, bestWeapon, target);
		fo::func::inven_wield(source, bestWeapon, fo::InvenType::INVEN_TYPE_RIGHT_HAND);
		_asm call fo::funcoffs::combat_turn_run_;
		#ifndef NDEBUG
			fo::func::debug_printf("\n[AI] WieldPid: %d MP: %d", bestWeapon->protoId, source->critter.movePoints);
		#endif
	}
}

static bool weaponIsSwitch = 0;
static void __declspec(naked) ai_try_attack_hook() {
	__asm {
		test edi, edi;                        // begin turn?
		jnz  end;
		test weaponIsSwitch, 1;
		jnz  end;
		cmp  [esp + 0x364 - 0x44 + 4], 0;     // check safety_range
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
		mov  weaponIsSwitch, 0;
		jmp  fo::funcoffs::combat_check_bad_shot_;
	}
}

static void __declspec(naked) ai_try_attack_hook_switch() {
	__asm {
		mov weaponIsSwitch, 1;
		jmp fo::funcoffs::ai_switch_weapons_;
	}
}

static bool __fastcall sf_ai_check_target(fo::GameObject* source, fo::GameObject* target) {

	int distance = fo::func::obj_dist(source, target);
	if (distance == 0) return false;

	bool shotIsBlock = fo::func::combat_is_shot_blocked(source, source->tile, target->tile, target, 0);

	int pathToTarget = fo::func::make_path_func(source, source->tile, target->tile, 0, 0, (void*)fo::funcoffs::obj_blocking_at_);
	if (shotIsBlock && !pathToTarget) { // shot and move block to target
		return true;                    // picking alternate target
	}

	fo::AIcap* cap = fo::func::ai_cap(source);
	if (shotIsBlock && pathToTarget > 1) { // shot block to target, can move
		switch (cap->disposition) {
		case AIpref::defensive:
			pathToTarget += 5;
			break;
		case AIpref::aggressive: // ai aggressive never does not change its target if the move-path to the target is not blocked
			pathToTarget = 1;
			break;
		case AIpref::berserk:
			pathToTarget /= 2;
			break;
		}
		if (pathToTarget > (source->critter.movePoints * 2)) {
			return true; // target is far -> picking alternate target
		}
	}
	else if (!shotIsBlock) { // can shot to target
		fo::GameObject* itemHand = fo::func::inven_right_hand(source); // current item
		if (!itemHand && !pathToTarget) return true; // no item and move block to target -> picking alternate target
		if (!itemHand) return false;

		fo::Proto* proto = GetProto(itemHand->protoId);
		if (proto && proto->item.type == ItemType::item_type_weapon) {
			int hitMode = fo::func::ai_pick_hit_mode(source, itemHand, target);
			int maxRange = fo::func::item_w_range(source, hitMode);
			int diff = distance - maxRange;
			if (diff > 0) {
				if (!pathToTarget // move block to target and shot out of range -> picking alternate target
					|| cap->disposition == AIpref::coward || diff > fo::func::roll_random(8, 12)) return true;
			}
		} // can shot or move, and item not weapon
	} // can shot and move / can move and block shot / can shot and block move
	return false;
}

static const char* reTargetMsg = "\n[AI] I can't get at my target. Try picking alternate.";

static const DWORD ai_danger_source_hack_find_Pick = 0x42908C;
static const DWORD ai_danger_source_hack_find_Ret  = 0x4290BB;
static void __declspec(naked) ai_danger_source_hack_find() {
	__asm {
		push eax;
		push edx;
		mov  edx, eax;
		mov  ecx, esi;
		call sf_ai_check_target;
		pop  edx;
		test al, al;
		pop  eax;
		jnz  reTarget;
		add  esp, 0x1C;
		pop  ebp;
		pop  edi;
		jmp  ai_danger_source_hack_find_Ret;
reTarget:
		push reTargetMsg;
		call fo::funcoffs::debug_printf_;
		add  esp, 4;
		jmp  ai_danger_source_hack_find_Pick;
	}
}

static void __declspec(naked) ai_danger_source_hack() {
	__asm {
		mov  eax, esi;
		call fo::funcoffs::ai_get_attack_who_value_;
		mov  dword ptr [esp + 0x34 - 0x1C + 4], eax; // attack_who
		retn;
	}
}

static long _fastcall sf_ai_check_weapon_switch( fo::GameObject* target, fo::GameObject* source) {
	fo::GameObject* item = fo::func::ai_search_inven_weap(source, 1, target);
	if (!item) return true; // no weapon in inventory, true to allow the to search continue weapon on the map
	long wType = fo::func::item_w_subtype(item, AttackType::ATKTYPE_RWEAPON_PRIMARY);
	if (wType < fo::THROWING) { // melee weapon, check the distance before switching
		if (fo::func::obj_dist(source, target) > 2) return false;
	}
	return true;
}

static void __declspec(naked) ai_try_attack_hook_switch_fix() {
	__asm {
		mov  eax, [eax + movePoints];
		test eax, eax;
		jz   noSwitch; // if movePoints == 0
		cmp  dword ptr [esp + 0x364 - 0x3C + 4], 0;
		jz   switch;   // no weapon in hand slot
		push edx;
		mov  edx, esi;
		call sf_ai_check_weapon_switch;
		pop  edx;
		test eax, eax;
		jz   noSwitch;
		mov  ecx, ebp;
switch:
		mov  eax, esi;
		jmp  fo::funcoffs::ai_switch_weapons_;
noSwitch:
		dec  eax; // -1 - for exit from ai_try_attack_
		retn;
	}
}

static void _fastcall sf_ai_move_away_from_target(fo::GameObject* source, fo::GameObject* target, fo::GameObject* sWeapon, long hit) {
	long type = fo::GetCritterKillType(source);
	if (type > 1) return; // if not men & women

	long wTypeR = fo::func::item_w_subtype(sWeapon, hit);
	if (wTypeR <= fo::MELEE) return; // source has a melee weapon

	fo::AIcap* cap = fo::func::ai_cap(source);
	if (cap->disposition == AIpref::berserk) return;
	if (fo::func::obj_dist(source, target) >= 3) return;

	fo::GameObject* itemHandR = fo::func::inven_right_hand(target);
	if (!itemHandR && target != fo::var::obj_dude) return; // target is unarmed
	wTypeR = fo::func::item_w_subtype(itemHandR, AttackType::ATKTYPE_RWEAPON_PRIMARY);

	long wTypeL = fo::NONE;
	if (target == fo::var::obj_dude) {
		fo::GameObject* itemHandL = fo::func::inven_left_hand(target);
		if (itemHandL) wTypeL = fo::func::item_w_subtype(itemHandL, AttackType::ATKTYPE_RWEAPON_PRIMARY);
	}
	if (cap->disposition == AIpref::aggressive) {
		if (wTypeR == fo::GUNS || wTypeL == fo::GUNS) return;
	}
	fo::func::ai_move_away(source, target, source->critter.movePoints);
}

static void __declspec(naked) ai_try_attack_hack_move() {
	__asm {
		mov  eax, [esi + movePoints];
		test eax, eax;
		jz   noMovePoint;
		mov  eax, dword ptr [esp + 0x364 - 0x3C + 4]; // right_weapon
		push [esp + 0x364 - 0x38 + 4]; // hit_mode
		mov  edx, ebp;
		mov  ecx, esi;
		push eax;
		call sf_ai_move_away_from_target;
noMovePoint:
		mov  eax, -1;
		retn;
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

//////////////////////////////////////////////////////////////////////////////////////////////

static void __fastcall CombatAttackHook(DWORD source, DWORD target) {
	sources[target] = source;
	targets[source] = target;
}

static void __declspec(naked) combat_attack_hook() {
	_asm {
		push ecx;
		push edx;
		push eax;
		mov  ecx, eax;         // source
		call CombatAttackHook; // edx - target
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

static const DWORD BlockCombatHook1Ret1 = 0x45F6B4;
static const DWORD BlockCombatHook1Ret2 = 0x45F6D7;
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
	HookCall(0x426A95, combat_attack_hook);  // combat_attack_this_
	HookCall(0x42A796, combat_attack_hook);  // ai_attack_

	MakeJump(0x45F6AF, BlockCombatHook1);    // intface_use_item_
	HookCall(0x4432A6, BlockCombatHook2);    // game_handle_input_
	combatBlockedMessage = Translate("sfall", "BlockedCombat", "You cannot enter combat at this time.");

	RetryCombatMinAP = GetConfigInt("CombatAI", "NPCsTryToSpendExtraAP", -1);
	if (RetryCombatMinAP == -1) RetryCombatMinAP = GetConfigInt("Misc", "NPCsTryToSpendExtraAP", 0); // compatibility older versions
	if (RetryCombatMinAP > 0) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, RetryCombatHook); // combat_turn_
		dlogr(" Done", DL_INIT);
	}

	// Enables the ability to use the AttackWho value from the AI-packet for the NPC
	if (GetConfigInt("CombatAI", "NPCAttackWhoFix", 0)) {
		MakeCall(0x428F70, ai_danger_source_hack, 3);
	}

	// Enables the use of the RunAwayMode value from the AI-packet for the NPC
	// the min_hp value will be calculated as a percentage of the maximum number of NPC health points, instead of using fixed min_hp values
	if (GetConfigInt("CombatAI", "NPCRunAwayMode", 0)) {
		MakeCall(0x42B1DC, combat_ai_hack);
	}

	///////////////////// Combat AI behavior fixes /////////////////////////

	// When npc does not have enough AP to use the weapon, it begin looking in the inventory another weapon to use,
	// if no suitable weapon is found, then are search the nearby objects(weapons) on the ground to pick-up them
	// This fix prevents pick-up of the object located on the ground, if npc does not have the full amount of AP (ie, the action does occur at not the beginning of its turn)
	// or if there is not enough AP to pick up the object on the ground. Npc will not spend its AP for inappropriate use
	if (GetConfigInt("CombatAI", "ItemPickUpFix", 0)) {
		HookCall(0x429CAF, ai_search_environ_hook);
	}

	// Fixed switching weapons when action points is zero
	if (GetConfigInt("CombatAI", "NPCSwitchingWeaponFix", 0)) {
		HookCall(0x42AB57, ai_try_attack_hook_switch_fix);
	}

	// Fix to allow running away NPC to use drugs
	HookCall(0x42B1E3, combat_ai_hook_FleeFix);
	// Fix that with a low chance of to hit the target, the NPC did not switch to "flee" mode
	HookCalls(ai_try_attack_hook_FleeFix, {0x42ABA8, 0x42ACE5});
	// Disabled flee
	BlockCall(0x42ADF6); // ai_try_attack_

	/////////////////// Combat AI improve behavior //////////////////////////

	// Before starting his turn npc will always check if it has better weapons in inventory, than there is a current weapon
	int BetterWeapons = GetConfigInt("CombatAI", "TakeBetterWeapons", 0);
	if (BetterWeapons) {
		HookCall(0x42A92F, ai_try_attack_hook);
		HookCall(0x42A905, ai_try_attack_hook_switch);
		LookupOnGround = (BetterWeapons > 1);    // always check the items available on the ground
	}

	switch (GetConfigInt("CombatAI", "TryToFindTargets", 0)) {
	case 1:
		MakeJump(0x4290B6, ai_danger_source_hack_find);
		break;
	case 2:
		SafeWrite16(0x4290B3, 0xDFEB); // jmp 0x429094
		SafeWrite8(0x4290B5, 0x90);
	}

	if (GetConfigInt("CombatAI", "SmartBehavior", 0) || GetConfigInt("CombatAI", "CheckShotOnMove", 0)) {
		// Checks the movement path for the possibility à shot, if the shot to the target is blocked
		HookCall(0x42A125, ai_move_steps_closer_hook);
		MakeCall(0x42A178, ai_move_steps_closer_hack_move, 1);
		MakeCall(0x42A14F, ai_move_steps_closer_hack_run, 1);

		// Don't pickup a weapon if its magazine is empty and there are no ammo for it
		HookCall(0x429CF2, ai_search_environ_hook_weapon);
		// Ìove away from the target if the target is near
		MakeCalls(ai_try_attack_hack_move, {0x42AE40, 0x42AE7F});
	}
}

DWORD _stdcall AIGetLastAttacker(DWORD target) {
	iter itr = sources.find(target);
	return (itr != sources.end()) ? itr->second: 0;
}

DWORD _stdcall AIGetLastTarget(DWORD source) {
	iter itr = targets.find(source);
	return (itr != targets.end()) ? itr->second : 0;
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
