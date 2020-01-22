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

namespace sfall
{
namespace script
{

class OpcodeContext;

// input_functions
void __declspec() op_input_funcs_available();

void sf_key_pressed(OpcodeContext& ctx);

void __declspec() op_tap_key();

//// *** From helios *** ////
void __declspec() op_get_mouse_x();

//Return mouse y position
void __declspec() op_get_mouse_y();

//Return pressed mouse button (1=left, 2=right, 3=left+right, 4=middle)
void sf_get_mouse_buttons(OpcodeContext&);

//Return the window number under the mous
void __declspec() op_get_window_under_mouse();

//Return screen width
void __declspec() op_get_screen_width();

//Return screen height
void __declspec() op_get_screen_height();

//Stop game, the same effect as open charsscreen or inventory
void __declspec() op_stop_game();

//Resume the game when it is stopped
void __declspec() op_resume_game();

//Create a message window with given string
void sf_create_message_window(OpcodeContext&);

void __declspec() op_get_viewport_x();

void __declspec() op_get_viewport_y();

void __declspec() op_set_viewport_x();

void __declspec() op_set_viewport_y();

void sf_add_iface_tag(OpcodeContext&);

void sf_show_iface_tag(OpcodeContext&);

void sf_hide_iface_tag(OpcodeContext&);

void sf_is_iface_tag_active(OpcodeContext&);

void sf_intface_redraw(OpcodeContext&);

void sf_intface_show(OpcodeContext&);

void sf_intface_hide(OpcodeContext&);

void sf_intface_is_hidden(OpcodeContext&);

void sf_tile_refresh_display(OpcodeContext&);

void sf_get_cursor_mode(OpcodeContext&);

void sf_set_cursor_mode(OpcodeContext&);

void sf_display_stats(OpcodeContext&);

void sf_set_iface_tag_text(OpcodeContext&);

void sf_inventory_redraw(OpcodeContext&);

void sf_dialog_message(OpcodeContext&);

void sf_create_win(OpcodeContext&);

void sf_show_window(OpcodeContext&);

void sf_hide_window(OpcodeContext&);

void sf_set_window_flag(OpcodeContext&);

void sf_draw_image(OpcodeContext&);

void sf_draw_image_scaled(OpcodeContext&);

void sf_unwield_slot(OpcodeContext&);

void sf_get_window_attribute(OpcodeContext&);

}
}
