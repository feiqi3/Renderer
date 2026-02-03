#ifndef STANDARD_PBR_RENDER_ENTITY_H_
#define STANDARD_PBR_RENDER_ENTITY_H_
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/GPUShared/PBREntity.h"
#include "Renderer/Texture.h"
namespace Render {
	class StandardPBRRenderEntity : public RenderEntity {
	public:
		MaterialTemplate* getMaterialTemplate();

		void updateUniforms(rs_commandbuffer* cmd, Material* pass);

		void setBaseColTex(TexturePtr tex);
		TexturePtr getBaseColTex();

		void setNormalTex(TexturePtr tex);
		TexturePtr getNormalTex();

		void setMetallicRoughnessTex(TexturePtr tex);
		TexturePtr getMetallicRoughnessTex();

		void setAOTex(TexturePtr tex);
		TexturePtr getAOTex();

		void setBaseCol(vec4 col);
		vec4 getBaseCol();

		void setMetalRoughAO(vec4 v);
		vec4 getMetalRoughAO();

		void setEmissive(vec4 col);
		vec4 getEmissive();

		void setTexControl(vec4 v);
		vec4 getTexControl();

	private:
		void prepareBindingInfo();
	private:
		Material* mainPassMaterial;
		Pass*	  mainPass;
		//----------------------//
		rs_binding_pos pbrDataBindingPos;
		rs_binding_pos baseColTexBindingPos;
		rs_binding_pos baseColSamplerBindingPos;

		rs_binding_pos normalTexBindingPos;
		rs_binding_pos normalSamplerBindingPos;

		rs_binding_pos metallicRoughnessTexBindingPos;
		rs_binding_pos metallicRoughnessSamplerBindingPos;

		rs_binding_pos AOTexBindingPos;
		rs_binding_pos AOSamplerBindingPos;
		//----------------------//
		TexturePtr mBaseColorTex;
		TexturePtr mNormalTex;
		TexturePtr mMetallicRoughnessTex;
		TexturePtr mAOTexture;
		rs_sampler* mBaseColorSampler;
		rs_sampler* mNormalSampler;
		rs_sampler* mMetallicRoughnessSampler;
		rs_sampler* mAOSampler;
		MaterialTemplate* pbrMaterial = nullptr;
		GPUShared::PBRData pbrData;
		//----------------------//
	};
}

#endif