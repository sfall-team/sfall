/**
	X-Macro for defining engine function wrappers.

	Add one line for every function you want to call from C++ code.

	Format:
	WRAP_WATCOM_FUNCX(retType, name, argType1, argName1, ...)
	- X - number of arguments
	- retType - function return type
	- name - function name (without trailing underscore)
	- argType - type of argument
	- argName - name of argument (repeat for all of X arguments)

	NOTES: be careful not to use reserved words, including ASM instructions (push, pop, mov, div, etc.)
*/

WRAP_WATCOM_FUNC1(Program*, allocateProgram, const char*, filePath)
WRAP_WATCOM_FUNC0(void, art_flush)
WRAP_WATCOM_FUNC5(long, art_id, long, artType, long, lstIndex, long, animCode, long, weaponCode, long, directionCode)
WRAP_WATCOM_FUNC3(BYTE*, art_frame_data, FrmFrameData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, art_frame_width, FrmFrameData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, art_frame_length, FrmFrameData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC2(FrmFrameData*, art_ptr_lock, long, frmId, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, art_ptr_lock_data, long, frmId, long, frameNum, long, rotation, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, art_lock, long, frmId, DWORD*, lockPtr, long*, widthOut, long*, heightOut)
WRAP_WATCOM_FUNC1(long, art_ptr_unlock, DWORD, lockId)
WRAP_WATCOM_FUNC1(void*, dbase_open, const char*, fileName)
WRAP_WATCOM_FUNC1(void, dbase_close, void*, dbPtr)
WRAP_WATCOM_FUNC3(long, db_freadShortCount, DbFile*, file, WORD*, dest, long, count)
WRAP_WATCOM_FUNC3(long, db_freadIntCount, DbFile*, file, DWORD*, dest, long, count)
WRAP_WATCOM_FUNC2(long, db_fwriteByte, DbFile*, file, long, value)
WRAP_WATCOM_FUNC2(long, db_fwriteInt, DbFile*, file, long, value)
// perform combat turn for a given critter
WRAP_WATCOM_FUNC2(long, combat_turn, GameObject*, critter, long, isDudeTurn)
WRAP_WATCOM_FUNC1(long, critter_is_dead, GameObject*, critter)
WRAP_WATCOM_FUNC1(void, EndLoad, DbFile*, file)
WRAP_WATCOM_FUNC1(long, game_get_global_var, long, globalVar)
WRAP_WATCOM_FUNC1(long, gmouse_set_cursor, long, picNum)
WRAP_WATCOM_FUNC1(Window*, GNW_find, long, winRef)
WRAP_WATCOM_FUNC0(long, intface_is_item_right_hand)
WRAP_WATCOM_FUNC0(void, intface_toggle_item_state)
WRAP_WATCOM_FUNC0(void, intface_use_item)
WRAP_WATCOM_FUNC1(long, item_w_max_ammo, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_curr_ammo, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_weight, GameObject*, item)
// returns light level at given tile
WRAP_WATCOM_FUNC2(long, light_get_tile, long, elevation, long, tileNum)
WRAP_WATCOM_FUNC2(void, mouse_get_position, long*, outX, long*, outY)
WRAP_WATCOM_FUNC0(void, mouse_show)
WRAP_WATCOM_FUNC0(void, mouse_hide)
// calculates path and returns it's length
WRAP_WATCOM_FUNC6(long, make_path_func, GameObject*, objectFrom, long, tileFrom, long, tileTo, char*, pathDataBuffer, long, arg5, void*, blockingFunc)
// calculates bounding box (rectangle) for a given object
WRAP_WATCOM_FUNC2(void, obj_bound, GameObject*, object, BoundRect*, boundRect)
WRAP_WATCOM_FUNC3(long, register_object_animate, GameObject*, object, long, anim, long, delay)
WRAP_WATCOM_FUNC3(long, register_object_animate_and_hide, GameObject*, object, long, anim, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_animate_and_move_straight_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_animate_forever_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_animate_reverse_, GameObject*, object;
WRAP_WATCOM_FUNC3(long, register_object_change_fid, GameObject*, object, long, artFid, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_check_falling_, GameObject*, object;
WRAP_WATCOM_FUNC1(long, register_object_dec_rotation, GameObject*, object)
WRAP_WATCOM_FUNC1(long, register_object_erase, GameObject*, object)
WRAP_WATCOM_FUNC3(long, register_object_funset, GameObject*, object, long, action, long, delay)
WRAP_WATCOM_FUNC1(long, register_object_inc_rotation, GameObject*, object)
WRAP_WATCOM_FUNC3(long, register_object_light, GameObject*, object, long, lightRadius, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_move_on_stairs_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_straight_to_tile_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_to_object_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_to_tile_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_must_call_, GameObject*, object;
WRAP_WATCOM_FUNC1(long, register_object_must_erase, GameObject*, object)
// WRAP_WATCOM_FUNC3(long, register_object_outline_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_play_sfx_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_run_to_object_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_run_to_tile_, GameObject*, object;
WRAP_WATCOM_FUNC3(long, register_object_take_out, GameObject*, object, long, holdFrameId, long, nothing)
WRAP_WATCOM_FUNC3(long, register_object_turn_towards, GameObject*, object, long, tileNum, long, nothing)
// adds experience points to PC
WRAP_WATCOM_FUNC1(void, stat_pc_add_experience, long, amount)
// redraws the whole screen
WRAP_WATCOM_FUNC0(void, tile_refresh_display)
// redraws the given rectangle on screen
WRAP_WATCOM_FUNC2(void, tile_refresh_rect, BoundRect*, boundRect, long, elevation)
WRAP_WATCOM_FUNC1(long, text_font, long, fontNum)
WRAP_WATCOM_FUNC6(DWORD, win_add, long, x, long, y, long, width, long, height, long, bgColorIndex, long, flags)
WRAP_WATCOM_FUNC1(void, win_show, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_hide, DWORD, winRef)
WRAP_WATCOM_FUNC1(BYTE*, win_get_buf, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_draw, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_delete, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, wmCarUseGas, long, gasAmount)
WRAP_WATCOM_FUNC0(void, wmPartyWalkingStep)
WRAP_WATCOM_FUNC2(DbFile*, xfopen, const char*, fileName, const char*, flags)
WRAP_WATCOM_FUNC3(long, xfseek, DbFile*, file, long, fOffset, long, origin)
