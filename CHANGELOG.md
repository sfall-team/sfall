# Changelog

## 3.8.47
* Fixed potential undefined behavior in **FullItemDescInBarter**
* Fixed the behavior of `break` statement in script compiler and decompiler (`compile.exe` and `int2ssl.exe` in the **modders pack**)
* Improved the fix for removing floating text messages on the map when moving to another map or elevation
* Improved the error handling for missing main **.dat** files
* Removed glass glare from the **'Use Inventory Item On'** window to match other game interfaces
* Added a fix for a crash when a critter with a powered melee/unarmed weapon ran out of ammo and there was ammo nearby
* Added a fix to prevent a potential crash when using skills on non-critter objects
* Added a fix for the duplicate click sound for the 'Done' button in the **'Move Items'** and **'Set Timer'** windows
* Added a few fixes for slightly misaligned buttons in the pipboy, character screen, and barter screen
* Added missing sounds to the 'Done' and 'Cancel' buttons in the 'Custom' disposition of the combat control panel
* Added lowercase `a` as a hotkey for the 'Take All' button (previously only uppercase `A`)

## 3.8.46.1
* The compatibility mode check now targets only unsupported Windows versions
* Changed the way **ProcessorIdle** works to prevent conflicts with **CPU_USAGE_FIX** option in `f2_res.ini`
* Added a fix to prevent out-of-bounds selection in the file list when loading a character file
* Added a fix to remove visible newline control characters `\n` when examining items in the barter screen

## 3.8.46
* Fixed a bug introduced in 3.8.31 that broke the debug message about a missing critter art file
* Fixed an issue where an item with a unique ID in the inventory had its ID reset by a non-unique item
* Restored the position of the ammo bar when **ALTERNATE_AMMO_METRE=0** in `f2_res.ini`
* Improved the fix for the display issue in the pipboy when the automap list is too long
* Improved the compatibility mode check for newer Windows
* Changed **PipBoyAvailableAtGameStart** option and `set_pipboy_available` script function to no longer modify the vault suit movie state
* Added a fix for the main menu music not stopping when replaying the intro
* Added a fix for display issues when highlighting a multiline dialogue option
* Added a fix for the incorrect message being displayed when attempting to repair a dead robot
* Added a fix for the duplicate click sound when selecting a location in the Status section of the pipboy
* Added a fix for extra hidden buttons below the location list in the Status section of the pipboy
* Added a fix for map lighting from **Night Vision** perk not updating when loading a saved game
* Added a fix for an animation glitch when death animations and combat start simultaneously
* Added a fix to prevent the game from hanging when reloading a weapon overloaded with ammo via the interface bar
* Added a few fixes for issues related to weapons with negative ammo
* Added a tweak to replace death animations on critters with single-frame variants on map load
* Added more options for tweaking some engine perks to the **perks ini file**
* Reduced the green tone of the message window on the interface bar for text clarity

## 3.8.45.1
* Fixed a bug in **XPTable** that caused leveling issues with **Here and Now** perk
* Disabled unnecessary selfrun file creation in the recording mode (autoplay has been disabled in 3.8.30)

## 3.8.45
* Fixed a bug introduced in 3.8.29 that caused `game_loaded` script function to always return 1 when called from normal scripts
* Fixed a bug introduced in 3.8.44 that caused the information card for the **hero appearance mod** not to refresh properly
* Fixed a crash bug in **AutoSearchSFX** when an **ACM** file has a name longer than 12 characters
* Fixed an issue where sfall did not reset data properly after attempting to load a corrupted saved game
* Fixed the inconsistent behavior of the escaped percent sign `%` in `sprintf` and `string_format` script functions
* Fixed `show/hide_iface_tag` script functions to prevent unnecessary toggling of tags
* Fixed `using_skill` script function returning garbage values when the arguments are not the player and Sneak skill
* Added a fix for the engine not checking **'misc'** type items when correcting data for items on maps
* Added a fix to prevent the windows of **Tag!** and **Mutate!** perks from reappearing when there are still unused perks
* Added a tweak to restore the player's sneak state when switching between maps
* Added options to separately set the color of outlines for highlighted containers and corpses
* Updated **item highlighting mod** in the **modders pack** to match the feature set of the 4.x version
* New hook script: `hs_buildsfxweapon`

## 3.8.44
* Fixed a bug in **NPCsTryToSpendExtraAP** that caused NPCs to still be able to move in their combat turn after they killed themselves
* Fixed a bug in **PlayIdleAnimOnReload** that could cause critters to lose highlights after reloading weapons in combat
* Fixed a crash bug in `interface_art_draw` script function when drawing beyond window bounds
* Fixed the missing hotkeys for additional elevators derived from the original elevator types
* Fixed and improved the syntax parsing in script compiler (`compile.exe` in the **modders pack**)
* Improved the fix for using **'English'** as the fallback language directory for msg files to work on a per-line basis
* Improved the fix for saving/loading party member protos to include the car trunk
* Improved the performance of `draw_image`, `draw_image_scaled`, and `interface_art_draw` script functions
* Expanded `show/hide_iface_tag` script functions to work with the tag value 0 (sneak)
* Changed **AIDrugUsePerfFix** to be enabled by default
* Tweaked the behavior of **NPCAutoLevel** option and renamed it to **PartyMemberNonRandomLevelUp** to better match its behavior
* Removed the unnecessary error message about sndlist.lst when **AutoSearchSFX** is enabled
* Added a fix for the time interval for healing during world map travel being tied to the system timer instead of game time
* Added a fix for the interface bar obscuring the barter/trade interface when using the hi-res patch
* Added a tweak to prevent the **'forever'** type of animation on objects from stopping when entering combat
* Added an option to use the expanded world map interface (requires the hi-res patch)
* Added an option to pre-fill the **'Move Items'** window with the correct balancing amount when moving money/caps in the barter screen
* Added an option to set up a key to let you move items between inventory lists by simply clicking on them
* Added debug options to adjust the size and text font for the internal debug window
* Added an option for tweaking **Cautious Nature** perk to the **perks ini file**
* Added a new attribute type value to `get_window_attribute script` function
* Added `PAUSEWIN` flag to the game mode functions (when pausing the game using `Ctrl+P`)
* Added 2 new arguments to `HOOK_ENCOUNTER` hook script
* Added a healing skills example script to the example mods in the **modders pack**
* New script function: `signal_close_game`, `art_frame_data`, `set_worldmap_heal_time`

## 3.8.43.1
* Fixed a possible hang or crash introduced in 3.8.41 when running certain script sequences

## 3.8.43
* Restored support for **pre-SSE** processors because the code optimization resulting from dropping support is marginal
* Fixed a bug introduced in 3.8.29 that caused critters with crippled arms to be unable to attack in certain cases
* Fixed the critical hit messages of the right arm and left leg for super mutants
* Fixed `add_extra_msg_file` script function to prevent it from loading the same msg file more than once
* Removed **DisplaySwiftLearnerExp** from `ddraw.ini` because there is little reason to turn it off
* Removed **SpeedInterfaceCounterAnims=3**. Now the AC counter is always instantly updated when switching to other controlled critters in combat
* Added a fix for the game disappearing from the taskbar after using `Alt+Tab`
* Added a fix for the clickability issue of the **'Use Inventory Item On'** action when the selected item overlaps an object
* Added a fix for the xp exploit from canceling **Here and Now** perk when the player has **Swift Learner** perk
* Added support for loading an external **DirectDraw** wrapper (`ddraw.dll`) from the `<GameRoot>\wrapper\` directory for rendering
* Added a static books mod and a molotov fire damage mod to the example mods in the **modders pack**

## 3.8.42
* Fixed a bug in **XPTable** that could cause a hang when leveling up in some cases
* Fixed the critical hit messages of the right arm for some critter types
* Changed `add_mult_objs_to_inven` script function to allow adding more than 99999 instances of an object in one go
* Added a fix to use **'English'** as the fallback language directory for loading `credits.txt/quotes.txt`
* Added a fix for gaining two levels at once when leveling up from level 97
* Added a fix for the modulo operator treating negative integers as unsigned
* Added a fix to prevent integer overflow for the number of items in a stack in the inventory
* Added a fix to `COMBATDAMAGE` hook to prevent instadeath critical hits for no damage
* New script function: `reg_anim_animate_and_move`

## 3.8.41
* Fixed a bug introduced in 3.8.40 that broke the interoperability of saved arrays with older versions
* Fixed a crash when calling `start_gdialog` outside the `talk_p_proc` procedure for talking heads
* Fixed `create_object_sid` script function to allow creating an object with no script correctly when passing 0 as the script index number
* Changed the calculation of the **'best armor'** score to exclude the EMP stats (gameplay mod friendly)
* Tweaked the position of the ammo bar on the interface bar
* Removed **AdditionalWeaponAnims** from `ddraw.ini`. Now additional weapon animation codes are always available
* Updated **NPC combat control mod** in the modders pack to make the player's **Jinxed** trait/perk affect all controlled critters

## 3.8.40.1
* Improved compatibility with older Windows 2000 (**pre-SP4**)

## 3.8.40
* Implemented a `mods_order.txt` to improve and simplify mod ordering and add support for mod managers. This replaces previous **.dat** file autoloading. Please refer to `ddraw.ini` for details
* Implemented a **custom config file parser**, which greatly improves the performance of sfall initialization and reading files from scripts
* Fixed a bug when updating the maximum HP stat of critters on the map when loaded for the first time
* Fixed `set_pc_base/extra_stat` and `set_critter_base/extra_stat` script functions not updating derived stats when setting SPECIAL stats
* Fixed `wield_obj_critter` script function, which can now put non-weapon/armor items in the critter's hand
* Fixed `get_tile_fid` script function to work on elevations 1 and 2 instead of always elevation 0
* Fixed `create_spatial` script function not setting the script index number upon object creation
* Fixed incorrect behavior of the subtraction operator involving floats and negative integers
* Fixed the backward compatibility of `get_npc_level` script function
* Fixed a crash bug in `display_msg` and `debug_msg` script functions when printing a string longer than 260 characters
* Fixed a crash bug in `AMMOCOST` hook when returning ammo cost of 0 for burst attacks
* Fixed various issues in script compiler and decompiler (`compile.exe` and `int2ssl.exe` in the **modders pack**)
* Improved and tweaked the behavior of **ComputeSpray_\*** options
* Expanded `atoi` script function to support parsing binary strings
* Expanded `string_format` script function to support more arguments and conversion types
* Changed **CheckWeaponAmmoCost** to be enabled by default and affect only hook type 1 of `HOOK_AMMOCOST` hook script
* Changed how `HOOK_DESCRIPTIONOBJ` hook script handles its return value. Now you can return normal strings directly in the hook
* Re-added the check for valid objects to `get/set_object_data` script functions (only disabled in combat for accessing the combat data)
* Removed the debug message about a missing critter art file from displaying in the game (added in 3.8.22)
* Removed the old built-in **NPC combat control** and added a script-based version to the example mods in the **modders pack**
* Added an option to let you use the command cursor to specify targets for party members to attack in combat
* Added an option to specify an additional directory for ini files used by scripts
* Added options to override the names of sound files used by the engine
* Added a debug option to duplicate logs to a dedicated console window alongside the game window
* Added a lower limit of -99% and an upper limit of 999% to the hit chance value to prevent a display issue
* Added more options for tweaking some engine perks to the **perks ini file**
* Added a new argument and a new return value to `HOOK_STEAL` hook script
* Added a burst control example script and a resting encounters mod to the example mods in the **modders pack**
* Updated the compute damage example script in the **modders pack**. Now it should be easier to write one's own damage formula
* Updated **NPC armor appearance mod** in the **modders pack** to prevent NPCs from equipping **unusable** weapons
* Updated **item highlighting mod** in the **modders pack** to match the feature set of the 4.x version
* Increased the setting range of the combat speed slider in the preferences screen
* Backported script functions from 4.0/4.1: `set_dude_obj`, `get_object_ai_data`
* Backported hook scripts from 4.x: `hs_combatturn`, `hs_rollcheck`, `hs_bestweapon`, `hs_canuseweapon`
* New script functions: `set_spray_settings`, `get/set_combat_free_move`, `get_ini_config`, `string_find`

## 3.8.38
* Fixed a bug introduced in 3.8.30 that could cause unexpected behavior for burst attacks at close range
* Fixed a bug introduced in 3.8.31 that caused the game to display incorrect names for corpses in some cases
* Fixed a crash bug introduced in 3.8.33.1 when not using `HOOK_COMBATDAMAGE` hook script
* Fixed a crash bug in **FullItemDescInBarter** when a weapon/ammo has no item description
* Fixed `create_object_sid` script function not setting the script index number upon object creation
* Fixed the broken `read_string` script function
* Changed the way disabled unsafe script functions work. Now they don't cause scripts to end abruptly
* Removed **StackEmptyWeapons** from `ddraw.ini`. Now unloaded weapons will always stack, no matter what type of ammo was loaded previously
* Removed **CreditsAtBottom** from `ddraw.ini`. Now sfall built-in credits are shown at the beginning when from the main menu and at the end during the ending
* Added a fix for the player's traits not being displayed on the character screen in certain cases
* Added a fix for incorrect death endings being shown under certain conditions
* Added an option to fix the bug of using First Aid/Doctor skills when using them on the player
* Added a new **[ExtraPatches]** section to allow setting multiple custom paths for loading game data
* Added a tweak to the ammo information in the inventory for Glovz's damage formula and Haenlomal's YAAM
* Added a tweak to allow premade characters to have less than two traits
* Added a tweak to `INVENTORYMOVE` hook for getting the amount of dropped money/caps
* Added support for closing the game by pressing `Alt+F4`
* Backported script functions from 4.0/4.1: `set_rest_heal_time`, `set_rest_mode`, `get/set_can_rest_on_map`, `get_string_pointer`, `item_make_explosive`
* Backported hook scripts from 4.x: `hs_cartravel`, `hs_setglobalvar`, `hs_resttimer`, `hs_useanimobj`, `hs_explosivetimer`, `hs_descriptionobj`, `hs_useskillon`, `hs_onexplosion`, `hs_setlighting`, `hs_sneak`, `hs_stdprocedure`, `hs_targetobject`, `hs_encounter`

## 3.8.37
* Dropped support for older **pre-SSE** processors in favor of more optimized code. Now sfall requires a processor with **SSE** support
* Fixed a bug that could prevent loading files from the `art\<language>\` directory
* Fixed `REMOVEINVENOBJ` hook to match the values of `RMOBJ_*` constants correctly
* Expanded `set_pipboy_available` script function to match **PipBoyAvailableAtGameStart** option
* Expanded `message_str_game` script function to support `editor.msg` file
* Increased the default number of sound buffers available for sound effects from 4 to 8
* Changed the way **AllowDShowSound** works. Now mode 2 is combined with mode 1
* Removed **MoreTiles** from `ddraw.ini`. Now the maximum number of tile FRMs is always 16383
* Backported script function from 4.1: `dialog_message`

## 3.8.36
* Minor code fixes and improvements to sfall and engine functions
* Fixed **UseWalkDistance** having no effect when trying to use a ladder
* Fixed `float_msg` script function not setting the purple or black text color correctly
* Changed mode 1 of **NPC combat control** to require sfall debugging mode to control all critters in combat
* Added a fix for NPC stuck in a loop of reloading the solar scorcher when out of ammo
* Added a boundary check to `set_terrain_name` script function
* New script function: `get_terrain_name`

## 3.8.35
* Fixed the handling of obsolete script functions that are still recognized by script compiler and decompiler
* Improved the fix for updating the maximum HP stat of critters on the map when loaded for the first time
* Removed **DivisionOperatorFix** from `ddraw.ini` because there is little reason to turn it off
* Removed **ComputeSprayMod** from `ddraw.ini`. Now **ComputeSpray_\*** options no longer require a master switch
* Added a fix for a crash when the player equips a weapon overloaded with ammo
* Added a fix for being able to use the **'Push'** action on members of the player's team in combat when they are knocked down
* Added missing sounds to the markers on the world map interface (similar to Fallout 1, from Ghosthack)

## 3.8.34
* Minor fixes to **LocalMapXLimit/LocalMapYLimit** options and the built-in **item highlighting**
* Removed **FadeBackgroundMusic** option because the fix in 3.8.33 doesn't work reliably in all cases
* Added a fix for being unable to plant items on non-biped critters with the **'Barter'** flag set (e.g. Skynet and Goris)
* Updated the ammo ini loader mod in the **modders pack**

## 3.8.33.1
* Backported the **code injection system for game hooks** from 4.1. In previous versions, the code of game hooks was always executed even if there was no corresponding hook script. Now the code of a game hook is only injected into the game when the corresponding hook script exists
* Fixed a bug introduced in 3.8.31 that caused the game to print an incorrect item name in some cases
* Fixed screenshots for **DX9** graphics modes. Now the screenshots are saved in **PNG** format when in **DX9** mode
* Improved the functionality of **AllowDShowMovies**. Now you can take screenshots and press any key to skip **AVI** movies
* Added a tweak to prevent **'Failure initializing input devices'** error for **DX9** windowed mode

## 3.8.33
* Code refactoring for the script extender to improve the error handling in script functions
* Fixed a bug introduced in 3.8.31 that broke the fix for party members with drug addictions
* Fixed a bug introduced in 3.8.31 that caused AI to be unable to use some weapons
* Fixed a bug introduced in 3.8.31 that caused the music not to be played after loading a saved game on the same map
* Fixed critical bugs in **FadeBackgroundMusic** that caused crashes in various situations
* Fixed incorrect display of name and damage values for unarmed attacks in the inventory in some cases
* Fixed the black screen issue in **DX9** mode when returning to the game after using `Alt+Tab`
* Fixed the mouse cursor lag in the save/load game screens
* Fixed and expanded the mouse drop area for the PC's and NPC's inventory on the barter screen
* Changed the **'Radiated'** on the character screen to be highlighted in gray when the player still has an impending radiation effect
* Changed **SkipCompatModeCheck** to not require sfall debugging mode
* Removed **UseCommandLine** from `ddraw.ini`. Now sfall will always check command line arguments for another ini file
* Removed **ArraysBehavior** and **RemoveWindowRounding** from `ddraw.ini` because there is little reason to turn them off
* Removed **SkipSizeCheck** from `ddraw.ini` and the executable file size check from sfall
* Removed the dependency on `d3dcompiler_42.dll` for **DX9** graphics modes
* Added a fix for incorrect value of the limit number of floating text messages
* Added a tweak to allow printing new floating text messages when their limit is exceeded
* Added a debug option to set up a key to toggle the display of the hex grid on the map on or off (like in the mapper)

## 3.8.32
* Changed the fix for grave type containers in 3.8.31 to an option, to fix compatibility with existing grave scripts
* Changed **OverrideArtCacheSize** to set the art cache size to 261 instead of 256
* Changed **Enable** option in the `[Speed]` section to no longer affect **SpeedMultiInitial**
* Tweaked the window title bar for **DX9** windowed mode
* Optimized the calculation process in Glovz's damage formula and Haenlomal's YAAM for burst attacks

## 3.8.31
* Fixed the Jet addiction not being removed when using the antidote in some cases
* Fixed the key repeat delay and rate when enabling the game speed tweak
* Fixed a possible crash at the end of the playback of alternative sound files
* Fixed a rounding error in Glovz's damage formula
* Improved the behavior of **SpeedInterfaceCounterAnims=3**
* Improved the rendering performance of **DX9** graphics modes
* Improved and tweaked the page control in **ExtraSaveSlots**
* Changed the way **AutoQuickSave** works. Now it sets the number of pages used for quick saving
* Changed **SingleCore** to set processor affinity to the second processor core if available
* Excluded the walking animation from the debug message about a missing critter art file for stationary critters
* Removed unnecessary check on action points when AI reloads a weapon (added in 3.8.30)
* Removed **AffectPlayback** option because it's not practical
* Added a fix for a crash when opening a file with name containing a `%` character
* Added a fix to prevent the main menu music from stopping when entering the load game screen
* Added a fix to display the palette color with index 255 correctly in **DX9** mode when using the hi-res patch
* Added a fix for grave type containers in the open state not executing the `use_p_proc` procedure
* Added a tweak to update unarmed attacks after leveling up
* Added a tweak to keep the selected attack mode for unarmed attacks when closing the inventory or when the combat ends
* Added a tweak to display the actual damage values of unarmed attacks in the inventory
* Added an option to enable fade effects for background music when stopping and starting the playback
* Added an option to automatically search for new **SFX** sound files at game startup
* Added a config file to change the requirements and effects of unarmed attacks
* Added a config file to change some engine parameters for the game mechanics
* Added the ability to continuously play the music when moving to another map that uses the same music
* Added the ability to set custom names for unarmed attacks in the inventory to **TranslationsINI**
* Added support for using the newline control character `\n` in the object description in `pro_*.msg` files
* Added support for the new **'Healing Item'** flag to item protos. Now AI will also use items with this flag for healing in combat
* Added support for the new **'Cannot Use'** flag to the misc flags of item objects. This flag makes a weapon object unusable in combat
* Added missing sounds to the buttons on the world map interface (similar to Fallout 1 behavior)
* Added 5 `metarule3` macros for controlling the save slot with scripts to `sfall.h` in the **modders pack**
* Added a `sfall.dat` resource file, which contains the required files for various features. The localized `sfall_xx.dat` is also supported
* New script functions: `set_scr_name`, `obj_is_openable`
* Included Brazilian Portuguese and Polish translation (from Felipefpl and Jaiden)

## 3.8.30.2
* Fixed a bug introduced in 3.8.30 that caused a hang when opening the pipboy if the value of a quest location in `quests.txt` is less than 1500
* Fixed a bug introduced in 3.8.30 that caused a black screen after starting a new game without the hi-res patch
* Fixed a bug in `INVENTORYMOVE` hook that caused duplicate items when canceling moving an item into bag/backpack
* Updated French translation (from HawK-EyE)

## 3.8.30.1
* Merged Windows 2000 support into the standard version of sfall. Using **DX9** graphics modes now requires `d3dx9_42.dll` and `d3dcompiler_42.dll`
* Fixed a crash bug introduced in 3.8.30 with the fix for animation registration
* Fixed a bug in **AIDrugUsePerfFix** that could cause a hang in combat
* Fixed the extra check for friendly fire not working if the `area_attack_mode` parameter in the AI packet is not set or set to `no_pref`
* Added a fix for `chem_primary_desire` values in party member AI packets not being saved and reset correctly

## 3.8.30
* Fixed the original engine issues with being unable to register animations in certain situations in the game
* Fixed a crash bug introduced in 3.8.29 with the fix for the **'Leave'** event procedure in `AddRegionProc` function
* Fixed a bug in **ObjCanSeeObj_ShootThru_Fix** that could cause a hang in some cases
* Fixed the check of the ammo cost for a shot in **CheckWeaponAmmoCost**
* Fixed `set_critter_burst_disable` script function, which now applies only to weapons with the burst attack as the secondary mode
* Fixed the error handling in `create_object_sid` script function to prevent a crash when the proto is missing
* Fixed `METARULE_CURRENT_TOWN` metarule function not returning the correct index of the current town at the start of a new game
* Fixed the auto-close containers mod for gravesites and not closing containers in some cases (in the **modders pack**)
* Fixed and improved the functionality of **AllowLargeTiles**. Now it requires a `gridmask.frm` file (included in the **modders pack**) in the `art\tiles\` directory
* Restored the functionality of `obj_under_cursor` script function for the movement cursor (changed in 3.8.29)
* Improved the **hero appearance mod** to be able to load the interface text from `text\<language>\game\AppIface.msg`
* Improved the functionality of **TranslationsINI** to also search for the ini file relative to the `text\<language>\` directory
* Improved the fix for a duplicate `obj_dude` script being created when loading a saved game
* Rewrote the priority score calculation in **AIBestWeaponFix**, and changed the option to be enabled by default
* Changed **FastShotFix** to be disabled by default
* Changed the filename of **NPC armor appearance mod** to `gl_npcarmor_lite.int` (in the **modders pack**)
* Removed **DataLoadOrderPatch** from `ddraw.ini` because there is little reason to turn it off
* Added a fix for the script attached to an object not being initialized properly upon object creation
* Added a fix to prevent the player name from being displayed at the bottom of the dialog review window when the text is longer than one screen
* Added a fix for the in-game quest list not being in the same order as in `quests.txt`
* Added a fix for multihex critters hitting themselves when they miss an attack with ranged weapons
* Added a fix for the placement of multihex critters in the player's party when entering a map or elevation
* Added a fix to the starting position of the player's marker on the world map when starting a new game
* Added a fix for AI not checking the safety of weapons based on the selected attack mode
* Added a fix for the incorrect check and AP cost when AI reloads a weapon
* Added a fix to AI behavior to prevent the use of healing drugs when not necessary
* Added a fix for the incorrect object type search when loading a game saved in combat mode
* Added a few fixes for issues with knocked out/down critters. Now the combat doesn't automatically end if the target is only knocked out
* Added a tweak to prevent NPC aggression when non-hostile NPCs accidentally hit the player or members of the player's team
* Added a tweak to play the **'magic hands'** animation when using an item on an object. This also prevents a few issues with scripted animations not playing
* Added a tweak to remove the unspent skill points limit
* Added an option to disable the special handling of city areas 45 and 46 in the engine when visiting Area 45
* Added an option to load alternative dialog msg and subtitle files for female PC (translation friendly)
* Added an option to allow using the caret character `^` in dialog msg files to specify alternative text in dialogue based on the player's gender
* Added a new value to **AIDrugUsePerfFix** to allow NPCs to use only the drugs listed in `chem_primary_desire` and healing drugs
* Added support for loading premade character **GCD/BIO** files from the `premade\<language>\` directory for non-English languages
* Added support for loading fonts from the `fonts\<language>\` directory for non-English languages
* Added a debug option to control messages relating to engine fixes in the debug log
* Added a debug message about a missing combat object
* Added a new argument to `HOOK_CALCAPCOST` hook script
* New script function: `set_quest_failure_value`

## 3.8.29
* Fixed a crash bug introduced in 3.8.27 when editing global/map variables in the debug editor
* Fixed the critical hits from unaimed shots not matching the ones from aimed torso shots for some critter types
* Fixed the display issue in **DX9** mode when returning to the game after using `Alt+Tab` in dialogue
* Fixed `obj_under_cursor` script function, which now returns 0 if the cursor is in movement mode
* Improved compatibility with some older **DX9** graphics cards
* Changed `intface_redraw` script function with one argument to be able to redraw the specified interface window
* Changed the filename of the debug editor to `FalloutDebug.exe` (in the **modders pack**)
* Excluded **SFX** sounds from the search for alternative formats
* Removed the tweak that adds the city name in the description for empty save slots (added in 3.8.27)
* Removed the ability to change the poison level for NPCs from poison and `set_critter_stat` script functions (reverted to vanilla behavior)
* Added a fix to update the maximum HP stat of critters on the map when loaded for the first time
* Added a fix to the poison/radiation-related engine functions when taking control of an NPC
* Added a fix to AI weapon switching when not having enough AP to make an attack. Now AI will try to change attack mode before deciding to switch weapon
* Added a fix for the carry weight penalty of the **Small Frame** trait not being applied to bonus Strength points
* Added a fix for the flags of non-door objects being set/unset when using `obj_close/open` script functions
* Added a fix for the **'Leave'** event procedure in `AddRegionProc` function not being triggered when the cursor moves to a non-scripted window
* Added support for **ACM** files at 44.1 kHz sample rate
* Added stereo support for **SFX** and speech **ACM** files at 44.1 kHz sample rate
* Added support for panning **SFX** sounds and reduced the volume for objects located on a different elevation of the map
* Added more options for tweaking some engine perks to the **perks ini file**
* Added options for tweaking tag skills to the **skills ini file**
* Added an option about the behavior of maximum HP calculation to the **stats ini file**
* Added 3 new attribute type values to `get_window_attribute` script function
* Added additional universal opcodes `sfall_func7` and `sfall_func8` (`compile.exe` and `int2ssl.exe` in the **modders pack** are also updated)
* Added an auto-close containers mod and **NPC armor appearance mod** to the example mods in the **modders pack**
* Added `snd2acm_fix.exe` (snd2acm with a fix wrapper) to the **modders pack** for writing the correct sample rate and channel info from **WAV** files to **ACM** format
* Backported script functions from 4.0/4.1: `get_ini_sections`, `get_ini_section`, `add_extra_msg_file`
* Backported 4 new modes for `metarule2_explosions` function from 4.1
* Backported hook scripts from 4.2: `hs_adjustfid`, `hs_gamemodechange`
* Backported `SPECIAL` game mode flag from 4.2 (when switching from dialog mode to barter mode, or a party member joins/leaves in the dialog screen)
* New script functions: `interface_overlay`

## 3.8.28.1
* Fixed a few minor bugs introduced in 3.8.28
* Improved the functionality of **ExtraGameMsgFileList** to allow manually assigning numbers to specific msg files
* Added support for drawing **PCX** images to `draw_image`, `draw_image_scaled`, and `interface_art_draw` script functions
* New script function: `win_fill_color`
* Cleaned up `define_lite.h` and `command_lite.h` in the **modders pack**

## 3.8.28
* Fixed a bug introduced in 3.8.23 that could cause unexpected behavior in global scripts
* Fixed a bug in **ObjCanSeeObj_ShootThru_Fix** that caused the source to be unable to see the target if it has the **'ShootTrhu'** flag set
* Fixed the encounter messages still being limited to 50 entries per table when **EncounterTableSize** is set to greater than 50
* Fixed temporary arrays in scripts being cleared when flushing the keyboard buffer
* Fixed the broken `Print()` script function
* Improved the field of view check in **ObjCanSeeObj_ShootThru_Fix**
* Improved the functionality of **GlobalShaderFile** to be able to load multiple shader files
* Improved the performance of **DX9** graphics modes
* Extended the upper limit of `set_pickpocket_max` and `set_hit_chance_max` script functions to 999
* Expanded `get_window_attribute` script function to support the automap interface window
* Removed the check for valid objects from `get/set_object_data` script functions to make them work with other structured data
* Removed **InterfaceDontMoveOnTop** from `ddraw.ini` because there is little reason to turn it off
* Added a fix for the engine building the path to the central hex of a multihex object
* Added a fix for the flags of critters in the line of fire not being taken into account when calculating the hit chance penalty of ranged attacks
* Added a fix to the check for ranged weapons in the **Fast Shot** trait and **FastShotFix**
* Added a fix for the background image of the character portrait on the player's inventory screen
* Added the original Fallout 1 behavior of the **Fast Shot** trait to **FastShotFix**
* Added a tweak to keep the selected attack mode when moving the weapon between active item slots
* Added an option to enable linear texture filtering for **DX9** graphics modes
* Added support for **ACM** audio file playback and volume control to `soundplay` script function
* Added support for transparent interface/script windows
* Added the ability to change the poison level for NPCs to `poison` and `set_critter_stat` script functions
* Added a volume control to reduce the volume for `play_sfall_sound` script function
* Added an argument to `intface_redraw` script function to redraw all interface windows
* Added a new argument to `HOOK_TOHIT` hook script
* Added a new argument to `HOOK_COMBATDAMAGE` hook script
* Added a new hook type to `HOOK_WITHINPERCEPTION` hook script (when AI determines whether it sees a potential target)
* Added two sharpen filter files as global shader examples to the **modders pack**
* Backported script functions from 4.0: `car_gas_amount`, `set_car_intface_art`
* New script functions: `interface_art_draw`, `interface_print`, `combat_data`
* Updated the ammo ini loader mod in the **modders pack**

## 3.8.27
* Fixed the default values for **Movie1 - Movie17** options
* Fixed the playback of additional movies defined in **Movie18 - Movie32** options
* Fixed **OverrideMusicDir=2** not overriding the music path properly
* Fixed incorrect Melee Damage stat value being displayed when **BonusHtHDamageFix=1** and **DisplayBonusDamage=0**
* Fixed `attack_complex script` function not setting result flags correctly for the attacker and the target
* Fixed and improved **SFX** and speech playback for alternative sound files
* Fixed and improved the behavior of nested timer events in global scripts
* Improved the functionality of **AllowDShowMovies**: added volume control support and a new value to force **AVI** videos to fit the screen width, and fixed movie subtitles not showing up
* Improved the behavior of `HOOK_KEYPRESS` hook script
* Changed **AttackComplexFix** to make `attacker_results` and `target_results` arguments work independently of each other
* Changed **ObjCanSeeObj_ShootThru_Fix** to allow critters to see through other critters and added a check for the direction the source is facing
* Changed the behavior of replacing FRM aliases for critters. Now FRM files from their aliases are taken only if the critter doesn't have its own files
* Added a fix for `ANIM_charred_body`/`ANIM_charred_body_sf` animations not being available to most appearances
* Added a fix to remove floating text messages on the map when moving to another map or elevation
* Added a fix for a visual glitch on the black edges of the map when the map borders for the hi-res patch are set smaller than the screen size
* Added a fix to prevent the execution of `critter_p_proc` and game events when playing movies
* Added a fix to prevent crashes and loading maps when the death animation causes the player to cross an exit grid
* Added a fix to limit the maximum distance for the knockback animation to 20 hexes
* Added a tweak to allow setting custom colors from the game palette for object outlines
* Added a tweak to add the city name in the description for empty save slots
* Added an option to use Fallout's normal text font for death screen subtitles
* Added a debug option to force sfall to search for global scripts every time the game loads rather than only the first time
* Added a debug message about a corrupted proto file
* Added a function extension for vanilla `metarule3` function and added `set_horrigan_days` and `clear_keyboard_buffer` macros to sfall.h in the **modders pack**
* Added `DAM_PRESERVE_FLAGS` flag to `attack_complex` script function to keep the existing result flags when setting new flags
* Updated the example global shader file in the **modders pack**
* Updated German and Russian translations

## 3.8.26
* Fixed a **hero appearance mod** issue that caused the player's gender not to be reset properly when creating a new character
* Fixed a **hero appearance mod** issue that caused the player to lose some fire/electrical death animations
* Fixed a bug introduced in 3.8.23 that broke the **PlayIdleAnimOnReload** option
* Improved the pathfinding in the engine function when a multihex object is in the line of fire
* Improved the functionality of `display_stats` script function to also update player's stats on the character screen
* Improved the fix for incorrect positioning after exiting small/medium locations
* Added a fix to prevent critters from overlapping other object tiles when moving to the retargeted tile
* Added a fix to prevent showing an empty perk selection window (crash when clicking on the empty perk list)
* Added a fix for NPC stuck in an animation loop in combat when trying to move close to a multihex critter
* Increased the maximum text width of the player name on the character screen
* New script functions: `get_stat_max/min`

## 3.8.25.1
* Fixed the extra check for friendly fire treating non-critter objects as friendly critters
* Changed the debug message about a missing critter art file to be displayed in the game only when in sfall debugging mode

## 3.8.25
* Fixed a bug introduced in 3.8.19 that could cause the combat to end automatically in some cases
* Fixed a bug introduced in 3.8.20 that caused AI to miscalculate the hit chance in determining whether to use secondary attacks if **BodyHit_Torso** and **BodyHit_Torso_Uncalled** modifiers were not equal
* Fixed a bug introduced in 3.8.23 that caused **CorpseDeleteTime** not to set the timer correctly
* Fixed a bug introduced in 3.8.24 that caused **AllowDShowSound=2** not to work
* Fixed `loot_obj` script function not returning the correct object when switching to another corpse in the loot screen
* Fixed and improved the functionality of **ReloadWeaponKey** for using any non-weapon item
* Improved the fix for items on the ground being obscured by a pool of blood after the corpse is removed
* Added a fix for AI skipping a target simply because its weapon is currently empty
* Added a fix for AI not always considering the safe distance when using grenades or rockets
* Added a fix to AI behavior for **'Snipe'** distance preference (`distance=snipe` in `AI.txt`). Now the attacker will try to shoot the target instead of always running away from it at the beginning of the turn
* Added a fix to reduce friendly fire in burst attacks. Now there is an extra roll for AI to not use burst attacks if a friendly critter is in the line of fire
* Added a check for the weapon range and the AP cost when AI is choosing weapon attack modes
* Added a tweak to allow party members to keep their current target as one of the potential targets when choosing new targets at the beginning of their turn
* Added a tweak to the displayed message if the main target of a missed attack has the **'Flat'** flag set
* Added an option to skip loading game settings from saved games

## 3.8.24
* Fixed stuttering when playing **AVI** movies
* Fixed a crash when playing **MVE** movies in **DX9** mode with the CPU doing the palette conversion and without the hi-res patch
* Fixed a crash when using older versions of the hi-res patch
* Fixed a crash when the script attached to an object on the map is missing and there is another script using sfall arrays
* Fixed a possible crash or player's turn being skipped when returning to the game after using `Alt+Tab` in combat
* Fixed a bug introduced in 3.8.19 that could cause a hang when loading a saved game
* Fixed and improved the playback of alternative sound files
* Re-added **TownMapHotkeysFix** option to `ddraw.ini` for mod testing
* Changed `play/stop_sfall_sound` script functions to return/accept the **ID** number of the played sound instead of a raw pointer
* Changed the **'Radiated'** on the character screen to be highlighted in red when player's stats are affected by radiation exposure
* Code refactoring for various script functions
* Added a new **'fullscreen windowed'** mode to **DX9** graphics modes
* Added saving the position of the game window to `ddraw.ini`
* Added a fix for the player's money not being displayed in the dialog window after leaving the barter/combat control interface
* Added a fix for a crash or animation glitch of the critter in combat when an explosion from explosives and the AI attack animation are performed simultaneously
* Added a fix for the **'Fill_W'** flag in `worldmap.txt` not uncovering all tiles to the left edge of the world map
* Added a fix for leaving the map after reloading a saved game if the player died on the world map from radiation
* Added a fix to prevent the player from dying if a stat is less than 1 when removing radiation effects
* Added a fix for the same effect message being displayed when removing radiation effects
* Added a fix for NPCs not fully reloading a weapon if it has an ammo capacity larger than one box of ammo
* Added an option to display messages about radiation for the active geiger counter
* Added an option to change the displayed message when you recover from the negative effects of radiation exposure
* Added a new value to **AllowUnsafeScripting** to disable the memory address check in unsafe script functions
* Added a new value of 3 to the **'mark_state'** argument of `mark_area_known` script function to uncover locations without radius (Fallout 1 behavior)
* Added a new mode to `play_sfall_sound` script function

## 3.8.23
* Fixed the timing of setting `WORLDMAP`, `DIALOG`, `PIPBOY`, `INVENTORY`, `INTFACEUSE`, and `INTFACELOOT` game mode flags
* Fixed the execution of the timer event in global scripts
* Fixed the palette and the movie playback in **DX9** mode
* Improved the functionality of `create_message_window` script function to support the newline control character `\n`
* Removed **TownMapHotkeysFix** and **DisplaySecondWeaponRange** from `ddraw.ini` because there is little reason to turn them off
* Added a fix for duplicate critters being added to the list of potential targets for AI
* Added a fix for the playback of the speech sound file for the death screen being ended abruptly in some cases
* Added a fix for the barter button on the dialog window not animating until after leaving the barter screen
* Added a fix for the division operator treating negative integers as unsigned
* Added a fix for trying to loot corpses with the **'NoSteal'** flag set
* Added options to use more than one save slot for quick saving
* Added options to draw a dotted line while traveling on the world map (similar to Fallout 1, from Ghosthack)
* Added an option to display terrain types when hovering the cursor over the player's marker on the world map (from Ghosthack)
* Added a flashing icon to the Horrigan encounter and scripted force encounters by default
* Added new flags to `force_encounter_with_flags` script function
* Added a procedure and macros for comparing unsigned integers to `lib.math.h` in the **modders pack**
* Increased the maximum text width of the total weight display in the inventory
* New script functions: `string_to_case`, `set_terrain_name`, `get_window_attribute`, `set_town_title`, `message_box`, `div` operator (unsigned integer division)

## 3.8.22
* Fixed and improved the functionality of `substr` script function
* Restored and fixed **RemoveWindowRounding** option that was removed in 3.8.12
* Improved the functionality of `inventory_redraw` script function
* Changed the return value of `get_script` script function from -1 to 0 for unscripted objects
* Changed the debug message about a missing critter art file to also be displayed in the game
* Code refactoring for various script functions
* Added a fix to prevent the player from moving when clicking on a script-created window and prevent the mouse cursor from being toggled when hovering over a **'hidden'** window
* Added a fix to prevent crashes when **DebugMode** is enabled and there is a `%` character in the printed message
* Added an option to load a global shader file at game startup and added an example global shader file to the **modders pack**
* Added an option to highlight lootable corpses as well as items
* Added support for executing the `timed_event_p_proc` procedure in global scripts
* Backported script functions from 4.1: `draw_image`, `draw_image_scaled`
* New script functions: `add_g_timer_event`, `remove_timer_event`, `reg_anim_callback`, `get_sfall_arg_at`, `hide/show_window`, `set_window_flag`, `get_text_width`, `string_compare`, `string_format`, `objects_in_radius`, `tile_by_position`

## 3.8.21.1
* Fixed a crash bug introduced in 3.8.21 with the fix for corpses blocking line of fire

## 3.8.21
* Fixed a bug in `save_array` script function that could corrupt `sfallgv.sav` when saving a new array under the same key
* Fixed a crash bug in **PremadePaths** when a name exceeds 11 characters
* Fixed potential crashes in `list_next/end` script functions
* Fixed `move_obj_inven_to_obj/drop_obj` script functions not removing the equipped armor properly for the player and party members
* Fixed `inven_unwield` script function not updating the active item button on the interface bar for the player
* Fixed `art_change_fid_num` script function not setting player's FID correctly when the **hero appearance mod** is enabled
* Fixed `critter_add/rm_trait` script functions ignoring the **'amount'** argument. Note: pass negative amount values to `critter_rm_trait` to remove all ranks of the perk (vanilla behavior)
* Fixed the xp bonus set by `set_swiftlearner_mod` script function not being reset on game load
* Fixed the player name while controlling other critters
* Improved the display of the car fuel gauge on the world map interface
* Improved the **hero appearance mod** to search for files in **.dat** files and folders simultaneously
* Improved `HOOK_INVENWIELD` hook script to run for the player and NPCs when removing equipped items, and added a new argument to it
* Expanded `set_critter_stat` script function to allow changing the base stats of `STAT_unused` and `STAT_dmg_*` for the player, and `STAT_unused` for other critters
* Changed **AllowUnsafeScripting** to not require sfall debugging mode
* Removed **NPCStage6Fix** and **CorpseLineOfFireFix** from `ddraw.ini` because there is little reason to turn them off
* Added a fix to prevent the player from equipping a weapon when the current appearance has no animation for it
* Added a fix to use **'English'** as the fallback language directory for loading msg files
* Added a fix for party member's equipped weapon being placed in the incorrect item slot after leveling up
* Added a new value to **AIBestWeaponFix** to change the priority multiplier for having weapon perk to 3x (the original is 5x)
* Added a new flag to **MainMenuFontColour** to only change the font color of the Fallout/sfall version text
* Added an option to set the number of additional notification boxes to the interface
* Added optional options to enable modification sections for perks and traits to the **perks ini file**
* Added support for displaying AP cost up to 19 on the active item button on the interface bar. This requires an extended `MVENUM.frm` file (included in the **modders pack**) in the `art\intrface\` directory
* Added a check for valid objects to `get/set_object_data` script functions
* Added a debug message about a missing critter art file to **DebugMode**
* The number of simultaneously displayed notification boxes now depends on the game resolution
* Backported script functions from 4.0/4.1: `set_unjam_locks_time`, `set_iface_tag_text`, `add_iface_tag`
* New script functions: `unwield_slot`, `add_trait`, `get_inven_ap_cost`

## 3.8.20
* Backported `HEROWIN` and `DIALOGVIEW` (when reviewing the current conversation) game mode flags from 4.1
* Fixed `create_message_window` script function to prevent it from creating multiple message windows
* Fixed `obj_art_fid` script function returning incorrect player's FID when the **hero appearance mod** is enabled
* Fixed a crash bug in `message_str_game` script function when passing a negative fileId value
* Fixed **MainMenuFontColour** not changing the font color of the copyright text on the main menu
* Fixed some arguments in `BARTERPRICE` hook when trading with a party member
* Fixed a crash bug introduced in 3.8.18 when calling `game_time_advance` in the `map_exit_p_proc` procedure with active explosives on the map
* Fixed some of sfall features that depend on the hi-res patch not working properly in some cases
* Fixed a bug introduced in 3.8.18 that broke the **AllowLargeTiles** option
* Changed the `DAM_BACKWASH` flag to be set for the attacker before calculating combat damage when taking self-damage from explosions
* Changed **CorpseLineOfFireFix** to be enabled by default
* Removed the dependency of **Body_Torso** from **Body_Uncalled** hit modifier, and re-added **BodyHit_Torso** to `ddraw.ini`. Now you can use `set_bodypart_hit_modifier` script function to set them individually
* Replaced the **'Take All'** hotkey mod with an extended UI hotkeys mod in the **modders pack**
* Added a fix to prevent the car from being lost when entering a location via the Town/World button and then leaving on foot
* Added a fix for items on the ground being obscured by a pool of blood after the corpse is removed
* Added a fix for player's position if the entrance tile is blocked when entering a map
* Added a fix for the player stuck at **'climbing'** frame after ladder climbing animation
* Added an option to change the path and filename of the critical table file
* Added an option to change the font color of the button text on the main menu
* Added an option to change some of Fallout 2 engine functions to Fallout 1 behavior
* Added support for the new **'Energy Weapon'** flag to weapon protos. This flag forces the weapon to use Energy Weapons skill regardless of its damage type
* Added options for tweaking some engine perks to the **perks ini file**
* Added a new flag to `force_encounter_with_flags` script function
* Added `COUNTERWIN` flag to the game mode functions (when moving multiple items or setting a timer)
* Added a new argument to `HOOK_BARTERPRICE` hook script
* Added a compute damage example script to the **modders pack**
* Slightly increased the width of the car fuel gauge on the world map interface
* New script function: `register_hook_proc_spec`

## 3.8.19.1
* Fixed the error handling for loading `sfallgv.sav` to improve backward compatibility with older saved games
* Changed **NPC combat control** to keep player's **Awareness** perk while controlling other critters
* Changed **DebugMode** and **HideObjIsNullMsg** to not require sfall debugging mode

## 3.8.19
* Fixed some scrolling bugs in **WorldMapSlots** option
* Fixed fade in/out of the screen in **DX9** mode (partially)
* Fixed the last procedure in a script being unable to be called through a variable containing its name as a string
* Fixed a bug introduced in 3.8.18 that caused the `DAM_KNOCKED_DOWN` flag not to be reset for knocked out party members when leaving a map
* Improved the functionality of the debug editor (in the **modders pack**)
* Improved the fix for **'NPC turns into a container'** bug
* Changed **DataLoadOrderPatch** to be enabled by default
* Changed **ItemCounterDefaultMax** to not set the counter to maximum when in the barter screen
* Added a fix for the broken **'reserve movement'** function
* Added a fix for the up/down button images on the world map interface
* Added a fix for the position of the destination marker for small/medium location circles when using the location list
* Added a fix for player's movement in combat being interrupted when trying to use objects with **Bonus Move** APs available
* Added a fix for the incorrect coordinates for small/medium location circles that the engine uses to highlight their sub-tiles
* Added a fix for visited tiles on the world map being darkened again when a location is added next to them
* Added a fix for **Scout** perk being taken into account when setting the visibility of locations with `mark_area_known` script function
* Added a fix for combat not ending automatically when there are no hostile critters
* Added a fix for critters/items on a map having duplicate object IDs
* Added a fix for knocked down critters not playing stand up animation when the combat ends
* Added a fix for dead NPCs reloading their weapons when the combat ends
* Added an option to change the base value of the duration of the knockout effect
* Added a check for the `DAM_KNOCKED_OUT` flag to `wield_obj_critter/inven_unwield` script functions
* Added a new value to **SkipOpeningMovies** to also skip the splash screen
* New script function: `metarule_exist`

## 3.8.18.1
* Fixed a bug introduced in 3.8.14 that could crash the game when calling knockback modifier functions

## 3.8.18
* Fixed broken `get/mod_kill_counter` script functions when **ExtraKillTypes** is enabled
* Fixed the argument value of `dialogue_reaction` script function
* Fixed getting perks and traits from the real **dude_obj** while controlling other critters
* Improved the functionality of `HOOK_INVENWIELD` hook script
* Improved the fix for the removal of party member's corpse to prevent save file corruption. Now party member's corpse is removed in the same way as all other critter corpses
* Changed the engine functions for saving party member protos and removing the drug effects for NPC to be called after executing the `map_exit_p_proc` procedure
* Changed `create_message_window` script function to be available when other game interfaces are opened
* Changed how `HOOK_FINDTARGET` hook script handles its return values. Now you don't have to specify all 4 targets to override normal sorting
* Removed **DialogOptions9Lines** from `ddraw.ini` because there is little reason to turn it off
* Removed **LoadProtoMaxLimit** from `ddraw.ini`. Now the proto limit is automatically increased when needed
* Added a fix for party member's stats being reset to the base level when their base protos with the read-only attribute set are placed in the **proto\critters\\** directory
* Added a fix for NPC stuck in a loop of picking up items in combat and the incorrect message being displayed when the NPC cannot pick up an item due to not enough space in the inventory
* Added a fix to allow fleeing NPC to use drugs
* Added a fix for AI not checking minimum HP properly for using stimpaks
* Added a fix for NPC stuck in fleeing mode when the hit chance of a target was too low
* Added a missing option for testing **AllowSoundForFloats** to the **[Debugging]** section of `ddraw.ini`
* Added a new return value to `HOOK_BARTERPRICE` hook script to modify the value of player's goods
* Added a new argument to `HOOK_WITHINPERCEPTION` hook script
* Added 4 new events to `HOOK_INVENTORYMOVE` hook script
* Added an ammo ini loader mod and a lite version of **item highlighting mod** to the example mods in the **modders pack**
* Slightly increased the maximum text width of the information card on the character screen
* Backported script functions from 4.0/4.1: `real_dude_obj`, `dialog_obj`, `loot_obj`, `npc_engine_level_up`

## 3.8.17
* Backported the improved `HOOK_AMMOCOST` hook script from 4.1 to fix the issues with **CheckWeaponAmmoCost**
* Fixed the argument numbering in error messages when validating arguments
* Fixed a bug in **CheckWeaponAmmoCost** that caused NPCs not to switch to other weapons when there is not enough ammo to shoot
* Fixed the position of the 32-bit talking heads when the game resolution is higher than 640x480
* Changed `hero_select_win` function to require an `AppHeroWin.frm` file (included in the **modders pack**) in the **art\intrface\\** directory
* Added a fix for unexplored areas being revealed on the automap when entering a map
* Added a fix for the overflow of the automap tables when the number of maps in `maps.txt` is more than 160
* Added a fix for a duplicate `obj_dude` script being created when loading a saved game
* Added a fix to prevent the reserved object IDs of the player and party members from being generated for other objects
* Added a fix for the display issue in the pipboy when the automap list is too long
* Added a fix for the `start` procedure not being called correctly if the required standard script procedure is missing (from Crafty)
* Added an option to disable the special handling of map IDs 19 and 37 in the engine when entering the maps
* Added support for the new **'automap=yes/no'** parameter to `maps.txt`. This parameter overrides the hardcoded values for displaying the map in the pipboy automaps for the first 160 maps
* Added files for using 32-bit images for talking heads to the **modders pack**
* Improved the functionality of the debug editor (in the **modders pack**)
* Improved the functionality of **Use32BitHeadGraphics** to allow using 32-bit images without having to patch talking head FRM files
* New script function: `set_unique_id`

## 3.8.16
* Fixed a crash bug introduced in 3.8.15 when using various inventory items while a **books ini file** is loaded
* Fixed the return value of `has_skill` script function for incorrect skill numbers
* Fixed the negative skill points of a skill not being taken into account when calculating the skill level
* Fixed incorrect skill point cost for negative skill levels when using a **skills ini file**
* Fixed the screen not returning to the player when moving a controlled critter to another map elevation
* Fixed some functionality issues of fake perks
* Fixed the broken `get_perk_available` script function
* Expanded `get/inc_npc_level` script functions to accept party member PIDs
* Removed **MultiPatches** from `ddraw.ini`. Now Fallout always loads multiple patch files at once
* Added a fix for the reserved item FRM being displayed in the top-left corner when in the loot/barter screens
* Added a fix for the active effects of drugs not being saved properly
* Added a fix for NPC stuck in a loop of reloading melee/unarmed weapons when out of ammo
* Added a fix for critters not being healed over time when entering a map with **'dead_bodies_age=No'** set in `maps.txt`
* Added a fix for corpses being removed early after returning to the map
* Added a fix for the removal of party member's corpse. Now items in party member's inventory are not removed along with the corpse
* Added an option to change the timer for deleting corpses on a map after you leave
* Added an option to override the global variable number used to show the special death message of the Modoc toilet explosion
* Added a new argument to `HOOK_REMOVEINVENOBJ` hook script

## 3.8.15
Various bug fixes and features based on the work by Mr.Stalin:
* Fixed a hang on startup if there is an invalid character for SPECIAL stats in the `skills ini` file
* Fixed `set_self` function for `use_obj_on_obj`, `attack`, and `attack_complex` vanilla functions
* Fixed `attack_complex` script function still causing minimum damage to the target when the attacker misses
* Fixed `critter_mod_skill` script function taking a negative amount value as a positive
* Fixed a crash when calling `use_obj/use_obj_on_obj` without using `set_self` in global scripts
* Fixed `pickup_obj`, `drop_obj`, and `use_obj` script functions not working properly in some cases
* Fixed **TimeLimit=-3** not returning the correct year, and removed the setting value of -2 (Now it works the same as -3)
* Fixed the mouse cursor lag on the world map when **WorldMapFPSPatch** is enabled
* Fixed issues with the game speed tweak. Now the game speed will be temporarily set to normal speed when in the inventory or dialogue, and it doesn't affect the endgame slideshow
* Fixed and improved the functionality of **UseFileSystemOverride** and `fs_*` script functions
* Improved the functionality of `get/set_sfall_global` script functions to print error messages to debug output if the name of sfall global variable is not 8 characters long
* Improved the error handling for saving/loading sfall data files in savegames
* Expanded `abs` math script function to support returning integers
* Added a fix for critters not attacking the player in combat when loading a game saved in combat mode
* Added a fix for player's turn being skipped when loading a game saved in combat mode
* Added an option to fix and repurpose the unused **called_shot/num_attacks** arguments of `attack_complex` script function
* Added an option to make the game speed tweak also affect the playback speed of **MVE** video files without an audio track
* Added a debug option to hide error messages in debug output when a null value is passed to the function as an object
* Increased the maximum number of books in **BooksFile** to 50
* New script function: `art_cache_clear`

## 3.8.14.1
* Fixed a bug introduced in 3.8.14 that broke the calculation of the skill point cost for increasing skill levels (from Mr.Stalin)

## 3.8.14
Various bug fixes and features based on the work by Mr.Stalin:
* Fixed a crash bug when using sorting functions on an associative array
* Improved the functionality of **ElevatorsFile** to allow changing the FRM images of the elevator panel and creating new elevator types
* Expanded `resize_array` function to support sorting associative arrays by keys or values
* Expanded `create/temp_array` functions to allow creating a new **'lookup'** type of associative array
* Changed `INVENTORYMOVE` hook to be called before displaying the **'Move Items'** window for dragging and dropping ammo on weapons
* Added a fix to prevent sfall from trying to load global scripts with an extension that exceeds three characters (e.g. gl_test.int123)
* Added a fix to the following script functions to ensure they set the correct object: `set_critter_burst_disable`, `set_critter_pickpocket_mod`, `set_critter_skill_mod`, `set_critter_hit_chance_mod`, `set_*_knockback`
* Added an option to change the distance at which the player will switch to walking when trying to use objects or pick up items

## 3.8.13
* Changed **PartyMemberExtraInfo** to not show **'Addict'** text (in dark green) on the combat control panel if the party member is not addicted to drugs
* Changed `read_byte`, `read_short`, `read_int`, and `read_string` script functions to not require **AllowUnsafeScripting**
* Added an option to display experience points with the bonus from **Swift Learner** perk when gained from non-scripted situations (from Crafty)

Various bug fixes and features based on the work by Mr.Stalin:
* Fixed the missing return value of -1 for `HOOK_USEOBJON` hook script
* Fixed the return values of a hook getting corrupted if another hook was called during the execution of the current hook
* Fixed a bug in the **hero appearance mod** that caused the player to disappear after saving the game when player's base FID is greater than 255
* Added a fix for missing AC/DR mod stats when examining ammo in the barter screen
* Added a fix for being unable to sell/give items in the barter screen when the player/party member is overloaded
* Added a fix for AI still taking distance into account when calculating hit chance using the **'no_range'** flag
* Added a fix for AI not taking `chem_primary_desire` in `AI.txt` as a preference list when using drugs in the inventory
* Added a fix to display a pop-up message box about **death from radiation**
* Added a fix to prevent hook scripts from being executed when the depth limit is exceeded
* Added an option to display full item description for weapon/ammo in the barter screen
* Added a new value to **SpeedInterfaceCounterAnims** to update the AC counter instantly when switching to other controlled critters in combat
* Backported script functions from 4.0: `lock_is_jammed`, `unjam_lock`
* New script functions: `obj_under_cursor`, `get/set_object_data`

## 3.8.12
* Changed the debug editor to require sfall debugging mode
* Removed **RemoveWindowRounding** option because it doesn't affect anything

Original engine bug fixes and various features based on the work by Crafty:
* Added a fix for the encounter description being displayed in two lines instead of one
* Added a fix for the maximum text width of the player name in the inventory
* Added a fix for the **'mood'** argument of `start_gdialog` script function being ignored for talking heads
* Added a fix for **Heave Ho!** perk increasing Strength stat above 10 when determining the maximum range of thrown weapons
* Added an option to display party member's current level/AC/addict flag on the combat control panel
* Added a new value to **DebugMode** to send debug output to both the screen and `debug.log`
* Added a new return value to `HOOK_KEYPRESS` hook script to override the pressed key
* Added `INTFACEUSE`, `INTFACELOOT`, and `BARTER` flags to the game mode functions

Various bug fixes and features based on the work by Mr.Stalin:
* Fixed broken `get/reset_critical_table` script functions
* Fixed **DX9** mode not showing movie subtitles properly when not using the hi-res patch
* Fixed **DisplayBonusDamage** not being applied to Melee Damage stat on the character screen when **BonusHtHDamageFix** is enabled
* Improved the functionality of **CritterInvSizeLimitMode** and added party member's current/max inventory size info to the combat control panel
* Improved the functionality of **AllowDShowSound**: added volume control support and the ability to play alternative music files even if original **ACM** files are not present in the music folder, and fixed initialization crash bug when **DX9** mode is disabled
* Improved the functionality of **ExtraSaveSlots**: added sound effect when clicking on the navigation buttons
* Improved the functionality of **StartGDialogFix** to fix a crash when calling `start_gdialog` outside the `talk_p_proc` procedure for talking heads
* Improved and expanded the functionality of **UseScrollingQuestsList** to display page numbers and add another set of scroll buttons
* Expanded `is_iface_tag_active` script function to check tag values of 0/1/2 (sneak/poisoned/radiated)
* Added a fix for the broken `obj_can_hear_obj` script function
* Added a fix for the underline position in the inventory display window when the item name is longer than one line
* Added a fix for AI being unable to use the picked up object immediately when there is a different object with the same ID
* Added a fix for the exploit that allows you to gain excessive skill points from **Tag!** perk before leaving the character screen
* Added an option to prevent the inventory/loot/automap interfaces from being placed on top of other script-created windows
* Added an option to change the limit of how many protos per type can be loaded into memory at once, and improved the functionality of `set_proto_data` script function to be able to automatically increase the limit when needed
* Added an option to skip the **'Move Items'** window when taking items from containers or corpses and not holding down **ItemFastMoveKey**
* Added a new mode to `metarule2_explosions` function
* Backported script functions from 4.0/4.1: `item_weight`, `get/set_outline`, `get/set_flags`, `tile_refresh_display`, `outlined_object`, `get/set_cursor_mode`, `display_stats`, `get/set_map_enter_position`, `attack_is_aimed`, `inventory_redraw`, `get_current_inven_size`, `create_win`

## 3.8.11
Various bug fixes based on the work by Mr.Stalin:
* Fixed an issue where the file IDs of additional game msg files were shifted when a file in **ExtraGameMsgFileList** was missing
* Fixed `obj_can_see_obj` script function not checking if source and target objects are on the same elevation before calling `HOOK_WITHINPERCEPTION` hook script
* Added a fix for the display issue in the pipboy when a quest list is too long with **UseScrollingQuestsList** diabled
* Added a fix for the clickability issue of the holodisk list in the pipboy
* Added a fix for multihex critters moving too close and overlapping their targets in combat
* Added a fix for AI not checking weapon perks properly when choosing the best weapon

## 3.8.10
* Added an option to use Fallout's normal text font instead of DOS-like font on the world map
* Added an option to increase the maximum number of tile FRMs (from Crafty)

Original engine bug fixes and various features based on the work by Mr.Stalin:
* Added a fix for a crash when the critter goes through a door with animation triggers
* Added a fix for critters killed in combat by scripting still being able to move in their combat turn if the `distance` parameter in their AI packages is set to `stay_close/charge`, or **NPCsTryToSpendExtraAP** is enabled
* Added support for adding custom background FRM to the character screen of the **hero appearance mod**
* Added an option to display the range of the secondary attack mode in the inventory when you switch weapon modes in active item slots
* Added an option to set up a key to let you move/drop a whole stack of items at once without the **'Move Items'** window
* Added an option to change the counter in the **'Move Items'** window to start with maximum number
* Expanded `get_mouse_buttons` function to return a value for the middle mouse button

## 3.8.9
* Fixed a broken functionality of **ExtraSaveSlots option**. Now sfall will remember the last selected save game slot. The position data is saved to/loaded from an auto-generated `slotdat.ini` in your savegame folder
* Fixed the last additional notification boxes to the interface being missing
* Fixed the broken `get_attack_type` script function (from Mr.Stalin and Crafty)
* Added a fix for being at incorrect hex after map change when the exit hex in source map is at the same position as some exit hex in destination map (from Crafty)
* Added a math script function: `floor2`
* New script function: `set_ini_setting` (from Mr.Stalin)

## 3.8.8
* Fixed `add_mult_objs_to_inven` script function adding only 500 instances of an object when the value of the **'count'** argument is over 99999
* Improved the fix for player's base EMP DR to make sure the value is set correctly

## 3.8.7
* Fixed `sneak_success` script function not checking if the player is currently sneaking
* Added a fix for player's base EMP DR not being properly initialized when creating a new character and then starting the game
* Improved the functionality of **UseScrollWheel**. Now you can scroll through items in the loot/barter screens, and text in the message window (from Crafty)

## 3.8.6
* Fixed a crash bug in **NPC combat control** when trying to control a temporary party member that has no data in `party.txt`

## 3.8.5
* Fixed an issue where the game was rendered before the **hero appearance mod** was loaded
* Included Chinese and Russian translations

Original engine bug fixes and various features based on the work by Crafty:
* Added a fix for the exploit that **Bonus Move** APs are replenished when you save and load the game in combat
* Added a fix for the displayed message when the attack randomly hits a target that is not a critter and has a script attached
* Added a fix for `damage_p_proc` being called for misses if the target is not a critter
* Added a fix for the double damage effect of **Silent Death** perk not being applied to critical hits
* Implemented standard script procedures: `combat_is_starting_p_proc` (called when a combat starts, but doesn't mean that the critter is in combat) and `combat_is_over_p_proc` (called when a combat ends)
* Added a new argument to `HOOK_COMBATDAMAGE` hook script
* Added an option to prevent the player from running while sneaking without **Silent Running** perk

## 3.8.4
* Fixed a crash introduced in 3.8.3 when calling `destroy_object` or `destroy_mult_objs`
* Fixed a **hero appearance mod** issue that caused the race and style not to be loaded properly from savegames
* Added an option to set the color of outlines for highlighted items and containers

## 3.8.3
* Fixed a crash when pressing **reload weapon key** while in the main menu
* Fixed `metarule2_explosions` not being reset properly
* Fixed global scripts not running on the world map when disabling the world map speed patch
* Fixed inconsistent behavior of motion sensor flag 2
* Added 3 new arguments to `HOOK_TOHIT` hook script
* Added a fix for the bag/backpack exploit that lets you keep items that are supposed to be removed from the inventory
* Added a fix for the original engine issue that caused Sequence stat value not to be printed correctly when using **'print to file'** option
* Improved the functionality of **ScrollMod**
* Improved the functionality of **ExplosionsEmitLight**. Now it will check if an item was lit prior to being thrown/shot
* Changed **SkipSizeCheck** and **ExtraCRC** to not require sfall debugging mode
* Removed the obsolete **WorldMapFPS**, **ForceLowResolutionTimer**, and **WorldMapDelay** options

Original engine bug fixes and various features based on the work by Crafty:
* Added a fix for a crash when clicking on empty space in the inventory list opened by **'Use Inventory Item On'** (backpack) action icon
* Added a fix for negative SPECIAL values in the character creation
* Added a fix for the game hanging in an endless loop in combat mode when calling `anim` script functions inside `damage_p_proc`
* Added 3 new arguments to `HOOK_BARTERPRICE` hook script

## 3.8.2
* Fixed broken `call_offset_*` script functions
* Fixed **OverrideMusicDir** not using the correct path string
* Fixed a bug in `metarule2_explosions` function that caused damage type change not to work
* Fixed a crash when calling `reg_anim_obj_run_to_tile` after `reg_anim_combat_check`
* Changed `sfallgv.sav` to be loaded before other save game files to make saved arrays available in the start procedure
* Changed **BodyHit_Torso** to **BodyHit_Torso_Uncalled** because it sets both *body_torso* and *body_uncalled* hit modifiers

Original engine bug fixes and various features based on the work by Crafty:
* Added a fix for display issues when calling `gdialog_mod_barter` with critters with no **'Barter'** flag set
* Added a fix for the original engine issue that caused items to disappear from the inventory when you try to drag them to bag/backpack in the inventory list and are overloaded
* Added a fix for the original engine issue that caused the game not to check player's inventory properly when putting items into the bag/backpack in the hands
* Added a fix for the original engine issue that caused the current carry weight and the range of equipped weapons being lowered temporarily when opening bag/backpack
* Added a fix for a crash when trying to open bag/backpack on the table in the bartering interface
* Added the ability to move items from bag/backpack to the main inventory list by dragging them on the character portrait (similar to Fallout 1 behavior)
* Changed **WorldMapEncounterFix/Rate** to work independently of **WorldMapFPSPatch**

## 3.8.1
* Fixed pressing `F6` not displaying the **Quick Save** screen

## 3.8
* Improved **NPC combat control** and fixed various issues with it
* Fixed bugs in `set*_stat_min` script functions that set max values instead of min
* Unified the style of global/hook script log entries
* Added new universal opcodes **sfall_funcX** that allow adding new script functions without changing script compiler and decompiler
* Added new script functions: `spatial_radius`, `critter_inven_obj2`, `intface_redraw`, `intface_hide`, `intface_show`, `intface_is_hidden`, `exec_map_update_scripts`

Original engine bug fixes and various features based on the work by Crafty:
* sfall can now load global/hook scripts from **.dat** files
* Improved the **'print to file'** fix and added the ability to use long filenames in **.dat** files
* Improved the functionality of **ProcessorIdle**

## 3.7.4
* Improved the functionality of **DisableHorrigan** to work on old saved games
* Replaced **NumberPatchLoop** with a simpler **MultiPatches** toggle
* Added a new value to **PipBoyAvailableAtGameStart** to make the pipboy available by only skipping the vault suit movie check
* Changed **BodypartHitModX** to **BodyHit_(body parts)** in `ddraw.ini` to make them easier to understand
* Removed **BodyHit_Uncalled** from `ddraw.ini`. Now *body_torso* and *body_uncalled* hit modifiers share the same value, and `set_bodypart_hit_modifier` script function is also tweaked to match the change
* Removed **CarryWeightLimit** option because it can be scripted with `set_stat_max` script function
* Removed **GainStatPerkFix** from `ddraw.ini` because there is little reason to turn it off

Original engine bug fixes and various features based on the work by Crafty:
* sfall now loads global/hook scripts, shaders, 32-bit talking head images, and **AVI** movies from **master_patches** path in `fallout2.cfg` instead of the fixed **'data\\'** directory
* Added a fix for the original engine issue that caused incorrect positioning after exiting small locations (e.g. Ghost Farm)
* Added an option to use a modified data load order for the engine to find game data

## 3.7.3
* Changed the displayed version number to the same as the internal version number
* Added the ability to read additional game msg files, as opposed to dialog msg files (from Vennor)
* Added an option to display sfall built-in credits at the bottom of `credits.txt` contents instead of at the top

Original engine bug fixes and various features based on the work by Crafty:
* Fixed `get_screen_width/height` script functions not returning correct values when using the hi-res patch
* Fixed incorrect savegame path detection when reading/writing `sfallgv.sav` and `sfallfs.sav` under certain circumstances
* Improved the unlimited ammo exploit fix to prevent crashes with some faulty scripting
* Added a fix for the original engine issue that caused action points to be initialized incorrectly at the beginning of each turn, thus giving a hidden bonus to your opponent's armor class and reducing your hit chance
* Added a fix for `destroy_p_proc` not being called if the critter is killed by explosives when you leave the map
* Added a fix for incorrect death animations being used when killing critters with `kill_critter_type` script function
* Added a fix for the original engine issue that caused Fallout to check the horizontal position on the y-axis instead of x when setting coordinates on the world map
* Changed the way **SpeedInterfaceCounterAnims=2** works. Now it updates the HP/AC counters instantly in all cases
* Added an option to display numbered dialogue options

## 3.7b
* Fixed broken `sfall_ver_*` script functions
* Fixed potential undefined behavior and crashes in sfall arrays (from Vennor)
* Optimized some code to make the compiled DLLs about 10 KB smaller in size
* Switched to using the precomputed CRC table instead of creating the CRC table at runtime. This could potentially improve startup time in emulated or partially emulated environments (from Oppen)
* Added rounding calculation to **ComputeSprayMod** for a more balanced bullet distribution in burst attacks
* Re-added **CarChargingFix** option to `ddraw.ini` for mods that have custom vehicles
* Removed **MultiHexPathingFix** from `ddraw.ini` because there is little reason to turn it off

Original engine bug fixes and various features based on the work by Crafty:
* Fixed a crash bug introduced with the inventory drag and drop fix
* Added a new value to **SpeedInterfaceCounterAnims** to update the HP/AC counters instantly when the number is not negative
* Added an option to skip weapon equip/unequip animations when performing various actions
* Added an option to control the speed of pipboy alarm clock animations
* Added an option to change the carry weight limit

## 3.7a
* Partially refactored all the source code to use proper engine function/variable names
* Removed a few options from `ddraw.ini` that should never be turned off

Original engine bug fixes and various features based on the work by Crafty:
* Added a fix for being unable to sell used geiger counters or stealth boys
* Added a fix for not counting in the weight of equipped items on NPC when stealing or bartering
* Added a fix for only using one box of ammo when reloading a weapon above the ammo in the inventory list by drag and drop
* Added an option to remove the ability to enter unvisited areas on a town map by hitting number keys
* Added an option to leave the music playing in dialogue with talking heads
* Added an option to skip the **'Move Items'** window when using drap and drop to reload weapons in the inventory

## 3.7
* Fixed incorrect bonuses on AC and Melee Damage from the **Heavy Handed** trait when using `perks.ini`
* New hook script: `hs_invenwield` (when causing a critter to wield/unwield an armor or a weapon)
* Expanded `message_str_game` script function to support all `pro_*.msg` files as well
* **ExtraCRC** can now accept multiple CRC values
* Merged sfall debugging features into the normal version, thus removing the need of a separate debugging version for modders
* Removed the obsolete Jim's damage formula
* Removed a few options from `ddraw.ini` that should never be turned off

Original engine bug fixes and various features based on the work by Crafty:
* Fixed **SpeedInterfaceCounterAnims** not displaying the correct negative value when HP drops below zero
* Rewrote the **Sharpshooter** perk fix to match its description correctly
* Rewrote the dodgy door fix to apply to both melee and ranged attacks
* Added a fix for a crash when leaving the map while waiting for someone to die of a super stimpak overdose
* Added a fix for the original engine issue that caused party members to have incorrect stats when they level up while on drugs
* Added a fix for the unlimited ammo exploit
* Added a fix for negative values in Skilldex window
* Added a fix for the clickability issue in the pipboy and an exploit that allows resting in restricted areas
* Added a fix to prevent **'Too Many Items'** bug from corrupting save files
* Added a fix for the exploit that you can gain stats from more than two doses of a specific chem after save/load
* Added a fix for the original engine issues with reverse order of items in memory relative to visual order in the inventory list
* Added a fix for the original engine issue that caused party members to be able to unequip multiple of the same armor and reduce their stats to below the proper values
* Added a fix for the original engine issues that caused the game not to check NPC's addictions properly and the Jet Antidote not to work on NPCs
* Added a fix for the maximum text width of the item weight (Wt.) in party member trading window
* Added a fix for the original engine issue that caused NPCs to become unresponsive and act like walking containers if you move to another map while they are under **'lost next turn'** critical miss effect
* Added a fix for the original engine issues with being able to charge the car by using cells on other scenery/critters, and cells getting consumed even when the car is already fully charged
* Added an option to stack empty identical weapons, no matter what type of ammo was loaded previously
* Added an option to highlight containers as well as items
* Added an option to allow 9 options (lines of text) to be displayed correctly in the dialog window
* Added an option to display additional points of damage from **Bonus HtH/Ranged Damage** perks in the inventory

__For changelogs prior to version 3.7, please refer to the `sfall-readme.txt` in the [release files](https://github.com/sfall-team/sfall/releases).__
