#pragma once

namespace sfall
{

class DebugEditor {
public:
	static const char* name() { return "Debug Editor"; }
	static void init();

	static void OnAfterGameInit();

	static void KeyPressedHook(DWORD scanCode, bool pressed);
};

}
