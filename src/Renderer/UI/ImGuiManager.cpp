#include "render_resource.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/UI/ImGuiManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/EnginePass.h"
#include "Renderer/SamplerResourceManager.h"
#include "imgui.h"



namespace Render {
	class ImGuiRender : public RenderEntity {
	public:
		MaterialPtr mMat = nullptr;
	public:


		ImGuiRender(MaterialPtr mat) {
			this->setRenderMask(RenderMask::Gui2D);
			mMat = mat;
		}
		Material* getMaterial() override {
			return mMat.get();
		}
		
		AxisAlignedBoundingBox getWorldBounding()override {
			return AxisAlignedBoundingBox();
		}

		void updateEntityCommonData() override {
			return;
		}
		void updateEntityCommonDataImpl(Pass*) override {
			return;
		}
	};

	class ImGuiManagerPrivate {
	public:
		uint32_t vtxSize = 0; // By default
		rs_buffer* vertexBuffer;
		uint32_t idxSize = 0; // By default
		rs_buffer* idxBuffer;
		uint32_t textureSize = 0; // By default
		rs_buffer* textureIndexBuffer;
			
		MaterialPtr material = nullptr;
		ImGuiRender* imguiRender = nullptr;
		void init();
		void onReSizeBuffer(uint32_t vtx,uint32_t idx,uint32_t texSize);
		
		void deinit();
	
	
	};

	void ImGuiManagerPrivate::init()
	{
		ShaderStageInfo shaderStage = {
			{ShaderStage::Vertex	,"../shader/ImGuiBindless.vs"},
			{ShaderStage::Fragment	,"../shader/ImGuiBindless.ps"},
		};
		RenderState renderState{};

		BlendState blendInfo{};
		blendInfo.blendEnable = true;
		blendInfo.colorBlendOp = BlendOp::Add;

		blendInfo.srcAlphaBlend = BlendFactor::One;
		blendInfo.dstAlphaBlend = BlendFactor::OneMinusSrcAlpha;

		blendInfo.srcColorBlend = BlendFactor::SrcAlpha;
		blendInfo.dstColorBlend = BlendFactor::OneMinusSrcAlpha;

		renderState.blendStates.push_back(blendInfo);

		VertexInputDescription vtxIA{};

		auto materialTemplate = MaterialTemplateManager::instance()->createMaterialTemplate(
			Name("ImGuiMatTemp"), shaderStage, renderState, vtxIA
		);
		materialTemplate->createMaterialPass(PassName::GUIOverlay);
		material = MaterialManager::instance()->createMaterial<Material>(Name("ImGui"),materialTemplate);
		imguiRender = new ImGuiRender(material);
		
	}

	void ImGuiManagerPrivate::onReSizeBuffer(uint32_t vtx, uint32_t idx, uint32_t tex)
	{
		BufferDesc desc;
		desc.bufUsage = BufferType_Storage;
		desc.mappable = true;


		if (vtxSize < vtx) {
			const uint32_t ImDrawVertSize = sizeof(ImDrawVert);
			vtxSize = vtx;
			RenderSystem::instance()->destroyBuffer(vertexBuffer);
			desc.byteSize = ImDrawVertSize * vtxSize;
			vertexBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
			material->bindParameter(
				"u_vertexList", vertexBuffer
			);
		}

		if (idxSize < idx) {
			RenderSystem::instance()->destroyBuffer(idxBuffer);
			desc.byteSize = sizeof(ImDrawIdx) * idxSize;
			idxBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
			material->bindParameter(
				"u_indexList", idxBuffer
			);
		}

		if (textureSize < tex) {
			RenderSystem::instance()->destroyBuffer(textureIndexBuffer);
			desc.byteSize = sizeof(uint32_t) * textureSize;
			textureIndexBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
			material->bindParameter(
				"u_textureList", textureIndexBuffer
			);
		}
	}

	void ImGuiManagerPrivate::deinit()
	{
		delete imguiRender;
		RenderSystem::instance()->destroyBuffer(vertexBuffer);
		RenderSystem::instance()->destroyBuffer(textureIndexBuffer);
		RenderSystem::instance()->destroyBuffer(idxBuffer);
	}

	IMGUIManager::IMGUIManager()
	{
		mDP = new ImGuiManagerPrivate;
	}

	void IMGUIManager::draw()
	{
		
		//This act as a imgui engine backend, use engine api to render widgets
		ImGui::EndFrame();
		ImGui::Render();
		auto imDrawData = ImGui::GetDrawData();
		//TODO: parse drawData, generate vertex buffer, index buffer, update texture to global bindless data
		//And dispatch drawcall.
		


		ImGui::NewFrame();

	}

	IMGUIManager::~IMGUIManager()
	{
		delete mDP;
		mDP = NULL;
	}

	ImTextureID toImTex(rs_image_view* view)
	{
		return ImTextureID(view);
	}

}


