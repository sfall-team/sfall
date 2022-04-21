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

/*
	For functions that have 3 or more arguments, it is preferable to use the fastcall calling convention
	because the compiler can build a better/optimized code when calling the engine functions
*/
WRAP_WATCOM_FFUNC4(long, _word_wrap, const char*, text, int, maxWidth, DWORD*, buf, BYTE*, count)
WRAP_WATCOM_FFUNC4(fo::GameObject*, ai_best_weapon, fo::GameObject*, source, fo::GameObject*, prevItem, fo::GameObject*, checkItem, fo::GameObject*, target)
WRAP_WATCOM_FFUNC3(bool, ai_can_use_weapon, fo::GameObject*, critter, fo::GameObject*, item, DWORD, hitMode)
WRAP_WATCOM_FFUNC3(long, ai_have_ammo, fo::GameObject*, critter, fo::GameObject*, item, fo::GameObject**, outAmmo)
WRAP_WATCOM_FFUNC3(long, ai_pick_hit_mode, fo::GameObject*, source, fo::GameObject*, item, fo::GameObject*, target)
WRAP_WATCOM_FFUNC3(long, ai_magic_hands, fo::GameObject*, source, fo::GameObject*, object, long, msgNumber)
WRAP_WATCOM_FFUNC3(fo::GameObject*, ai_search_inven_weap, fo::GameObject*, source, long, apCheck, fo::GameObject*, target)
WRAP_WATCOM_FFUNC3(void, check_for_death, fo::GameObject*, critter, long, amountDamage, long*, flags)
WRAP_WATCOM_FFUNC6(long, combat_safety_invalidate_weapon_func, fo::GameObject*, source, fo::GameObject*, weapon, long, hitMode, fo::GameObject*, targetA, DWORD*, outSafeRange, fo::GameObject*, targetB)
WRAP_WATCOM_FFUNC3(void, correctFidForRemovedItem, fo::GameObject*, critter, fo::GameObject*, item, long, slotFlag)
WRAP_WATCOM_FFUNC7(long, createWindow, const char*, winName, DWORD, x, DWORD, y, DWORD, width, DWORD, height, long, color, long, flags)
WRAP_WATCOM_FFUNC8(void, cscale, BYTE*, src, long, sWidth, long, sHeight, long, sStride, BYTE*, dst, long, width, long, height, long, stride)
WRAP_WATCOM_FFUNC4(long, determine_to_hit, fo::GameObject*, source, fo::GameObject*, target, long, bodyPart, long, hitMode)
WRAP_WATCOM_FFUNC3(void, display_inventory, long, inventoryOffset, long, visibleOffset, long, mode)
WRAP_WATCOM_FFUNC4(void, display_target_inventory, long, inventoryOffset, long, visibleOffset, DWORD*, targetInventory, long, mode)
WRAP_WATCOM_FFUNC3(fo::FrmFrameData*, frame_ptr, fo::FrmHeaderData*, frm, long, frame, long, direction)
WRAP_WATCOM_FFUNC3(void, GNW_win_refresh, fo::Window*, win, fo::BoundRect*, rect, long*, buffer)
WRAP_WATCOM_FFUNC3(void, intface_update_items, long, animate, long, modeLeft, long, modeRight)
WRAP_WATCOM_FFUNC3(fo::GameObject*, inven_find_type, fo::GameObject*, critter, long, itemType, DWORD*, slot)
WRAP_WATCOM_FFUNC3(long, inven_wield, fo::GameObject*, critter, fo::GameObject*, item, long, slot)
WRAP_WATCOM_FFUNC3(long, item_add_force, fo::GameObject*, critter, fo::GameObject*, item, long, count)
WRAP_WATCOM_FFUNC3(long, item_remove_mult, fo::GameObject*, critter, fo::GameObject*, item, long, count) // WARNING: HOOK_REMOVEINVENOBJ uses the return address in this function
WRAP_WATCOM_FFUNC3(long, item_w_mp_cost, fo::GameObject*, source, long, hitMode, long, isCalled)
// Calculates path and returns it's length
WRAP_WATCOM_FFUNC6(long, make_path_func, fo::GameObject*, objectFrom, long, tileFrom, long, tileTo, char*, pathDataBuffer, long, checkTileTo, void*, blockingFunc)
WRAP_WATCOM_FFUNC7(long, make_straight_path_func, fo::GameObject*, objFrom, DWORD, tileFrom, DWORD, tileTo, void*, arrayPtr, DWORD*, outObject, long, flags, void*, blockingFunc)
WRAP_WATCOM_FFUNC3(long, message_find, DWORD*, msgFile, long, msgNumber, DWORD*, outBuf)
WRAP_WATCOM_FFUNC4(long, mouse_click_in, long, x, long, y, long, x_offs, long, y_offs)
WRAP_WATCOM_FFUNC4(long, mouse_in, long, x, long, y, long, x_offs, long, y_offs)
WRAP_WATCOM_FFUNC3(fo::GameObject*, obj_blocking_at, fo::GameObject*, object, long, tile, long, elevation)
WRAP_WATCOM_FFUNC4(long, obj_connect, fo::GameObject*, object, long, tile, long, elevation, RECT*, rect)
WRAP_WATCOM_FFUNC4(long, obj_dist_with_tile, fo::GameObject*, source, long, sourceTile, fo::GameObject*, target, long, targetTile)
WRAP_WATCOM_FFUNC4(long, obj_move_to_tile, fo::GameObject*, object, long, tile, long, elevation, RECT*, rect)
WRAP_WATCOM_FFUNC3(long, obj_new_sid_inst, fo::GameObject*, object, long, sType, long, scriptIndex)
WRAP_WATCOM_FFUNC3(fo::GameObject*, object_under_mouse, long, crSwitch, long, inclDude, long, elevation)
WRAP_WATCOM_FFUNC4(void, qsort, void*, base, long, number, long, elSize, DWORD, comp)
WRAP_WATCOM_FFUNC4(long, queue_add, long, time, fo::GameObject*, object, void*, data, long, qType)
WRAP_WATCOM_FFUNC4(void, register_object_call, long*, target, long*, source, void*, func, long, delay)
WRAP_WATCOM_FFUNC4(long, register_object_move_to_object, fo::GameObject*, source, fo::GameObject*, target, long, distance, long, delay)
WRAP_WATCOM_FFUNC4(long, register_object_run_to_object, fo::GameObject*, source, fo::GameObject*, target, long, distance, long, delay)
WRAP_WATCOM_FFUNC3(long, register_object_play_sfx, fo::GameObject*, object, const char*, sfxName, long, delay)
WRAP_WATCOM_FFUNC3(long, scr_get_local_var, long, sid, long, varId, long*, value)
WRAP_WATCOM_FFUNC3(long, scr_set_local_var, long, sid, long, varId, long, value)
WRAP_WATCOM_FFUNC3(long, square_coord, long, square, long*, outX, long*, outY)
WRAP_WATCOM_FFUNC6(long, text_object_create, fo::GameObject*, object, const char*, text, long, font, long, colorText, long, colorOutline, fo::BoundRect*, rect)
WRAP_WATCOM_FFUNC3(long, tile_coord, long, tile, long*, outX, long*, outY) // the fourth argument of the function is not used
WRAP_WATCOM_FFUNC3(long, tile_num_in_direction, long, tile, long, rotation, long, distance)
WRAP_WATCOM_FFUNC8(void, trans_cscale, void*, fromBuff, long, width, long, height, long, fromPitch, void*, toBuff, long, toWidth, long, toHeight, long, toPitch)
WRAP_WATCOM_FFUNC3(void, win_clip, fo::Window*, window, fo::RectList**, rects, void*, buffer)
WRAP_WATCOM_FFUNC6(void, win_print, long, winID, const char*, text, long, textWidth, long, xPos, long, yPos, long, colorFlags)
WRAP_WATCOM_FFUNC9(long, windowWrapLineWithSpacing, long, winID, const char*, text, long, width, long, height, long, x, long, y, long, color, long, alignment, long, lineSpacing)
WRAP_WATCOM_FFUNC4(void, wmInterfaceDrawSubTileRectFogged, BYTE*, surface, long, width, long, height, long, surfaceWidth)

WRAP_WATCOM_FFUNC3(const char*, interpretGetString, fo::Program*, scriptPtr, DWORD, dataType, DWORD, strId)

//WRAP_WATCOM_FFUNC6(void, drawScaledBuf, BYTE*, dst, long, dstW, long, dstH, const BYTE*, src, long, w, long, h)

/* stdcall */
WRAP_WATCOM_FUNC1(fo::AIcap*, ai_cap, fo::GameObject*, critter)
WRAP_WATCOM_FUNC2(void, ai_print_msg, fo::GameObject*, object, long, mode)
WRAP_WATCOM_FUNC2(fo::GameObject*, ai_retrieve_object, fo::GameObject*, critter, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(fo::GameObject*, ai_search_environ, fo::GameObject*, critter, long, itemType)
WRAP_WATCOM_FUNC1(fo::Program*, allocateProgram, const char*, filePath)
WRAP_WATCOM_FUNC2(long, anim_can_use_door, fo::GameObject*, source, fo::GameObject*, object)
WRAP_WATCOM_FUNC1(bool, art_exists, long, artFid)
WRAP_WATCOM_FUNC0(void, art_flush)
WRAP_WATCOM_FUNC1(const char*, art_get_name, long, artFID)
WRAP_WATCOM_FUNC5(long, art_id, long, artType, long, lstIndex, long, animCode, long, weaponCode, long, directionCode)
WRAP_WATCOM_FUNC3(BYTE*, art_frame_data, fo::FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, art_frame_width, fo::FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, art_frame_length, fo::FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC2(fo::FrmHeaderData*, art_ptr_lock, long, frmId, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, art_ptr_lock_data, long, frmId, long, frameNum, long, rotation, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, art_lock, long, frmId, DWORD*, lockPtr, long*, widthOut, long*, heightOut)
WRAP_WATCOM_FUNC1(long, art_ptr_unlock, DWORD, lockId)
WRAP_WATCOM_FUNC2(long, barter_compute_value, fo::GameObject*, source, fo::GameObject*, target)
WRAP_WATCOM_FUNC1(long, block_for_tocks, long, ticks)
WRAP_WATCOM_FUNC2(long, combat_turn, fo::GameObject*, critter, long, isDudeTurn) // Perform combat turn for a given critter
WRAP_WATCOM_FUNC1(long, critter_body_type, fo::GameObject*, critter)
WRAP_WATCOM_FUNC1(long, critter_is_dead, fo::GameObject*, critter)
WRAP_WATCOM_FUNC1(const char*, critter_name, fo::GameObject*, critter) // Returns the name of the critter
WRAP_WATCOM_FUNC1(void, critter_pc_set_name, const char*, newName) // Change the name of playable character
WRAP_WATCOM_FUNC1(long, critterIsOverloaded, fo::GameObject*, critter)
WRAP_WATCOM_FUNC1(void, display_print, const char*, msg) // Displays message in main UI console window
WRAP_WATCOM_FUNC0(void, display_stats)
WRAP_WATCOM_FUNC1(long, critter_kill_count_type, fo::GameObject*, critter)
WRAP_WATCOM_FUNC2(void, endgame_load_palette, long, artType, long, fid)
WRAP_WATCOM_FUNC1(void, EndLoad, fo::DbFile*, file)
// Execute script proc by internal proc number (from script's proc table, basically a sequential number of a procedure as defined in code, starting from 1)
WRAP_WATCOM_FUNC2(void, executeProcedure, fo::Program*, sptr, long, procNum)
WRAP_WATCOM_FUNC1(long, folder_print_line, const char*, text)
WRAP_WATCOM_FUNC1(long, folder_print_seperator, const char*, text)
WRAP_WATCOM_FUNC1(const char*, findCurrentProc, fo::Program*, program) // Returns the name of current procedure by program pointer
WRAP_WATCOM_FUNC1(long, FMtext_width, const char*, text)
WRAP_WATCOM_FUNC1(void, freeColorBlendTable, long, color)
WRAP_WATCOM_FUNC1(long, game_get_global_var, long, globalVar)
WRAP_WATCOM_FUNC0(long, get_input)
WRAP_WATCOM_FUNC1(fo::BlendColorTableData*, getColorBlendTable, long, color)
// Searches for message ID in given message file and places result in result argument
WRAP_WATCOM_FUNC3(const char*, getmsg, const fo::MessageList*, fileAddr, fo::MessageNode*, result, long, messageId)
WRAP_WATCOM_FUNC1(void, gdialogDisplayMsg, const char*, message)
WRAP_WATCOM_FUNC1(void, gmouse_3d_set_mode, long, mode)
WRAP_WATCOM_FUNC1(long, gmouse_set_cursor, long, picNum)
WRAP_WATCOM_FUNC1(long, gsound_background_volume_get_set, long, setVolume)
WRAP_WATCOM_FUNC1(void, gsound_play_sfx_file, const char*, name) // Plays SFX sound with given name
WRAP_WATCOM_FUNC1(fo::Window*, GNW_find, long, winRef)
WRAP_WATCOM_FUNC2(long, interpret, fo::Program*, program, long, arg2)
// Finds procedure ID for given script program pointer and procedure name
WRAP_WATCOM_FUNC2(long, interpretFindProcedure, fo::Program*, scriptPtr, const char*, procName)
WRAP_WATCOM_FUNC1(long, interpretPopShort, fo::Program*, scriptPtr) // Pops value type from Data stack (must be followed by InterpretPopLong)
WRAP_WATCOM_FUNC1(long, interpretPopLong, fo::Program*, scriptPtr)  // Pops value from Data stack (must be preceded by InterpretPopShort)
WRAP_WATCOM_FUNC2(long, intface_get_attack, DWORD*, hitMode, DWORD*, isSecondary)
WRAP_WATCOM_FUNC0(long, intface_is_item_right_hand)
WRAP_WATCOM_FUNC0(long, intface_is_hidden)
WRAP_WATCOM_FUNC0(void, intface_redraw) // Redraws the main game interface windows (useful after changing some data like active hand, etc.)
WRAP_WATCOM_FUNC0(void, intface_toggle_item_state)
WRAP_WATCOM_FUNC1(void, intface_update_ac, long, animate)
WRAP_WATCOM_FUNC2(void, intface_update_move_points, long, ap, long, freeAP)
WRAP_WATCOM_FUNC0(void, intface_use_item)
WRAP_WATCOM_FUNC1(fo::GameObject*, inven_left_hand, fo::GameObject*, critter) // Item in critter's left hand slot
WRAP_WATCOM_FUNC1(fo::GameObject*, inven_right_hand, fo::GameObject*, critter) // Item in critter's right hand slot
WRAP_WATCOM_FUNC2(fo::GameObject*, inven_pid_is_carried_ptr, fo::GameObject*, invenObj, long, pid)
WRAP_WATCOM_FUNC2(long, inven_unwield, fo::GameObject*, critter, long, slot)
WRAP_WATCOM_FUNC1(fo::GameObject*, inven_worn, fo::GameObject*, critter) // Critter worn item (armor)
WRAP_WATCOM_FUNC1(long, is_pc_flag, long, flag)
WRAP_WATCOM_FUNC0(long, is_pc_sneak_working)
WRAP_WATCOM_FUNC2(long, is_within_perception, fo::GameObject*, source, fo::GameObject*, target)
WRAP_WATCOM_FUNC1(long, isPartyMember, fo::GameObject*, obj)
WRAP_WATCOM_FUNC1(long, item_c_curr_size, fo::GameObject*, critter)
WRAP_WATCOM_FUNC1(long, item_caps_total, fo::GameObject*, object)
WRAP_WATCOM_FUNC2(long, item_d_take_drug, fo::GameObject*, source, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_get_type, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(fo::GameObject*, item_hit_with, fo::GameObject*, critter, long, hitMode)
WRAP_WATCOM_FUNC1(long, item_m_dec_charges, fo::GameObject*, item) // Returns 0 on success, -1 if the item has no charges
WRAP_WATCOM_FUNC1(long, item_size, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_total_cost, fo::GameObject*, object)
WRAP_WATCOM_FUNC1(long, item_total_weight, fo::GameObject*, object)
WRAP_WATCOM_FUNC1(long, item_w_anim_code, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_anim_weap, fo::GameObject*, item, fo::AttackType, hitMode)
WRAP_WATCOM_FUNC2(long, item_w_compute_ammo_cost, fo::GameObject*, item, DWORD*, rounds)
WRAP_WATCOM_FUNC1(long, item_w_curr_ammo, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_damage, fo::GameObject*, critter, long, hitMode)
WRAP_WATCOM_FUNC1(long, item_w_dam_div, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_dam_mult, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_dr_adjust, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_max_ammo, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_primary_mp_cost, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_range, fo::GameObject*, critter, long, hitMode)
WRAP_WATCOM_FUNC2(long, item_w_reload, fo::GameObject*, weapon, fo::GameObject*, ammo)
WRAP_WATCOM_FUNC1(long, item_w_rounds, fo::GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_secondary_mp_cost, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(long, item_w_subtype, fo::GameObject*, item, long, hitMode)
WRAP_WATCOM_FUNC1(long, item_weight, fo::GameObject*, item)
WRAP_WATCOM_FUNC2(long, light_get_tile, long, elevation, long, tileNum) // Returns light level at given tile
WRAP_WATCOM_FUNC2(long, load_frame, const char*, filename, fo::FrmFile**, frmPtr)
WRAP_WATCOM_FUNC1(fo::Program*, loadProgram, const char*, fileName)
WRAP_WATCOM_FUNC1(const char*, map_get_short_name, long, mapID)
WRAP_WATCOM_FUNC2(void, MapDirErase, const char*, folder, const char*, ext)
WRAP_WATCOM_FUNC1(void, mem_free, void*, mem)
WRAP_WATCOM_FUNC2(void*, mem_realloc, void*, lpmem, DWORD, msize)
WRAP_WATCOM_FUNC2(long, message_add, fo::MessageList*, file, fo::MessageNode*, msg)
WRAP_WATCOM_FUNC1(long, message_exit, fo::MessageList*, msgList) // Destroys message list
WRAP_WATCOM_FUNC1(long, message_filter, fo::MessageList*, file)
WRAP_WATCOM_FUNC2(long, message_load, fo::MessageList*, msgList, const char*, msgFilePath) // Loads MSG file into given MessageList
WRAP_WATCOM_FUNC2(long, message_make_path, char*, outpath, char*, path)
WRAP_WATCOM_FUNC2(long, message_search, const fo::MessageList*, file, fo::MessageNode*, msg)
WRAP_WATCOM_FUNC2(void, mouse_get_position, long*, outX, long*, outY)
WRAP_WATCOM_FUNC0(void, mouse_show)
WRAP_WATCOM_FUNC0(void, mouse_hide)
WRAP_WATCOM_FUNC0(long, new_obj_id)
WRAP_WATCOM_FUNC2(void, obj_bound, fo::GameObject*, object, fo::BoundRect*, boundRect) // Calculates bounding box (rectangle) for a given object
WRAP_WATCOM_FUNC1(long, obj_destroy, fo::GameObject*, object)
WRAP_WATCOM_FUNC2(long, obj_dist, fo::GameObject*, obj_src, fo::GameObject*, obj_trg)
WRAP_WATCOM_FUNC2(void, obj_drop, fo::GameObject*, source, fo::GameObject*, objectToDrop)
WRAP_WATCOM_FUNC2(long, obj_erase_object, fo::GameObject*, object, fo::BoundRect*, boundRect)
WRAP_WATCOM_FUNC0(fo::GameObject*, obj_find_first)
WRAP_WATCOM_FUNC0(fo::GameObject*, obj_find_next)
WRAP_WATCOM_FUNC2(fo::GameObject*, obj_find_first_at_tile, long, elevation, long, tileNum)
WRAP_WATCOM_FUNC0(fo::GameObject*, obj_find_next_at_tile)
WRAP_WATCOM_FUNC1(bool, obj_is_openable, fo::GameObject*, object)
WRAP_WATCOM_FUNC2(long, obj_pid_new, fo::GameObject*, object, long, pid)
WRAP_WATCOM_FUNC1(long, obj_lock_is_jammed, fo::GameObject*, object) // Checks/unjams jammed locks
WRAP_WATCOM_FUNC1(void, obj_unjam_lock, fo::GameObject*, object)
WRAP_WATCOM_FUNC1(long, partyMemberGetCurLevel, fo::GameObject*, obj)
WRAP_WATCOM_FUNC1(void, pc_flag_on, long, flag)
WRAP_WATCOM_FUNC2(void, perk_add_effect,  fo::GameObject*, critter, long, perkId)
WRAP_WATCOM_FUNC2(long, perk_can_add, fo::GameObject*, critter, long, perkId)
WRAP_WATCOM_FUNC2(long, perk_level, fo::GameObject*, critter, long, perkId)
//WRAP_WATCOM_FUNC2(void, perk_remove_effect, fo::GameObject*, critter, long, perkId)
WRAP_WATCOM_FUNC6(long, pick_death, fo::GameObject*, attacker, fo::GameObject*, target, fo::GameObject*, weapon, long, amount, long, anim, long, hitFromBack)
WRAP_WATCOM_FUNC0(void, process_bk)
WRAP_WATCOM_FUNC2(const char*, proto_get_msg_info, long, pid, long, msgType) // msgType: 0 - name, 1 - desc
WRAP_WATCOM_FUNC0(void, proto_dude_update_gender)
WRAP_WATCOM_FUNC2(void, proto_make_path, char*, buffer, long, pid)
// Places pointer to a prototype structure into ptrPtr and returns 0 on success or -1 on failure
WRAP_WATCOM_FUNC2(long, proto_ptr, long, pid, fo::Proto**, ptrPtr)
WRAP_WATCOM_FUNC2(void, queue_clear_type, long, qType, void*, func) // removes all events of the given type and performs func before removal
WRAP_WATCOM_FUNC2(void*, queue_find_first, fo::GameObject*, object, long, qType)
WRAP_WATCOM_FUNC2(void*, queue_find_next, fo::GameObject*, object, long, qType)
WRAP_WATCOM_FUNC2(void, queue_remove_this, fo::GameObject*, object, long, qType)
WRAP_WATCOM_FUNC1(long, register_begin, long, regType)
WRAP_WATCOM_FUNC0(long, register_end)
WRAP_WATCOM_FUNC3(long, register_object_animate, fo::GameObject*, object, long, anim, long, delay)
WRAP_WATCOM_FUNC3(long, register_object_animate_and_hide, fo::GameObject*, object, long, anim, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_animate_and_move_straight_, fo::GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_animate_forever_, fo::GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_animate_reverse_, fo::GameObject*, object;
WRAP_WATCOM_FUNC3(long, register_object_change_fid, fo::GameObject*, object, long, artFid, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_check_falling_, fo::GameObject*, object;
WRAP_WATCOM_FUNC1(long, register_object_dec_rotation, fo::GameObject*, object)
WRAP_WATCOM_FUNC1(long, register_object_erase, fo::GameObject*, object)
WRAP_WATCOM_FUNC3(long, register_object_funset, fo::GameObject*, object, long, action, long, delay)
WRAP_WATCOM_FUNC1(long, register_object_inc_rotation, fo::GameObject*, object)
WRAP_WATCOM_FUNC3(long, register_object_light, fo::GameObject*, object, long, lightRadius, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_move_on_stairs_, fo::GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_straight_to_tile_, fo::GameObject*, object;
// WRAP_WATCOM_FUNC3(long, register_object_must_call_, fo::GameObject*, object;
WRAP_WATCOM_FUNC1(long, register_object_must_erase, fo::GameObject*, object)
// WRAP_WATCOM_FUNC3(long, register_object_outline_, fo::GameObject*, object;
WRAP_WATCOM_FUNC3(long, register_object_take_out, fo::GameObject*, object, long, holdFrameId, long, nothing)
WRAP_WATCOM_FUNC3(long, register_object_turn_towards, fo::GameObject*, object, long, tileNum, long, nothing)
WRAP_WATCOM_FUNC2(long, roll_random, long, minValue, long, maxValue)
WRAP_WATCOM_FUNC1(long*, runProgram, fo::Program*, progPtr)
WRAP_WATCOM_FUNC1(long, selectWindowID, long, sWinID)
WRAP_WATCOM_FUNC1(void, scr_build_lookup_table, fo::ScriptInstance*, script)
WRAP_WATCOM_FUNC1(fo::ScriptInstance*, scr_find_first_at, long, elevation)
WRAP_WATCOM_FUNC0(fo::ScriptInstance*, scr_find_next_at)
WRAP_WATCOM_FUNC1(fo::GameObject*, scr_find_obj_from_program, fo::Program*, program)
WRAP_WATCOM_FUNC1(long, scr_find_sid_from_program, fo::Program*, program)
WRAP_WATCOM_FUNC2(long, scr_new, long*, scriptID, long, sType)
// Saves pointer to script object into scriptPtr using scriptID
WRAP_WATCOM_FUNC2(long, scr_ptr, long, scriptId, fo::ScriptInstance**, scriptPtr) // Returns 0 on success, -1 on failure
WRAP_WATCOM_FUNC1(long, scr_remove, long, scriptID)
WRAP_WATCOM_FUNC1(void, set_focus_func, void*, func)
WRAP_WATCOM_FUNC2(long, skill_dec_point_force, fo::GameObject*, critter, long, skill)
WRAP_WATCOM_FUNC2(long, skill_inc_point_force, fo::GameObject*, critter, long, skill)
WRAP_WATCOM_FUNC1(long, skill_is_tagged, long, skill)
WRAP_WATCOM_FUNC2(long, skill_level, fo::GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC2(long, stat_get_base, fo::GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC2(long, stat_get_base_direct, fo::GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC2(long, stat_get_bonus, fo::GameObject*, critter, long, statID)
WRAP_WATCOM_FUNC3(long, stat_set_bonus, fo::GameObject*, critter, long, statID, long, amount)
WRAP_WATCOM_FUNC2(void, skill_get_tags, long*, tags, long, num)
WRAP_WATCOM_FUNC2(void, skill_set_tags, long*, tags, long, num)
WRAP_WATCOM_FUNC2(long, stat_level, fo::GameObject*, critter, long, statId)
WRAP_WATCOM_FUNC1(void, stat_pc_add_experience, long, amount) // Adds experience points to PC
WRAP_WATCOM_FUNC1(long, text_font, long, fontNum)
WRAP_WATCOM_FUNC2(long, tile_dist, long, scrTile, long, dstTile)
WRAP_WATCOM_FUNC2(long, tile_dir, long, scrTile, long, dstTile)
WRAP_WATCOM_FUNC2(long, tile_idistance, long, sourceTile, long, targetTile)
WRAP_WATCOM_FUNC1(long, tile_on_edge, long, tile)
WRAP_WATCOM_FUNC0(void, tile_refresh_display) // Redraws the whole screen
WRAP_WATCOM_FUNC2(void, tile_refresh_rect, fo::BoundRect*, boundRect, long, elevation) // Redraws the given rectangle on screen
WRAP_WATCOM_FUNC1(long, trait_level, long, traitID)
WRAP_WATCOM_FUNC6(long, win_add, long, x, long, y, long, width, long, height, long, bgColorIndex, long, flags)
WRAP_WATCOM_FUNC1(void, win_delete, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_draw, DWORD, winRef)
WRAP_WATCOM_FUNC2(void, win_draw_rect, DWORD, winRef, RECT*, rect)
WRAP_WATCOM_FUNC1(BYTE*, win_get_buf, DWORD, winRef)
WRAP_WATCOM_FUNC2(long, win_get_top_win, long, x, long, y)
WRAP_WATCOM_FUNC1(void, win_hide, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, win_show, DWORD, winRef)
WRAP_WATCOM_FUNC0(long, windowWidth)
WRAP_WATCOM_FUNC1(void, wmCarUseGas, long, gasAmount)
WRAP_WATCOM_FUNC1(long, wmEvalTileNumForPlacement, long, tile)
WRAP_WATCOM_FUNC0(void, wmPartyWalkingStep)
WRAP_WATCOM_FUNC1(void, wmRefreshInterfaceOverlay, long, isRedraw)

/* Database functions */
WRAP_WATCOM_FUNC1(bool, db_access, const char*, fileName) // Checks if given file exists in DB
WRAP_WATCOM_FUNC1(long, db_fclose, fo::DbFile*, file)
WRAP_WATCOM_FUNC2(fo::DbFile*, db_fopen, const char*, path, const char*, mode)
//WRAP_WATCOM_FUNC1(long, db_fgetc, fo::DbFile*, file)
WRAP_WATCOM_FUNC3(char*, db_fgets, char*, buf, long, max_count, fo::DbFile*, file)
WRAP_WATCOM_FFUNC4(long, db_fread, void*, buf, long, elsize, long, count, fo::DbFile*, file)
WRAP_WATCOM_FFUNC3(long, db_fseek, fo::DbFile*, file, long, pos, long, origin)
WRAP_WATCOM_FUNC2(void, db_free_file_list, char***, fileList, DWORD, arg2) // Destroys filelist array created by db_get_file_list
WRAP_WATCOM_FUNC2(long, db_freadByte, fo::DbFile*, file, BYTE*, _out)
WRAP_WATCOM_FUNC2(long, db_freadShort, fo::DbFile*, file, WORD*, _out)
WRAP_WATCOM_FUNC2(long, db_freadInt, fo::DbFile*, file, DWORD*, _out)
WRAP_WATCOM_FFUNC3(long, db_freadByteCount, fo::DbFile*, file, BYTE*, dest, long, count)
WRAP_WATCOM_FFUNC3(long, db_freadShortCount, fo::DbFile*, file, WORD*, dest, long, count)
WRAP_WATCOM_FFUNC3(long, db_freadIntCount, fo::DbFile*, file, DWORD*, dest, long, count)
//WRAP_WATCOM_FFUNC3(long, db_freadLongCount, fo::DbFile*, file, DWORD*, dest, long, count)
WRAP_WATCOM_FUNC2(long, db_fwriteByte, fo::DbFile*, file, long, value)
WRAP_WATCOM_FUNC2(long, db_fwriteInt, fo::DbFile*, file, long, value)
WRAP_WATCOM_FFUNC3(long, db_fwriteByteCount, fo::DbFile*, file, const BYTE*, cptr, long, count)
//WRAP_WATCOM_FFUNC3(long, db_fwriteLongCount, fo::DbFile*, file, DWORD*, dest, long, count)
WRAP_WATCOM_FUNC2(long, db_dir_entry, const char*, fileName, DWORD*, sizeOut) // Check fallout file and get file size (result 0 - file exists)
// Searches files in DB by given path/filename mask and stores result in fileList
// fileList is a pointer to a variable, that will be assigned with an address of an array of char* strings
WRAP_WATCOM_FUNC2(long, db_get_file_list, const char*, searchMask, char***, fileList) // Returns number of elements in *fileList
WRAP_WATCOM_FUNC1(void*, dbase_open, const char*, fileName)
WRAP_WATCOM_FUNC1(void, dbase_close, void*, dbPtr)

WRAP_WATCOM_FUNC2(fo::DbFile*, xfopen, const char*, fileName, const char*, flags)
WRAP_WATCOM_FFUNC3(long, xfseek, fo::DbFile*, file, long, fOffset, long, origin)
