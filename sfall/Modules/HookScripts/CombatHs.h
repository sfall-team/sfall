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

}
