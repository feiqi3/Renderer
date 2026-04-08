#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/GPUShared/SkyBoxData.h"
namespace Render {


	class SkyBoxRenderEntity : public RenderEntity {
	public:
		SkyBoxRenderEntity();
		Material* getMaterial() override;
		static MaterialPtr getSkyBoxMaterial();
		void setSkyboxCubemap(TexturePtr texture,SamplerPtr sampler);
		void setGPUData(const GPUShared::SkyBoxData& data);
	private:
		MaterialPtr mSkyboxMaterial;
		
	};
}