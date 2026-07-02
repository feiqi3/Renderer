#include "common/Name.h"
#include "RenderPass.h"
#include <vector>
namespace Render {
	class RenderPassManager {
	public:
		RenderPass* getRenderPass(const Name& passName);
		std::vector<Name> getAllRenderPassNames()const;
		~RenderPassManager();
		RenderPassManager();
		void onFrameBegin();
		
		void registerRenderPass(RenderPass* pass);
		void unregisterRenderPass(RenderPass* pass);

	private:
		friend class RenderPass;
		void addRenderPass(const Name& passName, RenderPass* pass);
		void removeRenderPass(const Name& name);
		std::unordered_map<Name, RenderPass*> mRenderPasses;
		void destroyRenderPass(RenderPass* pass);
	private: 
		RenderPass* mVirtualRenderPass = nullptr;
	};
}