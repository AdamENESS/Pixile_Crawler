#include "IEngine.h"
#include "static_plugin.h"
#include "CMazeGenerator.h"
#include "CInstancedMazeMesh.h"

#ifdef WIN32
BOOL APIENTRY DllMain(HINSTANCE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
#endif

#ifdef WIN32
extern "C" __declspec(dllexport) IEnginePlugin * LoadEnessPlugin(IEngine * pEngine
#ifdef _DEBUG
	, IReferenceCountedAllocationData * pReferenceCountedAllocationData
#endif
)
#else
void LoadEnessPluginPython(IEngine * pEngine
#ifdef _DEBUG
	, IReferenceCountedAllocationData * pReferenceCountedAllocationData
#endif
)
#endif
{
#ifdef _DEBUG
	IReferenceCounted::SetReferenceCountedAllocationData(pReferenceCountedAllocationData);
#endif
	DECLARE_ATTRIBUTE_OBJECT(pEngine->GetAttributeManager(), CMazeGenerator, IAttributeObject);
	DECLARE_ATTRIBUTE_OBJECT(pEngine->GetAttributeManager(), CInstancedMazeMesh, ISceneNode);
#ifdef WIN32
	return NULL;
#endif
}

#ifndef WIN32
static int noop_initialise_plugin = ns::register_plugin(LoadEnessPluginChevron);
#endif
