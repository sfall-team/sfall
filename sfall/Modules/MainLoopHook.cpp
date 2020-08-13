#include "..\FalloutEngine\Fallout2.h"
#include "..\SafeWrite.h"

#include "MainLoopHook.h"

namespace sfall
{

static Delegate<> onMainLoop;
static Delegate<> onCombatLoop;
static Delegate<> onAfterCombatAttack;

bool MainLoopHook::displayWinUpdateState = false;

static void MainGameLoopResetStates() {
	MainLoopHook::displayWinUpdateState = false;
}

static void __stdcall MainGameLoop() {
	onMainLoop.invoke();
	MainGameLoopResetStates();
}

static void __stdcall CombatLoop() {
	onCombatLoop.invoke();
}

static void __stdcall AfterCombatAttack() {
	onAfterCombatAttack.invoke();
}

static void __declspec(naked) MainGameLoopHook() {
	__asm {
		push ecx;
		call fo::funcoffs::get_input_;
		push edx;
		push eax;
		call MainGameLoop;
		pop  eax;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) CombatLoopHook() {
	__asm {
		push ecx;
		push edx;
		//push eax;
		call CombatLoop;
		//pop  eax;
		pop  edx;
		call fo::funcoffs::get_input_;
		pop  ecx; // fix to prevent the combat turn from being skipped after using Alt+Tab
		retn;
	}
}

static void __declspec(naked) AfterCombatAttackHook() {
	__asm {
		push ecx;
		push edx;
		call AfterCombatAttack;
		pop  edx;
		pop  ecx;
		mov  eax, 1;
		retn;
	}
}

void MainLoopHook::init() {
	HookCall(0x480E7B, MainGameLoopHook);       // hook the main game loop
	HookCall(0x422845, CombatLoopHook);         // hook the combat loop
	MakeCall(0x4230D5, AfterCombatAttackHook);
}

Delegate<>& MainLoopHook::OnMainLoop() {
	return onMainLoop;
}

Delegate<>& MainLoopHook::OnCombatLoop() {
	return onCombatLoop;
}

Delegate<>& MainLoopHook::OnAfterCombatAttack() {
	return onAfterCombatAttack;
}

}
