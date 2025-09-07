#include "Renderer/RenderDebuger.h"
#include "vulkan/vulkan_render_function.h"
namespace Render {
	RenderMarker::RenderMarker(rs_commandbuffer* cmd, const std::string& label, float r, float g, float b, float a) : mCommandBuffer(cmd)
	{
		Vulkan::cmdBeginMark((Vulkan::rs_commandbuffer_vk*)mCommandBuffer, label, r, g, b, a);
	}

	RenderMarker::~RenderMarker()
	{
		Vulkan::cmdEndMark((Vulkan::rs_commandbuffer_vk*)mCommandBuffer);
	}

	void RenderMarker::insertMarker(rs_commandbuffer* cmd, const std::string& label, float r, float g, float b, float a) {
		Vulkan::cmdInsertMark((Vulkan::rs_commandbuffer_vk*)mCommandBuffer, label, r, g, b, a);
	}

}
