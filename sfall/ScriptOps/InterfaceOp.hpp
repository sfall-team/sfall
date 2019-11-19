/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "main.h"

#include "HeroAppearance.h"
#include "input.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"

// input_functions
static void __declspec(naked) InputFuncsAvailable() {
	__asm {
		push ecx;
		mov  edx, 1; // They're always available from 2.9 on
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) KeyPressed() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		push ecx;
		push eax;
		call KeyDown;
		mov edx, eax;
		pop ecx;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) funcTapKey() {
	__asm {
		push ecx;
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		cmp  eax, 255;
		jge  end;
		push eax;
		call TapKey;
end:
		pop  ecx;
		retn;
	}
}

//// *** From helios *** ////
static void __declspec(naked) get_mouse_x() {
	__asm {
		push ecx;
		mov  edx, ds:[_mouse_x_];
		add  edx, ds:[_mouse_hotx];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

//Return mouse y position
static void __declspec(naked) get_mouse_y() {
	__asm {
		push ecx;
		mov  edx, ds:[_mouse_y_];
		add  edx, ds:[_mouse_hoty];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

//Return pressed mouse button (1=left, 2=right, 3=left+right, 4=middle)
static void __declspec(naked) get_mouse_buttons() {
	__asm {
		push ecx;
		push edx;
		mov edx, ds:[_last_buttons];
		test edx, edx;
		jnz skip;
		cmp byte ptr MiddleMouseDown, 0;
		jz skip;
		mov edx, 4;
skip:
		mov ecx, eax;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

//Return the window number under the mous
static void __declspec(naked) get_window_under_mouse() {
	__asm {
		push ecx;
		mov  edx, ds:[_last_button_winID];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

//Return screen width
static void __declspec(naked) get_screen_width() {
	__asm {
		push ecx;
		mov  edx, ds:[_scr_size + 8]; // _scr_size.offx
		sub  edx, ds:[_scr_size];     // _scr_size.x
		inc  edx;
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

//Return screen height
static void __declspec(naked) get_screen_height() {
	__asm {
		push ecx;
		mov  edx, ds:[_scr_size + 12]; // _scr_size.offy
		sub  edx, ds:[_scr_size + 4];  // _scr_size.y
		inc  edx;
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

//Stop game, the same effect as open charsscreen or inventory
static void __declspec(naked) stop_game() {
	__asm {
		jmp map_disable_bk_processes_;
	}
}

//Resume the game when it is stopped
static void __declspec(naked) resume_game() {
	__asm {
		jmp map_enable_bk_processes_;
	}
}

static bool dialogShow = false;
static void _stdcall create_message_window2() {
	const ScriptValue &strArg = opHandler.arg(0);
	if (strArg.isString()) {
		if (dialogShow) return;
		const char* str = strArg.strValue();
		if (!str || str[0] == 0) return;
		dialogShow = true;
		DialogOut(str);
		dialogShow = false;
	} else {
		OpcodeInvalidArgs("create_message_window");
	}
}

static void __declspec(naked) create_message_window() {
	_WRAP_OPCODE(create_message_window2, 1, 0)
}

static void __declspec(naked) GetViewportX() {
	__asm {
		push ecx;
		mov  edx, ds:[_wmWorldOffsetX];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) GetViewportY() {
	__asm {
		push ecx;
		mov  edx, ds:[_wmWorldOffsetY];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) SetViewportX() {
	__asm {
		push ecx;
		_GET_ARG_INT(end);
		mov  ds:[_wmWorldOffsetX], eax;
end:
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) SetViewportY() {
	__asm {
		push ecx;
		_GET_ARG_INT(end);
		mov  ds:[_wmWorldOffsetY], eax;
end:
		pop  ecx;
		retn;
	}
}

static void sf_add_iface_tag() {
	int result = AddExtraBox();
	if (result == -1) opHandler.printOpcodeError("add_iface_tag() - cannot add new tag as the maximum limit of 126 tags has been reached.");
	opHandler.setReturn(result);
}

static void _stdcall ShowIfaceTag2() {
	const ScriptValue &tagArg = opHandler.arg(0);
	if (tagArg.isInt()) {
		int tag = tagArg.asInt();
		if (tag == 3 || tag == 4) {
			__asm mov  eax, tag;
			__asm call pc_flag_on_;
		} else {
			AddBox(tag);
		}
	} else {
		OpcodeInvalidArgs("show_iface_tag");
	}
}

static void __declspec(naked) ShowIfaceTag() {
	_WRAP_OPCODE(ShowIfaceTag2, 1, 0)
}

static void _stdcall HideIfaceTag2() {
	const ScriptValue &tagArg = opHandler.arg(0);
	if (tagArg.isInt()) {
		int tag = tagArg.asInt();
		if (tag == 3 || tag == 4) {
			__asm mov  eax, tag;
			__asm call pc_flag_off_;
		} else {
			RemoveBox(tag);
		}
	} else {
		OpcodeInvalidArgs("hide_iface_tag");
	}
}

static void __declspec(naked) HideIfaceTag() {
	_WRAP_OPCODE(HideIfaceTag2, 1, 0)
}

static void IsIfaceTagActive2() {
	const ScriptValue &tagArg = opHandler.arg(0);
	if (tagArg.isInt()) {
		bool result = false;
		int tag = tagArg.asInt();
		if (tag >= 0 && tag < 5) {
			if (tag == 1 || tag == 2) { // Poison/Radiation
				tag += 2;
				int* boxslot = (int*)_bboxslot;
				for (int i = 0; i < 6; i++) {
					int value = boxslot[i];
					if (value == tag || value == -1) {
						result = (value != -1);
						break;
					}
				}
			} else { // Sneak/Level/Addict
				TGameObj* obj = *ptr_obj_dude;
				char* proto = GetProtoPtr(obj->pid);
				int flagBit = 1 << tag;
				result = ((*(int*)(proto + 0x20) & flagBit) != 0);
			}
		} else {
			result = GetBox(tag);
		}
		opHandler.setReturn(result);
	} else {
		OpcodeInvalidArgs("is_iface_tag_active");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) IsIfaceTagActive() {
	_WRAP_OPCODE(IsIfaceTagActive2, 1, 1)
}

static void sf_intface_redraw() {
	InterfaceRedraw();
}

static void sf_intface_show() {
	__asm call intface_show_;
}

static void sf_intface_hide() {
	__asm call intface_hide_;
}

static void sf_intface_is_hidden() {
	int isHidden;
	__asm {
		call intface_is_hidden_;
		mov isHidden, eax;
	}
	opHandler.setReturn(isHidden);
}

static void sf_tile_refresh_display() {
	__asm call tile_refresh_display_;
}

static void sf_get_cursor_mode() {
	int cursorMode;
	__asm {
		call gmouse_3d_get_mode_;
		mov  cursorMode, eax;
	}
	opHandler.setReturn(cursorMode);
}

static void sf_set_cursor_mode() {
	int cursorMode = opHandler.arg(0).asInt();
	__asm {
		mov  eax, cursorMode;
		call gmouse_3d_set_mode_;
	}
}

static void sf_display_stats() {
// calling the function outside of inventory screen will crash the game
	if (GetLoopFlags() & INVENTORY) {
		__asm call display_stats_;
	}
}

static void sf_set_iface_tag_text() {
	int boxTag = opHandler.arg(0).asInt();
	int maxBox = BarBoxes_MaxBox();

	if (boxTag > 4 && boxTag <= maxBox) {
		BarBoxes_SetText(boxTag, opHandler.arg(1).strValue(), opHandler.arg(2).asInt());
	} else {
		opHandler.printOpcodeError("set_iface_tag_text() - tag value must be in the range of 5 to %d.", maxBox);
	}
}

static void sf_inventory_redraw() {
	int mode;
	DWORD loopFlag = GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER);
	switch (loopFlag) {
		case INVENTORY:
			mode = 0;
			break;
		case INTFACEUSE:
			mode = 1;
			break;
		case INTFACELOOT:
			mode = 2;
			break;
		case BARTER:
			mode = 3;
			break;
		default:
			return;
	}
	long redrawSide = (opHandler.numArgs() > 0) ? opHandler.arg(0).rawValue() : -1; // -1 - both
	if (redrawSide <= 0) {
		ptr_stack_offset[*ptr_curr_stack] = 0;
		DisplayInventory(0, -1, mode);
	}
	if (redrawSide && mode >= 2) {
		ptr_target_stack_offset[*ptr_target_curr_stack] = 0;
		DisplayTargetInventory(0, -1, *ptr_target_pud, mode);
		RedrawWin(*ptr_i_wid);
	}
}

static void sf_create_win() {
	int flags;
	if (opHandler.numArgs() > 5) {
		flags = opHandler.arg(5).asInt();
	} else {
		flags = WIN_MoveOnTop;
	}

	if (CreateWindowFunc(opHandler.arg(0).asString(),
		opHandler.arg(1).asInt(), opHandler.arg(2).asInt(), // y, x
		opHandler.arg(3).asInt(), opHandler.arg(4).asInt(), // w, h
		256, flags) == -1)
	{
		opHandler.printOpcodeError("create_win() - couldn't create window.");
	}
}

static void sf_show_window() {
	if (opHandler.numArgs() > 0) {
		const char* name = opHandler.arg(0).strValue();
		sWindow sWin;
		for (size_t i = 0; i < 16; i++) {
			sWin = *(sWindow*)&ptr_sWindows[i * 23]; // sWindow struct = 92 bytes
			if (_stricmp(name, sWin.name) == 0) {
				ShowWin(sWin.wID);
				return;
			}
		}
		opHandler.printOpcodeError("show_window() - window '%s' is not found.", name);
	} else {
		__asm call windowShow_;
	}
}

static void sf_hide_window() {
	if (opHandler.numArgs() > 0) {
		const char* name = opHandler.arg(0).strValue();
		sWindow sWin;
		for (size_t i = 0; i < 16; i++) {
			sWin = *(sWindow*)&ptr_sWindows[i * 23]; // sWindow struct = 92 bytes
			if (_stricmp(name, sWin.name) == 0) {
				HideWin(sWin.wID);
				return;
			}
		}
		opHandler.printOpcodeError("hide_window() - window '%s' is not found.", name);
	} else {
		__asm call windowHide_;
	}
}

static void sf_set_window_flag() {
	long bitFlag = opHandler.arg(1).rawValue();
	switch (bitFlag) {
		case WIN_MoveOnTop:
		case WIN_Hidden:
		case WIN_Exclusive:
		case WIN_Transparent:
			break;
		default:
			return; // unsupported set flag
	}
	bool mode = opHandler.arg(2).asBool();
	if (opHandler.arg(0).isString()) {
		const char* name = opHandler.arg(0).strValue();
		sWindow sWin;
		for (size_t i = 0; i < 16; i++) {
			sWin = *(sWindow*)&ptr_sWindows[i * 23]; // sWindow struct = 92 bytes
			if (_stricmp(name, sWin.name) == 0) {
				WINinfo* win = GetWinStruct(sWin.wID);
				if (mode) {
					sWin.flags |= bitFlag;
					win->flags |= bitFlag;
				} else {
					sWin.flags &= ~bitFlag;
					win->flags &= ~bitFlag;
				}
				return;
			}
		}
		opHandler.printOpcodeError("set_window_flag() - window '%s' is not found.", name);
	} else {
		long wid = opHandler.arg(0).rawValue();
		WINinfo* win = GetWinStruct((wid > 0) ? wid : *ptr_i_wid); // i_wid - set flag to current game interface window
		if (win == nullptr) return;
		if (mode) {
			win->flags |= bitFlag;
		} else {
			win->flags &= ~bitFlag;
		}
	}
}

static void sf_draw_image() {
	if (*(DWORD*)_currentWindow == -1) {
		opHandler.printOpcodeError("draw_image() - no created/selected window for the image.");
		return;
	}
	long direction = 0;
	const char* file = nullptr;
	if (opHandler.arg(0).isInt()) { // art id
		long fid = opHandler.arg(0).rawValue();
		if (fid == -1) return;
		long _fid = fid & 0xFFFFFFF;
		file = ArtGetName(_fid); // .frm
		if (_fid >> 24 == OBJ_TYPE_CRITTER) {
			direction = (fid >> 28);
			if (direction && !DbAccess(file)) {
				file = ArtGetName(fid); // .fr#
			}
		}
	} else {
		file = opHandler.arg(0).strValue(); // path to frm file
	}
	FrmFile* frmPtr = nullptr;
	if (LoadFrame(file, &frmPtr)) {
		opHandler.printOpcodeError("draw_image() - cannot open the file: %s", file);
		return;
	}
	FrmFrameData* framePtr = (FrmFrameData*)&frmPtr->width;
	if (direction > 0 && direction < 6) {
		BYTE* offsOriFrame = (BYTE*)framePtr;
		offsOriFrame += frmPtr->oriFrameOffset[direction];
		framePtr = (FrmFrameData*)offsOriFrame;
	}
	// initialize other args
	int frameno = 0, x = 0, y = 0, noTrans = 0;
	switch (opHandler.numArgs()) {
		case 5:
			noTrans = opHandler.arg(4).rawValue();
		case 4:
			y = opHandler.arg(3).rawValue();
		case 3:
			x = opHandler.arg(2).rawValue();
		case 2:
			frameno = opHandler.arg(1).rawValue();
	}
	if (frameno > 0) {
		int maxFrames = frmPtr->frames - 1;
		if (frameno > maxFrames) frameno = maxFrames;
		while (frameno-- > 0) {
			BYTE* offsFrame = (BYTE*)framePtr;
			offsFrame += framePtr->size + (sizeof(FrmFrameData) - 1);
			framePtr = (FrmFrameData*)offsFrame;
		}
	}
	// with x/y frame offsets
	WindowDisplayBuf(x + frmPtr->xshift[direction], framePtr->width, y + frmPtr->yshift[direction], framePtr->height, framePtr->data, noTrans);
	__asm {
		mov  eax, frmPtr;
		call mem_free_;
	}
}

static void sf_draw_image_scaled() {
	if (*(DWORD*)_currentWindow == -1) {
		opHandler.printOpcodeError("draw_image_scaled() - no created/selected window for the image.");
		return;
	}
	long direction = 0;
	const char* file = nullptr;
	if (opHandler.arg(0).isInt()) { // art id
		long fid = opHandler.arg(0).rawValue();
		if (fid == -1) return;
		long _fid = fid & 0xFFFFFFF;
		file = ArtGetName(_fid); // .frm
		if (_fid >> 24 == OBJ_TYPE_CRITTER) {
			direction = (fid >> 28);
			if (direction && !DbAccess(file)) {
				file = ArtGetName(fid); // .fr#
			}
		}
	} else {
		file = opHandler.arg(0).strValue(); // path to frm file
	}
	FrmFile* frmPtr = nullptr;
	if (LoadFrame(file, &frmPtr)) {
		opHandler.printOpcodeError("draw_image_scaled() - cannot open the file: %s", file);
		return;
	}
	FrmFrameData* framePtr = (FrmFrameData*)&frmPtr->width;
	if (direction > 0 && direction < 6) {
		BYTE* offsOriFrame = (BYTE*)framePtr;
		offsOriFrame += frmPtr->oriFrameOffset[direction];
		framePtr = (FrmFrameData*)offsOriFrame;
	}
	// initialize other args
	int frameno = 0, x = 0, y = 0, wsize = 0;
	const int argNums = opHandler.numArgs();
	switch (argNums) {
		case 6:
		case 5:
			wsize = opHandler.arg(4).rawValue();
		case 4:
			y = opHandler.arg(3).rawValue();
		case 3:
			x = opHandler.arg(2).rawValue();
		case 2:
			frameno = opHandler.arg(1).rawValue();
	}
	if (frameno > 0) {
		int maxFrames = frmPtr->frames - 1;
		if (frameno > maxFrames) frameno = maxFrames;
		while (frameno-- > 0) {
			BYTE* offsFrame = (BYTE*)framePtr;
			offsFrame += framePtr->size + (sizeof(FrmFrameData) - 1);
			framePtr = (FrmFrameData*)offsFrame;
		}
	}
	if (argNums < 3) {
		DisplayInWindow(framePtr->width, framePtr->width, framePtr->height, framePtr->data); // scaled to window size (w/o transparent)
	} else {
		// draw to scale
		long s_width, s_height;
		if (argNums < 5) {
			s_width = framePtr->width;
			s_height = framePtr->height;
		} else {
			s_width = wsize;
			s_height = (argNums > 5) ? opHandler.arg(5).rawValue() : -1;
		}
		// scale with aspect ratio if w or h is set to -1
		if (s_width <= -1 && s_height > 0) {
			s_width = s_height * framePtr->width / framePtr->height;
		} else if (s_height <= -1 && s_width > 0) {
			s_height = s_width * framePtr->height / framePtr->width;
		}
		if (s_width <= 0 || s_height <= 0) return;

		long w_width = WindowWidth();
		long xy_pos = (y * w_width) + x;
		TransCscale(framePtr->width, framePtr->height, s_width, s_height, xy_pos, w_width, framePtr->data); // custom scaling
	}
	__asm {
		mov  eax, frmPtr;
		call mem_free_;
	}
}

static void sf_unwield_slot() {
	long slot = static_cast<long>(opHandler.arg(1).rawValue());
	if (slot < INVEN_TYPE_WORN || slot > INVEN_TYPE_LEFT_HAND) {
		opHandler.printOpcodeError("unwield_slot() - incorrect slot number.");
		return;
	}
	TGameObj* critter = opHandler.arg(0).asObject();
	if (critter->pid >> 24 != OBJ_TYPE_CRITTER) {
		opHandler.printOpcodeError("unwield_slot() - the object is not a critter.");
		return;
	}
	bool isDude = (critter == *ptr_obj_dude);
	bool update = false;
	if (slot && (GetLoopFlags() && (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER)) == false) {
		if (InvenUnwield(critter, (slot == INVEN_TYPE_LEFT_HAND) ? 0 : 1) == 0) {
			update = isDude;
		}
	} else {
		// force unwield for opened inventory
		bool forceAdd = false;
		TGameObj* item = nullptr;
		if (slot != INVEN_TYPE_WORN) {
			if (!isDude) return;
			long* itemRef = nullptr;
			if (slot == INVEN_TYPE_LEFT_HAND) {
				item = *ptr_i_lhand;
				itemRef = (long*)_i_lhand;
			} else {
				item = *ptr_i_rhand;
				itemRef = (long*)_i_rhand;
			}
			if (item) {
				if (!CorrectFidForRemovedItem_wHook(critter, item, (slot == INVEN_TYPE_LEFT_HAND) ? 0x1000000 : 0x2000000)) {
					return;
				}
				*itemRef = 0;
				forceAdd = true;
				update = true;
			}
		} else {
			if (isDude) item = *ptr_i_worn;
			if (!item) {
				item = InvenWorn(critter);
			} else {
				*ptr_i_worn = nullptr;
				forceAdd = true;
			}
			if (item) {
				if (!CorrectFidForRemovedItem_wHook(critter, item, 0x4000000)) {
					if (forceAdd) *ptr_i_worn = item;
					return;
				}
				if (isDude) IntfaceUpdateAc(0);
			}
		}
		if (forceAdd) ItemAddForce(critter, item, 1);
	}
	if (update) IntfaceUpdateItems(0, -1, -1);
}
