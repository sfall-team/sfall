#include "Module.h"

namespace sfall 
{

class BarBoxes : public Module {
public:
	const char* name() { return "BarBoxes"; }
	void init();
};

int _stdcall GetBox(int i);
void _stdcall AddBox(int i);
void _stdcall RemoveBox(int i);

}
