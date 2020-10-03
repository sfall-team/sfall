/**
	X-Macro for defining engine function wrappers.

	Add one line for every function you want to call from C++ code.

	Format:
	WRAP_WATCOM_FUNCX(retType, name, funcoff, argType1, argName1, ...)
	- X - number of arguments
	- retType - function return type
	- name - function name (without trailing underscore)
	- funcoff - engine function offset
	- argType - type of argument
	- argName - name of argument (repeat for all of X arguments)

	NOTES: be careful not to use reserved words, including ASM instructions (push, pop, mov, div, etc.)
*/

/*
	For functions that have 3 or more arguments, it is preferable to use the fastcall calling convention
	because the compiler builds the better/optimized code when calling the engine functions
*/
WRAP_WATCOM_FFUNC4(long, WordWrap, _word_wrap_, const char*, text, int, maxWidth, DWORD*, buf, BYTE*, count)
WRAP_WATCOM_FFUNC3(void, CheckForDeath, check_for_death_, TGameObj*, critter, long, amountDamage, long*, flags)
WRAP_WATCOM_FFUNC3(void, CorrectFidForRemovedItem, correctFidForRemovedItem_, TGameObj*, critter, TGameObj*, item, long, slotFlag)
WRAP_WATCOM_FFUNC7(long, CreateWindowFunc, createWindow_, const char*, winName, DWORD, x, DWORD, y, DWORD, width, DWORD, height, long, color, long, flags)
WRAP_WATCOM_FFUNC4(long, DetermineToHit, determine_to_hit_, TGameObj*, source, TGameObj*, target, long, bodyPart, long, hitMode)
WRAP_WATCOM_FFUNC3(void, DisplayInventory, display_inventory_, long, inventoryOffset, long, visibleOffset, long, mode)
WRAP_WATCOM_FFUNC4(void, DisplayTargetInventory, display_target_inventory_, long, inventoryOffset, long, visibleOffset, DWORD*, targetInventory, long, mode)
WRAP_WATCOM_FFUNC3(FrmFrameData*, FramePtr, frame_ptr_, FrmHeaderData*, frm, long, frame, long, direction)
WRAP_WATCOM_FFUNC3(void, GNWWinRefresh, GNW_win_refresh_, WINinfo*, win, BoundRect*, rect, long*, buffer)
WRAP_WATCOM_FFUNC3(void, IntfaceUpdateItems, intface_update_items_, long, animate, long, modeLeft, long, modeRight)
WRAP_WATCOM_FFUNC3(TGameObj*, InvenFindType, inven_find_type_, TGameObj*, critter, long, itemType, DWORD*, buf)
WRAP_WATCOM_FFUNC3(long, ItemAddForce, item_add_force_, TGameObj*, critter, TGameObj*, item, long, count)
WRAP_WATCOM_FFUNC3(long, ItemWMpCost, item_w_mp_cost_, TGameObj*, source, long, hitMode, long, isCalled)
WRAP_WATCOM_FFUNC7(void, MakeStraightPathFunc, make_straight_path_func_, TGameObj*, objFrom, DWORD, tileFrom, DWORD, tileTo, void*, rotationPtr, DWORD*, result, long, flags, void*, func)
WRAP_WATCOM_FFUNC3(long, MessageFind, message_find_, DWORD*, msgFile, long, msgNumber, DWORD*, outBuf)
WRAP_WATCOM_FFUNC4(long, MouseClickIn, mouse_click_in_, long, x, long, y, long, x_offs, long, y_offs)
WRAP_WATCOM_FFUNC4(long, MouseIn, mouse_in_, long, x, long, y, long, x_offs, long, y_offs)
WRAP_WATCOM_FFUNC3(TGameObj*, ObjBlockingAt, obj_blocking_at_, TGameObj*, object, long, tile, long, elevation)
WRAP_WATCOM_FFUNC3(long, ObjNewSidInst, obj_new_sid_inst_, TGameObj*, object, long, sType, long, scriptIndex)
WRAP_WATCOM_FFUNC3(long, ObjectUnderMouse, object_under_mouse_, long, crSwitch, long, inclDude, long, elevation)
WRAP_WATCOM_FFUNC4(void, RegisterObjectCall, register_object_call_, long*, target, long*, source, void*, func, long, delay)
WRAP_WATCOM_FFUNC3(long, ScrGetLocalVar, scr_get_local_var_, long, sid, long, varId, long*, value)
WRAP_WATCOM_FFUNC3(long, ScrSetLocalVar, scr_set_local_var_, long, sid, long, varId, long, value)
WRAP_WATCOM_FFUNC3(long, TileNumInDirection, tile_num_in_direction_, long, tile, long, rotation, long, distance)
WRAP_WATCOM_FFUNC8(void, TransCscale, trans_cscale_, void*, fromBuff, long, width, long, height, long, pitch, void*, toBuff, long, toWidth, long, toHeight, long, toPitch)
WRAP_WATCOM_FFUNC3(void, WinClip, win_clip_, WINinfo*, window, RectList**, rects, void*, buffer)

WRAP_WATCOM_FFUNC3(const char*, InterpretGetString, interpretGetString_, TProgram*, scriptPtr, DWORD, dataType, DWORD, strId)

/* stdcall */
WRAP_WATCOM_FUNC1(bool, ArtExists, art_exists_, long, artFid)
WRAP_WATCOM_FUNC0(void, ArtFlush, art_flush_)
WRAP_WATCOM_FUNC1(const char*, ArtGetName, art_get_name_, long, artFID)
WRAP_WATCOM_FUNC5(long, ArtId, art_id_, long, artType, long, lstIndex, long, animCode, long, weaponCode, long, directionCode)
WRAP_WATCOM_FUNC3(BYTE*, ArtFrameData, art_frame_data_, FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, ArtFrameWidth, art_frame_width_, FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC3(long, ArtFrameLength, art_frame_length_, FrmHeaderData*, frm, long, frameNum, long, rotation)
WRAP_WATCOM_FUNC2(FrmHeaderData*, ArtPtrLock, art_ptr_lock_, long, frmId, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, ArtPtrLockData, art_ptr_lock_data_, long, frmId, long, frameNum, long, rotation, DWORD*, lockPtr)
WRAP_WATCOM_FUNC4(BYTE*, ArtLock, art_lock_, long, frmId, DWORD*, lockPtr, long*, widthOut, long*, heightOut)
WRAP_WATCOM_FUNC1(long, ArtPtrUnlock, art_ptr_unlock_, DWORD, lockId)
WRAP_WATCOM_FUNC2(long, BarterComputeValue, barter_compute_value_, TGameObj*, source, TGameObj*, target)
WRAP_WATCOM_FUNC1(long, BlockForTocks, block_for_tocks_, long, ticks)
WRAP_WATCOM_FUNC1(const char*, CritterName, critter_name_, TGameObj*, critter) // Returns the name of the critter
WRAP_WATCOM_FUNC1(void, CritterPcSetName, critter_pc_set_name_, const char*, newName) // Change the name of playable character
/* Database functions */
WRAP_WATCOM_FUNC1(bool, DbAccess, db_access_, const char*, fileName) // Checks if given file exists in DB
WRAP_WATCOM_FUNC1(long, DbFClose, db_fclose_, DbFile*, file)
WRAP_WATCOM_FUNC2(DbFile*, DbFOpen, db_fopen_, const char*, path, const char*, mode)
WRAP_WATCOM_FUNC1(long, DbFGetc, db_fgetc_, DbFile*, file)
WRAP_WATCOM_FUNC3(char*, DbFGets, db_fgets_, char*, buf, long, max_count, DbFile*, file)
WRAP_WATCOM_FUNC4(long, DbFRead, db_fread_, void*, buf, long, elsize, long, count, DbFile*, file)
WRAP_WATCOM_FUNC3(long, DbFSeek, db_fseek_, DbFile*, file, long, pos, long, origin)
WRAP_WATCOM_FUNC2(void, DbFreeFileList, db_free_file_list_, char***, fileList, DWORD, arg2) // Destroys filelist array created by DbGetFileList
WRAP_WATCOM_FUNC2(long, DbFReadByte, db_freadByte_, DbFile*, file, BYTE*, rout)
WRAP_WATCOM_FUNC2(long, DbFReadShort, db_freadShort_, DbFile*, file, WORD*, rout)
WRAP_WATCOM_FUNC2(long, DbFReadInt, db_freadInt_, DbFile*, file, DWORD*, rout)
WRAP_WATCOM_FUNC3(long, DbFReadByteCount, db_freadByteCount_, DbFile*, file, BYTE*, cptr, long, count)
WRAP_WATCOM_FUNC3(long, DbFReadShortCount, db_freadShortCount_, DbFile*, file, WORD*, dest, long, count)
WRAP_WATCOM_FUNC3(long, DbFReadIntCount, db_freadIntCount_, DbFile*, file, DWORD*, dest, long, count)
WRAP_WATCOM_FUNC2(long, DbFWriteByte, db_fwriteByte_, DbFile*, file, long, value)
WRAP_WATCOM_FUNC2(long, DbFWriteInt, db_fwriteInt_, DbFile*, file, long, value)
WRAP_WATCOM_FUNC3(long, DbFWriteByteCount, db_fwriteByteCount_, DbFile*, file, const BYTE*, cptr, long, count)
WRAP_WATCOM_FUNC2(long, DbDirEntry, db_dir_entry_, const char*, fileName, DWORD*, sizeOut) // Check fallout file and get file size (result 0 - file exists)
// Searches files in DB by given path/filename mask and stores result in fileList
// fileList is a pointer to a variable, that will be assigned with an address of an array of char* strings
WRAP_WATCOM_FUNC2(long, DbGetFileList, db_get_file_list_, const char*, searchMask, char***, fileList) // Returns number of elements in *fileList
WRAP_WATCOM_FUNC2(long, DbInit, db_init_, const char*, path_dat, const char*, path_patches)
WRAP_WATCOM_FUNC1(void*, DbaseOpen, dbase_open_, const char*, fileName)
WRAP_WATCOM_FUNC1(void, DbaseClose, dbase_close_, void*, dbPtr)
////////////////////////
WRAP_WATCOM_FUNC1(void, DisplayPrint, display_print_, const char*, msg) // Displays message in main UI console window
WRAP_WATCOM_FUNC0(void, DisplayStats, display_stats_)
WRAP_WATCOM_FUNC1(long, CritterIsDead, critter_is_dead_, TGameObj*, critter)
// Execute script proc by internal proc number (from script's proc table, basically a sequential number of a procedure as defined in code, starting from 1)
WRAP_WATCOM_FUNC2(void, ExecuteProcedure, executeProcedure_, TProgram*, sptr, long, procNum)
WRAP_WATCOM_FUNC1(const char*, FindCurrentProc, findCurrentProc_, TProgram*, program) // Returns the name of current procedure by program pointer
WRAP_WATCOM_FUNC1(long, FMtextWidth, FMtext_width_, const char*, text)
WRAP_WATCOM_FUNC0(long, GetInputBtn, get_input_)
// Searches for message ID in given message file and places result in result argument
WRAP_WATCOM_FUNC3(const char*, Getmsg, getmsg_, const MSGList*, fileAddr, MSGNode*, result, long, messageId)
WRAP_WATCOM_FUNC0(long, Gmouse3dGetMode, gmouse_3d_get_mode_)
WRAP_WATCOM_FUNC1(void, Gmouse3dSetMode, gmouse_3d_set_mode_, long, mode)
WRAP_WATCOM_FUNC1(long, GmouseSetCursor, gmouse_set_cursor_, long, picNum)
WRAP_WATCOM_FUNC1(long, GsoundBackgroundVolumeGetSet, gsound_background_volume_get_set_, long, setVolume)
WRAP_WATCOM_FUNC1(void, GsoundPlaySfxFile, gsound_play_sfx_file_, const char*, name) // Plays SFX sound with given name
WRAP_WATCOM_FUNC1(WINinfo*, GNWFind, GNW_find_, long, winRef)
WRAP_WATCOM_FUNC2(long, Interpret, interpret_, TProgram*, program, long, arg2)
// Finds procedure ID for given script program pointer and procedure name
WRAP_WATCOM_FUNC2(long, InterpretFindProcedure, interpretFindProcedure_, TProgram*, scriptPtr, const char*, procName)
WRAP_WATCOM_FUNC1(DWORD, InterpretPopShort, interpretPopShort_, TProgram*, scriptPtr) // Pops value type from Data stack (must be followed by InterpretPopLong)
WRAP_WATCOM_FUNC1(DWORD, InterpretPopLong, interpretPopLong_, TProgram*, scriptPtr)   // Pops value from Data stack (must be preceded by InterpretPopShort)
WRAP_WATCOM_FUNC2(long, IntfaceGetAttack, intface_get_attack_, DWORD*, hitMode, DWORD*, isSecondary)
WRAP_WATCOM_FUNC0(long, IntfaceIsHidden, intface_is_hidden_)
WRAP_WATCOM_FUNC0(void, IntfaceRedraw, intface_redraw_) // Redraws the main game interface windows (useful after changing some data like active hand, etc.)
WRAP_WATCOM_FUNC0(void, IntfaceToggleItemState, intface_toggle_item_state_)
WRAP_WATCOM_FUNC1(void, IntfaceUpdateAc, intface_update_ac_, long, animate)
WRAP_WATCOM_FUNC2(void, IntfaceUpdateMovePoints, intface_update_move_points_, long, ap, long, freeAP)
WRAP_WATCOM_FUNC0(void, IntfaceUseItem, intface_use_item_)
WRAP_WATCOM_FUNC1(TGameObj*, InvenLeftHand, inven_left_hand_, TGameObj*, critter) // Item in critter's left hand slot
WRAP_WATCOM_FUNC1(TGameObj*, InvenRightHand, inven_right_hand_, TGameObj*, critter) // Item in critter's right hand slot
WRAP_WATCOM_FUNC2(TGameObj*, InvenPidIsCarriedPtr, inven_pid_is_carried_ptr_, TGameObj*, invenObj, long, pid)
WRAP_WATCOM_FUNC2(long, InvenUnwield, inven_unwield_, TGameObj*, critter, long, slot)
WRAP_WATCOM_FUNC1(TGameObj*, InvenWorn, inven_worn_, TGameObj*, critter) // Critter worn item (armor)
WRAP_WATCOM_FUNC1(long, IsPartyMember, isPartyMember_, TGameObj*, obj)
WRAP_WATCOM_FUNC2(long, IsWithinPerception, is_within_perception_, TGameObj*, source, TGameObj*, target)
WRAP_WATCOM_FUNC1(long, ItemCCurrSize, item_c_curr_size_, TGameObj*, critter)
WRAP_WATCOM_FUNC1(long, ItemCapsTotal, item_caps_total_, TGameObj*, object)
WRAP_WATCOM_FUNC1(long, ItemGetType, item_get_type_, TGameObj*, item)
WRAP_WATCOM_FUNC1(long, ItemMDecCharges, item_m_dec_charges_, TGameObj*, item) // Returns 0 on success, -1 if the item has no charges
WRAP_WATCOM_FUNC1(long, ItemSize, item_size_, TGameObj*, item)
WRAP_WATCOM_FUNC1(long, ItemTotalCost, item_total_cost_, TGameObj*, object)
WRAP_WATCOM_FUNC1(long, ItemTotalWeight, item_total_weight_, TGameObj*, object)
WRAP_WATCOM_FUNC2(long, ItemWAnimWeap, item_w_anim_weap_, TGameObj*, item, DWORD, hitMode)
WRAP_WATCOM_FUNC2(long, ItemWComputeAmmoCost, item_w_compute_ammo_cost_, TGameObj*, item, DWORD*, rounds)
WRAP_WATCOM_FUNC1(long, ItemWCurrAmmo, item_w_curr_ammo_, TGameObj*, item)
WRAP_WATCOM_FUNC1(long, ItemWMaxAmmo, item_w_max_ammo_, TGameObj*, item)
WRAP_WATCOM_FUNC2(long, ItemWRange, item_w_range_, TGameObj*, critter, long, hitMode)
WRAP_WATCOM_FUNC2(long, ItemWReload, item_w_reload_, TGameObj*, weapon, TGameObj*, ammo)
WRAP_WATCOM_FUNC1(long, ItemWRounds, item_w_rounds_, TGameObj*, item)
WRAP_WATCOM_FUNC1(long, ItemWeight, item_weight_, TGameObj*, item)
WRAP_WATCOM_FUNC2(long, LightGetTile, light_get_tile_, long, elevation, long, tileNum) // Returns light level at given tile
WRAP_WATCOM_FUNC2(long, LoadFrame, load_frame_, const char*, filename, FrmFile**, frmPtr)
WRAP_WATCOM_FUNC1(TProgram*, LoadProgram, loadProgram_, const char*, fileName)
WRAP_WATCOM_FUNC1(const char*, MapGetShortName, map_get_short_name_, long, mapID)
WRAP_WATCOM_FUNC2(void, MapDirErase, MapDirErase_, const char*, folder, const char*, ext)
WRAP_WATCOM_FUNC1(void, MemFree, mem_free_, void*, mem)
WRAP_WATCOM_FUNC2(void*, MemRealloc, mem_realloc_, void*, lpmem, DWORD, msize)
WRAP_WATCOM_FUNC1(long, MessageExit, message_exit_, MSGList*, msgList) // Destroys message list
WRAP_WATCOM_FUNC2(long, MessageLoad, message_load_, MSGList*, msgList, const char*, msgFilePath) // Loads MSG file into given MessageList
WRAP_WATCOM_FUNC2(long, MessageSearch, message_search_, const MSGList*, file, MSGNode*, msg)
WRAP_WATCOM_FUNC2(void, MouseGetPosition, mouse_get_position_, long*, outX, long*, outY)
WRAP_WATCOM_FUNC0(void, MouseShow, mouse_show_)
WRAP_WATCOM_FUNC0(void, MouseHide, mouse_hide_)
// Calculates path and returns it's length
WRAP_WATCOM_FUNC6(long, MakePathFunc, make_path_func_, TGameObj*, objectFrom, long, tileFrom, long, tileTo, char*, pathDataBuffer, long, arg5, void*, blockingFunc)
WRAP_WATCOM_FUNC0(long, NewObjId, new_obj_id_)
WRAP_WATCOM_FUNC2(void, ObjBound, obj_bound_, TGameObj*, object, BoundRect*, boundRect) // Calculates bounding box (rectangle) for a given object
WRAP_WATCOM_FUNC1(long, ObjDestroy, obj_destroy_, TGameObj*, object)
WRAP_WATCOM_FUNC2(long, ObjDist, obj_dist_, TGameObj*, obj_src, TGameObj*, obj_trg)
WRAP_WATCOM_FUNC2(long, ObjEraseObject, obj_erase_object_, TGameObj*, object, BoundRect*, boundRect)
WRAP_WATCOM_FUNC0(TGameObj*, ObjFindFirst, obj_find_first_)
WRAP_WATCOM_FUNC0(TGameObj*, ObjFindNext, obj_find_next_)
WRAP_WATCOM_FUNC2(TGameObj*, ObjFindFirstAtTile, obj_find_first_at_tile_, long, elevation, long, tileNum)
WRAP_WATCOM_FUNC0(TGameObj*, ObjFindNextAtTile, obj_find_next_at_tile_)
WRAP_WATCOM_FUNC2(long, ObjPidNew, obj_pid_new_, TGameObj*, object, long, pid)
WRAP_WATCOM_FUNC1(long, ObjLockIsJammed, obj_lock_is_jammed_, TGameObj*, object) // Checks/unjams jammed locks
WRAP_WATCOM_FUNC1(void, ObjUnjamLock, obj_unjam_lock_, TGameObj*, object)
WRAP_WATCOM_FUNC1(long, PartyMemberGetCurrentLevel, partyMemberGetCurLevel_, TGameObj*, obj)
WRAP_WATCOM_FUNC2(long, PerkCanAdd, perk_can_add_, TGameObj*, critter, long, perkId)
WRAP_WATCOM_FUNC2(long, PerkLevel, perk_level_, TGameObj*, critter, long, perkId)
WRAP_WATCOM_FUNC6(long, PickDeath, pick_death_, TGameObj*, attacker, TGameObj*, target, TGameObj*, weapon, long, amount, long, anim, long, hitFromBack)
WRAP_WATCOM_FUNC0(void, ProcessBk, process_bk_)
WRAP_WATCOM_FUNC0(void, ProtoDudeUpdateGender, proto_dude_update_gender_)
WRAP_WATCOM_FUNC2(long*, QueueFindFirst, queue_find_first_, TGameObj*, object, long, qType)
WRAP_WATCOM_FUNC2(long*, QueueFindNext, queue_find_next_, TGameObj*, object, long, qType)
WRAP_WATCOM_FUNC3(long, RegisterObjectAnimateAndHide, register_object_animate_and_hide_, TGameObj*, object, long, anim, long, delay)
WRAP_WATCOM_FUNC3(long, RegisterObjectChangeFid, register_object_change_fid_, TGameObj*, object, long, artFid, long, delay)
WRAP_WATCOM_FUNC3(long, RegisterObjectLight, register_object_light_, TGameObj*, object, long, lightRadius, long, delay)
WRAP_WATCOM_FUNC1(long, RegisterObjectMustErase, register_object_must_erase_, TGameObj*, object)
WRAP_WATCOM_FUNC3(long, RegisterObjectTakeOut, register_object_take_out_, TGameObj*, object, long, holdFrameId, long, nothing)
WRAP_WATCOM_FUNC3(long, RegisterObjectTurnTowards, register_object_turn_towards_, TGameObj*, object, long, tileNum, long, nothing)
WRAP_WATCOM_FUNC2(long, RollRandom, roll_random_, long, minValue, long, maxValue)
WRAP_WATCOM_FUNC1(long*, RunProgram, runProgram_, TProgram*, progPtr)
WRAP_WATCOM_FUNC1(TScript*, ScrFindFirstAt, scr_find_first_at_, long, elevation)
WRAP_WATCOM_FUNC0(TScript*, ScrFindNextAt, scr_find_next_at_)
WRAP_WATCOM_FUNC1(TGameObj*, ScrFindObjFromProgram, scr_find_obj_from_program_, TProgram*, program)
WRAP_WATCOM_FUNC2(long, ScrNew, scr_new_, long*, scriptID, long, sType)
// Saves pointer to script object into scriptPtr using scriptID
WRAP_WATCOM_FUNC2(long, ScrPtr, scr_ptr_, long, scriptId, TScript**, scriptPtr) // Returns 0 on success, -1 on failure
WRAP_WATCOM_FUNC1(long, ScrRemove, scr_remove_, long, scriptID)
WRAP_WATCOM_FUNC1(void, SetFocusFunc, set_focus_func_, void*, func)
WRAP_WATCOM_FUNC1(long, SkillIsTagged, skill_is_tagged_, long, skill)
WRAP_WATCOM_FUNC2(long, StatGetBaseDirect, stat_get_base_direct_, TGameObj*, critter, long, statID)
WRAP_WATCOM_FUNC2(long, StatLevel, stat_level_, TGameObj*, critter, long, statId)
WRAP_WATCOM_FUNC1(long, TextFont, text_font_, long, fontNum)
WRAP_WATCOM_FUNC2(long, TileDist, tile_dist_, long, scrTile, long, dstTile)
WRAP_WATCOM_FUNC2(long, TileDir, tile_dir_, long, scrTile, long, dstTile)
WRAP_WATCOM_FUNC0(void, TileRefreshDisplay, tile_refresh_display_) // Redraws the whole screen
WRAP_WATCOM_FUNC2(void, TileRefreshRect, tile_refresh_rect_, BoundRect*, boundRect, long, elevation) // Redraws the given rectangle on screen
WRAP_WATCOM_FUNC1(long, TraitLevel, trait_level_, long, traitID)
WRAP_WATCOM_FUNC6(long, WinAdd, win_add_, long, x, long, y, long, width, long, height, long, bgColorIndex, long, flags)
WRAP_WATCOM_FUNC1(void, WinShow, win_show_, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, WinHide, win_hide_, DWORD, winRef)
WRAP_WATCOM_FUNC1(BYTE*, WinGetBuf, win_get_buf_, DWORD, winRef)
WRAP_WATCOM_FUNC1(void, WinDraw, win_draw_, DWORD, winRef)
WRAP_WATCOM_FUNC2(void, WinDrawRect, win_draw_rect_, DWORD, winRef, RECT*, rect)
WRAP_WATCOM_FUNC1(void, WinDelete, win_delete_, DWORD, winRef)
WRAP_WATCOM_FUNC0(long, WindowWidth, windowWidth_)
WRAP_WATCOM_FUNC1(void, WmRefreshInterfaceOverlay, wmRefreshInterfaceOverlay_, long, isRedraw)
WRAP_WATCOM_FUNC2(DbFile*, XFOpen, xfopen_, const char*, fileName, const char*, flags)
WRAP_WATCOM_FUNC3(long, XFSeek, xfseek_, DbFile*, file, long, fOffset, long, origin)
