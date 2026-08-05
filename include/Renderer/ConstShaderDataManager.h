#ifndef CONST_SHADER_DATA_MANAGER_H_
#define CONST_SHADER_DATA_MANAGER_H_
#include "common/Singleton.h"
#include "function/scene.h"
namespace Render {
	class Camera;
	class ConstShaderDataManagerPrivate;
	struct rs_buffer;
	class HizClusteredLight;
	class ConstShaderDataManager : public Singleton<ConstShaderDataManager> {
	public:
		ConstShaderDataManager();
		~ConstShaderDataManager();

		void		 setClusterLightsData(HizClusteredLight* clusterLight, Scene* scene);
		void		 updateLightsData(Scene* scene);
		rs_drawdata* updateCameraDrawData(Camera* camera);
		rs_drawdata* updateSceneDrawData(Scene* camera);
		rs_drawdata* updateShadowDrawData(Scene* scene);
		rs_binding_pos getObjectCommonDataBindingPos();
		rs_binding_pos getSceneCommonDataBindingPos();
		rs_binding_pos getCameraCommonDataBindngPos();

		rs_binding_pos getGlobalBindlessUAVBuffersBindingPos();
		rs_binding_pos getGlobalBindlessUAVImagesBindingPos();
		rs_binding_pos getGlobalBindlessSamplersBindingPos();
		rs_binding_pos getGlobalBindlessTexturesBindingPos();

		rs_buffer*	   getLightDataBuffer();
	private:
		void createVirtualRenderPass();

	private:
		ConstShaderDataManagerPrivate* mDp;
	};
};

#endif