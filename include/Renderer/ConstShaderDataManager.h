#ifndef CONST_SHADER_DATA_MANAGER_H_
#define CONST_SHADER_DATA_MANAGER_H_
#include "common/Singleton.h"
#include "function/scene.h"
namespace Render {
	class Camera;
	class ConstShaderDataManagerPrivate;
	class ConstShaderDataManager : public Singleton<ConstShaderDataManager> {
	public:
		ConstShaderDataManager();
		~ConstShaderDataManager();

		rs_drawdata* updateCameraDrawData(Camera* camera);
		rs_drawdata* updateSceneDrawData(Scene* camera);
	private:
		void createVirtualRenderPass();
		ConstShaderDataManagerPrivate* mDp;
	};
};

#endif