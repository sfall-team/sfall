#include "Module.h"

namespace sfall
{

class Sound : public Module {
public:
	const char* name() { return "Sounds"; }
	void init();
};

}
