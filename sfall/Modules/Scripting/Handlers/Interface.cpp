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
#include "..\..\ScriptExtender.h"

#include "Interface.h"

void __declspec(naked) op_input_funcs_available() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, 1; //They're always available from 2.9 on
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_key_pressed() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
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
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_tap_key() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
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
		mov edx, ds:[VARPTR_mouse_x_];
		add edx, ds:[VARPTR_mouse_hotx];
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_
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
		mov edx, ds:[VARPTR_mouse_y_];
		add edx, ds:[VARPTR_mouse_hoty];
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_
			pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_mouse_buttons() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[VARPTR_last_buttons];
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_
			pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_window_under_mouse() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[VARPTR_last_button_winID];
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_
			pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_screen_width() {
	__asm {
		push edx
		push eax
		mov  edx, ds:[VARPTR_scr_size + 8]                // _scr_size.offx
		sub  edx, ds : [VARPTR_scr_size]                  // _scr_size.x
		inc  edx
		call FuncOffs::interpretPushLong_
		pop  eax
		mov  edx, 0xc001
		call FuncOffs::interpretPushShort_
		pop  edx
		retn
	}
}

void __declspec(naked) op_get_screen_height() {
	__asm {
		push edx
		push eax
		mov  edx, ds:[VARPTR_scr_size + 12]               // _scr_size.offy
		sub  edx, ds : [VARPTR_scr_size + 4]                // _scr_size.y
		inc  edx
		call FuncOffs::interpretPushLong_
		pop  eax
		mov  edx, 0xc001
		call FuncOffs::interpretPushShort_
		pop  edx
		retn
	}
}

void __declspec(naked) op_stop_game() {
	__asm {
		call FuncOffs::map_disable_bk_processes_;
		retn;
	}
}

void __declspec(naked) op_resume_game() {
	__asm {
		call FuncOffs::map_enable_bk_processes_;
		retn;
	}
}

void __declspec(naked) op_create_message_window() {
	__asm {
		pushad
		mov ebx, dword ptr ds : [VARPTR_curr_font_num];
		cmp ebx, 0x65;
		je end;

		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, ecx;
		call FuncOffs::interpretGetString_;
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
		call FuncOffs::dialog_out_;
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
		mov edx, ds:[VARPTR_wmWorldOffsetX];
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
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
		mov edx, ds:[VARPTR_wmWorldOffsetY];
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
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
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		mov ds : [VARPTR_wmWorldOffsetX], eax
			end :
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
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		mov ds : [VARPTR_wmWorldOffsetY], eax
			end :
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
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call AddBox;
		call FuncOffs::refresh_box_bar_win_;
		jmp end;
falloutfunc:
		call FuncOffs::pc_flag_on_;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_hide_iface_tag() {
	__asm {
		pushad;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call RemoveBox;
		call FuncOffs::refresh_box_bar_win_;
		jmp end;
falloutfunc:
		call FuncOffs::pc_flag_off_;
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
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ebx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
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
		mov eax, dword ptr ds : [VARPTR_obj_dude];
		mov edx, esp;
		mov eax, [eax + 0x64];
		call FuncOffs::proto_ptr_;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

void sf_intface_redraw(OpcodeContext& ctx) {
	Wrapper::intface_redraw();
}

void sf_intface_show(OpcodeContext& ctx) {
	__asm call FuncOffs::intface_show_
}

void sf_intface_hide(OpcodeContext& ctx) {
	__asm call FuncOffs::intface_hide_
}

void sf_intface_is_hidden(OpcodeContext& ctx) {
	int isHidden;
	__asm {
		call FuncOffs::intface_is_hidden_
		mov isHidden, eax;
	}
	ctx.setReturn(isHidden);
}
