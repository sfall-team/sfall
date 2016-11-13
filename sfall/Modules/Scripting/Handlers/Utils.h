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

#pragma once

void __declspec() op_sqrt();

void __declspec() op_abs();

void __declspec() op_sin();

void __declspec() op_cos();

void __declspec() op_tan();

void __declspec() op_arctan();

void __declspec() op_string_split();

void __declspec() op_atoi();

void __declspec() op_atof();

void __declspec() op_substr();

void __declspec() op_strlen();

void __declspec() op_sprintf();

void __declspec() op_ord();

void __declspec() op_typeof();

void __declspec() NegateFixHook();
void sf_power();

void __declspec() op_power();

void sf_log();

void __declspec() op_log();

void sf_exponent();

void __declspec() op_exponent();

void sf_ceil();

void __declspec() op_ceil();

void sf_round();

void __declspec() op_round();

void _stdcall sf_message_str_game();

void __declspec() op_message_str_game();

char* _stdcall mysubstr(char* str, int pos, int length);
