#pragma once

void BugFixesInit();
void BugFixes_OnGameLoad();
void BugFixes_Initialization();

void DrugsSaveFix(HANDLE file);
bool DrugsLoadFix(HANDLE file);
void ResetBodyState();

extern int tagSkill4LevelBase;
