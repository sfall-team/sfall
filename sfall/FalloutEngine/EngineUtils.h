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

//#include <string>
#include <vector>

#include "Functions.h"
#include "VarPointers.h"

//
// Various utility functions, based on FO engine functions
//

// rect_free_ function for inline implementation
__forceinline void sf_rect_free(RectList* rect) {
	RectList* front = *ptr_rectList;
	*ptr_rectList = rect;
	rect->nextRect = front;
}

// returns message string from given file or "Error" when not found
const char* GetMessageStr(const MSGList* fileAddr, long messageId);

// similar to GetMessageStr, but returns nullptr when no message is found
const char* MessageSearch(const MSGList* fileAddr, long messageId);

MSGNode* GetMsgNode(MSGList* msgList, int msgNum);

char* GetMsg(MSGList* msgList, int msgNum, int msgType);

Queue* QueueFind(TGameObj* object, long type);

// returns weapon animation code
long AnimCodeByWeapon(TGameObj* weapon);

bool CheckProtoID(DWORD pid);

// returns False if the prototype does not exist, or pointer to prototype by PID in the outProto argument
bool GetProto(long pid, sProto** outProto);

// returns pointer to prototype by PID
// Note: use this function if you need to get the proto immediately without extra checks
__forceinline sProto* GetProto(long pid) {
	sProto* proto;
	fo_proto_ptr(pid, &proto);
	return proto;
}

// wrapper for skill_get_tags with bounds checking
void SkillGetTags(long* result, long num);

// wrapper for skill_set_tags with bounds checking
void SkillSetTags(long* tags, long num);

long GetItemType(TGameObj* item);

__declspec(noinline) TGameObj* __stdcall GetItemPtrSlot(TGameObj* critter, InvenType slot);

AttackType GetHandSlotPrimaryAttack(HandSlot slot);
AttackType GetHandSlotSecondaryAttack(HandSlot slot);
HandSlotMode GetHandSlotMode(HandSlot slot);

long& GetActiveItemMode();

TGameObj* GetActiveItem();

// Hand: 0 - left, 1 - right
AttackType GetSlotHitMode(HandSlot hand);

long GetCurrentAttackMode();

AttackSubType GetWeaponType(DWORD weaponFlag);

long ObjIsOpenable(TGameObj* object);

bool HeroIsFemale();

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence();

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid);

// Returns window by x/y coordinate (hidden windows are ignored)
WINinfo* __fastcall GetTopWindowAtPos(long xPos, long yPos, bool bypassTrans = false);

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<TGameObj*> &objs, long sourceTile, long radius, long elev, long type = -1);

// Checks the blocking tiles and returns the first blocking object
TGameObj* CheckAroundBlockingTiles(TGameObj* source, long dstTile);

TGameObj* __fastcall MultiHexMoveIsBlocking(TGameObj* source, long dstTile);

long wmGetCurrentTerrainType();

void SurfaceCopyToMem(long fromX, long fromY, long width, long height, long fromWidth, BYTE* fromSurface, BYTE* toMem);

void DrawToSurface(long toX, long toY, long width, long height, long toWidth, long toHeight, BYTE* toSurface, BYTE* fromMem);

void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf, long toX, long toY, long toWidth, long toHeight, BYTE* toSurf, int maskRef);

void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf, long toX, long toY, long toWidth, long toHeight, BYTE* toSurf);

// Fills the specified interface window with index color
bool __stdcall WinFillRect(long winID, long x, long y, long width, long height, BYTE indexColor);

// Fills the specified interface window with index color 0 (black color)
void ClearWindow(long winID, bool refresh = true);

void PrintFloatText(TGameObj* object, const char* text, long colorText, long colorOutline = 207, long font = 101);

// Print text to surface
void __stdcall PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface);
void __stdcall PrintTextFM(const char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface);

// gets the height of the currently selected font
DWORD __stdcall GetTextHeight();

// gets the length of a string using the currently selected font
DWORD __stdcall GetTextWidth(const char* textMsg);
DWORD __stdcall GetTextWidthFM(const char* textMsg);

// get width of Char for current font
DWORD __stdcall GetCharWidth(BYTE charVal);
DWORD __stdcall GetCharWidthFM(BYTE charVal);

// get maximum string length for current font - if all characters were maximum width
DWORD __stdcall GetMaxTextWidth(const char* textMsg);

// get number of pixels between characters for current font
DWORD __stdcall GetCharGapWidth();

// get maximum character width for current font
DWORD __stdcall GetMaxCharWidth();

// Redraw the given object on screen (does not always redraws the whole object)
void RedrawObject(TGameObj* obj);

// Redraws all windows
void RefreshGNW(bool skipOwner = false);

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef);
