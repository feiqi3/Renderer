#ifndef RENDER_SYSTEM_H_
#define RENDER_SYSTEM_H_
#include <memory>
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
namespace Render{
	namespace Window {
		class rs_window;
	}
	namespace Vulkan {
		struct rs_context_vk;
	}
	class RenderSystemPrivate;
	class RenderSystem:public Common::NonCopyable {
	public:
		static void createRenderSystem(const BackEndInitDesc& backEndDesc, Window::rs_window* window);
		static void destroyRenderSystem();
		static RenderSystem* instance();

	public:
		inline Vulkan::rs_context_vk* getRenderContext()const {
			return (Vulkan::rs_context_vk*)mBackEndContext;
		}

		void beginFrame();

		//In Logic Frame only
		void* placeFramePendingData(void* data, uint32_t size);
		RenderSystem();
		~RenderSystem();
	private:
		uint64_t currentLogicFrame = -1;
		Vulkan::rs_context_vk* mBackEndContext;
		Window::rs_window* mWindow;
		std::unique_ptr<RenderSystemPrivate> mDp;
	private: 
		static RenderSystem* sRenderSystem;
	};
}

#endif