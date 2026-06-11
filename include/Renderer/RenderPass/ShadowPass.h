#ifndef SHADOW_PASS_H_
#define SHADOW_PASS_H_
#include "Renderer/RenderPass.h"
namespace Render {

	class DirLightShadowPass :public RenderPass{
	public:
		DirLightShadowPass();
		virtual StageMacroPairs	getPassStageShaderMacro(const Name& logicPassName)override;
	private:
	};

}

#endif