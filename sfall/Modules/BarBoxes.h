#pragma once

namespace sfall 
{

class BarBoxes {
private:
	static int boxCount;

public:
	static const char* name() { return "BarBoxes"; }
	static void init();
	static void exit();

	static void OnGameLoad();
	static void OnAfterGameInit();

	static int  MaxBox() { return boxCount - 1; }
	static void SetText(int box, const char* text, DWORD color);

	static bool GetBox(int i);
	static void AddBox(int i);
	static void RemoveBox(int i);
	static long AddExtraBox();
};

}
