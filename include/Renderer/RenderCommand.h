#ifndef RENDER_COMMAND_H_
#define RENDER_COMMAND_H_
#include "render_resource_createinfo.h"
#include "common/CoreDefs.h"
namespace Render {
	struct rs_pipeline;
	struct rs_drawdata;
	
	struct RenderCommand {
		//1. Pipeline to use
		rs_pipeline* pipeline;
		//2. Draw data
		rs_drawdata* drawData;
		//3. Render state 
		RenderInfo renderInfo;
		//4. Render mask
		u64 renderMask;
	};
}

#endif