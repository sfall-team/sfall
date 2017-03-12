#pragma once

#include "Module.h"

namespace sfall
{

class HighlightingMod : public Module
{
public:
	const char* name() { return "HighlightingMod"; }
	
	void init();
};

}