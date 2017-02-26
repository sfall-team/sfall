#pragma once

#include <memory>
#include <vector>

#include "Modules\Module.h"

// Singleton for managing all of Sfall modules.
class ModuleManager {
public:
	ModuleManager();

	~ModuleManager();

	void initAll();

	template<typename T> 
	void add()
	{
		_modules.emplace_back(new T());
	}

	static ModuleManager& getInstance();

private:
	// disallow copy constructor and copy assignment because we're dealing with unique_ptr's here
	ModuleManager(const ModuleManager&);
	void operator = (const ModuleManager&) {}

	std::vector<std::unique_ptr<Module>> _modules;

	static ModuleManager _instance;
};
