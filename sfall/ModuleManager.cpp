#include "ModuleManager.h"

ModuleManager ModuleManager::_instance;

ModuleManager::ModuleManager() {
}

ModuleManager::~ModuleManager() {
	for (auto it = _modules.cbegin(); it != _modules.cend(); it++) {
		(*it)->exit();
	}
}

void ModuleManager::initAll() {
	for (auto it = _modules.cbegin(); it != _modules.cend(); it++) {
		(*it)->init();
	}
}

ModuleManager& ModuleManager::getInstance() {
	return _instance;
}
