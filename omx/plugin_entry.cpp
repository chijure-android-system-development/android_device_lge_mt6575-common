/*
 * OMXMaster looks these up with dlsym(). They live apart from
 * omx_wrapper.cpp because HardwareAPI.h declares createOMXPlugin() with
 * C++ linkage.
 */
#include <media/hardware/OMXPluginBase.h>

namespace android {
OMXPluginBase *createMT6575OMXPlugin();
}

extern "C" android::OMXPluginBase *createOMXPlugin()
{
    return android::createMT6575OMXPlugin();
}

extern "C" void destroyOMXPlugin(android::OMXPluginBase *plugin)
{
    delete plugin;
}
