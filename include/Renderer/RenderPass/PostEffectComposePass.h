#ifndef POST_EFFECT_COMPOSE_PASS
#define POST_EFFECT_COMPOSE_PASS

#include "Renderer/RenderPass.h"
#include "Renderer/Texture.h"
#include "Renderer/GPUShared/PostEffectData.h"
namespace Render {

	class PostEffectComposePass :public RenderPass {
	public:
		PostEffectComposePass();
		~PostEffectComposePass();
		void setBloomTex(const TexturePtr& tex);
		void setMainRTColorTex(const TexturePtr& tex);
		void setBloomStrength(float strength);
		void collectRenderEntities(std::vector<RenderPack>& pack);
		void init() override;
	public:
		virtual void drawImpl(rs_commandbuffer* cmdbuffer);
	private:
		class PostEffectComposeEntity* entity = nullptr;
		GPUShared::PostEffectConfig mPostEffectCfg = {};
		rs_buffer* mPostEffectCfgBuffer = nullptr;
	};

};

#endif //POST_EFFECT_COMPOSE_PASS