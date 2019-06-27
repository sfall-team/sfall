#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Interface.h"

namespace sfall
{

static BYTE movePointBackground[16 * 9 * 5];
static fo::UnlistedFrm* ifaceFrm = nullptr;

static void* LoadIfaceFrm() {
	ifaceFrm = fo::LoadUnlistedFrm("IFACE_E.frm", fo::OBJ_TYPE_INTRFACE);
	if (!ifaceFrm) return nullptr;
	return ifaceFrm->frames[0].indexBuff;
}

static void __declspec(naked) intface_init_hook_lock() {
	__asm {
		pushadc;
		call LoadIfaceFrm;
		test eax, eax;
		jz   skip;
		pop  ecx;
		add  esp, 8;
		mov  dword ptr [ecx], 0;
		retn;
skip:
		popadc;
		jmp  fo::funcoffs::art_ptr_lock_data_;
	}
}

static void __declspec(naked) intface_init_hack() {
	__asm {
		add eax, 9276 - (54 / 2); // x offset
		mov edx, 144 - 90;        // width
		add [esp + 4], edx;
		add [esp + 0x10 + 4], edx;
		retn;
	}
}

static const DWORD intface_update_move_points_ret = 0x45EE3E;
static void __declspec(naked) intface_update_move_points_hack() {
	__asm {
		mov  eax, 16 * 9
		push eax;
		push 5;
		push eax;
		jmp  intface_update_move_points_ret;
	}
}

static void APBarRectPatch() {
	fo::var::movePointRect.x -= (54 / 2); // 54 = 144(new width) - 90(old width)
	fo::var::movePointRect.offx += (54 / 2);
}

bool ActionPointsBarPatch() {
	dlog("Applying expanded action points bar patch.", DL_INIT);
	bool result = false;
	if (*(DWORD*)0x4E4480 != 0x278805C7) { // check enabled HRP
		// check valid data
		if (!strcmp((const char*)0x10039340, "HR_IFACE_CHI_%i.frm")
			&& !strcmp((const char*)0x10039358, "HR_IFACE_%i.frm"))
		{	// patching HRP
			SafeWriteStr(0x10039363, "E.frm");
			SafeWriteStr(0x1003934F, "E.frm");
		} else {
			dlog(" HRP: Incorrect version!", DL_INIT);
		}
		result = true;
	} else {
		APBarRectPatch();
	}
	SafeWrite32(0x45E343, (DWORD)&movePointBackground);
	SafeWrite32(0x45EE3F, (DWORD)&movePointBackground);
	SafeWriteBatch<BYTE>(16, {0x45EE55, 0x45EE7B, 0x45EE82, 0x45EE9C, 0x45EEA0});
	SafeWriteBatch<DWORD>(9276 - (54 / 2), {0x45EE33, 0x45EEC8, 0x45EF16});

	HookCall(0x45D918, intface_init_hook_lock);
	MakeCall(0x45E356, intface_init_hack);
	MakeJump(0x45EE38, intface_update_move_points_hack, 1);
	dlogr(" Done", DL_INIT);
	return result;
}

void Interface::init() {
	if (GetConfigInt("Interface", "ActionPointsBar", 0)) {
		if (ActionPointsBarPatch()) LoadGameHook::OnAfterGameInit() += APBarRectPatch;
	}
}

void Interface::exit() {
	if (ifaceFrm) delete ifaceFrm;
}

}
