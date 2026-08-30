#include "renderdoc/renderdoc_inject.h"
#include "render_log.h"
#if defined(WIN32) && (defined(DEBUG) || defined(_DEBUG))
#include "windows.h"
#include "renderdoc/renderdoc_app.h"
RENDERDOC_API_1_5_0* rdoc_api = NULL;
void Render::TryInjectRenderdocDylib()
{
	auto moduleHandle = LoadLibrary("renderdoc.dll");
	if (moduleHandle != nullptr) {
		Log::warn("Renderdoc is injected!");
		pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			(pRENDERDOC_GetAPI)GetProcAddress(moduleHandle, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void**)&rdoc_api);
		if(rdoc_api){
			rdoc_api->SetCaptureOptionU32(
				RENDERDOC_CaptureOption::eRENDERDOC_Option_DebugOutputMute, 0
			);
		}
		else {
			Log::warn("Failed to load renderdoc api.");
		}


	}
	else {
		Log::warn("Renderdoc failed to inject! Make sure renderdoc.dll is in $env");
	}
}
#else

void Render::TryInjectRenderdocDylib()
{
	void;
}

#endif//WIN32 && (DEBUG || _DEBUG)


