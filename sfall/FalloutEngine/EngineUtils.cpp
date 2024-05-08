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

#include <cstdint>

#include "FunctionOffsets.h"
#include "Structs.h"
#include "VariableOffsets.h"

#include "EngineUtils.h"

// TODO: split these functions into several files
namespace fo
{
namespace util
{

static fo::MessageNode messageBuf;

const char* GetMessageStr(const fo::MessageList* file, long messageId) {
	return fo::func::getmsg(file, &messageBuf, messageId);
}

const char* MessageSearch(const fo::MessageList* file, long messageId) {
	messageBuf.number = messageId;
	if (fo::func::message_search(file, &messageBuf) == 1) {
		return messageBuf.message;
	}
	return nullptr;
}

fo::MessageNode* GetMsgNode(fo::MessageList* msgList, int msgNum) {
	if (msgList != nullptr && msgList->numMsgs > 0) {
		fo::MessageNode *msgNode = msgList->nodes;
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
char* GetMsg(fo::MessageList* msgList, int msgNum, int msgType) {
	fo::MessageNode *msgNode = GetMsgNode(msgList, msgNum);
	if (msgNode) {
		if (msgType == 2) {
			return msgNode->message;
		} else if (msgType == 1) {
			return msgNode->audio;
		}
	}
	return nullptr;
}

fo::Window* GetWindow(long winID) {
	return fo::var::window[fo::var::window_index[winID]];
}

fo::Queue* QueueFind(fo::GameObject* object, long type) {
	if (fo::var::queue) {
		fo::Queue* queue = fo::var::queue;
		while (queue->object != object && queue->type != type) {
			queue = queue->next;
			if (!queue) break;
		}
		return queue;
	}
	return nullptr;
}

long AnimCodeByWeapon(fo::GameObject* weapon) {
	if (weapon != nullptr) {
		fo::Proto* proto;
		if (GetProto(weapon->protoId, &proto) && proto->item.type == fo::item_type_weapon) {
			return proto->item.weapon.animationCode;
		}
	}
	return 0;
}

bool CheckProtoID(DWORD pid) {
	if (pid == 0) return false;
	long type = pid >> 24;
	if (type > fo::ObjType::OBJ_TYPE_MISC) return false;
	return (static_cast<long>(pid & 0xFFFF) < fo::var::protoLists[type].totalCount);
}

bool GetProto(long pid, fo::Proto** outProto) {
	return (fo::func::proto_ptr(pid, outProto) != -1);
}

bool CritterCopyProto(long pid, long* &proto_dst) {
	fo::Proto* proto;
	bool result = GetProto(pid, &proto);
	if (result) {
		proto_dst = reinterpret_cast<long*>(new int32_t[104]);
		std::memcpy(proto_dst, proto, 416);
	} else {
		proto_dst = nullptr;
	}
	return result;
}

void SkillGetTags(long* result, long num) {
	if (num > 4) num = 4;
	fo::func::skill_get_tags(result, num);
}

void SkillSetTags(long* tags, long num) {
	if (num > 4) num = 4;
	fo::func::skill_set_tags(tags, num);
}

long GetItemType(fo::GameObject* item) {
	return GetProto(item->protoId)->item.type;
}

__declspec(noinline) fo::GameObject* GetItemPtrSlot(fo::GameObject* critter, fo::InvenType slot) {
	fo::GameObject* itemPtr = nullptr;
	switch (slot) {
	case fo::InvenType::INVEN_TYPE_LEFT_HAND:
		itemPtr = fo::func::inven_left_hand(critter);
		break;
	case fo::InvenType::INVEN_TYPE_RIGHT_HAND:
		itemPtr = fo::func::inven_right_hand(critter);
		break;
	case fo::InvenType::INVEN_TYPE_WORN:
		itemPtr = fo::func::inven_worn(critter);
		break;
	}
	return itemPtr;
}

fo::AttackType GetHandSlotPrimaryAttack(fo::HandSlot slot) {
	return (fo::AttackType)fo::var::itemButtonItems[slot].primaryAttack;
}

fo::AttackType GetHandSlotSecondaryAttack(fo::HandSlot slot) {
	return (fo::AttackType)fo::var::itemButtonItems[slot].secondaryAttack;
}

fo::HandSlotMode GetHandSlotMode(fo::HandSlot slot) {
	return (fo::HandSlotMode)fo::var::itemButtonItems[slot].mode;
}

long& GetActiveItemMode() {
	return fo::var::itemButtonItems[fo::var::itemCurrentItem].mode;
}

fo::GameObject* GetActiveItem() {
	return fo::var::itemButtonItems[fo::var::itemCurrentItem].item;
}

fo::AttackType GetSlotHitMode(fo::HandSlot hand) { // 0 - left, 1 - right
	switch (fo::var::itemButtonItems[hand].mode) {
	case fo::HandSlotMode::Primary:
	case fo::HandSlotMode::Primary_Aimed: // called shot
		return GetHandSlotPrimaryAttack(hand);
	case fo::HandSlotMode::Secondary:
	case fo::HandSlotMode::Secondary_Aimed: // called shot
		return GetHandSlotSecondaryAttack(hand);
	case fo::HandSlotMode::Reload:
		return (fo::AttackType)(fo::AttackType::ATKTYPE_LWEAPON_RELOAD + hand);
	}
	return fo::AttackType::ATKTYPE_PUNCH;
}

long GetCurrentAttackMode() {
	if (fo::var::interfaceWindow != -1) {
		return GetSlotHitMode((fo::HandSlot)fo::var::itemCurrentItem);
	}
	return -1;
}

fo::AttackSubType GetWeaponType(DWORD weaponFlag) {
	static const fo::AttackSubType weapon_types[9] = {
		fo::AttackSubType::NONE,
		fo::AttackSubType::UNARMED,
		fo::AttackSubType::UNARMED,
		fo::AttackSubType::MELEE,
		fo::AttackSubType::MELEE,
		fo::AttackSubType::THROWING,
		fo::AttackSubType::RANGED,
		fo::AttackSubType::RANGED,
		fo::AttackSubType::RANGED
	};
	DWORD type = weaponFlag & 0xF;
	return (type < 9) ? weapon_types[type] : fo::AttackSubType::NONE;
}
/*
// Returns the maximum distance a critter can move in combat based on current APs
long __fastcall GetCombatMoveMaxDist(fo::GameObject* critter, long freeMove) {
	freeMove += critter->critter.movePoints;
	long flags = critter->critter.damageFlags & (fo::DamageFlag::DAM_CRIP_LEG_LEFT | fo::DamageFlag::DAM_CRIP_LEG_RIGHT);

	// both legs crippled (8 AP per hex)
	if (flags == (fo::DamageFlag::DAM_CRIP_LEG_LEFT | fo::DamageFlag::DAM_CRIP_LEG_RIGHT)) return (freeMove / 8);

	// not crippled
	if (flags == 0) return freeMove;

	// one leg crippled (4 AP per hex)
	return (freeMove / 4);
}
*/
long ObjIsOpenable(fo::GameObject* object) {
	long result = 0;
	if (fo::func::obj_is_openable(object)) {
		DWORD lock;
		fo::FrmFile* frm = fo::func::art_ptr_lock(object->artFid, &lock);
		if (frm) {
			if (frm->numFrames > 1) result = 1;
			fo::func::art_ptr_unlock(lock);
		}
	}
	return result;
}

bool HeroIsFemale() {
	return (fo::func::stat_level(fo::var::obj_dude, fo::Stat::STAT_gender) == fo::Gender::GENDER_FEMALE);
}

// Alternative implementation of item_d_check_addict_ engine function with critter argument and returned addict queue data
fo::QueueAddictData* __fastcall CheckAddictByPid(fo::GameObject* critter, long pid) {
	if (pid == -1) {
		// return queue of player addiction
		return (fo::QueueAddictData*)fo::func::queue_find_first(fo::var::obj_dude, fo::QueueType::addict_event);
	}

	fo::QueueAddictData* queue = (fo::QueueAddictData*)fo::func::queue_find_first(critter, fo::QueueType::addict_event);
	while (queue && queue->drugPid != pid) {
		queue = (fo::QueueAddictData*)fo::func::queue_find_next(critter, fo::QueueType::addict_event);
	}
	return queue; // return null or pointer to queue_addict
}

fo::QueueRadiationData* __fastcall GetRadiationEvent(long type) {
	fo::QueueRadiationData* queue = (fo::QueueRadiationData*)fo::func::queue_find_first(fo::var::obj_dude, fo::radiation_event);
	while (queue) {
		if (queue->init == type) return queue;
		queue = (fo::QueueRadiationData*)fo::func::queue_find_next(fo::var::obj_dude, fo::radiation_event);
	}
	return nullptr;
}

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence() {
	fo::QueueRadiationData* queue = (fo::QueueRadiationData*)fo::func::queue_find_first(fo::var::obj_dude, fo::radiation_event);
	while (queue) {
		if (queue->init && queue->level >= 2) return 1;
		queue = (fo::QueueRadiationData*)fo::func::queue_find_next(fo::var::obj_dude, fo::radiation_event);
	}
	return 0;
}

bool IsNpcFlag(fo::GameObject* npc, long flag) {
	fo::Proto* proto;
	if (GetProto(npc->protoId, &proto)) {
		return (proto->critter.critterFlags & (1 << flag)) != 0;
	}
	return false;
}

void ToggleNpcFlag(fo::GameObject* npc, long flag, bool set) {
	fo::Proto* proto;
	if (GetProto(npc->protoId, &proto)) {
		long bit = (1 << flag);
		if (set) {
			proto->critter.critterFlags |= bit;
		} else {
			proto->critter.critterFlags &= ~bit;
		}
	}
}

// Returns the position of party member in the existing table (1 is added to the index position)
long IsPartyMemberByPid(long pid) {
	size_t partyCount = fo::var::partyMemberMaxCount;
	if (partyCount) {
		DWORD* memberPids = fo::var::partyMemberPidList; // pids from party.txt
		for (size_t i = 0; i < partyCount; i++) {
			if (memberPids[i] == pid) return i + 1;
		}
	}
	return 0;
}

// Returns True if the NPC belongs to the player's potential (set in party.txt) party members (analog of broken isPotentialPartyMember_)
bool IsPartyMember(fo::GameObject* critter) {
	if (critter->id < fo::PLAYER_ID) return false;
	return (IsPartyMemberByPid(critter->protoId) > 0);
}

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid) {
	fo::ScriptInstance* script = nullptr;
	fo::func::scr_ptr(sid, &script);
	return (script) ? script->numLocalVars : 0;
}

// Returns window by x/y coordinate (hidden windows are ignored)
fo::Window* __fastcall GetTopWindowAtPos(long xPos, long yPos, bool bypassTrans) {
	long num = fo::var::num_windows - 1;
	if (num) {
		int cflags = fo::WinFlags::Hidden;
		if (bypassTrans) cflags |= fo::WinFlags::Transparent;
		do {
			fo::Window* win = fo::var::window[num];
			if (xPos >= win->wRect.left && xPos <= win->wRect.right && yPos >= win->wRect.top && yPos <= win->wRect.bottom) {
				if (!(win->flags & cflags)) {
					return win;
				}
			}
		} while (--num);
	}
	return fo::var::window[0];
}

static long GetRangeTileNumbers(long sourceTile, long radius, long &outEnd) {
	long hexRadius = 200 * (radius + 1);

	outEnd = sourceTile + hexRadius;
	if (outEnd > 40000) outEnd = 40000;

	long startTile = sourceTile - hexRadius;
	return (startTile < 0) ? 0 : startTile;
}

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<fo::GameObject*> &objs, long sourceTile, long radius, long elev, long type) {
	long endTile;
	for (long tile = GetRangeTileNumbers(sourceTile, radius, endTile); tile < endTile; tile++) {
		fo::GameObject* obj = fo::func::obj_find_first_at_tile(elev, tile);
		while (obj) {
			if (type == -1 || type == obj->Type()) {
				char multiHex = (obj->flags & fo::ObjectFlag::MultiHex) ? 1 : 0;
				if (fo::func::tile_dist(sourceTile, obj->tile) <= (radius + multiHex)) {
					objs.push_back(obj);
				}
			}
			obj = fo::func::obj_find_next_at_tile();
		}
	}
}

// Checks the blocking tiles and returns the first blocking object
fo::GameObject* CheckAroundBlockingTiles(fo::GameObject* source, long dstTile) {
	long rotation = 5;
	do {
		long chkTile = fo::func::tile_num_in_direction(dstTile, rotation, 1);
		fo::GameObject* obj = fo::func::obj_blocking_at(source, chkTile, source->elevation);
		if (obj) return obj;
	} while (--rotation >= 0);

	return nullptr;
}

fo::GameObject* __fastcall MultiHexMoveIsBlocking(fo::GameObject* source, long dstTile) {
	if (fo::func::tile_dist(source->tile, dstTile) > 1) {
		return CheckAroundBlockingTiles(source, dstTile);
	}
	// Checks the blocking arc of adjacent tiles
	long dir = fo::func::tile_dir(source->tile, dstTile);

	long chkTile = fo::func::tile_num_in_direction(dstTile, dir, 1);
	fo::GameObject* obj = fo::func::obj_blocking_at(source, chkTile, source->elevation);
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

// Returns the terrain type of the sub-tile at the specified coordinates on the world map
long wmGetTerrainType(long xPos, long yPos) {
	long* terrainId{};
	__asm {
		lea  ebx, terrainId;
		mov  edx, yPos;
		mov  eax, xPos;
		call fo::funcoffs::wmFindCurSubTileFromPos_;
	}
	return *terrainId;
}

// Returns the terrain type of the sub-tile at the the player's position on the world map
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
// TODO: Review SurfaceCopyToMem/DrawToSurface functions
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

void TranslucentDarkFill(BYTE* surface, long x, long y, long width, long height, long surfWidth) {
	BYTE* surf = surface + (y * surfWidth) + x;
	fo::func::wmInterfaceDrawSubTileRectFogged(surf, width, height, surfWidth);
}

// Fills the specified interface window with index color
bool WinFillRect(long winID, long x, long y, long width, long height, BYTE indexColor) {
	fo::Window* win = fo::func::GNW_find(winID);
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

void FillRect(BYTE* surface, long x, long y, long width, long height, long wPitch, BYTE indexColor) {
	surface += (wPitch * y) + x;
	while (height--) {
		std::memset(surface, indexColor, width);
		surface += wPitch;
	};
}

// Fills the specified interface window with index color 0 (black color)
void ClearWindow(long winID, bool refresh) {
	fo::Window* win = fo::func::GNW_find(winID);
	std::memset(win->surface, 0, win->width * win->height);
	if (refresh) {
		fo::func::GNW_win_refresh(win, &win->rect, nullptr);
	}
}

//---------------------------------------------------------
void PrintFloatText(fo::GameObject* object, const char* text, long colorText, long colorOutline, long font) {
	fo::BoundRect rect;
	if (!fo::func::text_object_create(object, text, font, colorText, colorOutline, &rect)) {
		fo::func::tile_refresh_rect(&rect, object->elevation);
	}
}

// print text to surface
void PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = (yPos * toWidth) + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call dword ptr ds:[FO_VAR_text_to_buf]; // calls FMtext_to_buf_ or GNW_text_to_buf_
	}
}

void PrintTextFM(const char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = (yPos * toWidth) + xPos;
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
DWORD GetTextHeight() {
//	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[FO_VAR_text_height]; //get text height
//		mov  TxtHeight, eax;
	}
//	return TxtHeight;
}

//---------------------------------------------------------
//gets the length of a string using the currently selected font
DWORD GetTextWidth(const char* TextMsg) {
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[FO_VAR_text_width]; //get text width
	}
}

DWORD GetTextWidthFM(const char* TextMsg) {
	return fo::func::FMtext_width(TextMsg); //get text width
}

//---------------------------------------------------------
//get width of Char for current font
DWORD GetCharWidth(BYTE charVal) {
	__asm {
		mov  al, charVal;
		call dword ptr ds:[FO_VAR_text_char_width];
	}
}

DWORD GetCharWidthFM(BYTE charVal) {
	__asm {
		mov  al, charVal;
		call fo::funcoffs::FMtext_char_width_;
	}
}

//---------------------------------------------------------
//get maximum string length for current font - if all characters were maximum width
DWORD GetMaxTextWidth(const char* TextMsg) {
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
DWORD GetCharGapWidth() {
//	DWORD gapWidth;
	__asm {
		call dword ptr ds:[FO_VAR_text_spacing];
//		mov  gapWidth, eax;
	}
//	return gapWidth;
}

//---------------------------------------------------------
//get maximum character width for current font
DWORD GetMaxCharWidth() {
//	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[FO_VAR_text_max];
//		mov  charWidth, eax;
	}
//	return charWidth;
}

void RedrawObject(fo::GameObject* obj) {
	fo::BoundRect rect;
	fo::func::obj_bound(obj, &rect);
	fo::func::tile_refresh_rect(&rect, obj->elevation);
}

// Redraws all windows
void RefreshGNW(bool skipOwner) {
	fo::var::setInt(FO_VAR_doing_refresh_all) = 1;
	for (size_t i = 0; i < fo::var::num_windows; i++) {
		if (skipOwner && fo::var::window[i]->flags & fo::WinFlags::OwnerFlag) continue;
		fo::func::GNW_win_refresh(fo::var::window[i], &fo::var::scr_size, 0);
	}
	fo::var::setInt(FO_VAR_doing_refresh_all) = 0;
}

}
}
