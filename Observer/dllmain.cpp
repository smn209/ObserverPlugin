#include <Windows.h>
#include "ObserverPlugin.h"

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ObserverPlugin instance;
    return &instance;
}

// the actual DllMain is defined in Base plugin (from GWToolbox)
// this file exists only to satisfy the build system

// note : not sure of this is needed, but it's here to avoid errors