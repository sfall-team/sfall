---
layout: page
title: Global scripts
nav_order: 2
---

# Global scripts

As well as the new functions, sfall also adds global scripts. These run independent of any loaded maps, but do not have an attached object. (i.e. using `self_obj` without using `set_self` first will crash the script.) To use a global script, the script must have a name which begins with `gl` and contains a procedure called `start`, `map_enter_p_proc`, `map_exit_p_proc`, or `map_update_p_proc`. The start procedure will be executed once when the player loads a saved game or starts a new game. The `map_*_p_proc` procedures will be executed once when a map is being entered/left/updated. If you wish the script to be executed repeatedly, call `set_global_script_repeat` on the first run of the start procedure using the number of frames between each run as the argument. (0 disables the script, 1 runs it every frame, 2 runs it every other frame etc.)

Global scripts have multiple modes, which can be set using the `set_global_script_type` function.
* In the default mode (i.e. mode 0) their execution is linked to the local map game loop, so the script will not run in dialogs or on the world map.
* In mode 1 their execution is linked to the player input, and so they will run whenever the mouse cursor is visible on screen, including the world map, character dialogs etc.
* In mode 2, execution is linked to the world map loop, so the script will only be executed on the world map and not on the local map or in any dialog windows.
* Mode 3 is a combination of modes 0 and 2, so scripts will be executed on both local maps and the world map, but not in dialog windows. Using mode 1 requires the input wrapper to be enabled. Use `available_global_script_types` to check what is available.
