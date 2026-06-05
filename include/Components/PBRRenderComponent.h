#ifndef PBR_RENDER_COMPONENT_H_
#define PBR_RENDER_COMPONENT_H_

#include "function/Component.h"
#include "Renderer/GltfLoader.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/Mesh.h"
#include "Components/RenderComponent.h"
namespace Render {
	class MaterialTemplate;
	class RenderEntity;
	class PBRRenderComponent : public RenderComponent {
	public:
		virtual void onAttach() override;
		virtual void setMesh(const MeshPtr& mesh);
		virtual void setMaterial(int submeshId, MaterialPtr mat);
		virtual MeshPtr getMesh(const MeshPtr& mesh);
		virtual MaterialPtr getMaterial(int submeshID, MaterialPtr& mat);

		void collectRenderEntities(std::vector<RenderEntity*>& outEntities)const override;
		virtual void onUpdate(float dt)override;
		virtual void onDestroy()override;
		~PBRRenderComponent();
	private :
		void updateRenderEntities();
		RenderEntity* createRenderEntity(int submeshID, MaterialPtr mat);
	private:
		MeshPtr mMesh;
		std::vector<MaterialPtr> mMaterials;
		std::vector<RenderEntity*> mRenderEntities;
	};
}

#endif