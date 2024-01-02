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

#pragma once

#include <functional>
#include <vector>

namespace sfall 
{

// Simple multi-cast Delegate implementation.
// Removing specific functions after they were added is not supported yet.
template <typename ...ArgT>
class Delegate {
public:
	// The type of functions that can be stored in this delegate.
	using Functor = std::function<void(ArgT...)>;
	// Function collection type.
	using FunctorCollection = std::vector<Functor>;

	// Creates an empty delegate.
	Delegate<ArgT...>() {}

	// Add a function to the delegate
	void add(Functor func) {
		_functors.emplace_back(std::move(func));
	}

	// Add all functions from another delegate of the same type.
	void add(const Delegate<ArgT...>& other)
	{
		for (auto& func : other.functors())
		{
			add(func);
		}
	}

	// Remove all functions from this delegate.
	void clear() {
		_functors.clear();
	}

	// Invoke all functions in the delegate, passing the arguments provided (if any).
	void invoke(ArgT... args) {
		for (auto& func : _functors) {
			func(args...);
		}
	}

	// The list of currently registered delegates.
	const FunctorCollection& functors() const {
		return _functors;
	}

	// Replaces the current list of functions with single function provided.
	//Delegate operator =(Functor func) {
	//	clear();
	//	add(std::move(func));
	//	return *this;
	//}

	// Removes all functions.
	Delegate operator=(std::nullptr_t) {
		clear();
		return *this;
	}

	// Adds a single function to the list.
	Delegate operator +=(Functor func) {
		add(std::move(func));
		return *this;
	}

	// Adds all functions from another delegate.
	Delegate operator +=(const Delegate other) {
		add(other);
		return *this;
	}

private:
	// The list of functions
	FunctorCollection _functors;

};

}
