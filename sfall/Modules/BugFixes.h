#pragma once

#include "Module.h"

namespace sfall
{

class BugFixes : public Module {
public:
	const char* name() { return "BugFixes"; }
	void init();
};

extern int tagSkill4LevelBase;

}
