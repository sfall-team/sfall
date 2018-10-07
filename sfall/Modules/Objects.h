#pragma once

#include "Module.h"

namespace sfall
{

class Objects : public Module {
public:
	const char* name() { return "Objects"; }
	void init();

	static void SetAutoUnjamLockTime(DWORD time);
	static void LoadProtoAutoMaxLimit();
};

}
