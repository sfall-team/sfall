/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

static void __declspec(naked) op_create_array() {
	__asm {
		pushaop;
		mov  edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov  ecx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov  edx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp  bx, VAR_TYPE_INT;
		jne  fail;
		cmp  dx, VAR_TYPE_INT;
		jne  fail;
		push ecx;
		push eax;
		call CreateArray;
		mov  edx, eax;
		jmp  end;
fail:
		xor  edx, edx;
		dec  edx;
end:
		mov  eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov  edx, VAR_TYPE_INT;
		mov  eax, edi;
		call fo::funcoffs::interpretPushShort_;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_set_array() {
	__asm {
		pushaop;
		mov  ebp, eax;
		//Get args
		call fo::funcoffs::interpretPopShort_;
		mov  edx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  edi, eax; // value
		mov  eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov  ecx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  esi, eax; // key
		mov  eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		xchg eax, edi; // arrayID
		//Error check:
		cmp  bx, VAR_TYPE_INT;
		jne  end;
		push 1; // arg 6: allow unset
		push edx; // arg 5: value type
		// value:
		cmp  dx, VAR_TYPE_STR2;
		jz   next;
		cmp  dx, VAR_TYPE_STR;
		jnz  notstring;
next:
		mov  ebx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretGetString_;
notstring:
		push eax; // arg 4: value
		// key:
		cmp  cx, VAR_TYPE_STR2;
		jz   next1;
		cmp  cx, VAR_TYPE_STR;
		jnz  notstring1;
next1:
		mov  edx, ecx;
		mov  ebx, esi;
		mov  eax, ebp;
		call fo::funcoffs::interpretGetString_;
		mov  esi, eax;
notstring1:
		push ecx; // arg 3: key type
		push esi; // arg 2: key
		push edi; // arg 1: arrayID
		call SetArray;
end:
		popaop;
		retn;
	}
}

/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
	example: vartext[5]
*/
static char* __stdcall GetArraySubstr(const char* str, size_t index) {
	if (index < strlen(str)) {
		wchar_t c = ((unsigned char*)str)[index];
		((wchar_t*)gTextBuffer)[0] = c;
	} else {
		gTextBuffer[0] = '\0';
	}
	return gTextBuffer; // returns char of string
}

static void __declspec(naked) op_get_array() {
	__asm {
		pushaop;
		mov  ebp, eax;
		//Get args
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  edi, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov  ecx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  esi, eax;
		mov  eax, ebp;
		// check first argument type
		cmp  cx, VAR_TYPE_STR;
		je   callsubstr;
		cmp  cx, VAR_TYPE_STR2;
		jne  proceedgetarray;
callsubstr:
		// use substr instead of get_array when used on string
		// in this case, check for second argument to be numeric
		cmp  bx, VAR_TYPE_INT;
		jne  fail;
		mov  eax, ebp;
		mov  ebx, esi;
		mov  edx, ecx;
		call fo::funcoffs::interpretGetString_;
		push edi;
		push eax;
		call GetArraySubstr;
		mov  edx, eax; // result substring
		mov  ebx, VAR_TYPE_STR; // result type
		jmp  end;
proceedgetarray:
		cmp  cx, VAR_TYPE_INT; // only int is allowed for arrayID in this case
		jne  fail;
		cmp  bx, VAR_TYPE_STR;
		je   next1;
		cmp  bx, VAR_TYPE_STR2;
		jne  notstring1;
next1:
		mov  ecx, ebx;
		mov  edx, ebx;
		mov  ebx, edi;
		mov  eax, ebp;
		call fo::funcoffs::interpretGetString_;
		mov  edi, eax;
		mov  ebx, ecx;
notstring1:
		mov  eax, esp; // ptr to resultType (will be changed in GetArray)
		push eax;
		push ebx;
		push edi;
		push esi;
		call GetArray;
		mov  edx, eax; // result data
		mov  ebx, [esp]; // resultType
		jmp  end;
fail:
		xor  edx, edx;
		mov  ebx, VAR_TYPE_INT;
end:
		cmp  bx, VAR_TYPE_STR;
		jne  notstring;
		mov  eax, ebp;
		call fo::funcoffs::interpretAddString_;
		mov  edx, eax;
notstring:
		mov  eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov  edx, ebx;
		mov  eax, ebp;
		call fo::funcoffs::interpretPushShort_;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_free_array() {
	__asm {
		pushaop;
		mov  edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp  bx, VAR_TYPE_INT;
		jne  end;
		push eax;
		call FreeArray;
end:
		popaop;
		retn;
	}
}

static void __declspec(naked) op_len_array() {
	__asm {
		pushaop;
		mov  edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp  bx, VAR_TYPE_INT;
		jne  fail;
		push eax;
		call LenArray;
		mov  edx, eax;
		jmp  end;
fail:
		xor  edx, edx;
end:
		mov  eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov  edx, VAR_TYPE_INT;
		mov  eax, edi;
		call fo::funcoffs::interpretPushShort_;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_resize_array() {
	__asm {
		pushaop;
		mov  ebp, eax;
		//Get args
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  edi, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov  ecx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  esi, eax;
		mov  eax, ebp;
		//Error check
		cmp  bx, VAR_TYPE_INT;
		jne  end;
		cmp  cx, VAR_TYPE_INT;
		jne  end;
		push edi;
		push esi;
		call ResizeArray;
end:
		popaop;
		retn;
	}
}

static void __declspec(naked) op_temp_array() {
	__asm {
		pushaop;
		mov  edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov  ecx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov  edx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp  bx, VAR_TYPE_INT;
		jne  fail;
		cmp  dx, VAR_TYPE_INT;
		jne  fail;
		push ecx;
		push eax;
		call CreateTempArray;
		mov  edx, eax;
		jmp  end;
fail:
		xor  edx, edx;
		dec  edx;
end:
		mov  eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov  edx, VAR_TYPE_INT;
		mov  eax, edi;
		call fo::funcoffs::interpretPushShort_;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_fix_array() {
	__asm {
		pushaop;
		mov  edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov  ebx, eax;
		mov  eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp  bx, VAR_TYPE_INT;
		jne  end;
		push eax;
		call FixArray;
end:
		popaop;
		retn;
	}
}

static void __declspec(naked) op_scan_array() {
		__asm {
		pushaop;
		mov  ebp, eax;
		//Get args
		call fo::funcoffs::interpretPopShort_;
		mov  edx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  edi, eax; // value (needle)
		mov  eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov  ecx, eax;
		mov  eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov  esi, eax; // arrayID (haystack)
		mov  eax, ebp;
		//Error check
		cmp  cx, VAR_TYPE_INT;
		jne  fail;
		cmp  dx, VAR_TYPE_STR;
		je   getstringvar;
		cmp  dx, VAR_TYPE_STR2;
		jne  success;
getstringvar:
		mov  ebx, edi;
		mov  eax, ebp;
		call fo::funcoffs::interpretGetString_;
		mov  edi, eax;
success:
		push esp;
		push edx;
		push edi;
		push esi;
		call ScanArray;
		mov  edx, eax; // result
		mov  ebx, [esp]; // resultType
		jmp  end;
fail:
		mov  ebx, VAR_TYPE_INT;
		xor  edx, edx;
		dec  edx; // returns -1
end:
		cmp  ebx, VAR_TYPE_STR;
		jne  resultnotstr;
		mov  eax, ebp;
		call fo::funcoffs::interpretAddString_;
		mov  edx, eax; // str ptr
resultnotstr:
		mov  eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov  edx, ebx;
		mov  eax, ebp;
		call fo::funcoffs::interpretPushShort_;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_save_array() {
	_OP_BEGIN(ebp);
		// Get args
	_GET_ARG_R32(ebp, ebx, edi);
	_GET_ARG_R32(ebp, edx, ecx);
	__asm {
		// arg check:
		cmp  bx, VAR_TYPE_INT;
		jne  end;
	}
	_CHECK_PARSE_STR(1, ebp, dx, ecx);
	__asm {
		push edi; // arg 3: arrayID
		push edx; // arg 2: keyType
		push ecx; // arg 1: key
		call SaveArray;
	}
end:
	_OP_END;
}

static void __declspec(naked) op_load_array() {
	_OP_BEGIN(ebp);
	_GET_ARG_R32(ebp, edx, ecx);
	_CHECK_PARSE_STR(1, ebp, dx, ecx);
	__asm {
		push edx; // arg 2: keyType
		push ecx; // arg 1: key
		call LoadArray;
	}
	_RET_VAL_INT32(ebp);
	_OP_END;
}

static void __declspec(naked) op_get_array_key() {
	_OP_BEGIN(ebp);
	_GET_ARG_R32(ebp, edx, ecx); // index
	_GET_ARG_R32(ebp, ebx, edi); // arrayID
	__asm {
		// arg check:
		cmp  bx, VAR_TYPE_INT;
		jne  wrongarg;
		cmp  dx, VAR_TYPE_INT;
		jne  wrongarg;
		push esp; // arg 3: *resultType
		push ecx; // arg 2: index
		push edi; // arg 1: arrayID
		call GetArrayKey;
	}
	_RET_VAL_POSSIBLY_STR(1, ebp, [esp]);
	goto end;
	__asm {
wrongarg:
		xor  eax, eax; // return 0 on wrong arguments
	}
	_RET_VAL_INT32(ebp);
end:
	_OP_END;
}

static void __declspec(naked) op_stack_array() {
	_OP_BEGIN(ebp);
	_GET_ARG_R32(ebp, edx, esi); // value
	_GET_ARG_R32(ebp, ebx, edi); // key
	__asm {
		push edx; // arg 4: valueType
		push ebx;
	}
	_CHECK_PARSE_STR(1, ebp, dx, esi);
	__asm {
		pop  ebx;
		mov  ecx, ebx;
	}
	_CHECK_PARSE_STR(2, ebp, bx, edi);
	__asm {
		push esi; // arg 3: value
		push ecx; // arg 2: keyType
		push edi; // arg 1: key
		call StackArray;
	}
	_RET_VAL_INT32(ebp);
	_OP_END;
}

// object LISTS
struct sList {
	TGameObj** obj;
	DWORD len;
	DWORD pos;

	sList(const std::vector<TGameObj*>* vec) : pos(0) {
		len = vec->size();
		obj = new TGameObj*[len];
		for (size_t i = 0; i < len; i++) {
			obj[i] = (*vec)[i];
		}
	}
};

static DWORD listID = 0xCCCCCC; // start ID

struct ListId {
	sList* list;
	DWORD id;

	ListId(sList* lst) : list(lst) {
		id = ++listID;
	}
};
static std::vector<ListId> mList;

static void __stdcall FillListVector(DWORD type, std::vector<TGameObj*>& vec) {
	if (type == 4) return; // LIST_TILES

	vec.reserve(100);
	if (type == 6) { // LIST_SPATIAL
		for (int elev = 0; elev <= 2; elev++) {
			TScript* scriptPtr = fo::func::scr_find_first_at(elev);
			while (scriptPtr != nullptr) {
				TGameObj* self_obj = scriptPtr->selfObject;
				if (self_obj == nullptr) {
					self_obj = fo::func::scr_find_obj_from_program(scriptPtr->program);
				}
				vec.push_back(self_obj);
				scriptPtr = fo::func::scr_find_next_at();
			}
		}
	/*} else if (type == 4) { // LIST_TILES
		DWORD** squares=(DWORD**)FO_VAR_squares;
		for(int elv=0;elv<2;elv++) {
			DWORD* esquares=squares[elv];
			for(int tile=0;tile<10000;tile++) {
				esquares[tile]=0x8f000002;
			}
		}*/
	} else {
		for (int elv = 0; elv < 3; elv++) {
			for (int tile = 0; tile < 40000; tile++) {
				TGameObj* obj = fo::func::obj_find_first_at_tile(elv, tile);
				while (obj) {
					DWORD otype = obj->Type();
					if (type == 9 || (type == 0 && otype == 1) || (type == 1 && otype == 0) || (type >= 2 && type <= 5 && type == otype)) {
						vec.push_back(obj);
					}
					obj = fo::func::obj_find_next_at_tile();
				}
			}
		}
	}
}

static DWORD __stdcall ListAsArray(DWORD type) {
	std::vector<TGameObj*> vec = std::vector<TGameObj*>();
	FillListVector(type, vec);
	size_t sz = vec.size();
	DWORD id = CreateTempArray(sz, 0);
	for (size_t i = 0; i < sz; i++) {
		arrays[id].val[i].set((long)vec[i]);
	}
	return id;
}

static void __stdcall op_list_as_array2() {
	const ScriptValue &typeArg = opHandler.arg(0);

	if (typeArg.isInt()) {
		DWORD arrayId = ListAsArray(typeArg.rawValue());
		opHandler.setReturn(arrayId);
	} else {
		OpcodeInvalidArgs("list_as_array");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_list_as_array() {
	_WRAP_OPCODE(op_list_as_array2, 1, 1)
}

static DWORD __stdcall ListBegin(DWORD type) {
	std::vector<TGameObj*> vec = std::vector<TGameObj*>();
	FillListVector(type, vec);
	sList* list = new sList(&vec);
	mList.push_back(list);
	return listID;
}

static void __stdcall op_list_begin2() {
	const ScriptValue &typeArg = opHandler.arg(0);

	if (typeArg.isInt()) {
		opHandler.setReturn(ListBegin(typeArg.rawValue()));
	} else {
		OpcodeInvalidArgs("list_begin");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_list_begin() {
	_WRAP_OPCODE(op_list_begin2, 1, 1)
}

static TGameObj* __stdcall ListNext(sList* list) {
	return (!list || list->pos == list->len) ? 0 : list->obj[list->pos++];
}

static void __stdcall op_list_next2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		TGameObj* obj = nullptr;
		DWORD id = idArg.rawValue();
		if (id != 0) {
			sList* list = nullptr;
			for (std::vector<ListId>::const_iterator it = mList.cbegin(), it_end =  mList.cend(); it != it_end; ++it) {
				if (it->id == id) {
					list = it->list;
					break;
				}
			}
			obj = ListNext(list);
		}
		opHandler.setReturn(obj);
	} else {
		OpcodeInvalidArgs("list_next");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_list_next() {
	_WRAP_OPCODE(op_list_next2, 1, 1)
}

static void __stdcall ListEnd(DWORD id) {
	if (id == 0) return;
	for (std::vector<ListId>::const_iterator it = mList.cbegin(), it_end = mList.cend(); it != it_end; ++it) {
		if (it->id == id) {
			delete[] it->list->obj;
			delete it->list;
			mList.erase(it);
			break;
		}
	}
}

static void __stdcall op_list_end2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		ListEnd(idArg.rawValue());
	} else {
		OpcodeInvalidArgs("list_end");
	}
}

static void __declspec(naked) op_list_end() {
	_WRAP_OPCODE(op_list_end2, 1, 0)
}
