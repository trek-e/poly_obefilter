#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;
	// Models will be added here
	p->addModel(modelHydraQuartetVCF);
}
