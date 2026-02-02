#ifndef STANDARD_PBR_RENDER_ENTITY_H_
#define STANDARD_PBR_RENDER_ENTITY_H_
#include "Renderer/RenderEntity.h"
namespace Render {
	class StandardPBRRenderEntity : public RenderEntity {
	public:
		MaterialTemplate* getMaterialTemplate();
		void updateUniforms(rs_commandbuffer* cmd, Material* pass);
	};
}

#endif