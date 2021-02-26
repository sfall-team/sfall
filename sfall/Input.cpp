#include "Logging.h"
#include "SafeWrite.h"
#include "ScriptExtender.h"

void Input_Init() {
	//if (GetConfigInt("Input", "Enable", 0)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(0x50FB70, "ddraw.dll");
		availableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}
}
