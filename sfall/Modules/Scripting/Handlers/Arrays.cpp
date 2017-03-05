/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"
#include "Utils.h"

#include "Arrays.h"

namespace sfall
{
namespace script
{

void sf_create_array(OpcodeContext& ctx) {
	auto arrayId = CreateArray(ctx.arg(0).asInt(), ctx.arg(1).asInt());
	ctx.setReturn(
		ScriptValue(DataType::INT, arrayId)
	);
}

void sf_set_array(OpcodeContext& ctx) {
	SetArray(
		ctx.arg(0).asInt(),
		ctx.arg(1),
		ctx.arg(2),
		true
	);
}

/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
*/
void sf_get_array(OpcodeContext& ctx) {
	if  (ctx.arg(0).isInt()) {
		ctx.setReturn(
			GetArray(ctx.arg(0).asInt(), ctx.arg(1))
		);
	} else if (ctx.arg(0).isString()) {
		if (ctx.arg(1).isInt()) {
			auto str = Substring(ctx.arg(0).asString(), ctx.arg(1).asInt(), 1);
			ctx.setReturn(str);
		} else {
			ctx.printOpcodeError("get_array - index must be numeric when used on a string.");
		}
	} else {
		ctx.printOpcodeError("get-array - argument 0 must be an array ID or a string.");
	}
}

void sf_free_array(OpcodeContext& ctx) {
	FreeArray(ctx.arg(0).asInt());
}

void sf_len_array(OpcodeContext& ctx) {
	ctx.setReturn(
		LenArray(ctx.arg(0).asInt())
	);
}

void sf_resize_array(OpcodeContext& ctx) {
	ResizeArray(ctx.arg(0).asInt(), ctx.arg(1).asInt());
}

void sf_temp_array(OpcodeContext& ctx) {
	auto arrayId = TempArray(ctx.arg(0).asInt(), ctx.arg(1).asInt());
	ctx.setReturn(
		ScriptValue(DataType::INT, arrayId)
	);
}

void sf_fix_array(OpcodeContext& ctx) {
	FixArray(ctx.arg(0).asInt());
}

void sf_scan_array(OpcodeContext& ctx) {
	ctx.setReturn(
		ScanArray(ctx.arg(0).asInt(), ctx.arg(1))
	);
}

void sf_save_array(OpcodeContext& ctx) {
	SaveArray(ctx.arg(0), ctx.arg(1).asInt());
}

void sf_load_array(OpcodeContext& ctx) {
	ctx.setReturn(
		static_cast<int>(LoadArray(ctx.arg(0)))
	);
}

void sf_get_array_key(OpcodeContext& ctx) {
	ctx.setReturn(
		GetArrayKey(ctx.arg(0).asInt(), ctx.arg(1).asInt())
	);
}

void sf_stack_array(OpcodeContext& ctx) {
	ctx.setReturn(
		static_cast<int>(StackArray(ctx.arg(0), ctx.arg(1)))
	);
}

// object LISTS

struct sList {
	fo::TGameObj** obj;
	DWORD len;
	DWORD pos;

	sList(const std::vector<fo::TGameObj*>* vec) {
		len = vec->size();
		obj = new fo::TGameObj*[len];
		for (size_t i = 0; i < len; i++) {
			obj[i] = (*vec)[i];
		}
		pos = 0;
	}
};

static void FillListVector(DWORD type, std::vector<fo::TGameObj*>& vec) {
	if (type == 6) {
		fo::TScript* scriptPtr;
		fo::TGameObj* self_obj;
		fo::TProgram* programPtr;
		for (int elev = 0; elev <= 2; elev++) {
			scriptPtr = Wrapper::scr_find_first_at(elev);
			while (scriptPtr != nullptr) {
				self_obj = scriptPtr->self_obj;
				if (self_obj == nullptr) {
					programPtr = scriptPtr->program_ptr;
					self_obj = Wrapper::scr_find_obj_from_program(programPtr);
				}
				vec.push_back(self_obj);
				scriptPtr = Wrapper::scr_find_next_at();
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
		for (int elv = 0; elv < 3; elv++) {
			for (int tile = 0; tile < 40000; tile++) {
				fo::TGameObj* obj = Wrapper::obj_find_first_at_tile(elv, tile);
				while (obj) {
					DWORD otype = (obj->pid & 0xff000000) >> 24;
					if (type == 9 || (type == 0 && otype == 1) || (type == 1 && otype == 0) || (type >= 2 && type <= 5 && type == otype)) {
						vec.push_back(obj);
					}
					obj = Wrapper::obj_find_next_at_tile();
				}
			}
		}
	}
}

static void* _stdcall ListBegin(DWORD type) {
	std::vector<fo::TGameObj*> vec = std::vector<fo::TGameObj*>();
	FillListVector(type, vec);
	sList* list = new sList(&vec);
	return list;
}

static DWORD _stdcall ListAsArray(DWORD type) {
	std::vector<fo::TGameObj*> vec = std::vector<fo::TGameObj*>();
	FillListVector(type, vec);
	DWORD id = TempArray(vec.size(), 4);
	for (DWORD i = 0; i < vec.size(); i++) {
		arrays[id].val[i].set((long)vec[i]);
	}
	return id;
}

static fo::TGameObj* _stdcall ListNext(sList* list) {
	if (list->pos == list->len) return 0;
	else return list->obj[list->pos++];
}

static void _stdcall ListEnd(sList* list) {
	delete[] list->obj;
	delete list;
}

void sf_list_begin(OpcodeContext& ctx) {
	auto list = ListBegin(ctx.arg(0).asInt());
	ctx.setReturn(
		ScriptValue(DataType::INT, reinterpret_cast<DWORD>(list))
	);
}

void sf_list_as_array(OpcodeContext& ctx) {
	auto arrayId = ListAsArray(ctx.arg(0).asInt());
	ctx.setReturn(
		ScriptValue(DataType::INT, arrayId)
	);
}

void sf_list_next(OpcodeContext& ctx) {
	// TODO: make it safer
	auto list = reinterpret_cast<sList*>(ctx.arg(0).rawValue());
	ctx.setReturn(
		ListNext(list)
	);
}

void sf_list_end(OpcodeContext& ctx) {
	// TODO: make it safer
	auto list = reinterpret_cast<sList*>(ctx.arg(0).rawValue());
	ListEnd(list);
}

}
}
