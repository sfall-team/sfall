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

static void __declspec(naked) SetGlobalScriptRepeat() {
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

static void __declspec(naked) SetGlobalScriptType() {
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

static void __declspec(naked) GetGlobalScriptTypes() {
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

static void __declspec(naked) SetGlobalVar() {
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

static void __declspec(naked) GetGlobalVarInt() {
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

static void __declspec(naked) GetGlobalVarFloat() {
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

static void __declspec(naked) GetSfallArg() {
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

static void __declspec(naked) GetSfallArgs() {
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

static void __declspec(naked) SetSfallArg() {
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

static void __declspec(naked) SetSfallReturn() {
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

static void __declspec(naked) InitHook() {
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

static void __declspec(naked) set_self() {
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

static void __declspec(naked) register_hook() {
	_WRAP_OPCODE(sf_register_hook, 1, 0)
}

static void __declspec(naked) register_hook_proc() {
	_WRAP_OPCODE(sf_register_hook, 2, 0)
}

static void __declspec(naked) sfall_ver_major() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_MAJOR;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) sfall_ver_minor() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_MINOR;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) sfall_ver_build() {
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
		opcodes[0x156] = ReadByte;
		opcodes[0x157] = ReadShort;
		opcodes[0x158] = ReadInt;
		opcodes[0x159] = ReadString;
	}
	opcodes[0x15a] = SetPCBaseStat;
	opcodes[0x15b] = SetPCExtraStat;
	opcodes[0x15c] = GetPCBaseStat;
	opcodes[0x15d] = GetPCExtraStat;
	opcodes[0x15e] = SetCritterBaseStat;
	opcodes[0x15f] = SetCritterExtraStat;
	opcodes[0x160] = GetCritterBaseStat;
	opcodes[0x161] = GetCritterExtraStat;
	opcodes[0x162] = funcTapKey;
	opcodes[0x163] = GetYear;
	opcodes[0x164] = GameLoaded;
	opcodes[0x165] = GraphicsFuncsAvailable;
	opcodes[0x166] = funcLoadShader;
	opcodes[0x167] = funcFreeShader;
	opcodes[0x168] = funcActivateShader;
	opcodes[0x169] = funcDeactivateShader;
	opcodes[0x16a] = SetGlobalScriptRepeat;
	opcodes[0x16b] = InputFuncsAvailable;
	opcodes[0x16c] = KeyPressed;
	opcodes[0x16d] = funcSetShaderInt;
	opcodes[0x16e] = funcSetShaderFloat;
	opcodes[0x16f] = funcSetShaderVector;
	opcodes[0x170] = funcInWorldMap;
	opcodes[0x171] = ForceEncounter;
	opcodes[0x172] = SetWorldMapPos;
	opcodes[0x173] = GetWorldMapXPos;
	opcodes[0x174] = GetWorldMapYPos;
	opcodes[0x175] = SetDMModel;
	opcodes[0x176] = SetDFModel;
	opcodes[0x177] = SetMoviePath;
	for (int i = 0x178; i < 0x189; i++) {
		opcodes[i] = funcSetPerkValue;
	}
	opcodes[0x189] = funcSetPerkName;
	opcodes[0x18a] = funcSetPerkDesc;
	opcodes[0x18b] = SetPipBoyAvailable;
	if (UsingExtraKillTypes()) {
		opcodes[0x18c] = GetKillCounter2;
		opcodes[0x18d] = ModKillCounter2;
	} else {
		opcodes[0x18c] = GetKillCounter;
		opcodes[0x18d] = ModKillCounter;
	}
	opcodes[0x18e] = GetPerkOwed;
	opcodes[0x18f] = SetPerkOwed;
	opcodes[0x190] = GetPerkAvailable;
	opcodes[0x191] = GetCritterAP;
	opcodes[0x192] = SetCritterAP;
	opcodes[0x193] = GetActiveHand;
	opcodes[0x194] = ToggleActiveHand;
	opcodes[0x195] = SetWeaponKnockback;
	opcodes[0x196] = SetTargetKnockback;
	opcodes[0x197] = SetAttackerKnockback;
	opcodes[0x198] = RemoveWeaponKnockback;
	opcodes[0x199] = RemoveTargetKnockback;
	opcodes[0x19a] = RemoveAttackerKnockback;
	opcodes[0x19b] = SetGlobalScriptType;
	opcodes[0x19c] = GetGlobalScriptTypes;
	opcodes[0x19d] = SetGlobalVar;
	opcodes[0x19e] = GetGlobalVarInt;
	opcodes[0x19f] = GetGlobalVarFloat;
	opcodes[0x1a0] = fSetPickpocketMax;
	opcodes[0x1a1] = fSetHitChanceMax;
	opcodes[0x1a2] = fSetSkillMax;
	opcodes[0x1a3] = EaxAvailable;
	//opcodes[0x1a4]=SetEaxEnvironmentFunc;
	opcodes[0x1a5] = IncNPCLevel;
	opcodes[0x1a6] = GetViewportX;
	opcodes[0x1a7] = GetViewportY;
	opcodes[0x1a8] = SetViewportX;
	opcodes[0x1a9] = SetViewportY;
	opcodes[0x1aa] = SetXpMod;
	opcodes[0x1ab] = SetPerkLevelMod;
	opcodes[0x1ac] = GetIniSetting;
	opcodes[0x1ad] = funcGetShaderVersion;
	opcodes[0x1ae] = funcSetShaderMode;
	opcodes[0x1af] = GetGameMode;
	opcodes[0x1b0] = funcForceGraphicsRefresh;
	opcodes[0x1b1] = funcGetShaderTexture;
	opcodes[0x1b2] = funcSetShaderTexture;
	opcodes[0x1b3] = funcGetTickCount;
	opcodes[0x1b4] = SetStatMax;
	opcodes[0x1b5] = SetStatMin;
	opcodes[0x1b6] = SetCarTown;
	opcodes[0x1b7] = fSetPCStatMax;
	opcodes[0x1b8] = fSetPCStatMin;
	opcodes[0x1b9] = fSetNPCStatMax;
	opcodes[0x1ba] = fSetNPCStatMin;
	opcodes[0x1bb] = fSetFakePerk;
	opcodes[0x1bc] = fSetFakeTrait;
	opcodes[0x1bd] = fSetSelectablePerk;
	opcodes[0x1be] = fSetPerkboxTitle;
	opcodes[0x1bf] = fIgnoreDefaultPerks;
	opcodes[0x1c0] = fRestoreDefaultPerks;
	opcodes[0x1c1] = fHasFakePerk;
	opcodes[0x1c2] = fHasFakeTrait;
	opcodes[0x1c3] = fAddPerkMode;
	opcodes[0x1c4] = fClearSelectablePerks;
	opcodes[0x1c5] = SetCritterHitChance;
	opcodes[0x1c6] = SetBaseHitChance;
	opcodes[0x1c7] = SetCritterSkillMod;
	opcodes[0x1c8] = SetBaseSkillMod;
	opcodes[0x1c9] = SetCritterPickpocket;
	opcodes[0x1ca] = SetBasePickpocket;
	opcodes[0x1cb] = SetPyromaniacMod;
	opcodes[0x1cc] = fApplyHeaveHoFix;
	opcodes[0x1cd] = SetSwiftLearnerMod;
	opcodes[0x1ce] = SetLevelHPMod;
	if (AllowUnsafeScripting) {
		opcodes[0x1cf] = WriteByte;
		opcodes[0x1d0] = WriteShort;
		opcodes[0x1d1] = WriteInt;
		for (int i = 0x1d2; i < 0x1dc; i++) {
			opcodes[i] = CallOffset;
		}
	}
	opcodes[0x1dc] = ShowIfaceTag;
	opcodes[0x1dd] = HideIfaceTag;
	opcodes[0x1de] = IsIfaceTagActive;
	opcodes[0x1df] = GetBodypartHitModifier;
	opcodes[0x1e0] = SetBodypartHitModifier;
	opcodes[0x1e1] = funcSetCriticalTable;
	opcodes[0x1e2] = funcGetCriticalTable;
	opcodes[0x1e3] = funcResetCriticalTable;
	opcodes[0x1e4] = GetSfallArg;
	opcodes[0x1e5] = SetSfallReturn;
	opcodes[0x1e6] = SetApAcBonus;
	opcodes[0x1e7] = GetApAcBonus;
	opcodes[0x1e8] = SetApAcEBonus;
	opcodes[0x1e9] = GetApAcEBonus;
	opcodes[0x1ea] = InitHook;
	opcodes[0x1eb] = GetIniString;
	opcodes[0x1ec] = funcSqrt;
	opcodes[0x1ed] = funcAbs;
	opcodes[0x1ee] = funcSin;
	opcodes[0x1ef] = funcCos;
	opcodes[0x1f0] = funcTan;
	opcodes[0x1f1] = funcATan;
	opcodes[0x1f2] = SetPalette;
	opcodes[0x1f3] = RemoveScript;
	opcodes[0x1f4] = SetScript;
	opcodes[0x1f5] = GetScript;
	opcodes[0x1f6] = NBCreateChar;
	opcodes[0x1f7] = fs_create;
	opcodes[0x1f8] = fs_copy;
	opcodes[0x1f9] = fs_find;
	opcodes[0x1fa] = fs_write_byte;
	opcodes[0x1fb] = fs_write_short;
	opcodes[0x1fc] = fs_write_int;
	opcodes[0x1fd] = fs_write_int;
	opcodes[0x1fe] = fs_write_string;
	opcodes[0x1ff] = fs_delete;
	opcodes[0x200] = fs_size;
	opcodes[0x201] = fs_pos;
	opcodes[0x202] = fs_seek;
	opcodes[0x203] = fs_resize;
	opcodes[0x204] = get_proto_data;
	opcodes[0x205] = set_proto_data;
	opcodes[0x206] = set_self;
	opcodes[0x207] = register_hook;
	opcodes[0x208] = fs_write_bstring;
	opcodes[0x209] = fs_read_byte;
	opcodes[0x20a] = fs_read_short;
	opcodes[0x20b] = fs_read_int;
	opcodes[0x20c] = fs_read_float;
	opcodes[0x20d] = list_begin;
	opcodes[0x20e] = list_next;
	opcodes[0x20f] = list_end;
	opcodes[0x210] = sfall_ver_major;
	opcodes[0x211] = sfall_ver_minor;
	opcodes[0x212] = sfall_ver_build;
	opcodes[0x213] = funcHeroSelectWin;
	opcodes[0x214] = funcSetHeroRace;
	opcodes[0x215] = funcSetHeroStyle;
	opcodes[0x216] = set_critter_burst_disable;
	opcodes[0x217] = get_weapon_ammo_pid;
	opcodes[0x218] = set_weapon_ammo_pid;
	opcodes[0x219] = get_weapon_ammo_count;
	opcodes[0x21a] = set_weapon_ammo_count;
	if (AllowUnsafeScripting) {
		opcodes[0x21b] = WriteString;
	}
	opcodes[0x21c] = get_mouse_x;
	opcodes[0x21d] = get_mouse_y;
	opcodes[0x21e] = get_mouse_buttons;
	opcodes[0x21f] = get_window_under_mouse;
	opcodes[0x220] = get_screen_width;
	opcodes[0x221] = get_screen_height;
	opcodes[0x222] = stop_game;
	opcodes[0x223] = resume_game;
	opcodes[0x224] = create_message_window;
	opcodes[0x225] = remove_trait;
	opcodes[0x226] = get_light_level;
	opcodes[0x227] = refresh_pc_art;
	opcodes[0x228] = get_attack_type;
	opcodes[0x229] = ForceEncounterWithFlags;
	opcodes[0x22a] = set_map_time_multi;
	opcodes[0x22b] = play_sfall_sound;
	opcodes[0x22c] = stop_sfall_sound;
	opcodes[0x22d] = create_array;
	opcodes[0x22e] = set_array;
	opcodes[0x22f] = get_array;
	opcodes[0x230] = free_array;
	opcodes[0x231] = len_array;
	opcodes[0x232] = resize_array;
	opcodes[0x233] = temp_array;
	opcodes[0x234] = fix_array;
	opcodes[0x235] = string_split;
	opcodes[0x236] = list_as_array;
	opcodes[0x237] = str_to_int;
	opcodes[0x238] = str_to_flt;
	opcodes[0x239] = scan_array;
	opcodes[0x23a] = get_tile_pid;
	opcodes[0x23b] = modified_ini;
	opcodes[0x23c] = GetSfallArgs;
	opcodes[0x23d] = SetSfallArg;
	opcodes[0x23e] = force_aimed_shots;
	opcodes[0x23f] = disable_aimed_shots;
	opcodes[0x240] = mark_movie_played;
	opcodes[0x241] = get_npc_level;
	opcodes[0x242] = set_critter_skill_points;
	opcodes[0x243] = get_critter_skill_points;
	opcodes[0x244] = set_available_skill_points;
	opcodes[0x245] = get_available_skill_points;
	opcodes[0x246] = mod_skill_points_per_level;
	opcodes[0x247] = set_perk_freq;
	opcodes[0x248] = get_last_attacker;
	opcodes[0x249] = get_last_target;
	opcodes[0x24a] = block_combat;
	opcodes[0x24b] = tile_under_cursor;
	opcodes[0x24c] = gdialog_get_barter_mod;
	opcodes[0x24d] = set_inven_ap_cost;
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
	opcodes[0x263] = funcPow;
	opcodes[0x264] = funcLog;
	opcodes[0x265] = funcExp;
	opcodes[0x266] = funcCeil;
	opcodes[0x267] = funcRound;
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
