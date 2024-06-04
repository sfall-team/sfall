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

class OpcodeContext;

void ClearInterfaceArtCache();

// input_functions
void op_input_funcs_available();

void op_set_pipboy_available(OpcodeContext&);

void PipboyAvailableRestore();

void op_key_pressed(OpcodeContext&);

void op_tap_key();

//// *** From helios *** ////
void op_get_mouse_x();

//Return mouse y position
void op_get_mouse_y();

//Return pressed mouse button (1=left, 2=right, 3=left+right, 4=middle)
void op_get_mouse_buttons(OpcodeContext&);

//Return the window number under the mous
void op_get_window_under_mouse();

//Return screen width
void op_get_screen_width();

//Return screen height
void op_get_screen_height();

//Create a message window with given string
void op_create_message_window(OpcodeContext&);

void mf_message_box(OpcodeContext&);

void op_get_viewport_x();

void op_get_viewport_y();

void op_set_viewport_x();

void op_set_viewport_y();

void mf_add_iface_tag(OpcodeContext&);

void op_show_iface_tag(OpcodeContext&);

void op_hide_iface_tag(OpcodeContext&);

void op_is_iface_tag_active(OpcodeContext&);

void mf_intface_redraw(OpcodeContext&);

void mf_intface_show(OpcodeContext&);

void mf_intface_hide(OpcodeContext&);

void mf_intface_is_hidden(OpcodeContext&);

void mf_tile_refresh_display(OpcodeContext&);

void mf_get_cursor_mode(OpcodeContext&);

void mf_set_cursor_mode(OpcodeContext&);

void mf_display_stats(OpcodeContext&);

void mf_set_iface_tag_text(OpcodeContext&);

void mf_inventory_redraw(OpcodeContext&);

void mf_dialog_message(OpcodeContext&);

void mf_create_win(OpcodeContext&);

void mf_show_window(OpcodeContext&);

void mf_hide_window(OpcodeContext&);

void mf_set_window_flag(OpcodeContext&);

void mf_draw_image(OpcodeContext&);

void mf_draw_image_scaled(OpcodeContext&);

void mf_art_frame_data(OpcodeContext&);

void mf_interface_art_draw(OpcodeContext&);

void mf_get_window_attribute(OpcodeContext&);

void mf_interface_print(OpcodeContext&);

void mf_win_fill_color(OpcodeContext&);

void mf_interface_overlay(OpcodeContext&);

}
}
