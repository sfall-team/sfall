#pragma once

// Combat-related hook scripts

namespace sfall
{

void InitCombatHookScripts();

void Inject_ToHitHook();
void Inject_AfterHitRollHook();
void Inject_CalcApCostHook();
void Inject_CombatDamageHook();
void Inject_FindTargetHook();
void Inject_ItemDamageHook();
void Inject_AmmoCostHook();
void Inject_CombatTurnHook();
void Inject_OnExplosionHook();
void Inject_SubCombatDamageHook();
void Inject_TargetObjectHook();

// Implementation of item_w_mp_cost_ engine function with the hook
long __fastcall sf_item_w_mp_cost(fo::GameObject* source, long hitMode, long isCalled);

}
