#pragma once

namespace sfall
{

class Interface {
public:
	static const char* name() { return "Interface"; }
	static void init();
	static void exit();

	static void OnGameLoad();
	static void OnBeforeGameInit();

	static long ActiveInterfaceWID();
	static fo::Window* GetWindow(long winType);
};

}
