#ifndef MAIN_CAMERA_RENDER_PASS_H
#define MAIN_CAMERA_RENDER_PASS_H
#include "RenderPass.h"
namespace Render {

	class MainCamPass : public RenderPass {
	public:
		MainCamPass();
		virtual void drawImpl(rs_commandbuffer* cmdbuffer,Camera* camera);
		void addToDraw(class RenderEntity* Entity);
	private:
		std::vector<RenderEntity*> mSceneEntity;
	};

}

#endif