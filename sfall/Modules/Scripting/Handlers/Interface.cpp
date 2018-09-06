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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\..\InputFuncs.h"
#include "..\..\BarBoxes.h"
#include "..\..\LoadGameHook.h"
#include "..\..\ScriptExtender.h"
#include "..\OpcodeContext.h"

#include "Interface.h"

namespace sfall
{
namespace script
{

void __declspec(naked) op_input_funcs_available() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, 1; //They're always available from 2.9 on
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void sf_key_pressed(OpcodeContext& ctx) {
	if (ctx.arg(0).isInt()) {
		ctx.setReturn(static_cast<int>(KeyDown(ctx.arg(0).asInt())));
	}
}

void __declspec(naked) op_tap_key() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 255;
		jge end;
		push eax;
		call TapKey;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_mouse_x() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_mouse_x_];
		add edx, ds:[FO_VAR_mouse_hotx];
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_mouse_y() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_mouse_y_];
		add edx, ds:[FO_VAR_mouse_hoty];
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_
		pop edx;
		pop ecx;
		retn;
	}
}

#define MOUSE_MIDDLE_BTN        (4)
void sf_get_mouse_buttons(OpcodeContext& ctx) {
	DWORD button = fo::var::last_buttons;
	if (button == 0 && middleMouseDown) {
		button = MOUSE_MIDDLE_BTN;
	}
	ctx.setReturn(button);
}

void __declspec(naked) op_get_window_under_mouse() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_last_button_winID];
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_screen_width() {
	__asm {
		push edx
		push eax
		mov  edx, ds:[FO_VAR_scr_size + 8]                // _scr_size.offx
		sub  edx, ds:[FO_VAR_scr_size]                    // _scr_size.x
		inc  edx
		call fo::funcoffs::interpretPushLong_
		pop  eax
		mov  edx, VAR_TYPE_INT
		call fo::funcoffs::interpretPushShort_
		pop  edx
		retn
	}
}

void __declspec(naked) op_get_screen_height() {
	__asm {
		push edx
		push eax
		mov  edx, ds:[FO_VAR_scr_size + 12]               // _scr_size.offy
		sub  edx, ds:[FO_VAR_scr_size + 4]                // _scr_size.y
		inc  edx
		call fo::funcoffs::interpretPushLong_
		pop  eax
		mov  edx, VAR_TYPE_INT
		call fo::funcoffs::interpretPushShort_
		pop  edx
		retn
	}
}

void __declspec(naked) op_stop_game() {
	__asm {
		call fo::funcoffs::map_disable_bk_processes_;
		retn;
	}
}

void __declspec(naked) op_resume_game() {
	__asm {
		call fo::funcoffs::map_enable_bk_processes_;
		retn;
	}
}

void __declspec(naked) op_create_message_window() {
	__asm {
		pushad
		mov ebx, dword ptr ds:[FO_VAR_curr_font_num];
		cmp ebx, 0x65;
		je end;

		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretGetString_;
		mov esi, eax;

		mov ecx, eax;
		mov eax, 3;
		push 1;
		mov al, ds:[0x006AB718];
		push eax;
		push 0;
		push eax;
		push 0x74;
		mov ecx, 0xC0;
		mov eax, esi;
		xor ebx, ebx;
		xor edx, edx;
		call fo::funcoffs::dialog_out_;
		//xor eax, eax;
end:
		popad;
		ret;
	}
}

void __declspec(naked) op_get_viewport_x() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_wmWorldOffsetX];
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_viewport_y() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_wmWorldOffsetY];
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_viewport_x() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov ds:[FO_VAR_wmWorldOffsetX], eax
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_viewport_y() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov ds:[FO_VAR_wmWorldOffsetY], eax
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void sf_show_iface_tag(OpcodeContext &ctx) {
	int tag = ctx.arg(0).asInt();
	if (tag == 3 || tag == 4) {
		_asm mov  eax, tag;
		_asm call fo::funcoffs::pc_flag_on_;
	} else {
		BarBoxes::AddBox(tag);
	}
}

void sf_hide_iface_tag(OpcodeContext &ctx) {
	int tag = ctx.arg(0).asInt();
	if (tag == 3 || tag == 4) {
		_asm mov  eax, tag;
		_asm call fo::funcoffs::pc_flag_off_;
	} else {
		BarBoxes::RemoveBox(tag);
	}
}

void sf_is_iface_tag_active(OpcodeContext &ctx) {
	bool result = false;
	int tag = ctx.arg(0).asInt();
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
			fo::GameObject* obj = fo::var::obj_dude;
			fo::Proto* proto = fo::GetProto(obj->protoId);
			int flagBit = 1 << tag;
			result = ((proto->critter.critterFlags & flagBit) != 0);
		}
	} else {
		result = BarBoxes::GetBox(tag);
	}
	ctx.setReturn(result);
}

void sf_intface_redraw(OpcodeContext& ctx) {
	fo::func::intface_redraw();
}

void sf_intface_show(OpcodeContext& ctx) {
	__asm call fo::funcoffs::intface_show_;
}

void sf_intface_hide(OpcodeContext& ctx) {
	__asm call fo::funcoffs::intface_hide_;
}

void sf_intface_is_hidden(OpcodeContext& ctx) {
	int isHidden;
	__asm {
		call fo::funcoffs::intface_is_hidden_;
		mov isHidden, eax;
	}
	ctx.setReturn(isHidden);
}

void sf_tile_refresh_display(OpcodeContext& ctx) {
	fo::func::tile_refresh_display();
}

void sf_get_cursor_mode(OpcodeContext& ctx) {
	int cursorMode;
	__asm {
		call fo::funcoffs::gmouse_3d_get_mode_;
		mov cursorMode, eax;
	}
	ctx.setReturn(cursorMode);
}

void sf_set_cursor_mode(OpcodeContext& ctx) {
	fo::func::gmouse_3d_set_mode(ctx.arg(0).asInt());
}

void sf_display_stats(OpcodeContext& ctx) {
// calling the function outside of inventory screen will crash the game
	if (GetLoopFlags() & INVENTORY) {
		fo::func::display_stats();
	}
}

void sf_set_iface_tag_text(OpcodeContext& ctx) {
	int boxTag = ctx.arg(0).asInt();

	int maxBox = BarBoxes::MaxBox();
	if (boxTag > 4 && boxTag <= maxBox) {
		BarBoxes::SetText(boxTag, ctx.arg(1).asString(), ctx.arg(2).asInt());
	} else {
		ctx.printOpcodeError("set_iface_tag_text() - tag value must be in the range of 5 to %d.", maxBox);
	}
}

void sf_inventory_redraw(OpcodeContext& ctx) {
	int mode = -1;
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

	if (!ctx.arg(0).asBool()) {
		int* stack_offset = (int*)FO_VAR_stack_offset;
		stack_offset[fo::var::curr_stack * 4] = 0;
		fo::func::display_inventory(0, -1, mode);
	} else if (mode >= 2) {
		int* target_stack_offset = (int*)FO_VAR_target_stack_offset;
		target_stack_offset[fo::var::target_curr_stack * 4] = 0;
		fo::func::display_target_inventory(0, -1, (DWORD*)fo::var::target_pud, mode);
		fo::func::win_draw(fo::var::i_wid);
	}
}

void sf_dialog_message(OpcodeContext& ctx) {
	DWORD loopFlag = GetLoopFlags();
	if ((loopFlag & DIALOGVIEW) == 0 && (loopFlag & DIALOG)) {
		const char* message = ctx.arg(0).asString();
		fo::func::gdialogDisplayMsg(message);
	}
}

void sf_inventory_redraw(OpcodeContext& ctx) {
	int mode = -1;
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

	if (!ctx.arg(0).asBool()) {
		int* stack_offset = (int*)FO_VAR_stack_offset;
		stack_offset[fo::var::curr_stack * 4] = 0;
		fo::func::display_inventory(0, -1, mode);
	} else if (mode >= 2) {
		int* target_stack_offset = (int*)FO_VAR_target_stack_offset;
		target_stack_offset[fo::var::target_curr_stack * 4] = 0;
		fo::func::display_target_inventory(0, -1, (DWORD*)fo::var::target_pud, mode);
		fo::func::win_draw(fo::var::i_wid);
	}
}

}
}
