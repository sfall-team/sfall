/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\OpcodeContext.h"

namespace sfall
{
namespace script
{

void op_active_hand();

void op_toggle_active_hand();

void op_set_inven_ap_cost();

void mf_get_inven_ap_cost(OpcodeContext&);

void op_obj_is_carrying_obj(OpcodeContext&);

void mf_critter_inven_obj2(OpcodeContext&);

void mf_item_weight(OpcodeContext&);

void mf_get_current_inven_size(OpcodeContext&);

void mf_unwield_slot(OpcodeContext&);

}
}
