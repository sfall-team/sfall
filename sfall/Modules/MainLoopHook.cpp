#include "..\FalloutEngine\Fallout2.h"
#include "..\SafeWrite.h"

#include "MainLoopHook.h"

namespace sfall 
{

static Delegate<> onMainLoop;
static Delegate<> onCombatLoop;
static Delegate<> onAfterCombatAttack;

void __stdcall MainGameLoopHook2() {
	onMainLoop.invoke();
}

void __stdcall CombatLoopHook2() {
	onCombatLoop.invoke();
}

void AfterCombatAttackHook2() {
	onAfterCombatAttack.invoke();
}

static void __declspec(naked) MainGameLoopHook() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		call fo::funcoffs::get_input_
		push eax;
		call MainGameLoopHook2;
		pop eax;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) CombatLoopHook() {
	__asm {
		pushad;
		call CombatLoopHook2;
		popad;
		jmp  fo::funcoffs::get_input_
	}
}

static void __declspec(naked) AfterCombatAttackHook() {
	__asm {
		pushad;
		call AfterCombatAttackHook2;
		popad;
		mov  eax, 1;
		push 0x4230DA;
		retn;
	}
}

void MainLoopHook::init() {
	HookCall(0x480E7B, MainGameLoopHook); //hook the main game loop
	HookCall(0x422845, CombatLoopHook); //hook the combat loop
	MakeJump(0x4230D5, AfterCombatAttackHook);
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
