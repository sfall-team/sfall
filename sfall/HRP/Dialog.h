/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace HRP
{

class Dialog {
public:
	static void init();

	static bool DIALOG_SCRN_ART_FIX;
	static bool DIALOG_SCRN_BACKGROUND;

	static void SetDialogExpandedHeight(long height);
};

}
