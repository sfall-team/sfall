// An easier way to introduce function wrappers. 
// Just add one line here and you're all set.

WRAP_WATCOM_FUNC3(int, register_object_animate, TGameObj*, object, int, anim, int, delay)
WRAP_WATCOM_FUNC3(int, register_object_animate_and_hide, TGameObj*, object, int, anim, int, delay)
// WRAP_WATCOM_FUNC3(int, register_object_animate_and_move_straight_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_animate_forever_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_animate_reverse_, TGameObj*, object;
WRAP_WATCOM_FUNC3(int, register_object_change_fid, TGameObj*, object, int, artFid, int, delay)
// WRAP_WATCOM_FUNC3(int, register_object_check_falling_, TGameObj*, object;
WRAP_WATCOM_FUNC1(int, register_object_dec_rotation, TGameObj*, object)
WRAP_WATCOM_FUNC1(int, register_object_erase, TGameObj*, object)
WRAP_WATCOM_FUNC3(int, register_object_funset, TGameObj*, object, int, action, int, delay)
WRAP_WATCOM_FUNC1(int, register_object_inc_rotation, TGameObj*, object)
WRAP_WATCOM_FUNC3(int, register_object_light, TGameObj*, object, int, lightRadius, int, delay)
// WRAP_WATCOM_FUNC3(int, register_object_move_on_stairs_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_move_straight_to_tile_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_move_to_object_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_move_to_tile_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_must_call_, TGameObj*, object;
WRAP_WATCOM_FUNC1(int, register_object_must_erase, TGameObj*, object)
// WRAP_WATCOM_FUNC3(int, register_object_outline_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_play_sfx_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_run_to_object_, TGameObj*, object;
// WRAP_WATCOM_FUNC3(int, register_object_run_to_tile_, TGameObj*, object;
WRAP_WATCOM_FUNC3(int, register_object_take_out, TGameObj*, object, int, holdFrameId, int, nothing)
WRAP_WATCOM_FUNC3(int, register_object_turn_towards, TGameObj*, object, int, tileNum, int, nothing)
// returns light level at given tile
WRAP_WATCOM_FUNC2(int, light_get_tile, int, elevation, int, tileNum)
