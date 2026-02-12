#ifndef MATERIAL_INSTANCE_H_
#define MATERIAL_INSTANCE_H_
#include"Renderer/MaterialTemplate.h"
#include "common/Name.h"
#include "Renderer/RenderSystem.h"
#include "Texture.h"
#include <string>
namespace Render {
	class Material {
	public:
		Material(MaterialTemplate* matTplt);
		~Material();


		struct _ParameterPair {
			rs_binding_pos	bindingPos;
			UniformType		parameterType;
			TexturePtr		texture;
			void*			rawPtr;
		};

		void									bindParameter(const std::string& paramName, TexturePtr tex);
		void									bindParameter(const std::string& paramName, rs_buffer* buffer);
		void									bindParameter(const std::string& paramName, rs_sampler* sampler);

		void									uploadUniform(Pass* pass);

	protected:
		std::optional<_ParameterPair&>			getParameterInfo(const std::string& paramName);
		MaterialTemplate* m_template;
		std::map < std::string, _ParameterPair> mParameterMap;
	};
}

#endif