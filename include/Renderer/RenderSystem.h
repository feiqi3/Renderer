#ifndef RENDER_SYSTEM_H_
#define RENDER_SYSTEM_H_
#include <memory>
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include <render_resource.h>
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
		void BeginRenderFrame();
	public:
		inline Vulkan::rs_context_vk* getRenderContext()const {
			return (Vulkan::rs_context_vk*)mBackEndContext;
		}

		rs_renderpass* createRenderPass(rs_rendertarget* renderTarget,PassDesc& passDescription);
		void destoyRenderPass(rs_renderpass* renderPass);

		rs_pipeline* createRenderPipeline(rs_renderpass* renderpass,PipelineDesc& pipelineDescription);
		void destroyRenderPipeline(rs_pipeline* pipeline);

		rs_image* createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int layersize, int mipmap);

		void beginFrame();

		//In Logic Frame only
		void updateUniformBufferData(rs_binding_pos binding,void* data, uint32_t size, RenderInfo& info);
		void updateUniform(rs_binding_pos binding, rs_buffer* buffer, RenderInfo& info);
		void updateUniform(rs_binding_pos binding, rs_image* image, RenderInfo& info);
		void updateImageData(rs_image* image, void* data, size_t byteSize, int x, int y, int z, int width, int height, int depth, int layerOffset, int layerSize, int mip);
		void updateBufferData(rs_buffer* buffer,void* data,size_t byteSize,size_t dstOffset);
		void* placeFramePendingData(void* data, uint32_t size);
		void submitCmdBuffer(rs_commandbuffer* cmdBuffer,std::vector<rs_semaphore*> wait,std::vector<rs_semaphore*> singal,rs_fence* fence);
		RenderSystem();
		~RenderSystem();
	private:
		uint64_t mCurLogicFrameInFlight = -1;
		uint64_t currentLogicFrame = -1;
		Vulkan::rs_context_vk* mBackEndContext;
		Window::rs_window* mWindow;
		std::unique_ptr<RenderSystemPrivate> mDp;
		bool mUseRenderThread = true;
	private: 
		static RenderSystem* sRenderSystem;
	};
}

#endif