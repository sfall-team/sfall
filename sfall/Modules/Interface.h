#pragma once

#include "Module.h"

namespace sfall
{

class Interface : public Module {
public:
	const char* name() { return "Interface"; }
	void init();
	void exit() override;

	static long ActiveInterfaceWID();
	static fo::Window* GetWindow(long winType);
};

}
