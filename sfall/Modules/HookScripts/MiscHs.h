#pragma once

namespace sfall
{
	void InitMiscHookScripts();

	void Inject_BarterPriceHook();
	void Inject_UseSkillHook();
	void Inject_StealCheckHook();
	void Inject_WithinPerceptionHook();
	void Inject_CarTravelHook();
	void Inject_SetGlobalVarHook();
	void Inject_RestTimerHook();

}
