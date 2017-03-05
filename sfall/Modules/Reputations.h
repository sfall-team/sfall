
#include "Module.h"

namespace sfall
{

class Reputations : public Module {
	const char* name() { return "Reputations"; }
	void init();
	void exit() override;
};

}
