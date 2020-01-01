#pragma once

#include "Module.h"

namespace sfall
{

class LoadDll : public Module {
public:
	const char* name() { return "LoadDll"; }
	void init();
};

}
