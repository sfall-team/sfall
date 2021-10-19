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

#include <cstdint>

#include "FunctionOffsets.h"
#include "Structs.h"
#include "VariableOffsets.h"

#include "EngineUtils.h"

namespace fo
{
namespace util
{

static MSGNode messageBuf;

const char* GetMessageStr(const MSGList* file, long messageId) {
	return fo::func::getmsg(file, &messageBuf, messageId);
}

const char* MessageSearch(const MSGList* file, long messageId) {
	messageBuf.number = messageId;
	if (fo::func::message_search(file, &messageBuf) == 1) {
		return messageBuf.message;
	}
	return nullptr;
}

MSGNode* GetMsgNode(MSGList* msgList, int msgNum) {
	if (msgList != nullptr && msgList->numMsgs > 0) {
		MSGNode *msgNode = msgList->nodes;
		long last = msgList->numMsgs - 1;
		long first = 0;
		long mid;

		// Use Binary Search to find msg
		while (first <= last) {
			mid = (first + last) / 2;
			if (msgNum > msgNode[mid].number)
				first = mid + 1;
			else if (msgNum < msgNode[mid].number)
				last = mid - 1;
			else
				return &msgNode[mid];
		}
	}
	return nullptr;
}

// Alternative version of getmsg_ function
char* GetMsg(MSGList* msgList, int msgNum, int msgType) {
	MSGNode *msgNode = GetMsgNode(msgList, msgNum);
	if (msgNode) {
		if (msgType == 2) {
			return msgNode->message;
		} else if (msgType == 1) {
			return msgNode->audio;
		}
	}
	return nullptr;
}

Queue* QueueFind(TGameObj* object, long type) {
	if (*fo::ptr::queue) {
		Queue* queue = *fo::ptr::queue;
		while (queue->object != object && queue->type != type) {
			queue = queue->next;
			if (!queue) break;
		}
		return queue;
	}
	return nullptr;
}

long AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		sProto* proto;
		if (GetProto(weapon->protoId, &proto) && proto->item.type == item_type_weapon) {
			return proto->item.weapon.animationCode;
		}
	}
	return 0;
}

bool CheckProtoID(DWORD pid) {
	if (pid == 0) return false;
	long type = pid >> 24;
	if (type > OBJ_TYPE_MISC) return false;
	return (static_cast<long>(pid & 0xFFFF) < fo::ptr::protoLists[type].totalCount);
}

bool GetProto(long pid, sProto** outProto) {
	return (fo::func::proto_ptr(pid, outProto) != -1);
}

void SkillGetTags(long* result, long num) {
	if (num > 4) num = 4;
	fo::func::skill_get_tags(result, num);
}

void SkillSetTags(long* tags, long num) {
	if (num > 4) num = 4;
	fo::func::skill_set_tags(tags, num);
}

long GetItemType(TGameObj* item) {
	return GetProto(item->protoId)->item.type;
}

__declspec(noinline) TGameObj* __stdcall GetItemPtrSlot(TGameObj* critter, InvenType slot) {
	TGameObj* itemPtr = nullptr;
	switch (slot) {
	case INVEN_TYPE_LEFT_HAND:
		itemPtr = fo::func::inven_left_hand(critter);
		break;
	case INVEN_TYPE_RIGHT_HAND:
		itemPtr = fo::func::inven_right_hand(critter);
		break;
	case INVEN_TYPE_WORN:
		itemPtr = fo::func::inven_worn(critter);
		break;
	}
	return itemPtr;
}

AttackType GetHandSlotPrimaryAttack(HandSlot slot) {
	return (AttackType)fo::ptr::itemButtonItems[slot].primaryAttack;
}

AttackType GetHandSlotSecondaryAttack(HandSlot slot) {
	return (AttackType)fo::ptr::itemButtonItems[slot].secondaryAttack;
}

HandSlotMode GetHandSlotMode(HandSlot slot) {
	return (HandSlotMode)fo::ptr::itemButtonItems[slot].mode;
}

long& GetActiveItemMode() {
	return fo::ptr::itemButtonItems[*fo::ptr::itemCurrentItem].mode;
}

TGameObj* GetActiveItem() {
	return fo::ptr::itemButtonItems[*fo::ptr::itemCurrentItem].item;
}

AttackType GetSlotHitMode(HandSlot hand) { // 0 - left, 1 - right
	switch (fo::ptr::itemButtonItems[hand].mode) {
	case HANDMODE_Primary:
	case HANDMODE_Primary_Aimed: // called shot
		return GetHandSlotPrimaryAttack(hand);
	case HANDMODE_Secondary:
	case HANDMODE_Secondary_Aimed: // called shot
		return GetHandSlotSecondaryAttack(hand);
	case HANDMODE_Reload:
		return (AttackType)(ATKTYPE_LWEAPON_RELOAD + hand);
	}
	return ATKTYPE_PUNCH;
}

long GetCurrentAttackMode() {
	if (*fo::ptr::interfaceWindow != -1) {
		return GetSlotHitMode((HandSlot)*fo::ptr::itemCurrentItem);
	}
	return -1;
}

AttackSubType GetWeaponType(DWORD weaponFlag) {
	static const AttackSubType weapon_types[9] = {
		ATKSUBTYPE_NONE,
		ATKSUBTYPE_UNARMED,
		ATKSUBTYPE_UNARMED,
		ATKSUBTYPE_MELEE,
		ATKSUBTYPE_MELEE,
		ATKSUBTYPE_THROWING,
		ATKSUBTYPE_GUNS,
		ATKSUBTYPE_GUNS,
		ATKSUBTYPE_GUNS
	};
	DWORD type = weaponFlag & 0xF;
	return (type < 9) ? weapon_types[type] : ATKSUBTYPE_NONE;
}

long ObjIsOpenable(TGameObj* object) {
	long result = 0;
	if (fo::func::obj_is_openable(object)) {
		DWORD lock;
		FrmHeaderData* frm = fo::func::art_ptr_lock(object->artFid, &lock);
		if (frm) {
			if (frm->numFrames > 1) result = 1;
			fo::func::art_ptr_unlock(lock);
		}
	}
	return result;
}

bool HeroIsFemale() {
	return (fo::func::stat_level(*fo::ptr::obj_dude, STAT_gender) == GENDER_FEMALE);
}

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence() {
	QueueRadiationData* queue = (QueueRadiationData*)fo::func::queue_find_first(*fo::ptr::obj_dude, radiation_event);
	while (queue) {
		if (queue->init && queue->level >= 2) return 1;
		queue = (QueueRadiationData*)fo::func::queue_find_next(*fo::ptr::obj_dude, radiation_event);
	}
	return 0;
}

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid) {
	TScript* script = nullptr;
	fo::func::scr_ptr(sid, &script);
	return (script) ? script->numLocalVars : 0;
}

// Returns window by x/y coordinate (hidden windows are ignored)
WINinfo* __fastcall GetTopWindowAtPos(long xPos, long yPos, bool bypassTrans) {
	long num = *fo::ptr::num_windows - 1;
	if (num) {
		int cflags = WinFlags::Hidden;
		if (bypassTrans) cflags |= WinFlags::Transparent;
		do {
			WINinfo* win = fo::ptr::window[num];
			if (xPos >= win->wRect.left && xPos <= win->wRect.right && yPos >= win->wRect.top && yPos <= win->wRect.bottom) {
				if (!(win->flags & cflags)) {
					return win;
				}
			}
		} while (--num);
	}
	return fo::ptr::window[0];
}

static long GetRangeTileNumbers(long sourceTile, long radius, long &outEnd) {
	long hexRadius = 200 * (radius + 1);

	outEnd = sourceTile + hexRadius;
	if (outEnd > 40000) outEnd = 40000;

	long startTile = sourceTile - hexRadius;
	return (startTile < 0) ? 0 : startTile;
}

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<TGameObj*> &objs, long sourceTile, long radius, long elev, long type) {
	long endTile;
	for (long tile = GetRangeTileNumbers(sourceTile, radius, endTile); tile < endTile; tile++) {
		TGameObj* obj = fo::func::obj_find_first_at_tile(elev, tile);
		while (obj) {
			if (type == -1 || type == obj->Type()) {
				bool multiHex = (obj->flags & ObjectFlag::MultiHex) ? true : false;
				if (fo::func::tile_dist(sourceTile, obj->tile) <= (radius + multiHex)) {
					objs.push_back(obj);
				}
			}
			obj = fo::func::obj_find_next_at_tile();
		}
	}
}

// Checks the blocking tiles and returns the first blocking object
TGameObj* CheckAroundBlockingTiles(TGameObj* source, long dstTile) {
	long rotation = 5;
	do {
		long chkTile = fo::func::tile_num_in_direction(dstTile, rotation, 1);
		TGameObj* obj = fo::func::obj_blocking_at(source, chkTile, source->elevation);
		if (obj) return obj;
	} while (--rotation >= 0);

	return nullptr;
}

TGameObj* __fastcall MultiHexMoveIsBlocking(TGameObj* source, long dstTile) {
	if (fo::func::tile_dist(source->tile, dstTile) > 1) {
		return CheckAroundBlockingTiles(source, dstTile);
	}
	// Checks the blocking arc of adjacent tiles
	long dir = fo::func::tile_dir(source->tile, dstTile);

	long chkTile = fo::func::tile_num_in_direction(dstTile, dir, 1);
	TGameObj* obj = fo::func::obj_blocking_at(source, chkTile, source->elevation);
	if (obj) return obj;

	// +1 direction
	long rotation = (dir + 1) % 6;
	chkTile = fo::func::tile_num_in_direction(dstTile, rotation, 1);
	obj = fo::func::obj_blocking_at(source, chkTile, source->elevation);
	if (obj) return obj;

	// -1 direction
	rotation = (dir + 5) % 6;
	chkTile = fo::func::tile_num_in_direction(dstTile, rotation, 1);
	obj = fo::func::obj_blocking_at(source, chkTile, source->elevation);
	if (obj) return obj;

	return nullptr;
}

// Returns the type of the terrain sub tile at the the player's position on the world map
long wmGetCurrentTerrainType() {
	long* terrainId = *(long**)FO_VAR_world_subtile;
	if (terrainId == nullptr) {
		__asm {
			lea  ebx, terrainId;
			mov  edx, dword ptr ds:[FO_VAR_world_ypos];
			mov  eax, dword ptr ds:[FO_VAR_world_xpos];
			call fo::funcoffs::wmFindCurSubTileFromPos_;
		}
	}
	return *terrainId;
}

//---------------------------------------------------------
// copy the area from the interface buffer to the data array
void SurfaceCopyToMem(long fromX, long fromY, long width, long height, long fromWidth, BYTE* fromSurface, BYTE* toMem) {
	fromSurface += fromY * fromWidth + fromX;
	for (long i = 0, h = 0; h < height; h++, i += width) {
		std::memcpy(&toMem[i], fromSurface, width);
		fromSurface += fromWidth;
	}
}

// safe copy data from memory to the area of the interface buffer
void DrawToSurface(long toX, long toY, long width, long height, long toWidth, long toHeight, BYTE* toSurface, BYTE* fromMem) {
	BYTE* _toSurface = toSurface + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurface;
	long i = 0;
	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurface + w > endToSurf) return;
			if (_toSurface >= toSurface) _toSurface[w] = fromMem[i++];
		}
		_toSurface += toWidth;
	}
}

// safe copy data from surface to surface with mask
void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf,
                   long toX, long toY, long toWidth, long toHeight, BYTE* toSurf, int maskRef)
{
	BYTE* _fromSurf = fromSurf + (fromY * fromWidth + fromX);
	BYTE* _toSurf =  toSurf + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurf;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurf + w > endToSurf) return;
			if (_toSurf >= toSurf && _fromSurf[w] != maskRef) _toSurf[w] = _fromSurf[w];
		}
		_fromSurf += fromWidth;
		_toSurf += toWidth;
	}
}

// safe copy data from surface to surface
void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf,
                   long toX, long toY, long toWidth, long toHeight, BYTE* toSurf)
{
	BYTE* _fromSurf = fromSurf + (fromY * fromWidth + fromX);
	BYTE* _toSurf = toSurf + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurf;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurf + w > endToSurf) return;
			if (_toSurf >= toSurf) _toSurf[w] = _fromSurf[w];
		}
		_fromSurf += fromWidth;
		_toSurf += toWidth;
	}
}

//void TranslucentDarkFill(BYTE* surface, long x, long y, long width, long height, long surfWidth) {
//	BYTE* surf = surface + (y * surfWidth) + x;
//	fo::func::wmInterfaceDrawSubTileRectFogged(surf, width, height, surfWidth);
//}

// Fills the specified interface window with index color
bool __stdcall WinFillRect(long winID, long x, long y, long width, long height, BYTE indexColor) {
	WINinfo* win = fo::func::GNW_find(winID);
	bool result = false;
	if ((x + width) > win->width) {
		width = win->width - x;
		result = true;
	}
	if ((y + height) > win->height) {
		height = win->height - y;
		result = true;
	}
	BYTE* surf = win->surface + (win->width * y) + x;
	long pitch = win->width - width;
	while (height--) {
		long w = width;
		while (w--) *surf++ = indexColor;
		surf += pitch;
	};
	return result;
}

// Fills the specified interface window with index color 0 (black color)
void ClearWindow(long winID, bool refresh) {
	WINinfo* win = fo::func::GNW_find(winID);
	std::memset(win->surface, 0, win->width * win->height);
	if (refresh) {
		fo::func::GNW_win_refresh(win, &win->rect, nullptr);
	}
}

//---------------------------------------------------------
void PrintFloatText(TGameObj* object, const char* text, long colorText, long colorOutline, long font) {
	BoundRect rect;
	if (!fo::func::text_object_create(object, text, font, colorText, colorOutline, &rect)) {
		fo::func::tile_refresh_rect(&rect, object->elevation);
	}
}

// print text to surface
void __stdcall PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = yPos * toWidth + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call dword ptr ds:[FO_VAR_text_to_buf];
	}
}

void __stdcall PrintTextFM(const char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = yPos * toWidth + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call fo::funcoffs::FMtext_to_buf_;
	}
}

//---------------------------------------------------------
//gets the height of the currently selected font
DWORD __stdcall GetTextHeight() {
//	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[FO_VAR_text_height]; //get text height
//		mov  TxtHeight, eax;
	}
//	return TxtHeight;
}

//---------------------------------------------------------
//gets the length of a string using the currently selected font
DWORD __stdcall GetTextWidth(const char* TextMsg) {
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[FO_VAR_text_width]; //get text width
	}
}

DWORD __stdcall GetTextWidthFM(const char* TextMsg) {
	return fo::func::FMtext_width(TextMsg); //get text width
}

//---------------------------------------------------------
//get width of Char for current font
DWORD __stdcall GetCharWidth(BYTE charVal) {
	__asm {
		mov  al, charVal;
		call dword ptr ds:[FO_VAR_text_char_width];
	}
}

DWORD __stdcall GetCharWidthFM(BYTE charVal) {
	__asm {
		mov  al, charVal;
		call fo::funcoffs::FMtext_char_width_;
	}
}

//---------------------------------------------------------
//get maximum string length for current font - if all characters were maximum width
DWORD __stdcall GetMaxTextWidth(const char* TextMsg) {
//	DWORD msgWidth;
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[FO_VAR_text_mono_width];
//		mov  msgWidth, eax;
	}
//	return msgWidth;
}

//---------------------------------------------------------
//get number of pixels between characters for current font
DWORD __stdcall GetCharGapWidth() {
//	DWORD gapWidth;
	__asm {
		call dword ptr ds:[FO_VAR_text_spacing];
//		mov  gapWidth, eax;
	}
//	return gapWidth;
}

//---------------------------------------------------------
//get maximum character width for current font
DWORD __stdcall GetMaxCharWidth() {
//	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[FO_VAR_text_max];
//		mov  charWidth, eax;
	}
//	return charWidth;
}

void RedrawObject(TGameObj* obj) {
	BoundRect rect;
	fo::func::obj_bound(obj, &rect);
	fo::func::tile_refresh_rect(&rect, obj->elevation);
}

// Redraws all windows
void RefreshGNW(bool skipOwner) {
	fo::var::setInt(FO_VAR_doing_refresh_all) = 1;
	for (size_t i = 0; i < *fo::ptr::num_windows; i++) {
		if (skipOwner && fo::ptr::window[i]->flags & WinFlags::OwnerFlag) continue;
		fo::func::GNW_win_refresh(fo::ptr::window[i], fo::ptr::scr_size, 0);
	}
	fo::var::setInt(FO_VAR_doing_refresh_all) = 0;
}

//////////////////////////// UNLISTED FRM FUNCTIONS ////////////////////////////

static bool LoadFrmHeader(UNLSTDfrm *frmHeader, DbFile* frmStream) {
	if (fo::func::db_freadInt(frmStream, &frmHeader->version) == -1)
		return false;
	else if (fo::func::db_freadShort(frmStream, &frmHeader->FPS) == -1)
		return false;
	else if (fo::func::db_freadShort(frmStream, &frmHeader->actionFrame) == -1)
		return false;
	else if (fo::func::db_freadShort(frmStream, &frmHeader->numFrames) == -1)
		return false;
	else if (fo::func::db_freadShortCount(frmStream, frmHeader->xCentreShift, 6) == -1)
		return false;
	else if (fo::func::db_freadShortCount(frmStream, frmHeader->yCentreShift, 6) == -1)
		return false;
	else if (fo::func::db_freadIntCount(frmStream, frmHeader->oriOffset, 6) == -1)
		return false;
	else if (fo::func::db_freadInt(frmStream, &frmHeader->frameAreaSize) == -1)
		return false;

	return true;
}

static bool LoadFrmFrame(UNLSTDfrm::Frame *frame, DbFile* frmStream) {
	//FRMframe *frameHeader = (FRMframe*)frameMEM;
	//BYTE* frameBuff = frame + sizeof(FRMframe);

	if (fo::func::db_freadShort(frmStream, &frame->width) == -1)
		return false;
	else if (fo::func::db_freadShort(frmStream, &frame->height) == -1)
		return false;
	else if (fo::func::db_freadInt(frmStream, &frame->size) == -1)
		return false;
	else if (fo::func::db_freadShort(frmStream, &frame->x) == -1)
		return false;
	else if (fo::func::db_freadShort(frmStream, &frame->y) == -1)
		return false;

	frame->indexBuff = new BYTE[frame->size];
	if (fo::func::db_fread(frame->indexBuff, 1, frame->size, frmStream) != frame->size)
		return false;

	return true;
}

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef) {
	if (folderRef > OBJ_TYPE_SKILLDEX) return nullptr;

	const char *artfolder = fo::ptr::art[folderRef].path; // address of art type name
	char frmPath[MAX_PATH];

	if (*fo::ptr::use_language) {
		sprintf_s(frmPath, MAX_PATH, "art\\%s\\%s\\%s", (const char*)fo::ptr::language, artfolder, frmName);
	} else {
		sprintf_s(frmPath, MAX_PATH, "art\\%s\\%s", artfolder, frmName);
	}

	UNLSTDfrm *frm = new UNLSTDfrm;

	DbFile* frmStream = fo::func::db_fopen(frmPath, "rb");

	if (!frmStream && *fo::ptr::use_language) {
		sprintf_s(frmPath, MAX_PATH, "art\\%s\\%s", artfolder, frmName);
		frmStream = fo::func::db_fopen(frmPath, "rb");
	}

	if (frmStream != nullptr) {
		if (!LoadFrmHeader(frm, frmStream)) {
			fo::func::db_fclose(frmStream);
			delete frm;
			return nullptr;
		}

		DWORD oriOffset_1st = frm->oriOffset[0];
		DWORD oriOffset_new = 0;
		frm->frames = new UNLSTDfrm::Frame[6 * frm->numFrames];
		for (int ori = 0; ori < 6; ori++) {
			if (ori == 0 || frm->oriOffset[ori] != oriOffset_1st) {
				frm->oriOffset[ori] = oriOffset_new;
				for (int fNum = 0; fNum < frm->numFrames; fNum++) {
					if (!LoadFrmFrame(&frm->frames[oriOffset_new + fNum], frmStream)) {
						fo::func::db_fclose(frmStream);
						delete frm;
						return nullptr;
					}
				}
				oriOffset_new += frm->numFrames;
			} else {
				frm->oriOffset[ori] = 0;
			}
		}

		fo::func::db_fclose(frmStream);
	} else {
		delete frm;
		return nullptr;
	}
	return frm;
}

}
}
