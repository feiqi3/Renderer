#ifndef MAIN_CAMERA_PASS_H_
#define MAIN_CAMERA_PASS_H_

#include "Renderer/RenderPass.h"
#include <Renderer/RenderEntity.h>

namespace Render {
	class MainCameraPass : public RenderPass{
	public:
		MainCameraPass();
		void addToDrawList(RenderEntity* entity);
		virtual void drawImpl(rs_commandbuffer* cmdbuffer) override;
	private:
		std::vector<RenderEntity*> mSceneEntity;
	};
}

#endif