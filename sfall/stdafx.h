#pragma once
#pragma message("Compiling precompiled headers.\n")

#ifdef WIN2K
#define WINVER       _WIN32_WINNT_WIN2K
#define _WIN32_WINNT _WIN32_WINNT_WIN2K
#else
#define WINVER       _WIN32_WINNT_WINXP
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif

#include <algorithm>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

//#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME
#include <Windows.h>
