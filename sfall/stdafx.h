#pragma once
#pragma message("Compiling precompiled headers.\n")

#define WINVER       _WIN32_WINNT_WIN2K
#define _WIN32_WINNT _WIN32_WINNT_WIN2K

#include <algorithm>
#include <functional>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cmath>

//#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME
#include <Windows.h>
