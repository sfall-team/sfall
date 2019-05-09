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
		push ecx;
		push edx;
		mov  edx, 1; // They're always available from 2.9 on
		_RET_VAL_INT(ecx);
		pop  edx;
		pop  ecx;
		retn;
	}
}

void sf_key_pressed(OpcodeContext& ctx) {
	ctx.setReturn(static_cast<int>(KeyDown(ctx.arg(0).rawValue())));
}

void __declspec(naked) op_tap_key() {
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

void __declspec(naked) op_get_mouse_x() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_mouse_x_];
		add  edx, ds:[FO_VAR_mouse_hotx];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_get_mouse_y() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_mouse_y_];
		add  edx, ds:[FO_VAR_mouse_hoty];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
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
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_last_button_winID];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_get_screen_width() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_scr_size + 8]; // _scr_size.offx
		sub  edx, ds:[FO_VAR_scr_size];     // _scr_size.x
		inc  edx;
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_get_screen_height() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_scr_size + 12]; // _scr_size.offy
		sub  edx, ds:[FO_VAR_scr_size + 4];  // _scr_size.y
		inc  edx;
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_stop_game() {
	__asm {
		jmp fo::funcoffs::map_disable_bk_processes_;
	}
}

void __declspec(naked) op_resume_game() {
	__asm {
		jmp fo::funcoffs::map_enable_bk_processes_;
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
		push 1;         // arg10
		mov al, byte ptr ds:[0x6AB718];
		push eax;       // a9
		push 0;         // *DisplayText
		push eax;       // ColorIndex
		push 0x74;      // y
		mov ecx, 0xC0;  // x
		mov eax, esi;   // text
		xor ebx, ebx;   // ?
		xor edx, edx;   // ?
		call fo::funcoffs::dialog_out_;
		//xor eax, eax;
end:
		popad;
		ret;
	}
}

void __declspec(naked) op_get_viewport_x() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_wmWorldOffsetX];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_get_viewport_y() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[FO_VAR_wmWorldOffsetY];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_set_viewport_x() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_wmWorldOffsetX], eax;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_set_viewport_y() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_wmWorldOffsetY], eax;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

void sf_add_iface_tag(OpcodeContext &ctx) {
	int result = BarBoxes::AddExtraBox();
	if (result == -1) ctx.printOpcodeError("%s() - cannot add new tag as the maximum limit of 126 tags has been reached.", ctx.getMetaruleName());
	ctx.setReturn(result);
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
		BarBoxes::SetText(boxTag, ctx.arg(1).strValue(), ctx.arg(2).asInt());
	} else {
		ctx.printOpcodeError("%s() - tag value must be in the range of 5 to %d.", ctx.getMetaruleName(), maxBox);
	}
}

void sf_inventory_redraw(OpcodeContext& ctx) {
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

	if (!ctx.arg(0).asBool()) {
		fo::var::stack_offset[fo::var::curr_stack] = 0;
		fo::func::display_inventory(0, -1, mode);
	} else if (mode >= 2) {
		fo::var::target_stack_offset[fo::var::target_curr_stack] = 0;
		fo::func::display_target_inventory(0, -1, fo::var::target_pud, mode);
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

void sf_create_win(OpcodeContext& ctx) {
	int flags = (ctx.arg(5).type() != DataType::NONE)
		? ctx.arg(5).asInt()
		: fo::WinFlags::MoveOnTop;

	if (fo::func::createWindow(ctx.arg(0).asString(),
		ctx.arg(1).asInt(), ctx.arg(2).asInt(), // y, x
		ctx.arg(3).asInt(), ctx.arg(4).asInt(), // w, h
		256, flags) == -1)
	{
		ctx.printOpcodeError("%s() - couldn't create window.", ctx.getMetaruleName());
	}
}

static void DrawImage(OpcodeContext& ctx, bool isScaled) {
	long rotation = 0;
	const char* file = nullptr;
	if (ctx.arg(0).isInt()) { // art id
		long fid = ctx.arg(0).rawValue();
		if (fid == -1) return;
		long _fid = fid & 0xFFFFFFF;
		file = fo::func::art_get_name(_fid); // .frm
		if (_fid >> 24 == fo::OBJ_TYPE_CRITTER) {
			rotation = (fid >> 28);
			DWORD sz;
			if (rotation && fo::func::db_file_exist(file, &sz)) {
				file = fo::func::art_get_name(fid); // .fr#
			}
		}
	} else {
		file = ctx.arg(0).strValue(); // path to frm file
	}
	fo::FrmFile* frmPtr = nullptr;
	if (fo::func::load_frame(file, &frmPtr)) {
		ctx.printOpcodeError("%s() - can't open the file '%s'.", ctx.getMetaruleName(), file);
		return;
	}
	fo::FrmFrameData* framePtr = (fo::FrmFrameData*)&frmPtr->width;
	if (rotation > 0 && rotation < 6) {
		BYTE* offsOriFrame = (BYTE*)framePtr;
		offsOriFrame += frmPtr->oriFrameOffset[rotation];
		framePtr = (fo::FrmFrameData*)offsOriFrame;
	}
	int frameno = ctx.arg(1).rawValue();
	if (frameno > 0) {
		int maxFrames = frmPtr->frames - 1;
		if (frameno > maxFrames) frameno = maxFrames;
		while (frameno-- > 0) {
			BYTE* offsFrame = (BYTE*)framePtr;
			offsFrame += framePtr->size + (sizeof(fo::FrmFrameData) - 1);
			framePtr = (fo::FrmFrameData*)offsFrame;
		}
	}
	if (isScaled && ctx.numArgs() < 3) {
		fo::func::displayInWindow(framePtr->width, framePtr->width, framePtr->height, framePtr->data); // scaled to window size
	} else {
		int x = ctx.arg(2).rawValue(), y = ctx.arg(3).rawValue();
		if (isScaled) { // draw to scale
			long s_width  = (ctx.numArgs() > 4) ? ctx.arg(4).rawValue() : framePtr->width;
			long s_height = (ctx.numArgs() > 5) ? ctx.arg(5).rawValue() : framePtr->height;
			long w_width = fo::func::windowWidth();
			long xy_pos = (x * w_width) + y;
			fo::func::trans_cscale(framePtr->width, framePtr->height, s_width, s_height, xy_pos, w_width, framePtr->data); // custom scaling
		} else {
			fo::func::windowDisplayBuf(x + frmPtr->xshift[rotation], framePtr->width, y + frmPtr->yshift[rotation], framePtr->height, framePtr->data, ctx.arg(4).rawValue());
		}
	}
	__asm {
		mov  eax, frmPtr;
		call fo::funcoffs::mem_free_;
	}
}

void sf_draw_image(OpcodeContext& ctx) {
	DrawImage(ctx, false);
}

void sf_draw_image_scaled(OpcodeContext& ctx) {
	DrawImage(ctx, true);
}

}
}
