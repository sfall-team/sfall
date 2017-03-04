
#include "Module.h"

class Sound : public Module {
	const char* name() { return "Sounds"; }
	void init();
};
