#pragma once

namespace sfall
{

class BugFixes {
public:
	static const char* name() { return "BugFixes"; }
	static void init();

	static void OnGameLoad();
	static void OnBeforeGameInit();
	static void OnAfterGameInit();

	static void DrugsSaveFix(HANDLE file);
	static bool DrugsLoadFix(HANDLE file);
};

extern int tagSkill4LevelBase;

void ResetBodyState();

}
