#pragma once

#include "Module.h"

namespace sfall
{

class BugFixes : public Module {
public:
	const char* name() { return "BugFixes"; }
	void init();

	static void DrugsSaveFix(HANDLE file);
	static bool DrugsLoadFix(HANDLE file);
};

extern int tagSkill4LevelBase;

extern void ResetBodyState();

}
