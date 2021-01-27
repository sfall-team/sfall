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

#include "Graphics.h"
#include "InputFuncs.h"
#include "Interface.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"

// input_functions
static void __declspec(naked) op_input_funcs_available() {
	__asm {
		mov  edx, 1; // They're always available from 2.9 on
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_key_pressed() {
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

static void __declspec(naked) op_tap_key() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		cmp  eax, 255;
		jge  end;
		push eax;
		call TapKey;
end:
		mov  ecx, esi;
		retn;
	}
}

//// *** From helios *** ////
static void __declspec(naked) op_get_mouse_x() {
	__asm {
		mov  edx, ds:[FO_VAR_mouse_x_];
		add  edx, ds:[FO_VAR_mouse_hotx];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

//Return mouse y position
static void __declspec(naked) op_get_mouse_y() {
	__asm {
		mov  edx, ds:[FO_VAR_mouse_y_];
		add  edx, ds:[FO_VAR_mouse_hoty];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

//Return pressed mouse button (1=left, 2=right, 3=left+right, 4=middle)
static void __declspec(naked) op_get_mouse_buttons() {
	__asm {
		push ecx;
		push edx;
		mov edx, ds:[FO_VAR_last_buttons];
		test edx, edx;
		jnz skip;
		cmp byte ptr middleMouseDown, 0;
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
static void __declspec(naked) op_get_window_under_mouse() {
	__asm {
		mov  edx, ds:[FO_VAR_last_button_winID];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

//Return screen width
static void __declspec(naked) op_get_screen_width() {
	__asm {
		mov  edx, ds:[FO_VAR_scr_size + 8]; // _scr_size.offx
		sub  edx, ds:[FO_VAR_scr_size];     // _scr_size.x
		inc  edx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

//Return screen height
static void __declspec(naked) op_get_screen_height() {
	__asm {
		mov  edx, ds:[FO_VAR_scr_size + 12]; // _scr_size.offy
		sub  edx, ds:[FO_VAR_scr_size + 4];  // _scr_size.y
		inc  edx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

//Stop game, the same effect as open charsscreen or inventory
static void __declspec(naked) op_stop_game() {
	__asm {
		jmp map_disable_bk_processes_;
	}
}

//Resume the game when it is stopped
static void __declspec(naked) op_resume_game() {
	__asm {
		jmp map_enable_bk_processes_;
	}
}

// copy and split
static void __stdcall SplitToBuffer(const char* str, const char** str_ptr, long &lines) {
	size_t i = 0;
	while (str[i]) {
		if (str[i] == '\n' && lines < 4) {
			gTextBuffer[i] = '\0';
			str_ptr[lines++] = &gTextBuffer[++i];
		} else {
			gTextBuffer[i] = str[i++];
		}
	};
	gTextBuffer[i] = '\0';
}

static void __stdcall op_create_message_window2() {
	const ScriptValue &strArg = opHandler.arg(0);
	if (strArg.isString()) {
		static bool dialogShow = false;
		if (dialogShow) return;

		const char* str = strArg.strValue();
		if (!str || str[0] == 0) return;

		long lines = 0;
		const char* str_ptr[4];
		SplitToBuffer(str, str_ptr, lines);

		dialogShow = true;
		DialogOutEx(gTextBuffer, str_ptr, lines, DIALOGOUT_NORMAL);
		dialogShow = false;
	} else {
		OpcodeInvalidArgs("create_message_window");
	}
}

static void __declspec(naked) op_create_message_window() {
	_WRAP_OPCODE(op_create_message_window2, 1, 0)
}

static void mf_message_box() {
	static int dialogShowCount = 0;

	long lines = 0;
	const char* str_ptr[4];
	SplitToBuffer(opHandler.arg(0).asString(), str_ptr, lines);

	long colors = 0x9191, flags = DIALOGOUT_NORMAL | DIALOGOUT_YESNO;
	if (opHandler.numArgs() > 1 && opHandler.arg(1).rawValue() != -1) flags = opHandler.arg(1).rawValue();
	if (opHandler.numArgs() > 2) {
		colors &= 0xFF00;
		colors |= (opHandler.arg(2).rawValue() & 0xFF);
	}
	if (opHandler.numArgs() > 3) {
		colors &= 0xFF;
		colors |= (opHandler.arg(3).rawValue() & 0xFF) << 8;
	}
	dialogShowCount++;
	*(DWORD*)FO_VAR_script_engine_running = 0;
	long result = DialogOutEx(gTextBuffer, str_ptr, lines, flags, colors);
	if (--dialogShowCount == 0) *(DWORD*)FO_VAR_script_engine_running = 1;

	opHandler.setReturn(result);
}

static void __declspec(naked) op_get_viewport_x() {
	__asm {
		mov  edx, ds:[FO_VAR_wmWorldOffsetX];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_get_viewport_y() {
	__asm {
		mov  edx, ds:[FO_VAR_wmWorldOffsetY];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_set_viewport_x() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_wmWorldOffsetX], eax;
end:
		retn;
	}
}

static void __declspec(naked) op_set_viewport_y() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_wmWorldOffsetY], eax;
end:
		retn;
	}
}

static void mf_add_iface_tag() {
	int result = BarBoxes_AddExtraBox();
	if (result == -1) opHandler.printOpcodeError("add_iface_tag() - cannot add new tag as the maximum limit of 126 tags has been reached.");
	opHandler.setReturn(result);
}

static void __stdcall op_show_iface_tag2() {
	const ScriptValue &tagArg = opHandler.arg(0);
	if (tagArg.isInt()) {
		int tag = tagArg.rawValue();
		if (tag == 3 || tag == 4) {
			__asm mov  eax, tag;
			__asm call pc_flag_on_;
		} else {
			BarBoxes_AddBox(tag);
		}
	} else {
		OpcodeInvalidArgs("show_iface_tag");
	}
}

static void __declspec(naked) op_show_iface_tag() {
	_WRAP_OPCODE(op_show_iface_tag2, 1, 0)
}

static void __stdcall op_hide_iface_tag2() {
	const ScriptValue &tagArg = opHandler.arg(0);
	if (tagArg.isInt()) {
		int tag = tagArg.rawValue();
		if (tag == 3 || tag == 4) {
			__asm mov  eax, tag;
			__asm call pc_flag_off_;
		} else {
			BarBoxes_RemoveBox(tag);
		}
	} else {
		OpcodeInvalidArgs("hide_iface_tag");
	}
}

static void __declspec(naked) op_hide_iface_tag() {
	_WRAP_OPCODE(op_hide_iface_tag2, 1, 0)
}

static void __stdcall op_is_iface_tag_active2() {
	const ScriptValue &tagArg = opHandler.arg(0);
	if (tagArg.isInt()) {
		bool result = false;
		int tag = tagArg.rawValue();
		if (tag >= 0 && tag < 5) {
			if (tag == 1 || tag == 2) { // Poison/Radiation
				tag += 2;
				int* boxslot = (int*)FO_VAR_bboxslot;
				for (int i = 0; i < 6; i++) {
					int value = boxslot[i];
					if (value == tag || value == -1) {
						result = (value != -1);
						break;
					}
				}
			} else { // Sneak/Level/Addict
				TGameObj* obj = *ptr_obj_dude;
				char* proto = GetProtoPtr(obj->protoId);
				int flagBit = 1 << tag;
				result = ((*(int*)(proto + 0x20) & flagBit) != 0);
			}
		} else {
			result = BarBoxes_GetBox(tag);
		}
		opHandler.setReturn(result);
	} else {
		OpcodeInvalidArgs("is_iface_tag_active");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_is_iface_tag_active() {
	_WRAP_OPCODE(op_is_iface_tag_active2, 1, 1)
}

static void mf_intface_redraw() {
	if (opHandler.numArgs() == 0) {
		fo_intface_redraw();
	} else {
		// fake redraw interfaces (TODO: need a real redraw of interface?)
		long winType = opHandler.arg(0).rawValue();
		if (winType == -1) {
			RefreshGNW(true); 
		} else {
			WINinfo* win = Interface_GetWindow(winType);
			if (win && (int)win != -1) fo_GNW_win_refresh(win, &win->rect, 0);
		}
	}
}

static void mf_intface_show() {
	__asm call intface_show_;
}

static void mf_intface_hide() {
	__asm call intface_hide_;
}

static void mf_intface_is_hidden() {
	opHandler.setReturn(fo_intface_is_hidden());
}

static void mf_tile_refresh_display() {
	fo_tile_refresh_display();
}

static void mf_get_cursor_mode() {
	opHandler.setReturn(*ptr_gmouse_3d_current_mode);
}

static void mf_set_cursor_mode() {
	fo_gmouse_3d_set_mode(opHandler.arg(0).rawValue());
}

static void mf_display_stats() {
	unsigned long flags = GetLoopFlags();
	if (flags & INVENTORY) {
		fo_display_stats(); // calling the function outside of inventory screen will crash the game
	} else if (flags & CHARSCREEN) {
		__asm {
			mov  eax, ds:[FO_VAR_obj_dude];
			call stat_recalc_derived_;
			xor  edx, edx;
			mov  eax, ds:[FO_VAR_obj_dude];
			call critter_adjust_hits_;
			push ebx;
			mov  eax, 7;
			call PrintBasicStat_;
			xor  eax, eax;
			call ListSkills_;
			call PrintLevelWin_;
			call ListDrvdStats_;
			pop  ebx;
		}
		fo_win_draw(*ptr_edit_win);
	}
}

static void mf_set_iface_tag_text() {
	int boxTag = opHandler.arg(0).rawValue();
	int maxBox = BarBoxes_MaxBox();

	if (boxTag > 4 && boxTag <= maxBox) {
		BarBoxes_SetText(boxTag, opHandler.arg(1).strValue(), opHandler.arg(2).rawValue());
	} else {
		opHandler.printOpcodeError("set_iface_tag_text() - tag value must be in the range of 5 to %d.", maxBox);
		opHandler.setReturn(-1);
	}
}

static void mf_inventory_redraw() {
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
		fo_display_inventory(0, -1, mode);
	}
	if (redrawSide && mode >= 2) {
		ptr_target_stack_offset[*ptr_target_curr_stack] = 0;
		fo_display_target_inventory(0, -1, *ptr_target_pud, mode);
		fo_win_draw(*ptr_i_wid);
	}
}

static void mf_create_win() {
	int flags = (opHandler.numArgs() > 5)
		? opHandler.arg(5).rawValue()
		: WinFlags::MoveOnTop;

	if (fo_createWindow(opHandler.arg(0).strValue(),
		opHandler.arg(1).rawValue(), opHandler.arg(2).rawValue(), // x, y
		opHandler.arg(3).rawValue(), opHandler.arg(4).rawValue(), // w, h
		(flags & WinFlags::Transparent) ? 0 : 256, flags) == -1)
	{
		opHandler.printOpcodeError("create_win() - couldn't create window.");
		opHandler.setReturn(-1);
	}
}

static void mf_show_window() {
	if (opHandler.numArgs() > 0) {
		const char* name = opHandler.arg(0).strValue();
		for (size_t i = 0; i < 16; i++) {
			if (_stricmp(name, ptr_sWindows[i].name) == 0) {
				fo_win_show(ptr_sWindows[i].wID);
				return;
			}
		}
		opHandler.printOpcodeError("show_window() - window '%s' is not found.", name);
	} else {
		__asm call windowShow_;
	}
}

static void mf_hide_window() {
	if (opHandler.numArgs() > 0) {
		const char* name = opHandler.arg(0).strValue();
		for (size_t i = 0; i < 16; i++) {
			if (_stricmp(name, ptr_sWindows[i].name) == 0) {
				fo_win_hide(ptr_sWindows[i].wID);
				return;
			}
		}
		opHandler.printOpcodeError("hide_window() - window '%s' is not found.", name);
	} else {
		__asm call windowHide_;
	}
}

static void mf_set_window_flag() {
	long bitFlag = opHandler.arg(1).rawValue();
	switch (bitFlag) {
		case WinFlags::DontMoveTop:
		case WinFlags::MoveOnTop:
		case WinFlags::Hidden:
		case WinFlags::Exclusive:
		case WinFlags::Transparent:
			break;
		default:
			return; // unsupported set flag
	}
	bool mode = opHandler.arg(2).asBool();
	if (opHandler.arg(0).isString()) {
		const char* name = opHandler.arg(0).strValue();
		for (size_t i = 0; i < 16; i++) {
			if (_stricmp(name, ptr_sWindows[i].name) == 0) {
				WINinfo* win = fo_GNW_find(ptr_sWindows[i].wID);
				if (mode) {
					ptr_sWindows[i].flags |= bitFlag;
					win->flags |= bitFlag;
				} else {
					ptr_sWindows[i].flags &= ~bitFlag;
					win->flags &= ~bitFlag;
				}
				return;
			}
		}
		opHandler.printOpcodeError("set_window_flag() - window '%s' is not found.", name);
	} else {
		long wid = opHandler.arg(0).rawValue();
		WINinfo* win = fo_GNW_find((wid > 0) ? wid : *ptr_i_wid); // i_wid - set flag to current game interface window
		if (win == nullptr) return;
		if (mode) {
			win->flags |= bitFlag;
		} else {
			win->flags &= ~bitFlag;
		}
	}
}

static void __fastcall FreeArtFile(FrmFile* frmPtr) {
	if (frmPtr->id == 'PCX') {
		__asm mov  eax, frmPtr;
		__asm mov  eax, [eax]frmPtr.pixelData;
		__asm call ds:[FO_VAR_freePtr];
		delete[] frmPtr;
	} else {
		__asm mov  eax, frmPtr;
		__asm call my_free_;
	}
}

static FrmFile* __stdcall LoadArtFile(const char* file, long frame, long direction, FrmFrameData* &framePtr, bool checkPCX) {
	FrmFile* frmPtr = nullptr;
	if (checkPCX) {
		const char* pos = strrchr(file, '.');
		if (pos && _stricmp(++pos, "PCX") == 0) {
			long w, h;
			BYTE* data = fo_loadPCX(file, &w, &h);
			if (!data) return nullptr;

			frmPtr = reinterpret_cast<FrmFile*>(new BYTE[78]);
			std::memset(frmPtr, 0, 74);

			frmPtr->id = 'PCX';
			frmPtr->width = static_cast<short>(w);
			frmPtr->height = static_cast<short>(h);
			frmPtr->pixelData = data;
			framePtr = frmPtr->GetFrameData(0, 0);
			return frmPtr;
		}
	}
	if (fo_load_frame(file, &frmPtr)) {
		return nullptr;
	}
	framePtr = frmPtr->GetFrameData(direction, frame);
	return frmPtr;
}

static long __stdcall GetArtFIDFile(long fid, const char* &file) {
	long direction = 0;
	long _fid = fid & 0xFFFFFFF;
	file = fo_art_get_name(_fid); // .frm
	if (_fid >> 24 == OBJ_TYPE_CRITTER) {
		direction = (fid >> 28);
		if (direction > 0 && !fo_db_access(file)) {
			file = fo_art_get_name(fid); // .fr#
		}
	}
	return direction;
}

static long __stdcall DrawImage(OpcodeHandler& opHandler, bool isScaled, const char* metaruleName) {
	if (!fo_selectWindowID(opHandler.program()->currentScriptWin) || *(DWORD*)FO_VAR_currentWindow == -1) {
		opHandler.printOpcodeError("%s() - no created or selected window.", metaruleName);
		return 0;
	}
	long direction = 0;
	const char* file = nullptr;

	bool isID = opHandler.arg(0).isInt();
	if (isID) { // art id
		long fid = opHandler.arg(0).rawValue();
		if (fid == -1) return -1;

		direction = GetArtFIDFile(fid, file);
	} else {
		file = opHandler.arg(0).strValue(); // path to frm/pcx file
	}

	FrmFrameData* framePtr;
	FrmFile* frmPtr = LoadArtFile(file, opHandler.arg(1).rawValue(), direction, framePtr, !isID);
	if (frmPtr == nullptr) {
		opHandler.printOpcodeError("%s() - cannot open the file: %s", metaruleName, file);
		return -1;
	}
	long result = 1;
	BYTE* pixelData = (frmPtr->id == 'PCX') ? frmPtr->pixelData : framePtr->data;

	if (isScaled && opHandler.numArgs() < 3) {
		fo_displayInWindow(framePtr->width, framePtr->width, framePtr->height, pixelData); // scaled to window size (w/o transparent)
	} else {
		int x = opHandler.arg(2).rawValue(), y = opHandler.arg(3).rawValue();
		if (isScaled) { // draw to scale
			long s_width, s_height;
			if (opHandler.numArgs() < 5) {
				s_width = framePtr->width;
				s_height = framePtr->height;
			} else {
				s_width = opHandler.arg(4).rawValue();
				s_height = (opHandler.numArgs() > 5) ? opHandler.arg(5).rawValue() : -1;
			}
			// scale with aspect ratio if w or h is set to -1
			if (s_width <= -1 && s_height > 0) {
				s_width = s_height * framePtr->width / framePtr->height;
			} else if (s_height <= -1 && s_width > 0) {
				s_height = s_width * framePtr->height / framePtr->width;
			}
			if (s_width <= 0 || s_height <= 0) {
				result = 0;
				goto exit;
			}

			long w_width = fo_windowWidth();
			long xy_pos = (y * w_width) + x;
			window_trans_cscale(framePtr->width, framePtr->height, s_width, s_height, xy_pos, w_width, pixelData); // custom scaling
		} else { // with x/y frame offsets
			fo_windowDisplayBuf(x + frmPtr->xshift[direction], framePtr->width, y + frmPtr->yshift[direction], framePtr->height, pixelData, opHandler.arg(4).rawValue());
		}
	}

exit:
	FreeArtFile(frmPtr);
	return result;
}

static void mf_draw_image() {
	opHandler.setReturn(DrawImage(opHandler, false, "draw_image"));
}

static void mf_draw_image_scaled() {
	opHandler.setReturn(DrawImage(opHandler, true, "draw_image_scaled"));
}

static long __stdcall InterfaceDrawImage(OpcodeHandler& opHandler, WINinfo* ifaceWin, const char* metaruleName) {
	const char* file = nullptr;
	bool useShift = false;
	long direction = -1, w = -1, h = -1;

	bool isID = opHandler.arg(1).isInt();
	if (isID) { // art id
		long fid = opHandler.arg(1).rawValue();
		if (fid == -1) return -1;

		useShift = (((fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER);
		direction = GetArtFIDFile(fid, file);
	} else {
		file = opHandler.arg(1).strValue(); // path to frm/pcx file
	}

	if (opHandler.numArgs() > 5) { // array params
		sArrayVar* sArray = GetRawArray(opHandler.arg(5).rawValue());
		if (sArray) {
			if (direction < 0) direction = sArray->val[0].intVal;
			int size = sArray->size();
			if (size > 1) w = sArray->val[1].intVal;
			if (size > 2) h = sArray->val[2].intVal;
		}
	}
	long frame = opHandler.arg(4).rawValue();

	FrmFrameData* framePtr;
	FrmFile* frmPtr = LoadArtFile(file, frame, direction, framePtr, !isID);
	if (frmPtr == nullptr) {
		opHandler.printOpcodeError("%s() - cannot open the file: %s", metaruleName, file);
		return -1;
	}
	int x = opHandler.arg(2).rawValue();
	int y = opHandler.arg(3).rawValue();

	if (useShift && direction >= 0) {
		x += frmPtr->xshift[direction];
		y += frmPtr->yshift[direction];
	}
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	int width  = (w >= 0) ? w : framePtr->width;
	int height = (h >= 0) ? h : framePtr->height;

	BYTE* surface = (ifaceWin->randY) ? GameRender_GetOverlaySurface(ifaceWin) : ifaceWin->surface;

	fo_trans_cscale(((frmPtr->id == 'PCX') ? frmPtr->pixelData : framePtr->data), framePtr->width, framePtr->height, framePtr->width,
	                surface + (y * ifaceWin->width) + x, width, height, ifaceWin->width
	);

	if (!(opHandler.arg(0).rawValue() & 0x1000000)) { // is set to "Don't redraw"
		fo_GNW_win_refresh(ifaceWin, &ifaceWin->rect, 0);
	}

	FreeArtFile(frmPtr);
	return 1;
}

static void mf_interface_art_draw() {
	long result = -1;
	WINinfo* win = Interface_GetWindow(opHandler.arg(0).rawValue() & 0xFF);
	if (win && (int)win != -1) {
		result = InterfaceDrawImage(opHandler, win, "interface_art_draw");
	} else {
		opHandler.printOpcodeError("interface_art_draw() - the game interface window is not created or invalid window type number.");
	}
	opHandler.setReturn(result);
}

static void mf_unwield_slot() {
	InvenType slot = static_cast<InvenType>(opHandler.arg(1).rawValue());
	if (slot < INVEN_TYPE_WORN || slot > INVEN_TYPE_LEFT_HAND) {
		opHandler.printOpcodeError("unwield_slot() - incorrect slot number.");
		opHandler.setReturn(-1);
		return;
	}
	TGameObj* critter = opHandler.arg(0).object();
	if (critter->IsNotCritter()) {
		opHandler.printOpcodeError("unwield_slot() - the object is not a critter.");
		opHandler.setReturn(-1);
		return;
	}
	bool isDude = (critter == *ptr_obj_dude);
	bool update = false;
	if (slot && (GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER)) == false) {
		if (fo_inven_unwield(critter, (slot == INVEN_TYPE_LEFT_HAND) ? 0 : 1) == 0) {
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
				itemRef = (long*)FO_VAR_i_lhand;
			} else {
				item = *ptr_i_rhand;
				itemRef = (long*)FO_VAR_i_rhand;
			}
			if (item) {
				if (!CorrectFidForRemovedItem_wHook(critter, item, (slot == INVEN_TYPE_LEFT_HAND) ? ObjectFlag::Left_Hand : ObjectFlag::Right_Hand)) {
					return;
				}
				*itemRef = 0;
				forceAdd = true;
				update = true;
			}
		} else {
			if (isDude) item = *ptr_i_worn;
			if (!item) {
				item = fo_inven_worn(critter);
			} else {
				*ptr_i_worn = nullptr;
				forceAdd = true;
			}
			if (item) {
				if (!CorrectFidForRemovedItem_wHook(critter, item, ObjectFlag::Worn)) {
					if (forceAdd) *ptr_i_worn = item;
					return;
				}
				if (isDude) fo_intface_update_ac(0);
			}
		}
		if (forceAdd) fo_item_add_force(critter, item, 1);
	}
	if (update) fo_intface_update_items(0, -1, -1);
}

static void mf_get_window_attribute() {
	WINinfo* win = Interface_GetWindow(opHandler.arg(0).rawValue());
	if (win == nullptr) {
		if (opHandler.arg(1).rawValue() != 0) {
			opHandler.printOpcodeError("get_window_attribute() - failed to get the interface window.");
			opHandler.setReturn(-1);
		}
		return;
	}
	if ((long)win == -1) {
		opHandler.printOpcodeError("get_window_attribute() - invalid window type number.");
		opHandler.setReturn(-1);
		return;
	}
	long result = 0;
	switch (opHandler.arg(1).rawValue()) {
	case -1: // rectangle map.left map.top map.right map.bottom
		result = CreateTempArray(-1, 0); // associative
		setArray(result, ScriptValue("left").rawValue(), VAR_TYPE_STR, win->rect.x, VAR_TYPE_INT, 0);
		setArray(result, ScriptValue("top").rawValue(), VAR_TYPE_STR, win->rect.y, VAR_TYPE_INT, 0);
		setArray(result, ScriptValue("right").rawValue(), VAR_TYPE_STR, win->rect.offx, VAR_TYPE_INT, 0);
		setArray(result, ScriptValue("bottom").rawValue(), VAR_TYPE_STR, win->rect.offy, VAR_TYPE_INT, 0);
		break;
	case 0: // check if window exists
		result = 1;
		break;
	case 1:
		result = win->rect.x;
		break;
	case 2:
		result = win->rect.y;
		break;
	case 3:
		result = win->width;
		break;
	case 4:
		result = win->height;
		break;
	}
	opHandler.setReturn(result);
}

static void mf_interface_print() { // same as vanilla PrintRect
	WINinfo* win = Interface_GetWindow(opHandler.arg(1).rawValue());
	if (win == nullptr || (int)win == -1) {
		opHandler.printOpcodeError("interface_print() - the game interface window is not created or invalid window type number.");
		opHandler.setReturn(-1);
		return;
	}
	const char* text = opHandler.arg(0).strValue();

	long x = opHandler.arg(2).rawValue();
	if (x < 0) x = 0;
	long y = opHandler.arg(3).rawValue();
	if (y < 0) y = 0;

	long color = opHandler.arg(4).rawValue();
	long width = opHandler.arg(5).rawValue();

	int maxHeight = win->height - y;
	int maxWidth = win->width - x;
	if (width <= 0) {
		width = maxWidth;
	} else if (width > maxWidth) {
		width = maxWidth;
	}

	color ^= 0x2000000; // fills background with black color if the flag is set (textnofill)
	if ((color & 0xFF) == 0) {
		__asm call windowGetTextColor_; // set from SetTextColor
		__asm mov  byte ptr color, al;
	}

	BYTE* surface;
	if (win->randY) { // if a surface was created, the engine will draw on it
		surface = win->surface;
		win->surface = GameRender_GetOverlaySurface(win); // replace the surface for the windowWrapLineWithSpacing_ function
	}

	if (color & 0x10000) { // shadow (textshadow)
		fo_windowWrapLineWithSpacing(win->wID, text, width, maxHeight, x, y, 0x201000F, 0, 0);
		color ^= 0x10000;
	}
	opHandler.setReturn(fo_windowWrapLineWithSpacing(win->wID, text, width, maxHeight, x, y, color, 0, 0)); // returns count of lines printed

	if (win->randY) win->surface = surface;

	// no redraw (textdirect)
	if (!(color & 0x1000000)) fo_GNW_win_refresh(win, &win->rect, 0);
}

static void mf_win_fill_color() {
	long result = fo_selectWindowID(opHandler.program()->currentScriptWin);
	long iWin = *(DWORD*)FO_VAR_currentWindow;
	if (!result || iWin == -1) {
		opHandler.printOpcodeError("win_fill_color() - no created or selected window.");
		opHandler.setReturn(-1);
		return;
	}
	if (opHandler.numArgs() > 0) {
		WinFillRect(ptr_sWindows[iWin].wID,
		            opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue(), // x, y
		            opHandler.arg(2).rawValue(), opHandler.arg(3).rawValue(), // w, h
		            static_cast<BYTE>(opHandler.arg(4).rawValue())
		);
	} else {
		ClearWindow(ptr_sWindows[iWin].wID, false); // full clear
	}
}

static void mf_interface_overlay() {
	long winType = opHandler.arg(0).rawValue();

	WINinfo* win = Interface_GetWindow(winType);
	if (!win || (int)win == -1) return;

	switch (opHandler.arg(1).rawValue()) {
	case 0:
		GameRender_DestroyOverlaySurface(win);
		break;
	case 1:
		GameRender_CreateOverlaySurface(win, winType);
		break;
	case 2: // clear
		if (opHandler.numArgs() > 2) {
			long w = opHandler.arg(4).rawValue();
			long h = opHandler.arg(5).rawValue();
			if (w <= 0 || h <= 0) return;

			long x = opHandler.arg(2).rawValue();
			long y = opHandler.arg(3).rawValue();
			if (x < 0 || y < 0) return;

			sRectangle rect = { x, y, w, h };
			GameRender_ClearOverlay(win, rect);
		} else {
			GameRender_ClearOverlay(win);
		}
		break;
	}
}
