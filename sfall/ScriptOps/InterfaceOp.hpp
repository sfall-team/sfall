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
#include "input.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"


// input_functions
static void __declspec(naked) InputFuncsAvailable() {
	__asm {
		push ecx;
		push edx;
		mov  edx, 1; // They're always available from 2.9 on
		_RET_VAL_INT2(ecx);
		pop  edx;
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
		push edx;
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		cmp  eax, 255;
		jge  end;
		push eax;
		call TapKey;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

//// *** From helios *** ////
static void __declspec(naked) get_mouse_x() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_mouse_x_];
		add  edx, ds:[_mouse_hotx];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

//Return mouse y position
static void __declspec(naked) get_mouse_y() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_mouse_y_];
		add  edx, ds:[_mouse_hoty];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
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
		push edx;
		push ecx;
		mov  edx, ds:[_last_button_winID];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

//Return screen width
static void __declspec(naked) get_screen_width() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_scr_size + 8]; // _scr_size.offx
		sub  edx, ds:[_scr_size];     // _scr_size.x
		inc  edx;
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

//Return screen height
static void __declspec(naked) get_screen_height() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_scr_size + 12]; // _scr_size.offy
		sub  edx, ds:[_scr_size + 4];  // _scr_size.y
		inc  edx;
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
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

//Create a message window with given string
static void __declspec(naked) create_message_window() {
	__asm {
		pushad
		//mov ebx, dword ptr ds:[_curr_font_num];
		//cmp ebx, 0x65;
		//je end;

		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, ecx;
		call interpretGetString_;
		mov esi, eax

		mov ecx, eax;
		mov eax, 3;
		push 1;         // arg10
		mov al, byte ptr ds:[0x6AB718];
		push eax;       // a9
		push 0;         // *DisplayText
		push eax;       // ColorIndex
		push 116;       // y
		mov ecx, 192;   // x
		mov eax, esi;   // text
		xor ebx, ebx;   // ?
		xor edx, edx;   // ?
		call dialog_out_;
		//xor eax, eax;
end:
		popad
		ret;
	}
}

static void __declspec(naked) GetViewportX() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_wmWorldOffsetX];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

static void __declspec(naked) GetViewportY() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_wmWorldOffsetY];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

static void __declspec(naked) SetViewportX() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		mov  ds:[_wmWorldOffsetX], eax;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) SetViewportY() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		mov  ds:[_wmWorldOffsetY], eax;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void ShowIfaceTag2() {
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
		opHandler.printOpcodeError("show_iface_tag() - argument is not an integer.");
	}
}

static void __declspec(naked) ShowIfaceTag() {
	_WRAP_OPCODE(ShowIfaceTag2, 1, 0)
}

static void HideIfaceTag2() {
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
		opHandler.printOpcodeError("hide_iface_tag() - argument is not an integer.");
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
		opHandler.printOpcodeError("is_iface_tag_active() - argument is not an integer.");
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

static void sf_inventory_redraw() {
	int mode;
	DWORD loopFlag = GetLoopFlags();
	if (loopFlag & INVENTORY) {
		mode = 0;
	} else if (loopFlag & INTFACEUSE) {
		mode = 1;
	} else if (loopFlag & INTFACELOOT) {
		mode = 2;
	} else if (loopFlag & BARTER) {
		mode = 3;
	} else {
		return;
	}

	if (!opHandler.arg(0).asBool()) {
		int* stack_offset = (int*)_stack_offset;
		stack_offset[*ptr_curr_stack * 4] = 0;
		DisplayInventory(0, -1, mode);
	} else if (mode >= 2) {
		int* target_stack_offset = (int*)_target_stack_offset;
		target_stack_offset[*ptr_target_curr_stack * 4] = 0;
		DisplayTargetInventory(0, -1, (DWORD*)*ptr_target_pud, mode);
		RedrawWin(*ptr_i_wid);
	}
}

static void sf_create_win() {
	int flags;
	if (opHandler.numArgs() == 6) {
		flags = opHandler.arg(5).asInt();
	} else {
		flags = 0x4; // MoveOnTop
	}

	if (CreateWindowFunc(opHandler.arg(0).asString(),
		opHandler.arg(1).asInt(), opHandler.arg(2).asInt(), // y, x
		opHandler.arg(3).asInt(), opHandler.arg(4).asInt(), // w, h
		256, flags) == -1)
	{
		opHandler.printOpcodeError("create_win() - couldn't create window.");
	}
}
