/*
	* sfall
	* Copyright (C) 2008-2016 The sfall team
	*
	* This program is free software: you can redistribute it and/or modify
	* it under the terms of the GNU General Public License as published by
	* the Free Software Foundation, either version 3 of the License, or
	* (at your option) any later version.
	*
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	* GNU General Public License for more details.
	*
	* You should have received a copy of the GNU General Public License
	* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

//
// Everything related to new sfall opcodes.
//


// TODO: move global-script related code into separate file
static void _stdcall SetGlobalScriptRepeat2(TProgram* script, int frames) {
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (globalScripts[d].prog.ptr == script) {
			if (frames == -1) {
				globalScripts[d].mode = !globalScripts[d].mode;
			} else {
				globalScripts[d].repeat = frames;
			}
			break;
		}
	}
}

static void __declspec(naked) op_set_global_script_repeat() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		push ecx;
		call SetGlobalScriptRepeat2;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void _stdcall SetGlobalScriptType2(TProgram* script, int type) {
	if (type <= 3) {
		for (size_t d = 0; d < globalScripts.size(); d++) {
			if (globalScripts[d].prog.ptr == script) {
				globalScripts[d].mode = type;
				break;
			}
		}
	}
}

static void __declspec(naked) op_set_global_script_type() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		push ecx;
		call SetGlobalScriptType2;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_available_global_script_types() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov edx, AvailableGlobalScriptTypes;
		mov ecx, eax;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void SetGlobalVarInternal(__int64 var, int val) {
	glob_itr itr = globalVars.find(var);
	if (itr == globalVars.end()) {
		globalVars.insert(glob_pair(var, val));
	} else {
		if (val == 0) {
			globalVars.erase(itr);    // applies for both float 0.0 and integer 0
		} else {
			itr->second = val;
		}
	}
}

static void _stdcall SetGlobalVar2(const char* var, int val) {
	if (strlen(var) != 8) {
		return;
	}
	SetGlobalVarInternal(*(__int64*)var, val);
}

static void _stdcall SetGlobalVar2Int(DWORD var, int val) {
	SetGlobalVarInternal(var, val);
}

static void __declspec(naked) op_set_sfall_global() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jz next;
		cmp dx, 0xc001;
		jnz end;
		push esi;
		push eax;
		call SetGlobalVar2Int;
		jmp end;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push esi;
		push eax;
		call SetGlobalVar2;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static DWORD GetGlobalVarInternal(__int64 val) {
	glob_citr itr = globalVars.find(val);
	if (itr == globalVars.end()) {
		return 0;
	} else {
		return itr->second;
	}
}

static DWORD _stdcall GetGlobalVar2(const char* var) {
	if (strlen(var) != 8) {
		return 0;
	}
	return GetGlobalVarInternal(*(__int64*)var);
}

static DWORD _stdcall GetGlobalVar2Int(DWORD var) {
	return GetGlobalVarInternal(var);
}

static void __declspec(naked) op_get_sfall_global_int() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		xor edx, edx;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp si, 0x9001;
		jz next;
		cmp si, 0x9801;
		jz next;
		cmp si, 0xc001;
		jnz end;
		push eax;
		call GetGlobalVar2Int;
		mov edx, eax;
		jmp end;
next:
		mov edx, esi;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		call GetGlobalVar2;
		mov edx, eax;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_get_sfall_global_float() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		xor edx, edx;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp si, 0x9001;
		jz next;
		cmp si, 0x9801;
		jz next;
		cmp si, 0xc001;
		jnz end;
		push eax;
		call GetGlobalVar2Int;
		mov edx, eax;
		jmp end;
next:
		mov edx, esi;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		call GetGlobalVar2;
		mov edx, eax;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xa001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_get_sfall_arg() {
	__asm {
		pushad;
		push eax;
		call GetHSArg;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static DWORD _stdcall GetSfallArgs2() {
	DWORD argCount = GetHSArgCount();
	DWORD id = TempArray(argCount, 4);
	DWORD* args = GetHSArgs();
	for (DWORD i = 0; i < argCount; i++) {
		arrays[id].val[i].set(*(long*)&args[i]);
	}
	return id;
}

static void __declspec(naked) op_get_sfall_args() {
	__asm {
		pushad;
		push eax;
		call GetSfallArgs2;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_set_sfall_arg() {
	__asm {
		pushad;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push edx;
		push eax;
		call SetHSArg;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) op_set_sfall_return() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz end;
		push eax;
		call SetHSReturn;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_init_hook() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, InitingHookScripts;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

static void _stdcall set_self2(TProgram* script, TGameObj* obj) {
	if (obj) {
		selfOverrideMap[script] = obj;
	} else {
		stdext::hash_map<TProgram*, TGameObj*>::iterator it = selfOverrideMap.find(script);
		if (it != selfOverrideMap.end()) {
			selfOverrideMap.erase(it);
		}
	}
}

static void __declspec(naked) op_set_self() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		push eax;
		push ebp;
		call set_self2;
end:
		popad;
		retn;
	}
}

// used for both register_hook and register_hook_proc
static void sf_register_hook() {
	int id = opHandler.arg(0).asInt();
	int proc = (opHandler.numArgs() > 1)
		? opHandler.arg(1).asInt()
		: -1;

	RegisterHook(opHandler.program(), id, proc);
}

static void __declspec(naked) op_register_hook() {
	_WRAP_OPCODE(sf_register_hook, 1, 0)
}

static void __declspec(naked) register_hook_proc() {
	_WRAP_OPCODE(sf_register_hook, 2, 0)
}

static void __declspec(naked) op_sfall_ver_major() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_MAJOR;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) op_sfall_ver_minor() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_MINOR;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) op_sfall_ver_build() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_BUILD;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}


void InitNewOpcodes() {
	bool AllowUnsafeScripting = IsDebug
		&& GetPrivateProfileIntA("Debugging", "AllowUnsafeScripting", 0, ".\\ddraw.ini") != 0;
	
	dlogr("Adding additional opcodes", DL_SCRIPT);
	if (AllowUnsafeScripting) {
		dlogr("  Unsafe opcodes enabled", DL_SCRIPT);
	} else {
		dlogr("  Unsafe opcodes disabled", DL_SCRIPT);
	}

	SafeWrite32(0x46E370, 0x300);	//Maximum number of allowed opcodes
	SafeWrite32(0x46ce34, (DWORD)opcodes);	//cmp check to make sure opcode exists
	SafeWrite32(0x46ce6c, (DWORD)opcodes);	//call that actually jumps to the opcode
	SafeWrite32(0x46e390, (DWORD)opcodes);	//mov that writes to the opcode

	if (AllowUnsafeScripting) {
		opcodes[0x156] = op_read_byte;
		opcodes[0x157] = op_read_short;
		opcodes[0x158] = op_read_int;
		opcodes[0x159] = op_read_string;
	}
	opcodes[0x15a] = op_set_pc_base_stat;
	opcodes[0x15b] = op_set_pc_extra_stat;
	opcodes[0x15c] = op_get_pc_base_stat;
	opcodes[0x15d] = op_get_pc_extra_stat;
	opcodes[0x15e] = op_set_critter_base_stat;
	opcodes[0x15f] = op_set_critter_extra_stat;
	opcodes[0x160] = op_get_critter_base_stat;
	opcodes[0x161] = op_get_critter_extra_stat;
	opcodes[0x162] = op_tap_key;
	opcodes[0x163] = op_get_year;
	opcodes[0x164] = op_game_loaded;
	opcodes[0x165] = op_graphics_funcs_available;
	opcodes[0x166] = op_load_shader;
	opcodes[0x167] = op_free_shader;
	opcodes[0x168] = op_activate_shader;
	opcodes[0x169] = op_deactivate_shader;
	opcodes[0x16a] = op_set_global_script_repeat;
	opcodes[0x16b] = op_input_funcs_available;
	opcodes[0x16c] = op_key_pressed;
	opcodes[0x16d] = op_set_shader_int;
	opcodes[0x16e] = op_set_shader_float;
	opcodes[0x16f] = op_set_shader_vector;
	opcodes[0x170] = op_in_world_map;
	opcodes[0x171] = op_force_encounter;
	opcodes[0x172] = op_set_world_map_pos;
	opcodes[0x173] = op_get_world_map_x_pos;
	opcodes[0x174] = op_get_world_map_y_pos;
	opcodes[0x175] = op_set_dm_model;
	opcodes[0x176] = op_set_df_model;
	opcodes[0x177] = op_set_movie_path;
	for (int i = 0x178; i < 0x189; i++) {
		opcodes[i] = op_set_perk_value;
	}
	opcodes[0x189] = op_set_perk_name;
	opcodes[0x18a] = op_set_perk_desc;
	opcodes[0x18b] = op_set_pipboy_available;
	if (UsingExtraKillTypes()) {
		opcodes[0x18c] = op_get_kill_counter2;
		opcodes[0x18d] = op_mod_kill_counter2;
	} else {
		opcodes[0x18c] = op_get_kill_counter;
		opcodes[0x18d] = op_mod_kill_counter;
	}
	opcodes[0x18e] = op_get_perk_owed;
	opcodes[0x18f] = op_set_perk_owed;
	opcodes[0x190] = op_get_perk_available;
	opcodes[0x191] = op_get_critter_current_ap;
	opcodes[0x192] = op_set_critter_current_ap;
	opcodes[0x193] = op_active_hand;
	opcodes[0x194] = op_toggle_active_hand;
	opcodes[0x195] = op_set_weapon_knockback;
	opcodes[0x196] = op_set_target_knockback;
	opcodes[0x197] = op_set_attacker_knockback;
	opcodes[0x198] = op_remove_weapon_knockback;
	opcodes[0x199] = op_remove_target_knockback;
	opcodes[0x19a] = op_remove_attacker_knockback;
	opcodes[0x19b] = op_set_global_script_type;
	opcodes[0x19c] = op_available_global_script_types;
	opcodes[0x19d] = op_set_sfall_global;
	opcodes[0x19e] = op_get_sfall_global_int;
	opcodes[0x19f] = op_get_sfall_global_float;
	opcodes[0x1a0] = op_set_pickpocket_max;
	opcodes[0x1a1] = op_set_hit_chance_max;
	opcodes[0x1a2] = op_set_skill_max;
	opcodes[0x1a3] = op_eax_available;
	//opcodes[0x1a4] = op_set_eax_environment;
	opcodes[0x1a5] = op_inc_npc_level;
	opcodes[0x1a6] = op_get_viewport_x;
	opcodes[0x1a7] = op_get_viewport_y;
	opcodes[0x1a8] = op_set_viewport_x;
	opcodes[0x1a9] = op_set_viewport_y;
	opcodes[0x1aa] = op_set_xp_mod;
	opcodes[0x1ab] = op_set_perk_level_mod;
	opcodes[0x1ac] = op_get_ini_setting;
	opcodes[0x1ad] = op_get_shader_version;
	opcodes[0x1ae] = op_set_shader_mode;
	opcodes[0x1af] = op_get_game_mode;
	opcodes[0x1b0] = op_force_graphics_refresh;
	opcodes[0x1b1] = op_get_shader_texture;
	opcodes[0x1b2] = op_set_shader_texture;
	opcodes[0x1b3] = op_get_uptime;
	opcodes[0x1b4] = op_set_stat_max;
	opcodes[0x1b5] = op_set_stat_min;
	opcodes[0x1b6] = op_set_car_current_town;
	opcodes[0x1b7] = op_set_pc_stat_max;
	opcodes[0x1b8] = op_set_pc_stat_min;
	opcodes[0x1b9] = op_set_npc_stat_max;
	opcodes[0x1ba] = op_set_npc_stat_min;
	opcodes[0x1bb] = op_set_fake_perk;
	opcodes[0x1bc] = op_set_fake_trait;
	opcodes[0x1bd] = op_set_selectable_perk;
	opcodes[0x1be] = op_set_perkbox_title;
	opcodes[0x1bf] = op_hide_real_perks;
	opcodes[0x1c0] = op_show_real_perks;
	opcodes[0x1c1] = op_has_fake_perk;
	opcodes[0x1c2] = op_has_fake_trait;
	opcodes[0x1c3] = op_perk_add_mode;
	opcodes[0x1c4] = op_clear_selectable_perks;
	opcodes[0x1c5] = op_set_critter_hit_chance_mod;
	opcodes[0x1c6] = op_set_base_hit_chance_mod;
	opcodes[0x1c7] = op_set_critter_skill_mod;
	opcodes[0x1c8] = op_set_base_skill_mod;
	opcodes[0x1c9] = op_set_critter_pickpocket_mod;
	opcodes[0x1ca] = op_set_base_pickpocket_mod;
	opcodes[0x1cb] = op_set_pyromaniac_mod;
	opcodes[0x1cc] = op_apply_heaveho_fix;
	opcodes[0x1cd] = op_set_swiftlearner_mod;
	opcodes[0x1ce] = op_set_hp_per_level_mod;
	if (AllowUnsafeScripting) {
		opcodes[0x1cf] = op_write_byte;
		opcodes[0x1d0] = op_write_short;
		opcodes[0x1d1] = op_write_int;
		for (int i = 0x1d2; i < 0x1dc; i++) {
			opcodes[i] = op_call_offset;
		}
	}
	opcodes[0x1dc] = op_show_iface_tag;
	opcodes[0x1dd] = op_hide_iface_tag;
	opcodes[0x1de] = op_is_iface_tag_active;
	opcodes[0x1df] = op_get_bodypart_hit_modifier;
	opcodes[0x1e0] = op_set_bodypart_hit_modifier;
	opcodes[0x1e1] = op_set_critical_table;
	opcodes[0x1e2] = op_get_critical_table;
	opcodes[0x1e3] = op_reset_critical_table;
	opcodes[0x1e4] = op_get_sfall_arg;
	opcodes[0x1e5] = op_set_sfall_return;
	opcodes[0x1e6] = op_set_unspent_ap_bonus;
	opcodes[0x1e7] = op_get_unspent_ap_bonus;
	opcodes[0x1e8] = op_set_unspent_ap_perk_bonus;
	opcodes[0x1e9] = op_get_unspent_ap_perk_bonus;
	opcodes[0x1ea] = op_init_hook;
	opcodes[0x1eb] = op_get_ini_string;
	opcodes[0x1ec] = op_sqrt;
	opcodes[0x1ed] = op_abs;
	opcodes[0x1ee] = op_sin;
	opcodes[0x1ef] = op_cos;
	opcodes[0x1f0] = op_tan;
	opcodes[0x1f1] = op_arctan;
	opcodes[0x1f2] = op_set_palette;
	opcodes[0x1f3] = op_remove_script;
	opcodes[0x1f4] = op_set_script;
	opcodes[0x1f5] = op_get_script;
	opcodes[0x1f6] = op_nb_create_char;
	opcodes[0x1f7] = op_fs_create;
	opcodes[0x1f8] = op_fs_copy;
	opcodes[0x1f9] = op_fs_find;
	opcodes[0x1fa] = op_fs_write_byte;
	opcodes[0x1fb] = op_fs_write_short;
	opcodes[0x1fc] = op_fs_write_int;
	opcodes[0x1fd] = op_fs_write_int;
	opcodes[0x1fe] = op_fs_write_string;
	opcodes[0x1ff] = op_fs_delete;
	opcodes[0x200] = op_fs_size;
	opcodes[0x201] = op_fs_pos;
	opcodes[0x202] = op_fs_seek;
	opcodes[0x203] = op_fs_resize;
	opcodes[0x204] = op_get_proto_data;
	opcodes[0x205] = op_set_proto_data;
	opcodes[0x206] = op_set_self;
	opcodes[0x207] = op_register_hook;
	opcodes[0x208] = op_fs_write_bstring;
	opcodes[0x209] = op_fs_read_byte;
	opcodes[0x20a] = op_fs_read_short;
	opcodes[0x20b] = op_fs_read_int;
	opcodes[0x20c] = op_fs_read_float;
	opcodes[0x20d] = op_list_begin;
	opcodes[0x20e] = op_list_next;
	opcodes[0x20f] = op_list_end;
	opcodes[0x210] = op_sfall_ver_major;
	opcodes[0x211] = op_sfall_ver_minor;
	opcodes[0x212] = op_sfall_ver_build;
	opcodes[0x213] = op_hero_select_win;
	opcodes[0x214] = op_set_hero_race;
	opcodes[0x215] = op_set_hero_style;
	opcodes[0x216] = op_set_critter_burst_disable;
	opcodes[0x217] = op_get_weapon_ammo_pid;
	opcodes[0x218] = op_set_weapon_ammo_pid;
	opcodes[0x219] = op_get_weapon_ammo_count;
	opcodes[0x21a] = op_set_weapon_ammo_count;
	if (AllowUnsafeScripting) {
		opcodes[0x21b] = op_write_string;
	}
	opcodes[0x21c] = op_get_mouse_x;
	opcodes[0x21d] = op_get_mouse_y;
	opcodes[0x21e] = op_get_mouse_buttons;
	opcodes[0x21f] = op_get_window_under_mouse;
	opcodes[0x220] = op_get_screen_width;
	opcodes[0x221] = op_get_screen_height;
	opcodes[0x222] = op_stop_game;
	opcodes[0x223] = op_resume_game;
	opcodes[0x224] = op_create_message_window;
	opcodes[0x225] = op_remove_trait;
	opcodes[0x226] = op_get_light_level;
	opcodes[0x227] = op_refresh_pc_art;
	opcodes[0x228] = op_get_attack_type;
	opcodes[0x229] = op_force_encounter_with_flags;
	opcodes[0x22a] = op_set_map_time_multi;
	opcodes[0x22b] = op_play_sfall_sound;
	opcodes[0x22c] = op_stop_sfall_sound;
	opcodes[0x22d] = op_create_array;
	opcodes[0x22e] = op_set_array;
	opcodes[0x22f] = op_get_array;
	opcodes[0x230] = op_free_array;
	opcodes[0x231] = op_len_array;
	opcodes[0x232] = op_resize_array;
	opcodes[0x233] = op_temp_array;
	opcodes[0x234] = op_fix_array;
	opcodes[0x235] = op_string_split;
	opcodes[0x236] = op_list_as_array;
	opcodes[0x237] = op_atoi;
	opcodes[0x238] = op_atof;
	opcodes[0x239] = op_scan_array;
	opcodes[0x23a] = op_get_tile_fid;
	opcodes[0x23b] = op_modified_ini;
	opcodes[0x23c] = op_get_sfall_args;
	opcodes[0x23d] = op_set_sfall_arg;
	opcodes[0x23e] = op_force_aimed_shots;
	opcodes[0x23f] = op_disable_aimed_shots;
	opcodes[0x240] = op_mark_movie_played;
	opcodes[0x241] = op_get_npc_level;
	opcodes[0x242] = op_set_critter_skill_points;
	opcodes[0x243] = op_get_critter_skill_points;
	opcodes[0x244] = op_set_available_skill_points;
	opcodes[0x245] = op_get_available_skill_points;
	opcodes[0x246] = op_mod_skill_points_per_level;
	opcodes[0x247] = op_set_perk_freq;
	opcodes[0x248] = op_get_last_attacker;
	opcodes[0x249] = op_get_last_target;
	opcodes[0x24a] = op_block_combat;
	opcodes[0x24b] = op_tile_under_cursor;
	opcodes[0x24c] = op_gdialog_get_barter_mod;
	opcodes[0x24d] = op_set_inven_ap_cost;
	opcodes[0x24e] = op_substr;
	opcodes[0x24f] = op_strlen;
	opcodes[0x250] = op_sprintf;
	opcodes[0x251] = op_ord;
	// opcodes[0x252]=  RESERVED
	opcodes[0x253] = op_typeof;
	opcodes[0x254] = op_save_array;
	opcodes[0x255] = op_load_array;
	opcodes[0x256] = op_get_array_key;
	opcodes[0x257] = op_stack_array;
	// opcodes[0x258]= RESERVED for arrays
	// opcodes[0x259]= RESERVED for arrays
	opcodes[0x25a] = op_reg_anim_destroy;
	opcodes[0x25b] = op_reg_anim_animate_and_hide;
	opcodes[0x25c] = op_reg_anim_combat_check;
	opcodes[0x25d] = op_reg_anim_light;
	opcodes[0x25e] = op_reg_anim_change_fid;
	opcodes[0x25f] = op_reg_anim_take_out;
	opcodes[0x260] = op_reg_anim_turn_towards;
	opcodes[0x261] = op_explosions_metarule;
	opcodes[0x262] = register_hook_proc;
	opcodes[0x263] = op_power;
	opcodes[0x264] = op_log;
	opcodes[0x265] = op_exponent;
	opcodes[0x266] = op_ceil;
	opcodes[0x267] = op_round;
	// opcodes[0x268]= RESERVED
	// opcodes[0x269]= RESERVED
	// opcodes[0x26a]=op_game_ui_redraw;
	opcodes[0x26b] = op_message_str_game;
	opcodes[0x26c] = op_sneak_success;
	opcodes[0x26d] = op_tile_light;
	opcodes[0x26e] = op_make_straight_path;
	opcodes[0x26f] = op_obj_blocking_at;
	opcodes[0x270] = op_tile_get_objects;
	opcodes[0x271] = op_get_party_members;
	opcodes[0x272] = op_make_path;
	opcodes[0x273] = op_create_spatial;
	opcodes[0x274] = op_art_exists;
	opcodes[0x275] = op_obj_is_carrying_obj;
	// universal opcodes
	opcodes[0x276] = op_sfall_metarule0;
	opcodes[0x277] = op_sfall_metarule1;
	opcodes[0x278] = op_sfall_metarule2;
	opcodes[0x279] = op_sfall_metarule3;
	opcodes[0x27a] = op_sfall_metarule4;
	opcodes[0x27b] = op_sfall_metarule5;
	opcodes[0x27c] = op_sfall_metarule6; // if you need more arguments - use arrays
}

/*
	Array for opcodes metadata.

	This is completely optional, added for convenience only.

	By adding opcode to this array, Sfall will automatically validate it's arguments using provided info.
	On fail, errors will be printed to debug.log and opcode will not be executed.
	If you don't include opcode in this array, you should take care of all argument validation inside handler itself.
*/
static const SfallOpcodeMetadata opcodeMetaArray[] = {
	{sf_register_hook, "register_hook[_proc]", {DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{sf_test, "validate_test", {DATATYPE_MASK_INT, DATATYPE_MASK_INT | DATATYPE_MASK_FLOAT, DATATYPE_MASK_STR, DATATYPE_NONE}},
	{sf_spatial_radius, "spatial_radius", {DATATYPE_MASK_VALID_OBJ}},
	{sf_critter_inven_obj2, "critter_inven_obj2", {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	//{op_message_str_game, {}}
};

static void InitOpcodeMetaTable() {
	int length = sizeof(opcodeMetaArray) / sizeof(SfallOpcodeMetadata);
	for (int i = 0; i < length; ++i) {
		opHandler.addOpcodeMetaData(&opcodeMetaArray[i]);
	}
}
