#include "..\Logging.h"
#include "..\SafeWrite.h"
#include "ScriptExtender.h"

#include "Input.h"

namespace sfall
{

void Input::init() {
	//if (IniReader::GetConfigInt("Input", "Enable", 0)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(0x50FB70, "ddraw.dll");
		::sfall::availableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}
}

}
