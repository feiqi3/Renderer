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

		rs_binding_pos getObjectCommonDataBindingPos();
		rs_binding_pos getSceneCommonDataBindingPos();
		rs_binding_pos getCameraCommonDataBindngPos();
	private:
		void createVirtualRenderPass();

	private:
		ConstShaderDataManagerPrivate* mDp;
	};
};

#endif