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

void __declspec(naked) op_show_iface_tag() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call AddBox;
		call fo::funcoffs::refresh_box_bar_win_;
		jmp end;
falloutfunc:
		call fo::funcoffs::pc_flag_on_;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_hide_iface_tag() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call RemoveBox;
		call fo::funcoffs::refresh_box_bar_win_;
		jmp end;
falloutfunc:
		call fo::funcoffs::pc_flag_off_;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_is_iface_tag_active() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ebx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call GetBox;
		mov edx, eax;
		jmp end;
falloutfunc:
		mov ecx, eax;
		mov eax, dword ptr ds:[FO_VAR_obj_dude];
		mov edx, esp;
		mov eax, [eax + 0x64];
		call fo::funcoffs::proto_ptr_;
		mov edx, 1;
		shl edx, cl;
		mov ecx, [esp];
		mov eax, [ecx + 0x20];
		and eax, edx;
		jz fail;
		xor edx, edx;
		inc edx;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebx;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

void sf_intface_redraw(OpcodeContext& ctx) {
	fo::func::intface_redraw();
}

void sf_intface_show(OpcodeContext& ctx) {
	__asm call fo::funcoffs::intface_show_
}

void sf_intface_hide(OpcodeContext& ctx) {
	__asm call fo::funcoffs::intface_hide_
}

void sf_intface_is_hidden(OpcodeContext& ctx) {
	int isHidden;
	__asm {
		call fo::funcoffs::intface_is_hidden_
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
		call fo::funcoffs::gmouse_3d_get_mode_
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

	if (boxTag > 4 && boxTag < 10) {
		BarBoxes::SetText(boxTag, ctx.arg(1).asString(), ctx.arg(2).asInt());
	} else {
		ctx.printOpcodeError("set_iface_tag_text() - tag value must be in the range of 5 to 9.");
	}
}

}
}
