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

using namespace fo;

/*
	For functions that have 3 or more arguments, it is preferable to use the fastcall calling convention
	because the compiler builds the better/optimized code when calling the engine functions
*/
WRAP_WATCOM_FFUNC4(long, _word_wrap, const char*, text, int, maxWidth, DWORD*, buf, BYTE*, count)
WRAP_WATCOM_FFUNC3(void, check_for_death, GameObject*, critter, long, amountDamage, long*, flags)
WRAP_WATCOM_FFUNC3(void, correctFidForRemovedItem, GameObject*, critter, GameObject*, item, long, slotFlag)
WRAP_WATCOM_FFUNC7(long, createWindow, const char*, winName, DWORD, x, DWORD, y, DWORD, width, DWORD, height, long, color, long, flags)
WRAP_WATCOM_FFUNC4(long, determine_to_hit, GameObject*, source, GameObject*, target, long, bodyPart, long, hitMode)
WRAP_WATCOM_FFUNC3(void, display_inventory, long, inventoryOffset, long, visibleOffset, long, mode)
WRAP_WATCOM_FFUNC4(void, display_target_inventory, long, inventoryOffset, long, visibleOffset, DWORD*, targetInventory, long, mode)
WRAP_WATCOM_FFUNC3(FrmFrameData*, frame_ptr, FrmHeaderData*, frm, long, frame, long, direction)
WRAP_WATCOM_FFUNC3(void, GNW_win_refresh, Window*, win, BoundRect*, rect, long*, buffer)
WRAP_WATCOM_FFUNC3(void, intface_update_items, long, animate, long, modeLeft,long, modeRight)
WRAP_WATCOM_FFUNC3(GameObject*, inven_find_type, GameObject*, critter, long, itemType, DWORD*, buf)
WRAP_WATCOM_FFUNC3(long, item_add_force, GameObject*, critter, GameObject*, item, long, count)
WRAP_WATCOM_FFUNC3(long, item_w_mp_cost, GameObject*, source, long, hitMode, long, isCalled)
WRAP_WATCOM_FFUNC7(void, make_straight_path_func, GameObject*, objFrom, DWORD, tileFrom, DWORD, tileTo, void*, rotationPtr, DWORD*, result, long, flags, void*, func)
WRAP_WATCOM_FFUNC3(long, message_find, DWORD*, msgFile, long, msgNumber, DWORD*, outBuf)
WRAP_WATCOM_FFUNC4(long, mouse_click_in, long, x, long, y, long, x_offs, long, y_offs)
WRAP_WATCOM_FFUNC4(long, mouse_in, long, x, long, y, long, x_offs, long, y_offs)
WRAP_WATCOM_FFUNC3(GameObject*, obj_blocking_at, GameObject*, object, long, tile, long, elevation)
WRAP_WATCOM_FFUNC4(long, obj_move_to_tile, GameObject*, object, long, tile, long, elevation, RECT*, rect)
WRAP_WATCOM_FFUNC3(long, obj_new_sid_inst, GameObject*, object, long, sType, long, scriptIndex)
WRAP_WATCOM_FFUNC3(GameObject*, object_under_mouse, long, crSwitch, long, inclDude, long, elevation)
WRAP_WATCOM_FFUNC4(long, queue_add, long, time, GameObject*, object, void*, data, long, qType)
WRAP_WATCOM_FFUNC4(void, register_object_call, long*, target, long*, source, void*, func, long, delay)
WRAP_WATCOM_FFUNC4(long, register_object_move_to_object, GameObject*, source, GameObject*, target, long, distance, long, delay)
WRAP_WATCOM_FFUNC4(long, register_object_run_to_object, GameObject*, source, GameObject*, target, long, distance, long, delay)
WRAP_WATCOM_FFUNC3(long, scr_get_local_var, long, sid, long, varId, long*, value)
WRAP_WATCOM_FFUNC3(long, scr_set_local_var, long, sid, long, varId, long, value)
WRAP_WATCOM_FFUNC3(long, tile_num_in_direction, long, tile, long, rotation,long, distance)
WRAP_WATCOM_FFUNC8(void, trans_cscale, void*, fromBuff, long, width, long, height, long, fromPitch, void*, toBuff, long, toWidth, long, toHeight, long, toPitch)
WRAP_WATCOM_FFUNC3(void, win_clip, Window*, window, RectList**, rects, void*, buffer)
WRAP_WATCOM_FFUNC9(long, windowWrapLineWithSpacing, long, winID, const char*, text, long, width, long, height, long, x, long, y, long, color, long, alignment, long, lineSpacing)

WRAP_WATCOM_FFUNC3(const char*, interpretGetString, Program*, scriptPtr, DWORD, dataType, DWORD, strId)

/* stdcall */
WRAP_WATCOM_FUNC1(AIcap*, ai_cap, GameObject*, critter)
WRAP_WATCOM_FUNC1(Program*, allocateProgram, const char*, filePath)
WRAP_WATCOM_FUNC1(bool, art_exists, long, artFid)
WRAP_WATCOM_FUNC0(void, art_flush)
WRAP_WATCOM_FUNC1(const char*, art_get_name, long, artFID)
WRAP_WATCOM_FUNC5(long, art_id, long, artType, long, lstIndex, long, animCode, long, weaponCode, long, directionCode)
WRAP_WATCOM_FUNC3(BYTE*, art_frame_data, FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, art_frame_width, FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, art_frame_length, FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC2(FrmHeaderData*, art_ptr_lock, long, frmId, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, art_ptr_lock_data, long, frmId, long, frameNum, long, rotation, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, art_lock, long, frmId, DWORD*, lockPtr, long*, widthOut, long*, heightOut)
WRAP_WATCOM_FUNC1(long, art_ptr_unlock, DWORD, lockId)
WRAP_WATCOM_FUNC2(long, barter_compute_value, GameObject*, source, GameObject*, target)
WRAP_WATCOM_FUNC1(long, block_for_tocks, long, ticks)
WRAP_WATCOM_FUNC1(const char*, critter_name, GameObject*, critter) // Returns the name of the critter
WRAP_WATCOM_FUNC1(void, critter_pc_set_name, const char*, newName) // Change the name of playable character
/* Database functions */
WRAP_WATCOM_FUNC1(bool, db_access, const char*, fileName) // Checks if given file exists in DB
WRAP_WATCOM_FUNC1(long, db_fclose, DbFile*, file)
WRAP_WATCOM_FUNC2(DbFile*, db_fopen, const char*, path, const char*, mode)
WRAP_WATCOM_FUNC1(long, db_fgetc, DbFile*, file)
WRAP_WATCOM_FUNC3(char*, db_fgets, char*, buf, long, max_count, DbFile*, file)
WRAP_WATCOM_FUNC4(long, db_fread, void*, buf, long, elsize, long, count, DbFile*, file)
WRAP_WATCOM_FUNC3(long, db_fseek, DbFile*, file, long, pos, long, origin)
WRAP_WATCOM_FUNC2(void, db_free_file_list, char***, fileList, DWORD, arg2) // Destroys filelist array created by db_get_file_list
WRAP_WATCOM_FUNC2(long, db_freadByte, DbFile*, file, BYTE*, _out)
WRAP_WATCOM_FUNC2(long, db_freadShort, DbFile*, file, WORD*, _out)
WRAP_WATCOM_FUNC2(long, db_freadInt, DbFile*, file, DWORD*, _out)
WRAP_WATCOM_FUNC3(long, db_freadByteCount, DbFile*, file, BYTE*, cptr, long, count)
WRAP_WATCOM_FUNC3(long, db_freadShortCount, DbFile*, file, WORD*, dest, long, count)
WRAP_WATCOM_FUNC3(long, db_freadIntCount, DbFile*, file, DWORD*, dest, long, count)
WRAP_WATCOM_FUNC2(long, db_fwriteByte, DbFile*, file, long, value)
WRAP_WATCOM_FUNC2(long, db_fwriteInt, DbFile*, file, long, value)
WRAP_WATCOM_FUNC3(long, db_fwriteByteCount, DbFile*, file, const BYTE*, cptr, long, count)
WRAP_WATCOM_FUNC2(long, db_dir_entry, const char*, fileName, DWORD*, sizeOut) // Check fallout file and get file size (result 0 - file exists)
// Searches files in DB by given path/filename mask and stores result in fileList
// fileList is a pointer to a variable, that will be assigned with an address of an array of char* strings
WRAP_WATCOM_FUNC2(long, db_get_file_list, const char*, searchMask, char***, fileList) // Returns number of elements in *fileList
WRAP_WATCOM_FUNC2(long, db_init, const char*, path_dat, const char*, path_patches)
WRAP_WATCOM_FUNC1(void*, dbase_open, const char*, fileName)
WRAP_WATCOM_FUNC1(void, dbase_close, void*, dbPtr)
////////////////////////
WRAP_WATCOM_FUNC1(void, display_print, const char*, msg) // Displays message in main UI console window
WRAP_WATCOM_FUNC0(void, display_stats)
WRAP_WATCOM_FUNC2(long, combat_turn, GameObject*, critter, long, isDudeTurn) // Perform combat turn for a given critter
WRAP_WATCOM_FUNC1(long, critter_is_dead, GameObject*, critter)
WRAP_WATCOM_FUNC1(void, EndLoad, DbFile*, file)
// Execute script proc by internal proc number (from script's proc table, basically a sequential number of a procedure as defined in code, starting from 1)
WRAP_WATCOM_FUNC2(void, executeProcedure, Program*, sptr, long, procNum)
WRAP_WATCOM_FUNC1(long, folder_print_line, const char*, text)
WRAP_WATCOM_FUNC1(long, folder_print_seperator, const char*, text)
WRAP_WATCOM_FUNC1(const char*, findCurrentProc, Program*, program) // Returns the name of current procedure by program pointer
WRAP_WATCOM_FUNC1(long, FMtext_width, const char*, text)
WRAP_WATCOM_FUNC1(long, game_get_global_var, long, globalVar)
WRAP_WATCOM_FUNC0(long, get_input)
// Searches for message ID in given message file and places result in result argument
WRAP_WATCOM_FUNC3(const char*, getmsg, const MessageList*, fileAddr, MessageNode*, result, long, messageId)
WRAP_WATCOM_FUNC1(void, gdialogDisplayMsg, const char*, message)
WRAP_WATCOM_FUNC0(long, gmouse_3d_get_mode)
WRAP_WATCOM_FUNC1(void, gmouse_3d_set_mode, long, mode)
WRAP_WATCOM_FUNC1(long, gmouse_set_cursor, long, picNum)
WRAP_WATCOM_FUNC1(long, gsound_background_volume_get_set, long, setVolume)
WRAP_WATCOM_FUNC1(void, gsound_play_sfx_file, const char*, name) // Plays SFX sound with given name
WRAP_WATCOM_FUNC1(Window*, GNW_find, long, winRef)
WRAP_WATCOM_FUNC2(long, interpret, Program*, program, long, arg2)
// Finds procedure ID for given script program pointer and procedure name
WRAP_WATCOM_FUNC2(long, interpretFindProcedure, Program*, scriptPtr, const char*, procName)
WRAP_WATCOM_FUNC1(long, interpretPopShort, Program*, scriptPtr) // Pops value type from Data stack (must be followed by InterpretPopLong)
WRAP_WATCOM_FUNC1(long, interpretPopLong, Program*, scriptPtr)  // Pops value from Data stack (must be preceded by InterpretPopShort)
WRAP_WATCOM_FUNC2(long, intface_get_attack, DWORD*, hitMode, DWORD*, isSecondary)
WRAP_WATCOM_FUNC0(long, intface_is_item_right_hand)
WRAP_WATCOM_FUNC0(long, intface_is_hidden)
WRAP_WATCOM_FUNC0(void, intface_redraw) // Redraws the main game interface windows (useful after changing some data like active hand, etc.)
WRAP_WATCOM_FUNC0(void, intface_toggle_item_state)
WRAP_WATCOM_FUNC1(void, intface_update_ac, long, animate)
WRAP_WATCOM_FUNC2(void, intface_update_move_points, long, ap, long, freeAP)
WRAP_WATCOM_FUNC0(void, intface_use_item)
WRAP_WATCOM_FUNC1(GameObject*, inven_left_hand, GameObject*, critter) // Item in critter's left hand slot
WRAP_WATCOM_FUNC1(GameObject*, inven_right_hand, GameObject*, critter) // Item in critter's right hand slot
WRAP_WATCOM_FUNC2(GameObject*, inven_pid_is_carried_ptr, GameObject*, invenObj, long, pid)
WRAP_WATCOM_FUNC2(long, inven_unwield, GameObject*, critter, long, slot)
WRAP_WATCOM_FUNC1(GameObject*, inven_worn, GameObject*, critter) // Critter worn item (armor)
WRAP_WATCOM_FUNC1(long, is_pc_flag, long, flag)
WRAP_WATCOM_FUNC0(long, is_pc_sneak_working)
WRAP_WATCOM_FUNC2(long, is_within_perception, GameObject*, source, GameObject*, target)
WRAP_WATCOM_FUNC1(long, isPartyMember, GameObject*, obj)
WRAP_WATCOM_FUNC1(long, item_c_curr_size, GameObject*, critter)
WRAP_WATCOM_FUNC1(long, item_caps_total, GameObject*, object)
WRAP_WATCOM_FUNC1(long, item_get_type, GameObject*, item)
WRAP_WATCOM_FUNC2(GameObject*, item_hit_with, GameObject*, critter, long, hitMode)
WRAP_WATCOM_FUNC1(long, item_m_dec_charges, GameObject*, item) // Returns 0 on success, -1 if the item has no charges
WRAP_WATCOM_FUNC1(long, item_size, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_total_cost, GameObject*, object)
WRAP_WATCOM_FUNC1(long, item_total_weight, GameObject*, object)
WRAP_WATCOM_FUNC1(long, item_w_anim_code, GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_anim_weap, GameObject*, item, DWORD, hitMode)
WRAP_WATCOM_FUNC2(long, item_w_compute_ammo_cost, GameObject*, item, DWORD*, rounds)
WRAP_WATCOM_FUNC1(long, item_w_curr_ammo, GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_damage, GameObject*, critter, long, hitMode)
WRAP_WATCOM_FUNC1(long, item_w_dam_div, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_dam_mult, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_dr_adjust, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_max_ammo, GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_range, GameObject*, critter, long, hitMode)
WRAP_WATCOM_FUNC2(long, item_w_reload, GameObject*, weapon, GameObject*, ammo)
WRAP_WATCOM_FUNC1(long, item_w_rounds, GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_subtype, GameObject*, item, long, hitMode)
WRAP_WATCOM_FUNC1(long, item_weight, GameObject*, item)
WRAP_WATCOM_FUNC2(long, light_get_tile, long, elevation, long, tileNum) // Returns light level at given tile
WRAP_WATCOM_FUNC2(long, load_frame, const char*, filename, FrmFile**, frmPtr)
WRAP_WATCOM_FUNC1(Program*, loadProgram, const char*, fileName)
WRAP_WATCOM_FUNC1(const char*, map_get_short_name, long, mapID)
WRAP_WATCOM_FUNC2(void, MapDirErase, const char*, folder, const char*, ext)
WRAP_WATCOM_FUNC1(void, mem_free, void*, mem)
WRAP_WATCOM_FUNC2(void*, mem_realloc, void*, lpmem, DWORD, msize)
WRAP_WATCOM_FUNC2(long, message_add, MessageList*, file, MessageNode*, msg)
WRAP_WATCOM_FUNC1(long, message_exit, MessageList*, msgList) // Destroys message list
WRAP_WATCOM_FUNC1(long, message_filter, MessageList*, file)
WRAP_WATCOM_FUNC2(long, message_load, MessageList*, msgList, const char*, msgFilePath) // Loads MSG file into given MessageList
WRAP_WATCOM_FUNC2(long, message_make_path, char*, outpath, char*, path)
WRAP_WATCOM_FUNC2(long, message_search, const MessageList*, file, MessageNode*, msg)
WRAP_WATCOM_FUNC2(void, mouse_get_position, long*, outX, long*, outY)
WRAP_WATCOM_FUNC0(void, mouse_show)
WRAP_WATCOM_FUNC0(void, mouse_hide)
// Calculates path and returns it's length
WRAP_WATCOM_FUNC6(long, make_path_func, GameObject*, objectFrom, long, tileFrom, long, tileTo, char*, pathDataBuffer, long, arg5, void*, blockingFunc)
WRAP_WATCOM_FUNC0(long, new_obj_id)
WRAP_WATCOM_FUNC2(void, obj_bound, GameObject*, object, BoundRect*, boundRect) // Calculates bounding box (rectangle) for a given object
WRAP_WATCOM_FUNC1(long, obj_destroy, GameObject*, object)
WRAP_WATCOM_FUNC2(long, obj_dist, GameObject*, obj_src, GameObject*, obj_trg)
WRAP_WATCOM_FUNC2(long, obj_erase_object, GameObject*, object, BoundRect*, boundRect)
WRAP_WATCOM_FUNC0(GameObject*, obj_find_first)
WRAP_WATCOM_FUNC0(GameObject*, obj_find_next)
WRAP_WATCOM_FUNC2(GameObject*, obj_find_first_at_tile, long, elevation, long, tileNum)
WRAP_WATCOM_FUNC0(GameObject*, obj_find_next_at_tile)
WRAP_WATCOM_FUNC2(long, obj_pid_new, GameObject*, object, long, pid)
WRAP_WATCOM_FUNC1(long, obj_lock_is_jammed, GameObject*, object) // Checks/unjams jammed locks
WRAP_WATCOM_FUNC1(void, obj_unjam_lock, GameObject*, object)
WRAP_WATCOM_FUNC1(long, partyMemberGetCurLevel, GameObject*, obj)
WRAP_WATCOM_FUNC1(void, pc_flag_on, long, flag)
WRAP_WATCOM_FUNC2(void, perk_add_effect,  GameObject*, critter, long, perkId)
WRAP_WATCOM_FUNC2(long, perk_can_add,  GameObject*, critter, long, perkId)
WRAP_WATCOM_FUNC2(long, perk_level, GameObject*, critter, long, perkId)
//WRAP_WATCOM_FUNC2(void, perk_remove_effect, GameObject*, critter, long, perkId)
WRAP_WATCOM_FUNC6(long, pick_death, GameObject*, attacker, GameObject*, target, GameObject*, weapon, long, amount, long, anim, long, hitFromBack)
WRAP_WATCOM_FUNC0(void, process_bk)
WRAP_WATCOM_FUNC0(void, proto_dude_update_gender)
// Places pointer to a prototype structure into ptrPtr and returns 0 on success or -1 on failure
WRAP_WATCOM_FUNC2(long, proto_ptr, long, pid, Proto**, ptrPtr)
WRAP_WATCOM_FUNC2(void*, queue_find_first, GameObject*, object, long, qType)
WRAP_WATCOM_FUNC2(void*, queue_find_next, GameObject*, object, long, qType)
WRAP_WATCOM_FUNC2(void, queue_remove_this, GameObject*, object, long, qType)
WRAP_WATCOM_FUNC1(long, register_begin, long, regType)
WRAP_WATCOM_FUNC0(long, register_end)
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
// WRAP_WATCOM_FUNC3(long, register_object_move_to_tile_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_must_call_, GameObject*, object;
WRAP_WATCOM_FUNC1(long, register_object_must_erase, GameObject*, object)
// WRAP_WATCOM_FUNC3(long, register_object_outline_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_play_sfx_, GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_run_to_tile_, GameObject*, object;
WRAP_WATCOM_FUNC3(long, register_object_take_out, GameObject*, object, long, holdFrameId, long, nothing)
WRAP_WATCOM_FUNC3(long, register_object_turn_towards, GameObject*, object, long, tileNum, long, nothing)
WRAP_WATCOM_FUNC2(long, roll_random, long, minValue, long, maxValue)
WRAP_WATCOM_FUNC1(long*, runProgram, Program*, progPtr)
WRAP_WATCOM_FUNC1(ScriptInstance*, scr_find_first_at, long, elevation)
WRAP_WATCOM_FUNC0(ScriptInstance*, scr_find_next_at)
WRAP_WATCOM_FUNC1(GameObject*, scr_find_obj_from_program, Program*, program)
WRAP_WATCOM_FUNC2(long, scr_new, long*, scriptID, long, sType)
// Saves pointer to script object into scriptPtr using scriptID
WRAP_WATCOM_FUNC2(long, scr_ptr, long, scriptId, ScriptInstance**, scriptPtr) // Returns 0 on success, -1 on failure
WRAP_WATCOM_FUNC1(long, scr_remove, long, scriptID)
WRAP_WATCOM_FUNC1(void, set_focus_func, void*, func)
WRAP_WATCOM_FUNC2(long, skill_dec_point_force, GameObject*, critter, long, skill)
WRAP_WATCOM_FUNC2(long, skill_inc_point_force, GameObject*, critter, long, skill)
WRAP_WATCOM_FUNC1(long, skill_is_tagged, long, skill)
WRAP_WATCOM_FUNC2(long, skill_level, GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC2(long, stat_get_base_direct, GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC2(long, stat_get_bonus, GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC3(long, stat_set_bonus, GameObject*, critter, long, statID, long, amount)
WRAP_WATCOM_FUNC2(void, skill_get_tags, long*, tags, long, num)
WRAP_WATCOM_FUNC2(void, skill_set_tags, long*, tags, long, num)
WRAP_WATCOM_FUNC2(long, stat_level, GameObject*, critter, long, statId)
WRAP_WATCOM_FUNC1(void, stat_pc_add_experience, long, amount) // Adds experience points to PC
WRAP_WATCOM_FUNC1(long, text_font, long, fontNum)
WRAP_WATCOM_FUNC2(long, tile_dist, long, scrTile, long, dstTile)
WRAP_WATCOM_FUNC2(long, tile_dir, long, scrTile, long, dstTile)
WRAP_WATCOM_FUNC0(void, tile_refresh_display) // Redraws the whole screen
WRAP_WATCOM_FUNC2(void, tile_refresh_rect, BoundRect*, boundRect, long, elevation) // Redraws the given rectangle on screen
WRAP_WATCOM_FUNC1(long, trait_level, long, traitID)
WRAP_WATCOM_FUNC6(long, win_add, long, x, long, y, long, width, long, height, long, bgColorIndex, long, flags)
WRAP_WATCOM_FUNC1(void, win_show, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_hide, DWORD, winRef)
WRAP_WATCOM_FUNC1(BYTE*, win_get_buf, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_draw, DWORD, winRef)
WRAP_WATCOM_FUNC2(void, win_draw_rect, DWORD, winRef, RECT*, rect)
WRAP_WATCOM_FUNC1(void, win_delete, DWORD, winRef)
WRAP_WATCOM_FUNC0(long, windowWidth)
WRAP_WATCOM_FUNC1(void, wmCarUseGas, long, gasAmount)
WRAP_WATCOM_FUNC0(void, wmPartyWalkingStep)
WRAP_WATCOM_FUNC1(void, wmRefreshInterfaceOverlay, long, isRedraw)
WRAP_WATCOM_FUNC2(DbFile*, xfopen, const char*, fileName, const char*, flags)
WRAP_WATCOM_FUNC3(long, xfseek, DbFile*, file, long, fOffset, long, origin)
