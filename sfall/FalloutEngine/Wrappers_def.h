// An easier way to introduce function wrappers. 
// Just add one line here and you're all set.

WRAP_WATCOM_FUNC0(void, intface_use_item)
// returns light level at given tile
WRAP_WATCOM_FUNC2(long, light_get_tile, long, elevation, long, tileNum)
WRAP_WATCOM_FUNC3(long, register_object_animate, TGameObj*, object, long, anim, long, delay)
WRAP_WATCOM_FUNC3(long, register_object_animate_and_hide, TGameObj*, object, long, anim, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_animate_and_move_straight_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_animate_forever_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_animate_reverse_, TGameObj*, object;
WRAP_WATCOM_FUNC3(long, register_object_change_fid, TGameObj*, object, long, artFid, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_check_falling_, TGameObj*, object;
WRAP_WATCOM_FUNC1(long, register_object_dec_rotation, TGameObj*, object)
WRAP_WATCOM_FUNC1(long, register_object_erase, TGameObj*, object)
WRAP_WATCOM_FUNC3(long, register_object_funset, TGameObj*, object, long, action, long, delay)
WRAP_WATCOM_FUNC1(long, register_object_inc_rotation, TGameObj*, object)
WRAP_WATCOM_FUNC3(long, register_object_light, TGameObj*, object, long, lightRadius, long, delay)
// WRAP_WATCOM_FUNC3(long, register_object_move_on_stairs_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_straight_to_tile_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_to_object_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_move_to_tile_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_must_call_, TGameObj*, object;
WRAP_WATCOM_FUNC1(long, register_object_must_erase, TGameObj*, object)
// WRAP_WATCOM_FUNC3(long, register_object_outline_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_play_sfx_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_run_to_object_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(long, register_object_run_to_tile_, TGameObj*, object;
WRAP_WATCOM_FUNC3(long, register_object_take_out, TGameObj*, object, long, holdFrameId, long, nothing)
WRAP_WATCOM_FUNC3(long, register_object_turn_towards, TGameObj*, object, long, tileNum, long, nothing)
WRAP_WATCOM_FUNC1(long, item_w_max_ammo, TGameObj*, item)
WRAP_WATCOM_FUNC1(long, item_w_curr_ammo, TGameObj*, item)
