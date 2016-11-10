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

static void __declspec(naked) create_array() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp bx, 0xc001;
		jne fail;
		cmp dx, 0xc001;
		jne fail;
		push ecx;
		push eax;
		call CreateArray;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) set_array() {
	__asm {
		pushad;
		mov ebp, eax;
		//Get args
		call FuncOffs::interpretPopShort_
		mov edx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov edi, eax; // value
		mov eax, ebp;
		call FuncOffs::interpretPopShort_
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov esi, eax; // key
		mov eax, ebp;
		call FuncOffs::interpretPopShort_
		mov ebx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		xchg eax, edi; // arrayID
		//Error check:
		cmp bx, 0xC001;
		jne end;
		push 1; // arg 6: allow unset
		push edx; // arg 5: value type
		// value:
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz notstring;
next:
		mov ebx, eax;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
notstring:
		push eax; // arg 4: value
		// key:
		cmp cx, 0x9001;
		jz next1;
		cmp cx, 0x9801;
		jnz notstring1;
next1:
		mov edx, ecx;
		mov ebx, esi;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
		mov esi, eax;
notstring1:
		push ecx; // arg 3: key type
		push esi; // arg 2: key
		push edi; // arg 1: arrayID
		call SetArray
end:
		popad;
		retn;
	}
}
/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
*/
static void __declspec(naked) get_array() {
	__asm {
		pushad;
		mov ebp, eax;
		//Get args
		call FuncOffs::interpretPopShort_
		mov ebx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, ebp;
		// check first argument type
		cmp cx, VAR_TYPE_STR;
		je callsubstr;
		cmp cx, VAR_TYPE_STR2;
		jne proceedgetarray;
callsubstr:
		// use substr instead of get_array when used on string
		// in this case, check for second argument to be numeric
		cmp bx, VAR_TYPE_INT;
		jne fail;
		mov eax, ebp;
		mov ebx, esi;
		mov edx, ecx;
		call FuncOffs::interpretGetString_;
		push 1;
		push edi;
		push eax;
		call mysubstr;
		mov edx, eax; // result substring
		mov ebx, VAR_TYPE_STR; // result type
		jmp end;

proceedgetarray:
		cmp cx, VAR_TYPE_INT; // only int is allowed for arrayID in this case
		jne fail;
		cmp bx, VAR_TYPE_STR;
		je next1;
		cmp bx, VAR_TYPE_STR2;
		jne notstring1;
next1:
		mov ecx, ebx;
		mov edx, ebx;
		mov ebx, edi;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
		mov edi, eax;
		mov ebx, ecx;
notstring1:
		mov eax, esp; // ptr to resultType (will be changed in GetArray)
		push eax;
		push ebx;
		push edi;
		push esi;
		call GetArray;
		mov edx, eax; // result data
		mov ebx, [esp]; // resultType
		jmp end;
fail:
		xor edx, edx;
		mov ebx, 0xc001;
end:
		cmp bx, 0x9801;
		jne notstring;
		mov eax, ebp;
		call FuncOffs::interpretAddString_;
		mov edx, eax;
notstring:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov edx, ebx;
		mov eax, ebp;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) free_array() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp bx, 0xc001;
		jne end;
		push eax;
		call FreeArray;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) len_array() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp bx, 0xc001;
		jne fail;
		push eax;
		call LenArray;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) resize_array() {
	__asm {
		pushad;
		mov ebp, eax;
		//Get args
		call FuncOffs::interpretPopShort_
		mov ebx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, ebp;
		//Error check
		cmp bx, 0xc001;
		jne end;
		cmp cx, 0xc001;
		jne end;
		push edi;
		push esi;
		call ResizeArray;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) temp_array() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp bx, 0xc001;
		jne fail;
		cmp dx, 0xc001;
		jne fail;
		push ecx;
		push eax;
		call TempArray;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) fix_array() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp bx, 0xc001;
		jne end;
		push eax;
		call FixArray;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) scan_array() {
		__asm {
		pushad;
		mov ebp, eax;
		//Get args
		call FuncOffs::interpretPopShort_
		mov edx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov edi, eax; // value (needle)
		mov eax, ebp;
		call FuncOffs::interpretPopShort_
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov esi, eax; // arrayID (haystack)
		mov eax, ebp;
		//Error check
		cmp cx, VAR_TYPE_INT;
		jne fail;
		cmp dx, VAR_TYPE_STR;
		je getstringvar;
		cmp dx, VAR_TYPE_STR2;
		jne success;
getstringvar:
		mov ebx, edi;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
		mov edi, eax;
success:
		push esp;
		push edx;
		push edi;
		push esi;
		call ScanArray;
		mov edx, eax; // result
		mov ebx, [esp]; // resultType
		jmp end;
fail:
		mov ebx, VAR_TYPE_INT;
		xor edx, edx;
		dec edx; // returns -1
end:
		cmp ebx, VAR_TYPE_STR;
		jne resultnotstr;
		mov eax, ebp;
		call FuncOffs::interpretAddString_;
		mov edx, eax; // str ptr
resultnotstr:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov edx, ebx;
		mov eax, ebp;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_save_array() {
	_OP_BEGIN(ebp)
		// Get args
	_GET_ARG_R32(ebp, ebx, edi)
	_GET_ARG_R32(ebp, edx, ecx)
	__asm {
		// arg check:
		cmp bx, VAR_TYPE_INT
		jne end
	}
	_CHECK_PARSE_STR(1, ebp, dx, ecx)
	__asm {
		push edi // arg 3: arrayID
		push edx // arg 2: keyType
		push ecx // arg 1: key
		call SaveArray
	}
end:
	_OP_END
}

static void __declspec(naked) op_load_array() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, edx, ecx)
	_CHECK_PARSE_STR(1, ebp, dx, ecx)
	__asm {
		push edx; // arg 2: keyType
		push ecx; // arg 1: key
		call LoadArray;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) op_get_array_key() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, edx, ecx) // index
	_GET_ARG_R32(ebp, ebx, edi) // arrayID
	__asm {
		// arg check:
		cmp bx, VAR_TYPE_INT;
		jne wrongarg;
		cmp dx, VAR_TYPE_INT;
		jne wrongarg;
		push esp; // arg 3: *resultType
		push ecx; // arg 2: index
		push edi; // arg 1: arrayID
		call GetArrayKey;
	}
	_RET_VAL_POSSIBLY_STR(1, ebp, [esp])
	goto end;
	__asm {
wrongarg:
		xor eax, eax; // return 0 on wrong arguments
	}
	_RET_VAL_INT(ebp)
end:
	_OP_END
}

static void __declspec(naked) op_stack_array() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, edx, esi) // value
	_GET_ARG_R32(ebp, ebx, edi) // key
	__asm {
		push edx // arg 4: valueType
		push ebx
	}
	_CHECK_PARSE_STR(1, ebp, dx, esi)
	__asm {
		pop ebx
		mov ecx, ebx
	}
	_CHECK_PARSE_STR(2, ebp, bx, edi)
	__asm {
		push esi // arg 3: value
		push ecx // arg 2: keyType
		push edi // arg 1: key
		call StackArray
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

// object LISTS

struct sList {
	DWORD* obj;
	DWORD len;
	DWORD pos;

	sList(const std::vector<DWORD>* vec) {
		len=vec->size();
		obj=new DWORD[len];
		for(DWORD i=0;i<len;i++) obj[i]=(*vec)[i];
		pos=0;
	}
};

static void FillListVector(DWORD type, std::vector<DWORD>& vec) {
	// TODO: fix style, use wrappers
	if (type == 6) {
		DWORD scriptPtr, self_obj, programPtr;
		for (int elev=0; elev<=2; elev++) {
			__asm {
				mov eax, elev;
				call FuncOffs::scr_find_first_at_;
				mov scriptPtr, eax;
			}
			while (scriptPtr != 0) {
				self_obj = *(DWORD*)(scriptPtr + 0x34);
				if (self_obj == 0) {
					programPtr = *(DWORD*)(scriptPtr + 0x18);
					__asm {
						mov eax, programPtr;
						call FuncOffs::scr_find_obj_from_program_;
						mov self_obj, eax;
					}
				}
				vec.push_back(self_obj);
				__asm {
					call FuncOffs::scr_find_next_at_;
					mov scriptPtr, eax;
				}
			}
		}
	} else if (type == 4) {
		// TODO: verify code correctness

		/*for(int elv=0;elv<2;elv++) {
			DWORD* esquares = &VarPtr::squares[elv];
			for(int tile=0;tile<10000;tile++) {
				esquares[tile]=0x8f000002;
			}
		}*/
		
	} else {
		for(int elv=0;elv<3;elv++) {
			for(int tile=0;tile<40000;tile++) {
				DWORD obj;
				__asm {
					mov edx, tile;
					mov eax, elv;
					call FuncOffs::obj_find_first_at_tile_;
					mov obj, eax;
				}
				while(obj) {
					DWORD otype = ((DWORD*)obj)[25];
					otype = (otype&0xff000000) >> 24;
					if(type==9 || (type==0&&otype==1) || (type==1&&otype==0) || (type>=2&&type<=5&&type==otype)) vec.push_back(obj);
					__asm {
						call FuncOffs::obj_find_next_at_tile_;
						mov obj, eax;
					}
				}
			}
		}
	}
}
static void* _stdcall list_begin2(DWORD type) {
	std::vector<DWORD> vec = std::vector<DWORD>();
	FillListVector(type, vec);
	sList* list=new sList(&vec);
	return list;
}
static DWORD _stdcall list_as_array2(DWORD type) {
	std::vector<DWORD> vec = std::vector<DWORD>();
	FillListVector(type, vec);
	DWORD id=TempArray(vec.size(), 4);
	for(DWORD i=0;i<vec.size();i++) {
		arrays[id].val[i].set((long)vec[i]);
	}
	return id;
}
static DWORD _stdcall list_next2(sList* list) {
	if(list->pos==list->len) return 0;
	else return list->obj[list->pos++];
}
static void _stdcall list_end2(sList* list) {
	delete[] list->obj;
	delete list;
}
static void __declspec(naked) list_begin() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		push eax;
		call list_begin2;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) list_as_array() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		push eax;
		call list_as_array2;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) list_next() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		push eax;
		call list_next2;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) list_end() {
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
		call list_end2;
end:
		popad;
		retn;
	}
}
