#pragma once

#include <vector>

#include "Modules\Module.h"

namespace sfall
{

// Singleton for managing all of sfall modules.
class ModuleManager {
public:
	ModuleManager();

	~ModuleManager();

	void initAll();

	template <typename T>
	void add()
	{
		// About emplace_back in VS2010, see: https://stackoverflow.com/q/4303513
		_modules.emplace_back(new T());
	}

	static ModuleManager& instance() {
		static ModuleManager instance;
		return instance;
	}

private:
	// disallow copy constructor and copy assignment
	ModuleManager(const ModuleManager&);
	void operator = (const ModuleManager&);

	std::vector<Module*> _modules;
};

}
