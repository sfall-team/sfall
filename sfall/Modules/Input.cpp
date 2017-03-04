
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\SafeWrite.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"

#include "Input.h"

static const DWORD dinputPos = 0x50FB70;

void Input::init() {
	//if(GetConfigInt("Input", "Enable", 0)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(dinputPos, "ddraw.dll");
		availableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}
	
	LoadGameHook::onGameReset += []() {
		ForceGraphicsRefresh(0);
	};
}
