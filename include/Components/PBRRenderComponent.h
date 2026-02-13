#ifndef PBR_RENDER_COMPONENT_H_
#define PBR_RENDER_COMPONENT_H_

#include "function/Component.h"
#include "Renderer/GltfLoader.h"
#include "Renderer/MaterialInstance.h"
namespace Render {
	class MaterialTemplate;
	class PBRRenderComponent : public Component{
	public:
		virtual void onAttach() override;
		static MaterialPtr createPBRMaterial(GLTFMaterial* material);

	private:

	private:
		MaterialTemplate* mPBRMatTemp = nullptr;
	};
}

#endif