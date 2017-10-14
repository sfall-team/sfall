
#include "Module.h"

namespace sfall
{

class Reputations : public Module {
public:
	const char* name() { return "Reputations"; }
	void init();
	void exit() override;
};

}
