#include "Renderer/MaterialTemplate.h"
#include "common/Name.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Texture.h"
#include "common/ResourceHandler.h"
#include <string>
#include <optional>
#include <map>

#ifndef MATERIAL_INSTANCE_H_
#define MATERIAL_INSTANCE_H_

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

		struct _ParameterPair {
			rs_binding_pos	bindingPos;
			UniformType		parameterType;
			TexturePtr		texture;
			void* rawPtr;
		};

		void									bindParameter(const std::string& paramName, TexturePtr tex);
		void									bindParameter(const std::string& paramName, rs_buffer* buffer);
		void									bindParameter(const std::string& paramName, rs_sampler* sampler);

		void									uploadUniform(Pass* pass);
		MaterialPass* getMaterialPass(const Name& name);

	protected:
		std::optional<_ParameterPair&>			getParameterInfo(const std::string& paramName);

		MaterialTemplatePtr m_template;
		std::map < std::string, _ParameterPair> mParameterMap;
	};
	using MaterialPtr = ResourceHandle<Material>;
}

#endif