#include "common/Name.h"
#include "RenderPass.h"
namespace Render {
	class SwapchainRenderPassHelper;
	class RenderPassManager {
	public:
		RenderPass* getRenderPass(const Name& passName);
		~RenderPassManager();
		RenderPassManager();
		void onFrameBegin();
		
		void registerRenderPass(RenderPass* pass);
		void unregisterRenderPass(RenderPass* pass);

		void markSwapchainRenderPass(RenderPass* pass);
		void unMarkSwapchainRenderPass(RenderPass* pass);
		void onSwapchainRebuild();
	private:
		friend class RenderPass;
		void addRenderPass(const Name& passName, RenderPass* pass);
		void removeRenderPass(const Name& name);
		std::unordered_map<Name, RenderPass*> mRenderPasses;
		void destroyRenderPass(RenderPass* pass);
		SwapchainRenderPassHelper* mSwapchainRpHelper = 0;
	private: 
		RenderPass* mVirtualRenderPass = nullptr;
	};
}