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
	static void __stdcall SetText(int box, const char* text, DWORD color);

	static bool __stdcall GetBox(int i);
	static void __stdcall AddBox(int i);
	static void __stdcall RemoveBox(int i);
	static long __stdcall AddExtraBox();
};

}
