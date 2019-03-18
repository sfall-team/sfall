#include "Module.h"

namespace sfall 
{

class BarBoxes : public Module {
private:
	static int boxCount;

public:
	const char* name() { return "BarBoxes"; }
	void init();
	void exit() override;

	static int  MaxBox() { return boxCount - 1; }
	static void SetText(int box, const char* text, DWORD color);

	static bool GetBox(int);
	static void AddBox(int);
	static void RemoveBox(int);
	static long AddExtraBox();
};

}
