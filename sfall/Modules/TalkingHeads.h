#include "Module.h"

namespace sfall
{

class TalkingHeads : public Module {
	const char* name() { return "TalkingHeads"; }
	void init();
};

}
