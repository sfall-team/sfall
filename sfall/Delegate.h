#pragma once

#include <functional>
#include <vector>

template <typename ...ArgT>
class Delegate {
public:
	using Functor = std::function<void(ArgT...)>;
	using FunctorCollection = std::vector<Functor>;

	Delegate<ArgT...>() {}

	void add(Functor func) {
		_functors.emplace_back(std::move(func));
	}

	void add(const Delegate<ArgT...>& other)
	{
		for (auto& func : other.functors())
		{
			add(func);
		}
	}

	void clear() {
		_functors.clear();
	}

	void invoke(ArgT... args) {
		for (auto& func : _functors) {
			func(args...);
		}
	}

	const FunctorCollection& functors() const {
		return _functors;
	}

	Delegate operator =(Functor func) {
		clear();
		add(std::move(func));
		return *this;
	}

	Delegate operator=(std::nullptr_t) {
		clear();
		return *this;
	}

	Delegate operator +=(Functor func) {
		add(std::move(func));
		return *this;
	}

	Delegate operator +=(const Delegate other) {
		add(other);
		return *this;
	}

private:
	FunctorCollection _functors;

};
