#pragma once
#pragma message("Compiling precompiled headers.\n")

#ifdef WIN2K
#define WINVER       0x0500
#define _WIN32_WINNT 0x0500
#else
#define WINVER       0x0501
#define _WIN32_WINNT 0x0501
#endif

#include <algorithm>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <Windows.h>
