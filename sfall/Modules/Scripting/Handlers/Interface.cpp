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

#include "..\..\..\FalloutEngine\AsmMacros.h"
#include "..\..\..\FalloutEngine\Fallout2.h"

#include "..\..\..\InputFuncs.h"
#include "..\..\BarBoxes.h"
#include "..\..\ExtraArt.h"
#include "..\..\LoadGameHook.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Interface.h"
#include "..\Arrays.h"

#include "..\..\..\Game\GUI\render.h"

#include "..\..\SubModules\WindowRender.h"

#include "Interface.h"

namespace sfall
{
namespace script
{

__declspec(naked) void op_input_funcs_available() {
	__asm {
		mov  edx, 1; // They're always available from 2.9 on
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static BYTE pipboyMovieCheck = 0;

void PipboyAvailableRestore() {
	if (pipboyMovieCheck) {
		SafeWrite8(0x497011, pipboyMovieCheck);
		pipboyMovieCheck = 0;
	}
}

void op_set_pipboy_available(OpcodeContext& ctx) {
	if (!pipboyMovieCheck) pipboyMovieCheck = *(BYTE*)0x497011; // should be either jnz or jmp(short)

	switch (ctx.arg(0).rawValue()) {
	case 0:
		fo::var::gmovie_played_list[3] = false;
		SafeWrite8(0x497011, CodeType::JumpNZ); // restore the vault suit movie check
		break;
	case 1:
		fo::var::gmovie_played_list[3] = true;
		break;
	case 2:
		SafeWrite8(0x497011, CodeType::JumpShort); // skip the vault suit movie check
		break;
	}
}

void op_key_pressed(OpcodeContext& ctx) {
	ctx.setReturn(KeyDown(ctx.arg(0).rawValue()));
}

__declspec(naked) void op_tap_key() {
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

__declspec(naked) void op_get_mouse_x() {
	__asm {
		mov  edx, ds:[FO_VAR_mouse_x_];
		add  edx, ds:[FO_VAR_mouse_hotx];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_get_mouse_y() {
	__asm {
		mov  edx, ds:[FO_VAR_mouse_y_];
		add  edx, ds:[FO_VAR_mouse_hoty];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

enum { MOUSE_MIDDLE_BTN = 4 };

void op_get_mouse_buttons(OpcodeContext& ctx) {
	DWORD button = fo::var::last_buttons;
	if (button == 0 && middleMouseDown) {
		button = MOUSE_MIDDLE_BTN;
	}
	ctx.setReturn(button);
}

__declspec(naked) void op_get_window_under_mouse() {
	__asm {
		mov  edx, ds:[FO_VAR_last_button_winID];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_get_screen_width() {
	__asm {
		mov  edx, ds:[FO_VAR_scr_size + 8]; // _scr_size.offx
		sub  edx, ds:[FO_VAR_scr_size];     // _scr_size.x
		inc  edx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_get_screen_height() {
	__asm {
		mov  edx, ds:[FO_VAR_scr_size + 12]; // _scr_size.offy
		sub  edx, ds:[FO_VAR_scr_size + 4];  // _scr_size.y
		inc  edx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

// copy and split
static void SplitToBuffer(const char* str, const char** str_ptr, long &lines) {
	size_t i = 0;
	while (str[i]) {
		if (str[i] == '\n' && lines < 4) {
			ScriptExtender::gTextBuffer[i] = '\0';
			str_ptr[lines++] = &ScriptExtender::gTextBuffer[++i];
		} else {
			ScriptExtender::gTextBuffer[i] = str[i++];
		}
	};
	ScriptExtender::gTextBuffer[i] = '\0';
}

void op_create_message_window(OpcodeContext &ctx) {
	static bool dialogShow = false;
	if (dialogShow) return;

	const char* str = ctx.arg(0).strValue();
	if (!str || str[0] == 0) return;

	long lines = 0;
	const char* str_ptr[4];
	SplitToBuffer(str, str_ptr, lines);

	dialogShow = true;
	fo::func::DialogOutEx(ScriptExtender::gTextBuffer, str_ptr, lines, fo::DIALOGOUT_NORMAL);
	dialogShow = false;
}

void mf_message_box(OpcodeContext &ctx) {
	static int dialogShowCount = 0;

	long lines = 0;
	const char* str_ptr[4];
	SplitToBuffer(ctx.arg(0).asString(), str_ptr, lines);

	long colors = 0x9191, flags = fo::DIALOGOUT_NORMAL | fo::DIALOGOUT_YESNO;
	if (ctx.numArgs() > 1 && ctx.arg(1).rawValue() != -1) flags = ctx.arg(1).rawValue();
	if (ctx.numArgs() > 2) {
		colors &= 0xFF00;
		colors |= (ctx.arg(2).rawValue() & 0xFF);
	}
	if (ctx.numArgs() > 3) {
		colors &= 0xFF;
		colors |= (ctx.arg(3).rawValue() & 0xFF) << 8;
	}
	dialogShowCount++;
	fo::var::setInt(FO_VAR_script_engine_running) = 0;
	long result = fo::func::DialogOutEx(ScriptExtender::gTextBuffer, str_ptr, lines, flags, colors);
	if (--dialogShowCount == 0) fo::var::setInt(FO_VAR_script_engine_running) = 1;

	ctx.setReturn(result);
}

__declspec(naked) void op_get_viewport_x() {
	__asm {
		mov  edx, ds:[FO_VAR_wmWorldOffsetX];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_get_viewport_y() {
	__asm {
		mov  edx, ds:[FO_VAR_wmWorldOffsetY];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_set_viewport_x() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_wmWorldOffsetX], eax;
end:
		retn;
	}
}

__declspec(naked) void op_set_viewport_y() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_wmWorldOffsetY], eax;
end:
		retn;
	}
}

void mf_add_iface_tag(OpcodeContext &ctx) {
	int result = BarBoxes::AddExtraBox();
	if (result == -1) ctx.printOpcodeError("%s() - cannot add new tag as the maximum limit of 126 tags has been reached.", ctx.getMetaruleName());
	ctx.setReturn(result);
}

void op_show_iface_tag(OpcodeContext &ctx) {
	int tag = ctx.arg(0).rawValue();
	if (tag == 0 || tag == 3 || tag == 4) {
		if (!fo::func::is_pc_flag(tag)) fo::func::pc_flag_on(tag);
	} else {
		BarBoxes::AddBox(tag);
	}
}

void op_hide_iface_tag(OpcodeContext &ctx) {
	int tag = ctx.arg(0).rawValue();
	if (tag == 0 || tag == 3 || tag == 4) {
		if (fo::func::is_pc_flag(tag)) fo::func::pc_flag_off(tag);
	} else {
		BarBoxes::RemoveBox(tag);
	}
}

void op_is_iface_tag_active(OpcodeContext &ctx) {
	bool result = false;
	int tag = ctx.arg(0).rawValue();
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
			fo::Proto* proto = fo::util::GetProto(obj->protoId);
			int flagBit = 1 << tag;
			result = ((proto->critter.critterFlags & flagBit) != 0);
		}
	} else {
		result = BarBoxes::GetBox(tag);
	}
	ctx.setReturn(result);
}

void mf_intface_redraw(OpcodeContext& ctx) {
	if (ctx.numArgs() == 0) {
		fo::func::intface_redraw();
	} else {
		// fake redraw interfaces (TODO: need a real redraw of interface?)
		long winType = ctx.arg(0).rawValue();
		if (winType == -1) {
			fo::util::RefreshGNW(true);
		} else {
			fo::Window* win = Interface::GetWindow(winType);
			if (win && (int)win != -1) game::gui::Render::GNW_win_refresh(win, &win->wRect, 0);
		}
	}
}

void mf_intface_show(OpcodeContext& ctx) {
	__asm call fo::funcoffs::intface_show_;
}

void mf_intface_hide(OpcodeContext& ctx) {
	__asm call fo::funcoffs::intface_hide_;
}

void mf_intface_is_hidden(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::intface_is_hidden());
}

void mf_tile_refresh_display(OpcodeContext& ctx) {
	fo::func::tile_refresh_display();
}

void mf_get_cursor_mode(OpcodeContext& ctx) {
	ctx.setReturn(fo::var::gmouse_3d_current_mode);
}

void mf_set_cursor_mode(OpcodeContext& ctx) {
	fo::func::gmouse_3d_set_mode(ctx.arg(0).rawValue());
}

void mf_display_stats(OpcodeContext& ctx) {
	unsigned long flags = GetLoopFlags();
	if (flags & LoopFlag::INVENTORY) {
		fo::func::display_stats(); // calling the function outside of inventory screen will crash the game
	} else if (flags & LoopFlag::CHARSCREEN) {
		__asm {
			mov  eax, ds:[FO_VAR_obj_dude];
			call fo::funcoffs::stat_recalc_derived_;
			xor  edx, edx;
			mov  eax, ds:[FO_VAR_obj_dude];
			call fo::funcoffs::critter_adjust_hits_;
			push ebx;
			mov  eax, 7;
			call fo::funcoffs::PrintBasicStat_;
			xor  eax, eax;
			call fo::funcoffs::ListSkills_;
			call fo::funcoffs::PrintLevelWin_;
			call fo::funcoffs::ListDrvdStats_;
			pop  ebx;
		}
		fo::func::win_draw(fo::var::edit_win);
	}
}

void mf_set_iface_tag_text(OpcodeContext& ctx) {
	int boxTag = ctx.arg(0).rawValue();
	int maxBox = BarBoxes::MaxBox();

	if (boxTag > 4 && boxTag <= maxBox) {
		BarBoxes::SetText(boxTag, ctx.arg(1).strValue(), ctx.arg(2).rawValue());
	} else {
		ctx.printOpcodeError("%s() - tag value must be in the range of 5 to %d.", ctx.getMetaruleName(), maxBox);
		ctx.setReturn(-1);
	}
}

void mf_inventory_redraw(OpcodeContext& ctx) {
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
	long redrawSide = (ctx.numArgs() > 0) ? ctx.arg(0).rawValue() : -1; // -1 - both
	if (redrawSide <= 0) {
		fo::var::stack_offset[fo::var::curr_stack] = 0;
		fo::func::display_inventory(0, -1, mode);
	}
	if (redrawSide && mode >= 2) {
		fo::var::target_stack_offset[fo::var::target_curr_stack] = 0;
		fo::func::display_target_inventory(0, -1, fo::var::target_pud, mode);
		fo::func::win_draw(fo::var::i_wid);
	}
}

void mf_dialog_message(OpcodeContext& ctx) {
	DWORD loopFlag = GetLoopFlags();
	if ((loopFlag & DIALOGVIEW) == 0 && (loopFlag & DIALOG)) {
		const char* message = ctx.arg(0).strValue();
		fo::func::gdialogDisplayMsg(message);
	}
}

void mf_create_win(OpcodeContext& ctx) {
	int flags = (ctx.numArgs() > 5)
	          ? ctx.arg(5).rawValue()
	          : fo::WinFlags::MoveOnTop;

	if (fo::func::createWindow(ctx.arg(0).strValue(),
	    ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), // x, y
	    ctx.arg(3).rawValue(), ctx.arg(4).rawValue(), // w, h
	    (flags & fo::WinFlags::Transparent) ? 0 : 256, flags) == -1)
	{
		ctx.printOpcodeError("%s() - couldn't create window.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_show_window(OpcodeContext& ctx) {
	if (ctx.numArgs() > 0) {
		const char* name = ctx.arg(0).strValue();
		for (size_t i = 0; i < 16; i++) {
			if (_stricmp(name, fo::var::sWindows[i].name) == 0) {
				fo::func::win_show(fo::var::sWindows[i].wID);
				return;
			}
		}
		ctx.printOpcodeError("%s() - window '%s' is not found.", ctx.getMetaruleName(), name);
	} else {
		__asm call fo::funcoffs::windowShow_;
	}
}

void mf_hide_window(OpcodeContext& ctx) {
	if (ctx.numArgs() > 0) {
		const char* name = ctx.arg(0).strValue();
		for (size_t i = 0; i < 16; i++) {
			if (_stricmp(name, fo::var::sWindows[i].name) == 0) {
				fo::func::win_hide(fo::var::sWindows[i].wID);
				return;
			}
		}
		ctx.printOpcodeError("%s() - window '%s' is not found.", ctx.getMetaruleName(), name);
	} else {
		__asm call fo::funcoffs::windowHide_;
	}
}

void mf_set_window_flag(OpcodeContext& ctx) {
	long bitFlag = ctx.arg(1).rawValue();
	switch (bitFlag) {
	case fo::WinFlags::DontMoveTop:
	case fo::WinFlags::MoveOnTop:
	case fo::WinFlags::Hidden:
	case fo::WinFlags::Exclusive:
	case fo::WinFlags::Transparent:
		break;
	default:
		return; // unsupported set flag
	}
	bool mode = ctx.arg(2).asBool();
	if (ctx.arg(0).isString()) {
		const char* name = ctx.arg(0).strValue();
		for (size_t i = 0; i < 16; i++) {
			if (_stricmp(name, fo::var::sWindows[i].name) == 0) {
				fo::Window* win = fo::func::GNW_find(fo::var::sWindows[i].wID);
				if (mode) {
					fo::var::sWindows[i].flags |= bitFlag;
					win->flags |= bitFlag;
				} else {
					fo::var::sWindows[i].flags &= ~bitFlag;
					win->flags &= ~bitFlag;
				}
				return;
			}
		}
		ctx.printOpcodeError("%s() - window '%s' is not found.", ctx.getMetaruleName(), name);
	} else {
		long wid = ctx.arg(0).rawValue();
		fo::Window* win = fo::func::GNW_find((wid > 0) ? wid : fo::var::i_wid); // i_wid - set flag to current game interface window
		if (win == nullptr) return;
		if (mode) {
			win->flags |= bitFlag;
		} else {
			win->flags &= ~bitFlag;
		}
	}
}

// raw frame data loaded from FRM or PCX
struct FrameData {
	BYTE* pixelData = nullptr;
	short width = 0;
	short height = 0;
	short xOffset = 0;
	short yOffset = 0;

	// Empty data.
	FrameData() {}

	// Data from FRM file.
	FrameData(fo::FrmFile* frmPtr, long direction, long frame) {
		fo::FrmFrameData* frmData = frmPtr->GetFrameData(direction, frame);
		pixelData = frmData->data;
		width = frmData->width;
		height = frmData->height;
		xOffset = frmPtr->xOffsets[direction];
		yOffset = frmPtr->yOffsets[direction];
	}

	// Data from PCX file.
	FrameData(PcxFile pcx) {
		pixelData = pcx.pixelData;
		width = (short)pcx.width;
		height = (short)pcx.height;
	}
};

static bool IsPCXFile(const char* file) {
	const char* pos = strrchr(file, '.');
	return pos && _stricmp(++pos, "PCX") == 0;
}

//static fo::FrmFile* LoadArtFileCached(const char* file, long frame, long direction, fo::FrmFrameData* &framePtr, bool checkPCX) {
static FrameData LoadFrameDataCached(const char* file, long frame, long direction) {
	if (IsPCXFile(file)) {
		return LoadPcxFileCached(file);
	}
	fo::FrmFile* frmPtr = LoadFrmFileCached(file);
	return (frmPtr != nullptr)
	       ? FrameData(frmPtr, direction, frame)
	       : FrameData();
}

static long GetArtFIDFile(long fid, char* outFilePath) {
	long direction = 0;
	long _fid = fid & 0xFFFFFFF;

	const char* artPathName = fo::func::art_get_name(_fid); // <cd>\art\type\file.frm

	if (_fid >> 24 == fo::OBJ_TYPE_CRITTER) {
		direction = (fid >> 28);
		if (direction > 0 && !fo::func::db_access(artPathName)) {
			artPathName = fo::func::art_get_name(fid); // .fr#
		}
	} else {
		if (fo::var::use_language) {
			const char* _artPathName = std::strchr(artPathName, '\\');
			if (_artPathName && *(++_artPathName)) {
				sprintf_s(outFilePath, MAX_PATH, "art\\%s\\%s", (const char*)fo::var::language, _artPathName);
				if (fo::func::db_access(outFilePath)) return direction;
			}
		}
	}
	std::strcpy(outFilePath, artPathName);
	return direction;
}

static FrameData LockFrameData(unsigned long fid, fo::util::ArtCacheLock& lock, long direction, long frame) {
	long objType = (fid >> 24) & 0xF;
	if (direction < 0) {
		// If direction is not specified, take it from FID.
		direction = (objType == fo::OBJ_TYPE_CRITTER)
		          ? (fid >> 28)
		          : 0;
	} else if (objType == fo::OBJ_TYPE_CRITTER) {
		// Apply direction to FID.
		fid = (direction << 28) | (fid & (0xFFFFFFF));
	}
	return FrameData(fo::func::art_ptr_lock(fid, &lock.entryPtr), direction, frame);
}

static long DrawImage(OpcodeContext& ctx, bool isScaled) {
	if (!fo::func::selectWindowID(ctx.program()->currentScriptWin) || fo::var::getInt(FO_VAR_currentWindow) == -1) {
		ctx.printOpcodeError("%s() - no created or selected window.", ctx.getMetaruleName());
		return 0;
	}
	FrameData frm;
	fo::util::ArtCacheLock cacheLock;

	bool isID = ctx.arg(0).isInt();
	long frame = ctx.arg(1).rawValue();
	if (isID) { // art id
		long fid = ctx.arg(0).rawValue();
		if (fid == -1) return -1;

		frm = LockFrameData(fid, cacheLock, -1, frame);
		if (frm.pixelData == nullptr) {
			ctx.printOpcodeError("%s() - cannot load art by FID: %d", ctx.getMetaruleName(), fid);
			return -1;
		}
	} else {
		const char* file = ctx.arg(0).strValue();
		frm = LoadFrameDataCached(file, frame, 0);
		if (frm.pixelData == nullptr) {
			ctx.printOpcodeError("%s() - cannot load art from file: %s", ctx.getMetaruleName(), file);
			return -1;
		}
	}

	long result = 1;
	if (isScaled && ctx.numArgs() < 3) {
		fo::func::displayInWindow(frm.width, frm.width, frm.height, frm.pixelData); // scaled to window size (w/o transparent)
	} else {
		int x = ctx.arg(2).rawValue(), y = ctx.arg(3).rawValue();
		if (isScaled) { // draw to scale
			long s_width, s_height;
			if (ctx.numArgs() < 5) {
				s_width = frm.width;
				s_height = frm.height;
			} else {
				s_width = ctx.arg(4).rawValue();
				s_height = (ctx.numArgs() > 5) ? ctx.arg(5).rawValue() : -1;
			}
			// scale with aspect ratio if w or h is set to -1
			if (s_width <= -1 && s_height > 0) {
				s_width = s_height * frm.width / frm.height;
			} else if (s_height <= -1 && s_width > 0) {
				s_height = s_width * frm.height / frm.width;
			}
			if (s_width <= 0 || s_height <= 0) {
				return 0;
			}

			long w_width = fo::func::windowWidth();
			long xy_pos = (y * w_width) + x;
			fo::func::window_trans_cscale(frm.width, frm.height, s_width, s_height, xy_pos, w_width, frm.pixelData); // custom scaling
		} else { // with x/y frame offsets
			fo::func::windowDisplayBuf(x + frm.xOffset, frm.width, y + frm.yOffset, frm.height, frm.pixelData, ctx.arg(4).rawValue());
		}
	}
	return result;
}

void mf_draw_image(OpcodeContext& ctx) {
	ctx.setReturn(DrawImage(ctx, false));
}

void mf_draw_image_scaled(OpcodeContext& ctx) {
	ctx.setReturn(DrawImage(ctx, true));
}

static DWORD GetArtFrameSize(OpcodeContext& ctx) {
	long width, height;
	bool isID = ctx.arg(0).isInt();
	if (isID) { // art id
		long fid = ctx.arg(0).rawValue();
		if (fid == -1) return -1;

		// Get data directly from game's art cache.
		DWORD lockPtr;
		fo::FrmFile* art = fo::func::art_ptr_lock(fid, &lockPtr);
		if (art == nullptr) {
			ctx.printOpcodeError("%s() - cannot load art by FID: %d", ctx.getMetaruleName(), fid);
			return -1;
		}
		DWORD result = fo::func::art_frame_width_length(art, ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), &width, &height);
		fo::func::art_ptr_unlock(lockPtr);
	} else {
		// Load data from DB.
		const char* file = ctx.arg(0).strValue(); // path to frm/pcx file
		FrameData frm = LoadFrameDataCached(file, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());
		if (frm.pixelData == nullptr) {
			ctx.printOpcodeError("%s() - cannot load art from file: %s", ctx.getMetaruleName(), file);
			return -1;
		}
		width = frm.width;
		height = frm.height;
	}

	DWORD arrayId = CreateTempArray(4, 0);
	SetArray(arrayId, 0, width);
	SetArray(arrayId, 1, height);
	return arrayId;
}

void mf_art_frame_data(OpcodeContext& ctx) {
	ctx.setReturn(GetArtFrameSize(ctx));
}

static long InterfaceDrawImage(OpcodeContext& ctx, fo::Window* ifaceWin) {
	bool useShift = false;
	long direction = -1, w = -1, h = -1;

	bool isID = ctx.arg(1).isInt();
	long frame = ctx.arg(4).rawValue();
	if (ctx.numArgs() > 5) { // array params
		sArrayVar* sArray = GetRawArray(ctx.arg(5).rawValue());
		if (sArray) {
			if (direction < 0) direction = sArray->val[0].intVal;
			int size = sArray->size();
			if (size > 1) w = sArray->val[1].intVal;
			if (size > 2) h = sArray->val[2].intVal;
		}
	}
	fo::util::ArtCacheLock cacheLock;
	FrameData frm;
	if (isID) { // art id
		long fid = ctx.arg(1).rawValue();
		if (fid == -1) return -1;

		useShift = (((fid & 0xF000000) >> 24) == fo::OBJ_TYPE_CRITTER);

		frm = LockFrameData(fid, cacheLock, direction, frame);
		if (frm.pixelData == nullptr) {
			ctx.printOpcodeError("%s() - cannot load art by FID: %d", ctx.getMetaruleName(), fid);
			return -1;
		}
	} else {
		const char* file = ctx.arg(1).strValue(); // path to frm/pcx file
		frm = LoadFrameDataCached(file, frame, direction);
		if (frm.pixelData == nullptr) {
			ctx.printOpcodeError("%s() - load art from file: %s", ctx.getMetaruleName(), file);
			return -1;
		}
	}

	int x = ctx.arg(2).rawValue();
	int y = ctx.arg(3).rawValue();

	if (useShift && direction >= 0) {
		x += frm.xOffset;
		y += frm.yOffset;
	}
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	int width  = (w >= 0) ? w : frm.width;
	int height = (h >= 0) ? h : frm.height;

	if (x + width > ifaceWin->width || y + height > ifaceWin->height) {
		ctx.printOpcodeError("%s() - attempt to draw beyond window bounds (%d, %d)", ctx.getMetaruleName(), ifaceWin->width, ifaceWin->height);
		return -1;
	}

	BYTE* surface = (ifaceWin->randY) ? WindowRender::GetOverlaySurface(ifaceWin) : ifaceWin->surface;

	fo::func::trans_cscale(frm.pixelData, frm.width, frm.height, frm.width,
	                       surface + (y * ifaceWin->width) + x, width, height, ifaceWin->width
	);

	if (!(ctx.arg(0).rawValue() & 0x1000000)) { // is set to "Don't redraw"
		game::gui::Render::GNW_win_refresh(ifaceWin, &ifaceWin->wRect, 0);
	}
	return 1;
}

void mf_interface_art_draw(OpcodeContext& ctx) {
	long result = -1;
	fo::Window* win = Interface::GetWindow(ctx.arg(0).rawValue() & 0xFF);
	if (win && (int)win != -1) {
		result = InterfaceDrawImage(ctx, win);
	} else {
		ctx.printOpcodeError("%s() - the game interface window is not created or invalid window type number.", ctx.getMetaruleName());
	}
	ctx.setReturn(result);
}

void mf_get_window_attribute(OpcodeContext& ctx) {
	fo::Window* win = Interface::GetWindow(ctx.arg(0).rawValue());
	if (win == nullptr) {
		if (ctx.arg(1).rawValue() != 0) {
			ctx.printOpcodeError("%s() - failed to get the interface window.", ctx.getMetaruleName());
			ctx.setReturn(-1);
		}
		return;
	}
	if ((long)win == -1) {
		ctx.printOpcodeError("%s() - invalid window type number.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	long result = 0;
	switch (ctx.arg(1).rawValue()) {
	case -1: // rectangle map.left map.top map.right map.bottom
		result = CreateTempArray(-1, 0); // associative
		SetArray(result, ScriptValue("left"), ScriptValue(win->rect.x), false);
		SetArray(result, ScriptValue("top"), ScriptValue(win->rect.y), false);
		SetArray(result, ScriptValue("right"), ScriptValue(win->rect.offx), false);
		SetArray(result, ScriptValue("bottom"), ScriptValue(win->rect.offy), false);
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
	case 5:
		result = win->wID;
		break;
	}
	ctx.setReturn(result);
}

void mf_interface_print(OpcodeContext& ctx) { // same as vanilla PrintRect
	fo::Window* win = Interface::GetWindow(ctx.arg(1).rawValue());
	if (win == nullptr || (int)win == -1) {
		ctx.printOpcodeError("%s() - the game interface window is not created or invalid window type number.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	const char* text = ctx.arg(0).strValue();

	long x = ctx.arg(2).rawValue();
	if (x < 0) x = 0;
	long y = ctx.arg(3).rawValue();
	if (y < 0) y = 0;

	long color = ctx.arg(4).rawValue();
	long width = ctx.arg(5).rawValue();

	int maxHeight = win->height - y;
	int maxWidth = win->width - x;
	if (width <= 0) {
		width = maxWidth;
	} else if (width > maxWidth) {
		width = maxWidth;
	}

	color ^= 0x2000000; // fills background with black color if the flag is set (textnofill)
	if ((color & 0xFF) == 0) {
		__asm call fo::funcoffs::windowGetTextColor_; // set from SetTextColor
		__asm mov  byte ptr color, al;
	}

	BYTE* surface;
	if (win->randY) { // if a surface was created, the engine will draw on it
		surface = win->surface;
		win->surface = WindowRender::GetOverlaySurface(win); // replace the surface for the windowWrapLineWithSpacing_ function
	}

	if (color & 0x10000) { // shadow (textshadow)
		fo::func::windowWrapLineWithSpacing(win->wID, text, width, maxHeight, x, y, 0x201000F, 0, 0);
		color ^= 0x10000;
	}
	ctx.setReturn(fo::func::windowWrapLineWithSpacing(win->wID, text, width, maxHeight, x, y, color, 0, 0)); // returns count of lines printed

	if (win->randY) win->surface = surface;

	// no redraw (textdirect)
	if (!(color & 0x1000000)) game::gui::Render::GNW_win_refresh(win, &win->wRect, 0);
}

void mf_win_fill_color(OpcodeContext& ctx) {
	long result = fo::func::selectWindowID(ctx.program()->currentScriptWin); // TODO: examine the issue of restoring program->currentScriptWin of the current window in op_pop_flags_
	long iWin = fo::var::getInt(FO_VAR_currentWindow);
	if (!result || iWin == -1) {
		ctx.printOpcodeError("%s() - no created or selected window.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	if (ctx.numArgs() > 0) {
		if (fo::util::WinFillRect(fo::var::sWindows[iWin].wID,
		                          ctx.arg(0).rawValue(), ctx.arg(1).rawValue(), // x, y
		                          ctx.arg(2).rawValue(), ctx.arg(3).rawValue(), // w, h
		                          static_cast<BYTE>(ctx.arg(4).rawValue())))
		{
			ctx.printOpcodeError("%s() - the fill area is truncated because it exceeds the current window.", ctx.getMetaruleName());
		}
	} else {
		fo::util::ClearWindow(fo::var::sWindows[iWin].wID, false); // full clear
	}
}

void mf_interface_overlay(OpcodeContext& ctx) {
	long winType = ctx.arg(0).rawValue();

	fo::Window* win = Interface::GetWindow(winType);
	if (!win || (int)win == -1) return;

	switch (ctx.arg(1).rawValue()) {
	case 0:
		WindowRender::DestroyOverlaySurface(win);
		break;
	case 1:
		WindowRender::CreateOverlaySurface(win, winType);
		break;
	case 2: // clear
		if (ctx.numArgs() > 2) {
			long w = ctx.arg(4).rawValue();
			long h = ctx.arg(5).rawValue();
			if (w <= 0 || h <= 0) return;

			long x = ctx.arg(2).rawValue();
			long y = ctx.arg(3).rawValue();
			if (x < 0 || y < 0) return;

			Rectangle rect = { x, y, w, h };
			WindowRender::ClearOverlay(win, rect);
		} else {
			WindowRender::ClearOverlay(win);
		}
		break;
	}
}

}
}
