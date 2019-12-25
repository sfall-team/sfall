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

#include "Arrays.h"

namespace sfall
{
namespace script
{

void sf_create_array(OpcodeContext& ctx) {
	auto arrayId = CreateArray(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
	ctx.setReturn(arrayId);
}

void sf_set_array(OpcodeContext& ctx) {
	SetArray(
		ctx.arg(0).rawValue(),
		ctx.arg(1),
		ctx.arg(2),
		true
	);
}

/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
	example: vartext[5]
*/
void sf_get_array(OpcodeContext& ctx) {
	if  (ctx.arg(0).isInt()) {
		ctx.setReturn(
			GetArray(ctx.arg(0).rawValue(), ctx.arg(1))
		);
	} else if (ctx.arg(0).isString()) {
		if (ctx.arg(1).isInt()) {
			size_t index = ctx.arg(1).rawValue();
			if (index < strlen(ctx.arg(0).strValue())) {
				wchar_t c = ((unsigned char*)ctx.arg(0).strValue())[index];
				((wchar_t*)ScriptExtender::gTextBuffer)[0] = c;
			} else {
				ScriptExtender::gTextBuffer[0] = '\0';
			}
			ctx.setReturn(ScriptExtender::gTextBuffer); // returns char of string
		} else {
			ctx.printOpcodeError("%s() - index must be numeric when used on a string.", ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError("%s() - argument 0 must be an array ID or a string.", ctx.getOpcodeName());
	}
}

void sf_free_array(OpcodeContext& ctx) {
	FreeArray(ctx.arg(0).rawValue());
}

void sf_len_array(OpcodeContext& ctx) {
	ctx.setReturn(
		LenArray(ctx.arg(0).rawValue())
	);
}

void sf_resize_array(OpcodeContext& ctx) {
	if (ResizeArray(ctx.arg(0).rawValue(), ctx.arg(1).rawValue())) {
		ctx.printOpcodeError("%s() - array sorting error.", ctx.getOpcodeName());
	}
}

void sf_temp_array(OpcodeContext& ctx) {
	auto arrayId = TempArray(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
	ctx.setReturn(arrayId);
}

void sf_fix_array(OpcodeContext& ctx) {
	FixArray(ctx.arg(0).rawValue());
}

void sf_scan_array(OpcodeContext& ctx) {
	ctx.setReturn(
		ScanArray(ctx.arg(0).rawValue(), ctx.arg(1))
	);
}

void sf_save_array(OpcodeContext& ctx) {
	SaveArray(ctx.arg(0), ctx.arg(1).rawValue());
}

void sf_load_array(OpcodeContext& ctx) {
	ctx.setReturn(
		LoadArray(ctx.arg(0))
	);
}

void sf_get_array_key(OpcodeContext& ctx) {
	ctx.setReturn(
		GetArrayKey(ctx.arg(0).rawValue(), ctx.arg(1).rawValue())
	);
}

void sf_stack_array(OpcodeContext& ctx) {
	ctx.setReturn(
		StackArray(ctx.arg(0), ctx.arg(1))
	);
}

// object LISTS
struct sList {
	fo::GameObject** obj;
	DWORD len;
	DWORD pos;

	sList(const std::vector<fo::GameObject*>* vec) : pos(0) {
		len = vec->size();
		obj = new fo::GameObject*[len];
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

static void FillListVector(DWORD type, std::vector<fo::GameObject*>& vec) {
	vec.reserve(100);
	if (type == 6) {
		fo::ScriptInstance* scriptPtr;
		fo::GameObject* self_obj;
		fo::Program* programPtr;
		for (int elev = 0; elev <= 2; elev++) {
			scriptPtr = fo::func::scr_find_first_at(elev);
			while (scriptPtr != nullptr) {
				self_obj = scriptPtr->selfObject;
				if (self_obj == nullptr) {
					programPtr = scriptPtr->program;
					self_obj = fo::func::scr_find_obj_from_program(programPtr);
				}
				vec.push_back(self_obj);
				scriptPtr = fo::func::scr_find_next_at();
			}
		}
	/*} else if (type == 4) {
		// TODO: verify code correctness
		for(int elv=0;elv<2;elv++) {
			DWORD* esquares = &fo::var::squares[elv];
			for(int tile=0;tile<10000;tile++) {
				esquares[tile]=0x8f000002;
			}
		}*/
	} else if (type != 4) {
		for (int elv = 0; elv < 3; elv++) {
			for (int tile = 0; tile < 40000; tile++) {
				fo::GameObject* obj = fo::func::obj_find_first_at_tile(elv, tile);
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

static DWORD ListAsArray(DWORD type) {
	std::vector<fo::GameObject*> vec = std::vector<fo::GameObject*>();
	FillListVector(type, vec);
	size_t sz = vec.size();
	DWORD id = TempArray(sz, 0);
	for (size_t i = 0; i < sz; i++) {
		arrays[id].val[i].set((long)vec[i]);
	}
	return id;
}

void sf_list_as_array(OpcodeContext& ctx) {
	auto arrayId = ListAsArray(ctx.arg(0).rawValue());
	ctx.setReturn(arrayId);
}

static DWORD ListBegin(DWORD type) {
	std::vector<fo::GameObject*> vec = std::vector<fo::GameObject*>();
	FillListVector(type, vec);
	sList* list = new sList(&vec);
	mList.emplace_back(list);
	return listID;
}

void sf_list_begin(OpcodeContext& ctx) {
	ctx.setReturn(ListBegin(ctx.arg(0).rawValue()));
}

static fo::GameObject* ListNext(sList* list) {
	return (!list || list->pos == list->len) ? 0 : list->obj[list->pos++];
}

void sf_list_next(OpcodeContext& ctx) {
	fo::GameObject* obj = nullptr;
	auto id = ctx.arg(0).rawValue();
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
	ctx.setReturn(obj);
}

static void ListEnd(DWORD id) {
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

void sf_list_end(OpcodeContext& ctx) {
	ListEnd(ctx.arg(0).rawValue());
}

}
}
