#include "Logging.h"

#include "ModuleManager.h"

namespace sfall 
{

ModuleManager::ModuleManager() {
}

ModuleManager::~ModuleManager() {
	for (auto it = _modules.cbegin(); it != _modules.cend(); ++it) {
		(*it)->exit();
	}
}

void ModuleManager::initAll() {
	for (auto it = _modules.cbegin(); it != _modules.cend(); ++it) {
		dlog_f("Initializing module %s...\n", DL_INIT, (*it)->name());
		(*it)->init();
	}
}

}
