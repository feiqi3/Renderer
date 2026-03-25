#ifndef MATERIAL_INSTANCE_H_
#define MATERIAL_INSTANCE_H_ 

#include "Renderer/MaterialTemplate.h"
#include "common/Name.h"
#include "Renderer/RenderSystem.h"
#include "common/ResourceHandler.h"
#include "Renderer/Texture.h"
#include <string>
#include <optional>
#include <map>
#include "Renderer/ResourceVariant.h"
#include "Renderer/SamplerResourceManager.h"

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

		struct _ParameterPair {
			rs_binding_pos        bindingPos;
			UniformType           parameterType;
			RenderResourceVariant var;
		};

		void bindParameter(const std::string& paramName, TexturePtr tex);
		void bindParameter(const std::string& paramName, rs_buffer* buffer);
		void bindParameter(const std::string& paramName, SamplerPtr sampler);

		template<class T>
		void bindParameter(const std::string& paramName, const T& data);

		void bindParameter(const std::string& paramName, const void* data, u32 size);

		void uploadUniform(Pass* pass);
		MaterialPass* getMaterialPass(const Name& name);
		void addMaterialPassToRender(const Name& passName);
		MaterialPass* getMaterialPassToRender(const Name& passName);
		void setRenderOrder(u32 order);
		u32 getRenderOrder() const;

	protected:
		std::optional<_ParameterPair*> getParameterInfo(const std::string& paramName);
		std::vector<Name>              passNamesToRender;
		MaterialTemplatePtr            m_template;
		std::map<std::string, _ParameterPair> mParameterMap;
		u32                            mRenderOrder = 0;
	};

	template<class T>
	void Material::bindParameter(const std::string& paramName, const T& data)
	{
		this->bindParameter(paramName, static_cast<const void*>(&data), sizeof(T));
	}

	using MaterialPtr = ResourceHandle<Material>;
}

#endif