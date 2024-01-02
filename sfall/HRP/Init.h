/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace HRP
{

class Setting {
public:
	static void init(const char*, std::string&);

	static DWORD GetAddress(DWORD addr);
	static bool VersionIsValid; // HRP 4.1.8 version validation
	static bool CheckExternalPatch();
	static bool ExternalEnabled();

	// Built-in High Resolution Patch
	static bool IsEnabled();

	static long ScreenWidth();
	static long ScreenHeight();
	static long ColorBits();
	static char ScaleX2();
};

}
