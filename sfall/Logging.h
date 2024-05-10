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

#define DL_MAIN      (0)
#define DL_INIT      (0x1)
#define DL_HOOK      (0x2)
#define DL_SCRIPT    (0x4)
#define DL_CRITICALS (0x8)
#define DL_FIX       (0x10)

#ifndef NO_SFALL_DEBUG
#include <stdio.h>
#include <string>

namespace sfall
{

void dlog(const char* msg, int type);
void dlog(const std::string& msg, int type);
void dlogr(const char* msg, int type);
void dlogr(const std::string& msg, int type);
void dlog_f(const char* fmt, int type, ...);

#ifndef NDEBUG
// Prints debug message to sfall log file for develop build
void devlog_f(const char* fmt, int type, ...);
#else
#define devlog_f(...) ((void)0)
#endif

void LoggingInit();

}
#else
#define dlog(a,b)
#define dlogr(a,b)
#define dlog_f(a, b, ...)
#endif
