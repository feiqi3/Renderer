#ifndef SKYBOX_RENDER_COMPONENT_H_
#define SKYBOX_RENDER_COMPONENT_H_
#include "function/Component.h"
#include "common/CommonMath.h"
#include "Renderer/Texture.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/GPUShared/SkyBoxData.h"
#include "Components/RenderComponent.h"
namespace Render {
	class SkyboxRenderComponent : public RenderComponent {
	public:
		SkyboxRenderComponent();
		~SkyboxRenderComponent();
		void setSkybox(TexturePtr texture);
		void setSampler(SamplerPtr sampler);
		void setRotation(const quat& q);
		void setExposure(float exposure);
		void setColor(vec3 color);

		void onUpdate(float dt)override;
	private:
		TexturePtr mSkyboxCubeMap;
		SamplerPtr mSkyboxSampler; 
		class SkyBoxRenderEntity* mEntity = nullptr;
		GPUShared::SkyBoxData mData;
		bool mDataDirty = true;
	};
}

#endif