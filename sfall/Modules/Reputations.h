
#include "Module.h"

class Reputations : public Module {
	const char* name() { return "Reputations"; }
	void init();
	void exit() override;
};
