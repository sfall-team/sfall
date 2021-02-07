#pragma once

namespace sfall
{
	void InitMiscHookScripts();
	void SourceUseSkillOnInit();

	void Inject_BarterPriceHook();
	void Inject_UseSkillOnHook();
	void Inject_UseSkillHook();
	void Inject_StealCheckHook();
	void Inject_SneakCheckHook();
	void Inject_WithinPerceptionHook();
	void Inject_CarTravelHook();
	void Inject_SetGlobalVarHook();
	void Inject_RestTimerHook();
	void Inject_ExplosiveTimerHook();
	void Inject_EncounterHook();
	void Inject_RollCheckHook();

	long PerceptionRangeHook_ScriptCheck(fo::GameObject* watcher, fo::GameObject* target, long result);
}
