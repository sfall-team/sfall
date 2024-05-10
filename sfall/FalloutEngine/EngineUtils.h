/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include <string>
#include <vector>

#include "Functions.h"
#include "Variables.h"

//
// Various utility functions, based on FO engine functions
//

namespace fo
{
namespace util
{

// To safely unlock cache entries after using art_ptr_lock and similar functions
struct ArtCacheLock {
	DWORD entryPtr = 0;

	ArtCacheLock() {}
	ArtCacheLock(DWORD _lock) : entryPtr(_lock) {}
	~ArtCacheLock() {
		if (entryPtr != 0) {
			fo::func::art_ptr_unlock(entryPtr);
			entryPtr = 0;
		}
	}
};

__inline void DisplayPrint(const std::string& str) {
	fo::func::display_print(str.c_str());
}

// rect_free_ function for inline implementation
__forceinline void rect_free(fo::RectList* rect) {
	fo::RectList* front = fo::var::rectList;
	fo::var::rectList = rect;
	rect->nextRect = front;
}

// returns message string from given file or "Error" when not found
const char* GetMessageStr(const fo::MessageList* file, long messageId);

// similar to GetMessageStr, but returns nullptr when no message is found
const char* MessageSearch(const fo::MessageList* file, long messageId);

fo::MessageNode* GetMsgNode(fo::MessageList* msgList, int msgNum);

char* GetMsg(fo::MessageList* msgList, int msgNum, int msgType);

fo::Window* GetWindow(long winID);

fo::Queue* QueueFind(fo::GameObject* object, long type);

// returns weapon animation code
long AnimCodeByWeapon(fo::GameObject* weapon);

bool CheckProtoID(DWORD pid);

// returns False if the prototype does not exist, or pointer to prototype by PID in the outProto argument
bool GetProto(long pid, fo::Proto** outProto);

// returns pointer to prototype by PID
// Note: use this function if you need to get the proto immediately without extra checks
__forceinline fo::Proto* GetProto(long pid) {
	fo::Proto* proto;
	fo::func::proto_ptr(pid, &proto);
	return proto;
}

bool CritterCopyProto(long pid, long* &proto_dst);

// wrapper for skill_get_tags with bounds checking
void SkillGetTags(long* result, long num);

// wrapper for skill_set_tags with bounds checking
void SkillSetTags(long* tags, long num);

long GetItemType(fo::GameObject* item);

__declspec(noinline) fo::GameObject* GetItemPtrSlot(fo::GameObject* critter, fo::InvenType slot);

fo::AttackType GetHandSlotPrimaryAttack(fo::HandSlot slot);
fo::AttackType GetHandSlotSecondaryAttack(fo::HandSlot slot);
fo::HandSlotMode GetHandSlotMode(fo::HandSlot slot);

long& GetActiveItemMode();

fo::GameObject* GetActiveItem();

// Hand: 0 - left, 1 - right
fo::AttackType GetSlotHitMode(fo::HandSlot hand);

long GetCurrentAttackMode();

fo::AttackSubType GetWeaponType(DWORD weaponFlag);

long ObjIsOpenable(fo::GameObject* object);

bool HeroIsFemale();

// Alternative implementation of item_d_check_addict_ engine function with critter argument and returned addict queue data
fo::QueueAddictData* __fastcall CheckAddictByPid(fo::GameObject* critter, long pid);

fo::QueueRadiationData* __fastcall GetRadiationEvent(long type);

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence();

bool IsNpcFlag(fo::GameObject* npc, long flag);

void ToggleNpcFlag(fo::GameObject* npc, long flag, bool set);

// Returns the position of party member in the existing table (begins from 1)
long IsPartyMemberByPid(long pid);

// Returns True if the NPC belongs to the player's potential (set in party.txt) party members (analog of broken isPotentialPartyMember_)
bool IsPartyMember(fo::GameObject* critter);

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid);

// Returns window by x/y coordinate (hidden windows are ignored)
fo::Window* __fastcall GetTopWindowAtPos(long xPos, long yPos, bool bypassTrans = false);

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<fo::GameObject*> &objs, long sourceTile, long radius, long elev, long type = -1);

// Checks the blocking tiles and returns the first blocking object
fo::GameObject* CheckAroundBlockingTiles(fo::GameObject* source, long dstTile);

fo::GameObject* __fastcall MultiHexMoveIsBlocking(fo::GameObject* source, long dstTile);

long wmGetTerrainType(long xPos, long yPos);

long wmGetCurrentTerrainType();

void SurfaceCopyToMem(long fromX, long fromY, long width, long height, long fromWidth, BYTE* fromSurface, BYTE* toMem);

void DrawToSurface(long toX, long toY, long width, long height, long toWidth, long toHeight, BYTE* toSurface, BYTE* fromMem);

void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf, long toX, long toY, long toWidth, long toHeight, BYTE* toSurf, int maskRef);

void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf, long toX, long toY, long toWidth, long toHeight, BYTE* toSurf);

void TranslucentDarkFill(BYTE* surface, long x, long y, long width, long height, long surfWidth);

// Fills the specified interface window with index color
bool WinFillRect(long winID, long x, long y, long width, long height, BYTE indexColor);

void FillRect(BYTE* surface, long x, long y, long width, long height, long wPitch, BYTE indexColor);

// Fills the specified interface window with index color 0 (black color)
void ClearWindow(long winID, bool refresh = true);

void PrintFloatText(fo::GameObject* object, const char* text, long colorText, long colorOutline = 207, long font = 101);

// Print text to surface
void PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface);
void PrintTextFM(const char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface);

// gets the height of the currently selected font
DWORD GetTextHeight();

// gets the length of a string using the currently selected font
DWORD GetTextWidth(const char* textMsg);
DWORD GetTextWidthFM(const char* textMsg);

// get width of Char for current font
DWORD GetCharWidth(BYTE charVal);
DWORD GetCharWidthFM(BYTE charVal);

// get maximum string length for current font - if all characters were maximum width
DWORD GetMaxTextWidth(const char* textMsg);

// get number of pixels between characters for current font
DWORD GetCharGapWidth();

// get maximum character width for current font
DWORD GetMaxCharWidth();

// Redraw the given object on screen (does not always redraws the whole object)
void RedrawObject(fo::GameObject* obj);

// Redraws all windows
void RefreshGNW(bool skipOwner = false);

}
}
