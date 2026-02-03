#ifndef PBR_RENDER_COMPONENT_H_
#define PBR_RENDER_COMPONENT_H_

#include "function/Component.h"
#include "Renderer/GltfLoader.h"
namespace Render {
	class MaterialTemplate;
	class PBRRenderComponent : public Component{
	public:
		virtual void onAttach() override;

	private:
		static MaterialTemplate* createPBRMaterial(GLTFMaterial* material);

	private:
		MaterialTemplate* mPBRMatTemp = nullptr;
	};
}

#endif