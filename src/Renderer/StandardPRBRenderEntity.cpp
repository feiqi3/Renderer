#include "Renderer/StandardPRBRenderEntity.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/EnginePass.h"
namespace Render {
	namespace {

		Name getPBRMaterialName() {
			return Name("StandardPBR");
		}

		MaterialTemplate* createPBRMaterial() {
			std::string PBRShaderVSName = "../shader/StandardPBR.vs";
			std::string PBRShaderPSName = "../shader/StandardPBR.ps";
			ShaderStageInfo pbrTemplateInfo{ {ShaderStage::Vertex,PBRShaderVSName }, {ShaderStage::Fragment, PBRShaderPSName } };
			RenderState normalState{};
			normalState.depthTestEnable = true;
			VertexInputDescription VtxIA{
			};

			uint32_t offset = 0;
			//Vertex
			InputAttribute AttVtx{
			};
			//POSITION
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float3;
			AttVtx.location = 0;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 12;

			//Normal
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float3;
			AttVtx.location = 1;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 12;
			//Texcoord0
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float2;
			AttVtx.location = 2;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 8;
			//Texcoord1
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float2;
			AttVtx.location = 3;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 8;

			//Color0
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float2;
			AttVtx.location = 4;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 4;
			InputBufferBinding binding{};
			binding.perInstance = false;
			binding.stride = offset;
			VtxIA.bindings.push_back(binding);
			RenderState renderstate{};
			auto materialTemplateToRet = MaterialTemplateManager::instance()->createMaterialTemplate(getPBRMaterialName(), pbrTemplateInfo,renderstate, VtxIA);
			if (!materialTemplateToRet)return nullptr;
			materialTemplateToRet->createVariant(RenderSystem::instance()->getRenderPass(PassName::MainCameraPass), {});
			return materialTemplateToRet;
		}
	}

	Render::MaterialTemplate* StandardPBRRenderEntity::getMaterialTemplate()
	{
		if (pbrMaterial == nullptr) {
			pbrMaterial = MaterialTemplateManager::instance()->getMaterialTemplate(getPBRMaterialName());
			if (!pbrMaterial) {
				pbrMaterial = createPBRMaterial();
				mainPassMaterial = pbrMaterial->getVarient(PassName::MainCameraPass);
				this->createPass(PassName::MainCameraPass);
			}
		}
		return pbrMaterial;
	}

	void StandardPBRRenderEntity::updateUniforms(rs_commandbuffer* cmd, Material* pass)
	{
		auto renderSys = RenderSystem::instance();
		{
			// 1. PBR Data
			renderSys->updateUniformBufferData(
				pbrDataBindingPos,
				&pbrData,
				sizeof(pbrData),
				mainPass
			);

			// 2. BaseColor
			if (mBaseColorTex) {
				renderSys->updateUniform(
					baseColTexBindingPos,
					mBaseColorTex->getRsImage(),
					mainPass
				);
			}
			if (mBaseColorSampler) {
				renderSys->updateUniform(
					baseColSamplerBindingPos,
					mBaseColorSampler,
					mainPass
				);
			}

			// 3. Normal
			if (mNormalTex) {
				renderSys->updateUniform(
					normalTexBindingPos,
					mNormalTex->getRsImage(),
					mainPass
				);
			}
			if (mNormalSampler) {
				renderSys->updateUniform(
					normalSamplerBindingPos,
					mNormalSampler,
					mainPass
				);
			}

			// 4. Metallic-Roughness
			if (mMetallicRoughnessTex) {
				renderSys->updateUniform(
					metallicRoughnessTexBindingPos,
					mMetallicRoughnessTex->getRsImage(),
					mainPass
				);
			}
			if (mMetallicRoughnessSampler) {
				renderSys->updateUniform(
					metallicRoughnessSamplerBindingPos,
					mMetallicRoughnessSampler,
					mainPass
				);
			}

			if (mAOTexture) {
				renderSys->updateUniform(
					AOTexBindingPos,
					mAOTexture->getRsImage(),
					mainPass
				);
			}
			if (mAOSampler) {
				renderSys->updateUniform(
					AOSamplerBindingPos,
					mAOSampler,
					mainPass
				);
			}
		}
	}

	void StandardPBRRenderEntity::setBaseCol(vec4 col)
	{
		pbrData.baseCol = col;
	}

	Render::vec4 StandardPBRRenderEntity::getBaseCol()
	{
		return pbrData.baseCol;

	}

	void StandardPBRRenderEntity::setMetalRoughAO(vec4 v)
	{
		pbrData.metalRoughAO = v;
	}

	Render::vec4 StandardPBRRenderEntity::getMetalRoughAO()
	{
		return pbrData.metalRoughAO;
	}

	void StandardPBRRenderEntity::setEmissive(vec4 col)
	{
		pbrData.emissiveFactor = col;
	}

	Render::vec4 StandardPBRRenderEntity::getEmissive()
	{
		return pbrData.emissiveFactor;
	}

	void StandardPBRRenderEntity::setTexControl(vec4 v)
	{
		pbrData.texControl = v;
	}

	vec4 StandardPBRRenderEntity::getTexControl()
	{
		return pbrData.texControl;
	}

	void StandardPBRRenderEntity::prepareBindingInfo()
	{
		auto getBindingOrFail = [&](std::string_view name, rs_binding_pos& out) -> bool {
			out = RenderSystem::instance()->getBindingPos(std::string(name).c_str(), mainPassMaterial);
			if (out == INVALID_BINDING_POS) {
				assert(false && "binding not found");
				return false;
			}
			return true;
			};
		getBindingOrFail("pbrData", pbrDataBindingPos);
		getBindingOrFail("u_baseColorTex", baseColTexBindingPos);
		getBindingOrFail("u_baseColorSampler", baseColSamplerBindingPos);
		getBindingOrFail("u_normalTex", normalTexBindingPos);
		getBindingOrFail("u_normalSampler", normalSamplerBindingPos);
		getBindingOrFail("u_metallicRoughnessTex", metallicRoughnessTexBindingPos);
		getBindingOrFail("u_metallicRoughnessSampler", metallicRoughnessSamplerBindingPos);
		getBindingOrFail("u_AOTex", AOTexBindingPos);
		getBindingOrFail("u_AOSampler", AOSamplerBindingPos);
	}

	void StandardPBRRenderEntity::setBaseColTex(TexturePtr tex) {
		mBaseColorTex = tex;
	}

	TexturePtr StandardPBRRenderEntity::getBaseColTex() {
		return mBaseColorTex;
	}

	void StandardPBRRenderEntity::setNormalTex(TexturePtr tex) {
		mNormalTex = tex;
	}

	TexturePtr StandardPBRRenderEntity::getNormalTex() {
		return mNormalTex;
	}

	void StandardPBRRenderEntity::setMetallicRoughnessTex(TexturePtr tex) {
		mMetallicRoughnessTex = tex;
	}

	TexturePtr StandardPBRRenderEntity::getMetallicRoughnessTex() {
		return mMetallicRoughnessTex;
	}

	void StandardPBRRenderEntity::setAOTex(TexturePtr tex) {
		mAOTexture = tex;
	}

	TexturePtr StandardPBRRenderEntity::getAOTex() {
		return mAOTexture;
	}

}
