/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace sfall
{

class WinProc {
public:
	static void init();

	static void SetWindowProc();

	static void SetHWND(HWND _window);
	static void SetTitle(long gWidth, long gHeight);
	static void SetSize(long w, long h);

	// Sets the window style and its position
	static void SetStyle(long windowStyle);

	static void SetMoveKeys();
	static void Moving();

	static void SetToCenter(long wWidth, long wHeight, long* outX, long* outY);
	static void LoadPosition();
	static void SavePosition(long mode);

	static const POINT* GetClientPos();
};

}
