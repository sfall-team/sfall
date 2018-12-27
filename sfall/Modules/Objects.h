#include "Module.h"

namespace sfall
{

enum UniqueID {
	Start = 0x10000000,
	End   = 0x7FFFFFFF
};

class Objects : public Module {
public:
	const char* name() { return "Objects"; }
	void init();

	static long uniqueID;

	static long SetObjectUniqueID(fo::GameObject* obj);
	static void SetAutoUnjamLockTime(DWORD time);
	static void LoadProtoAutoMaxLimit();
};

}
