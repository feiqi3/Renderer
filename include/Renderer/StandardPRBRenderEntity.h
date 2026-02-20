#ifndef STANDARD_PBR_RENDER_ENTITY_H_
#define STANDARD_PBR_RENDER_ENTITY_H_
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/GPUShared/PBREntity.h"
#include "Renderer/Texture.h"
namespace Render {
	class StandardPBRRenderEntity : public RenderEntity {
	public:
		Material* getMaterial()override { return mMaterial.get(); }
		inline void setPBRMaterial(const MaterialPtr& mat) {
			mMaterial = mat;
		}
		inline void updateUniforms(Pass* pass) override {
		}
	private:
		MaterialPtr mMaterial;
		Pass*		mainPass;

	};
}

#endif