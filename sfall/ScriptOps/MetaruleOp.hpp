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

#include <unordered_map>

#include "main.h"
#include "ScriptExtender.h"

static std::unordered_map<const char*, void(*)()> sfall_metarule_table;

static void sf_test() {
	DisplayConsoleMessage("Testing 1 2 3!");
}

static void InitMetaruleTable() {
	sfall_metarule_table["test"] = sf_test;
}

static void _stdcall op_sfall_metatule3_handler() {
	DWORD num = 0;
	if (IsOpArgStr(0)) {
		auto name = GetOpArgStr(0);
		auto lookup = sfall_metarule_table.find(name);
		if (lookup != sfall_metarule_table.end()) {
			lookup->second();
		} else {
			dlog_f("sfall_metarule3(name, a, b, c) - name '%s' is unknown.", DL_INIT, name);
		}
	} else {		
		dlog("sfall_metarule3(name, a, b, c) - name must be string.", DL_INIT);
	}
	SetOpReturn(num, DATATYPE_INT);
}

static void __declspec(naked) op_sfall_metatule3() {
	_WRAP_OPCODE(4, op_sfall_metatule3_handler)
}
