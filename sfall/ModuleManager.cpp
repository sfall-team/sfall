#include "Logging.h"

#include "ModuleManager.h"

namespace sfall 
{

ModuleManager::ModuleManager() {
}

ModuleManager::~ModuleManager() {
	for (std::vector<Module*>::const_iterator it = _modules.cbegin(); it != _modules.cend(); ++it) {
		(*it)->exit();
		delete *it;
	}
}

void ModuleManager::initAll() {
	for (std::vector<Module*>::const_iterator it = _modules.cbegin(); it != _modules.cend(); ++it) {
		dlog_f("Initializing module %s...\n", DL_INIT, (*it)->name());
		(*it)->init();
	}
}

}
