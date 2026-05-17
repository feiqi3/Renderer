#ifndef MATERIAL_INSTANCE_H_
#define MATERIAL_INSTANCE_H_ 

#include "Renderer/MaterialTemplate.h"
#include "common/Name.h"
#include "Renderer/RenderSystem.h"
#include "common/ResourceHandler.h"
#include "Renderer/Texture.h"
#include "Renderer/ResourceVariant.h"
#include "Renderer/SamplerResourceManager.h"
#include "common/SmallVector.h"

#include "Renderer/PipelineBindingTable.h"

#include <string>
#include <vector>

namespace Render {

	class RenderPass;

	class Material : public IResource {
	public:
		Material(const MaterialTemplatePtr& templatePtr);
		virtual ~Material();

		static const Name& typeName();
		virtual const Name& getTypeName() const override;
		virtual ResourceMemory getMemory() const override;
		virtual void OnUnload() override;
		virtual void OnLoaded() override {};
		virtual void OnUpdateParam(Pass* pass);

		void bindParameter(const std::string& paramName, TexturePtr tex, int element = 0);
		void bindParameter(const std::string& paramName, TexturePtr tex, ImageViewKey key, int element = 0);
		void bindParameter(const std::string& paramName, rs_buffer* buffer, int element = 0);
		void bindParameter(const std::string& paramName, SamplerPtr sampler, int element = 0);

		template<class T>
		void bindParameter(const std::string& paramName, const T& data);
		void bindParameter(const std::string& paramName, const void* data, u32 size);

		void uploadUniform(Pass* pass);

		// ====================================================================
		// ====================================================================
		MaterialPass* getMaterialPass(const Name& name);
		void addMaterialPassToRender(const Name& passName);
		MaterialPass* getMaterialPassToRender(const Name& passName);

		void setRenderOrder(u32 order);
		u32 getRenderOrder() const;

		MaterialBindingTable& getBindingTable() { return mBindingTable; }
		const MaterialBindingTable& getBindingTable() const { return mBindingTable; }
		inline uint32_t getGlobalMaterialIndex()const {
			return mMaterialIndex;
		}
		friend class MaterialManager;

	protected:

		std::vector<Name> passNamesToRender;
		MaterialTemplatePtr m_template;
		u32 mRenderOrder = 0;
		uint32_t mMaterialIndex = 0;
		// ====================================================================
		// ====================================================================

		MaterialBindingTable mBindingTable;
	};

	template<class T>
	void Material::bindParameter(const std::string& paramName, const T& data) {
		this->bindParameter(paramName, static_cast<const void*>(&data), sizeof(T));
	}

	using MaterialPtr = ResourceHandle<Material>;
}

#endif // MATERIAL_INSTANCE_H_