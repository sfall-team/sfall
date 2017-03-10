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
*/

WRAP_WATCOM_FUNC0(void, intface_use_item)
// returns light level at given tile
WRAP_WATCOM_FUNC2(long, light_get_tile, long, elevation, long, tileNum)
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
WRAP_WATCOM_FUNC1(long, item_w_max_ammo, GameObject*, item)
WRAP_WATCOM_FUNC1(long, item_w_curr_ammo, GameObject*, item)
