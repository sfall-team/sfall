#include "Module.h"

namespace sfall
{

class Sound : public Module {
	const char* name() { return "Sounds"; }
	void init();
};

}
