/*
*    sfall
*    Copyright (C) 2008-2016  The sfall team
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

extern char DefaultMaleModelName[65];
extern char DefaultFemaleModelName[65];

void ApplyCombatProcFix();

void ApplySpeedPatch();

void ApplyInputPatch();

void ApplyGraphicsPatch();

void ApplyPlayerModelPatches();

void ApplyDebugModePatch();

void ApplyNPCAutoLevelPatch();

void ApplyAdditionalWeaponAnimsPatch();

void ApplySkilldexImagesPatch();

void ApplyKarmaFRMsPatch();

void ApplySpeedInterfaceCounterAnimsPatch();

void ApplyScienceOnCrittersPatch();

void ApplyFashShotTraitFix();

void ApplyBoostScriptDialogLimitPatch();

void ApplyNumbersInDialoguePatch();

void ApplyInstantWeaponEquipPatch();

void ApplyDisplayKarmaChangesPatch();

void ApplyMultiPatchesPatch();

void ApplyPlayIdleAnimOnReloadPatch();

void ApplyCorpseLineOfFireFix();

void ApplyNpcExtraApPatch();

void ApplyNpcStage6Fix();

void ApplyMotionScannerFlagsPatch();

void ApplyEncounterTableSizePatch();

void ApplyObjCanSeeShootThroughPatch();

void ApplyOverrideMusicDirPatch();

void ApplyMiscPatches();

void MiscReset();

void _stdcall SetMapMulti(float d);