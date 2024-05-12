/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace HRP
{

class IFaceBar {
public:
	static void init();

	static long IFACE_BAR_MODE;
	static long IFACE_BAR_SIDE_ART;
	static long IFACE_BAR_WIDTH;
	static bool IFACE_BAR_SIDES_ORI;

	static long ALTERNATE_AMMO_METRE;
	static BYTE ALTERNATE_AMMO_LIGHT;
	static BYTE ALTERNATE_AMMO_DARK;

	static long display_width;
	static char* display_string_buf;

	static void Hide();
	static void Show();
};

}
