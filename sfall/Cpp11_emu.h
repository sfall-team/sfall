/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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

#include <cstddef>

// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/nullptr
const // It is a const object...
class nullptr_t {
	public:
		template<class T>
		inline operator T*() const // convertible to any type of null non-member pointer...
		{ return 0; }

		template<class C, class T>
		inline operator T C::*() const // or any type of null member pointer...
		{ return 0; }

	private:
		void operator&() const; // Can't take address of nullptr
} nullptr = {};

// http://stackoverflow.com/questions/33026118/c-stdbeginc-for-vs-2008
template<typename T, std::size_t N>
T* std_begin(T (&a)[N]) { return a; }

template<typename T, std::size_t N>
T* std_end(T (&a)[N]) { return a + N; }
