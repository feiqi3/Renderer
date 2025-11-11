#include "RenderPass.h"

namespace Render {
	class SwapchainRenderPassHelper;
	class RenderPassManager {
	public:
		RenderPass* getRenderPass(const std::string& passName);
		~RenderPassManager();
		RenderPassManager();
		void onFrameBegin();
		void markSwapchainRenderPass(RenderPass* pass);
		void unMarkSwapchainRenderPass(RenderPass* pass);
	private:
		friend class RenderPass;
		void addRenderPass(const std::string& passName, RenderPass* pass);
		void removeRenderPass(const std::string& name);
		std::unordered_map<std::string, RenderPass*> mRenderPasses;
		void destroyRenderPass(RenderPass* pass);
		SwapchainRenderPassHelper* mSwapchainRpHelper = 0;
	};
}