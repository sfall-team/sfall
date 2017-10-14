#include "Module.h"

namespace sfall
{

class TalkingHeads : public Module {
public:
	const char* name() { return "TalkingHeads"; }
	void init();
};

}
