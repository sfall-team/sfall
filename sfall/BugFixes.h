#pragma once

void BugFixes_Init();
void BugFixes_OnGameLoad();
void BugFixes_Initialization();

void BugFixes_DrugsSaveFix(HANDLE file);
bool BugFixes_DrugsLoadFix(HANDLE file);
void ResetBodyState();

extern int tagSkill4LevelBase;
