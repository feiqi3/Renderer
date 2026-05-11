#ifndef POST_EFFECT_COMPOSE_PASS
#define POST_EFFECT_COMPOSE_PASS

#include"Renderer/RenderPass.h"
namespace Render {

	class PostEffectComposePass :public RenderPass {
	public:
		PostEffectComposePass();
		~PostEffectComposePass();
	
	public:
		void createPostEffectComposePassMaterial();
		virtual void drawImpl(rs_commandbuffer* cmdbuffer);
	private:
		class PostEffectComposeEntity* entity;
	};

};

#endif //POST_EFFECT_COMPOSE_PASS