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

namespace sfall
{
namespace script
{

void op_get_perk_owed();

void op_set_perk_owed();

void op_set_perk_freq();

void op_get_perk_available(OpcodeContext&);

void op_set_perk_name(OpcodeContext&);

void op_set_perk_desc(OpcodeContext&);

void op_set_perk_value();

void op_set_selectable_perk(OpcodeContext&);

void op_set_fake_perk(OpcodeContext&);

void op_set_fake_trait(OpcodeContext&);

void mf_set_selectable_perk_npc(OpcodeContext&);

void mf_set_fake_perk_npc(OpcodeContext&);

void mf_set_fake_trait_npc(OpcodeContext&);

void op_set_perkbox_title();

void op_hide_real_perks();

void op_show_real_perks();

void op_clear_selectable_perks();

void op_has_fake_perk(OpcodeContext&);

void op_has_fake_trait(OpcodeContext&);

void mf_has_fake_perk_npc(OpcodeContext&);

void mf_has_fake_trait_npc(OpcodeContext&);

void op_perk_add_mode();

void op_remove_trait();

void op_set_pyromaniac_mod();

void op_apply_heaveho_fix();

void op_set_swiftlearner_mod();

void op_set_perk_level_mod();

void mf_add_trait(OpcodeContext&);

}
}
