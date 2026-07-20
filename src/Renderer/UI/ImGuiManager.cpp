#include "render_resource.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/UI/ImGuiManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/EnginePass.h"
#include "Renderer/RenderPass/DebugOverlayPass.h"
#include "Renderer/SamplerResourceManager.h"
#include "function/InputManager.h"
#include "imgui.h"
#include "Renderer/UI/ImGuiUtils.h"


namespace Render {
	struct ImGuiKeyMapContainer {
		ImGuiKey data[(int)KeyCode::Max]{};

		constexpr ImGuiKeyMapContainer() {
			data[(int)KeyCode::Space] = ImGuiKey_Space;
			data[(int)KeyCode::Apostrophe] = ImGuiKey_Apostrophe;
			data[(int)KeyCode::Comma] = ImGuiKey_Comma;
			data[(int)KeyCode::Minus] = ImGuiKey_Minus;
			data[(int)KeyCode::Period] = ImGuiKey_Period;
			data[(int)KeyCode::Slash] = ImGuiKey_Slash;
			data[(int)KeyCode::Semicolon] = ImGuiKey_Semicolon;
			data[(int)KeyCode::Equal] = ImGuiKey_Equal;
			data[(int)KeyCode::LeftBracket] = ImGuiKey_LeftBracket;
			data[(int)KeyCode::Backslash] = ImGuiKey_Backslash;
			data[(int)KeyCode::RightBracket] = ImGuiKey_RightBracket;
			data[(int)KeyCode::GraveAccent] = ImGuiKey_GraveAccent;
			data[(int)KeyCode::World1] = ImGuiKey_Oem102; 

			for (int i = 0; i <= 9; ++i) {
				data[(int)KeyCode::Key0 + i] = static_cast<ImGuiKey>(ImGuiKey_0 + i);
			}

			for (int i = 0; i < 26; ++i) {
				data[(int)KeyCode::A + i] = static_cast<ImGuiKey>(ImGuiKey_A + i);
			}

			data[(int)KeyCode::Escape] = ImGuiKey_Escape;
			data[(int)KeyCode::Enter] = ImGuiKey_Enter;
			data[(int)KeyCode::Tab] = ImGuiKey_Tab;
			data[(int)KeyCode::Backspace] = ImGuiKey_Backspace;
			data[(int)KeyCode::Insert] = ImGuiKey_Insert;
			data[(int)KeyCode::Delete] = ImGuiKey_Delete;
			data[(int)KeyCode::Right] = ImGuiKey_RightArrow;
			data[(int)KeyCode::Left] = ImGuiKey_LeftArrow;
			data[(int)KeyCode::Down] = ImGuiKey_DownArrow;
			data[(int)KeyCode::Up] = ImGuiKey_UpArrow;
			data[(int)KeyCode::PageUp] = ImGuiKey_PageUp;
			data[(int)KeyCode::PageDown] = ImGuiKey_PageDown;
			data[(int)KeyCode::Home] = ImGuiKey_Home;
			data[(int)KeyCode::End] = ImGuiKey_End;
			data[(int)KeyCode::CapsLock] = ImGuiKey_CapsLock;
			data[(int)KeyCode::ScrollLock] = ImGuiKey_ScrollLock;
			data[(int)KeyCode::NumLock] = ImGuiKey_NumLock;
			data[(int)KeyCode::PrintScreen] = ImGuiKey_PrintScreen;
			data[(int)KeyCode::Pause] = ImGuiKey_Pause;

			for (int i = 0; i < 24; ++i) {
				data[(int)KeyCode::F1 + i] = static_cast<ImGuiKey>(ImGuiKey_F1 + i);
			}

			for (int i = 0; i <= 9; ++i) {
				data[(int)KeyCode::Kp0 + i] = static_cast<ImGuiKey>(ImGuiKey_Keypad0 + i);
			}
			data[(int)KeyCode::KpDecimal] = ImGuiKey_KeypadDecimal;
			data[(int)KeyCode::KpDivide] = ImGuiKey_KeypadDivide;
			data[(int)KeyCode::KpMultiply] = ImGuiKey_KeypadMultiply;
			data[(int)KeyCode::KpSubtract] = ImGuiKey_KeypadSubtract;
			data[(int)KeyCode::KpAdd] = ImGuiKey_KeypadAdd;
			data[(int)KeyCode::KpEnter] = ImGuiKey_KeypadEnter;
			data[(int)KeyCode::KpEqual] = ImGuiKey_KeypadEqual;

			data[(int)KeyCode::LeftShift] = ImGuiKey_LeftShift;
			data[(int)KeyCode::LeftControl] = ImGuiKey_LeftCtrl;
			data[(int)KeyCode::LeftAlt] = ImGuiKey_LeftAlt;
			data[(int)KeyCode::LeftSuper] = ImGuiKey_LeftSuper;
			data[(int)KeyCode::RightShift] = ImGuiKey_RightShift;
			data[(int)KeyCode::RightControl] = ImGuiKey_RightCtrl;
			data[(int)KeyCode::RightAlt] = ImGuiKey_RightAlt;
			data[(int)KeyCode::RightSuper] = ImGuiKey_RightSuper;
			data[(int)KeyCode::Menu] = ImGuiKey_Menu;
		}

		constexpr ImGuiKey operator[] (KeyCode key)const {
			return data[(int)key];
		}
	};

	static constexpr ImGuiKeyMapContainer KeyCodeToImGuiMapInstance;
	static const ImGuiKey MouseButtonToImGuiKeyMap[(int)Render::MouseButton::Max] = {
	ImGuiKey_MouseLeft,   // Button1 / Left
	ImGuiKey_MouseRight,  // Button2 / Right
	ImGuiKey_MouseMiddle, // Button3 / Middle
	ImGuiKey_MouseX1,     // Button4
	ImGuiKey_MouseX2,     // Button5
	ImGuiKey_None,        // Button6
	ImGuiKey_None,        // Button7
	ImGuiKey_None,         // Button8 / Last
	};

	static ImGuiMouseButton_ toImMouseBtn(MouseButton btn) {
		switch (btn)
		{
		case Render::MouseButton::Button4:
		case Render::MouseButton::Button5:
		case Render::MouseButton::Button6:
		case Render::MouseButton::Button7:
		case Render::MouseButton::Button8:
		case Render::MouseButton::Max:
			return ImGuiMouseButton_COUNT;
			break;
		case Render::MouseButton::Left:
		{
			return ImGuiMouseButton_::ImGuiMouseButton_Left;
			break;
		}
		case Render::MouseButton::Right:
		{
			return ImGuiMouseButton_::ImGuiMouseButton_Right;
			break;
		}
		case Render::MouseButton::Middle:
		{
			return ImGuiMouseButton_::ImGuiMouseButton_Middle;
			break;
		}
		default:
			break;
		}
		return ImGuiMouseButton_COUNT;
	}

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
		bool isInited = false;

		uint32_t vtxSize = 0; // By default
		rs_buffer* vertexBuffer = nullptr;
		uint32_t idxSize = 0; // By default
		rs_buffer* idxBuffer = nullptr;
		uint32_t triToDraw = 0; // By default
		uint32_t drawCallNum = 0; // By default
		rs_buffer* triangleAttributeIdxBuffer = nullptr;
		rs_buffer* attributeBuffer = nullptr;//Perdraw call
		struct ViewportInfo{
			vec2 scale;
			vec2 offset;
		}mViewportInfo;

		struct PerDrawcallAttribute {
			ViewportInfo info;
			uint32_t texIdx;
			vec2 scissorMin;
			vec2 scissorMax;
			uint32_t isDepth;
		};

		MaterialPtr material = nullptr;
		ImGuiRender* imguiRender = nullptr;
		uint32_t imCreateTextureIDCnt = 1;
		SamplerPtr mImSampler = nullptr;

		//CONFIGS
		float scrollSens = 10.;

		//ID = 0 ---> invalid!
		std::map<uint32_t, std::pair<uint32_t,TexturePtr>> mImCreatedTextures{};
		decltype(std::chrono::steady_clock::now()) mLastFrameTime;

		void init();
		//TODO: set basic info when frame started.
		void onReSizeBuffer(uint32_t vtx,uint32_t idx,uint32_t texSize,uint32_t drawcallNums);
		void updateImTexture(ImTextureData* tex);
		uint32_t createImTexture(ImTextureData* tex);
		void	 destroyImTexture(ImTextureData* tex);
		void deinit();
	
	
	};

	void ImGuiManagerPrivate::init()
	{
		//----------------------------------------------------------//
		//--------------IMGUI SETUP INFO----------------------------//
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		//Only work in bindless mode.
		if (!RenderSystem::instance()->isBindlessEnabled())
			return;
		isInited = true;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		
		//TODO: backend support multi viewport?
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
		
		//io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
		
		//Backend setup
		io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
		
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		
		//----------------------------------------------------------//
		//-------------IMGUI RENDER INFO----------------------------//


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

		renderState.depthWriteEnable = false;
		renderState.depthTestEnable = false;
		VertexInputDescription vtxIA{};

		auto materialTemplate = MaterialTemplateManager::instance()->createMaterialTemplate(
			Name("ImGuiMatTemp"), shaderStage, renderState, vtxIA
		);
		materialTemplate->createMaterialPass(PassName::GUIOverlay);
		material = MaterialManager::instance()->createMaterial<Material>(Name("ImGui"),materialTemplate);
		if (!mImSampler) {
			mImSampler = SamplerResourceManager::instance()->getOrCreateSampler({});
		}
		material->bindParameter("u_sampler", mImSampler);

		imguiRender = new ImGuiRender(material);
		//----------------------------------------------------------//
		int winx, winy;
		RenderSystem::instance()->getWindowSize(winx, winy);
		io.DisplaySize.x = winx;
		io.DisplaySize.y = winy;
		ImGui::NewFrame();
	}

	void ImGuiManagerPrivate::onReSizeBuffer(uint32_t vtx, uint32_t idx, uint32_t triCount, uint32_t drawcallNum)
	{
		BufferDesc desc{};
		desc.bufUsage = BufferType_Storage;
		desc.mappable = true;


		if (vtxSize < vtx && vtx > 0) {
			const uint32_t ImDrawVertSize = sizeof(ImDrawVert);
			vtxSize = vtx;
			RenderSystem::instance()->destroyBuffer(vertexBuffer);
			vertexBuffer = nullptr;
			desc.byteSize = ImDrawVertSize * vtxSize;
			vertexBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
			material->bindParameter(
				"u_vertexList", vertexBuffer
			);
		}

		if (idxSize < idx && idx > 0) {
			idxSize = idx;
			RenderSystem::instance()->destroyBuffer(idxBuffer);
			idxBuffer = nullptr;
			desc.byteSize = sizeof(ImDrawIdx) * idxSize;
			idxBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
			material->bindParameter(
				"u_indexList", idxBuffer
			);
		}

		//triangleAttributeIdxBuffer size is always equals to idxSize / 3
		//map each triangle to drawcall's attribute
		uint32_t triangleToDrawSize = triCount;
		if(triToDraw < triangleToDrawSize && triangleToDrawSize > 0)
		{
			triToDraw = triangleToDrawSize;
			RenderSystem::instance()->destroyBuffer(triangleAttributeIdxBuffer);
			triangleAttributeIdxBuffer = nullptr;
			desc.byteSize = sizeof(u32) * triToDraw;
			triangleAttributeIdxBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
			material->bindParameter(
				"u_triAttIdxList", triangleAttributeIdxBuffer
			);
		}

		if (drawcallNum > this->drawCallNum)
		{
			this->drawCallNum = drawcallNum;
			
			BufferDesc descBuffer{};
			descBuffer.bufUsage = BufferType_Storage;
			descBuffer.mappable = true;
			descBuffer.byteSize = sizeof(PerDrawcallAttribute) * drawCallNum;
			attributeBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, descBuffer);
			material->bindParameter("u_attrList", attributeBuffer);
		}
		//if (textureSize < tex) {
		//	RenderSystem::instance()->destroyBuffer(indexAttributeIdxBuffer);
		//	desc.byteSize = sizeof(uint32_t) * textureSize;
		//	indexAttributeIdxBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, desc);
		//	material->bindParameter(
		//		"u_textureList", indexAttributeIdxBuffer
		//	);
		//}
	}

	void ImGuiManagerPrivate::updateImTexture(ImTextureData* tex)
	{
		int begx = tex->UpdateRect.x;
		int begy = tex->UpdateRect.y;
		int width = tex->UpdateRect.w;
		int height = tex->UpdateRect.h;
		std::vector<uint32_t> updateData;
		updateData.reserve(width * height);
		for (int y = begy;y < begy + height;++y) {
			for (int x = begx;x < begx + width;++x) {
				auto pix = *(uint32_t*)tex->GetPixelsAt(x, y);
				updateData.push_back(pix);
			}
		}
		auto backendID = (uint32_t)tex->BackendUserData;
		auto itor = mImCreatedTextures.find(backendID);
		if (itor == mImCreatedTextures.end()) {
			assert(false);
			return;
		}
		auto& texture_view_pair = itor->second;
		auto& texture = texture_view_pair.second;
		auto img = texture->getRsImage();

		RenderSystem::instance()->updateImageData(
			img, updateData.data(), updateData.size() * sizeof(uint32_t), begx, begy, 0, width, height, 1, 0, 1, 1);
		tex->SetStatus(ImTextureStatus_OK);
	}


	uint32_t ImGuiManagerPrivate::createImTexture(ImTextureData* tex)
	{
		IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
		IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
		auto backEndID = imCreateTextureIDCnt++;
		*((uint64_t*)& tex->BackendUserData) = (uint64_t)backEndID;
		auto texBackend = TextureResourceManager::instance()->createEmpty();
		std::vector<uint32_t> mPixelsInTile;
		mPixelsInTile.reserve(tex->Width * tex->Height);

		for (int y = 0;y < tex->Height;++y) {
			for (int x = 0;x < tex->Width;++x){
				mPixelsInTile.push_back( *(uint32_t*)tex->GetPixelsAt(x, y));
			}
		}

		auto image = RenderSystem::instance()->createImage2D(
			mPixelsInTile.data(), sizeof(uint32_t) * mPixelsInTile.size(), ImageFormat::RGBA8_UNORM, tex->Width, tex->Height, 1, 1, 1);
		texBackend->setRsImage(image);
		tex->SetTexID(toImTex(image->defaultView));
		tex->SetStatus(ImTextureStatus_OK);

		//For im created texture we need to extend its lifetime in bindless
		auto globalBindless = RenderSystem::instance()->getGlobalBindlessData();
		auto idx = RenderSystem::instance()->updateGlobalBindlessDataTexture(globalBindless, texBackend->getRsImage()->defaultView);

		mImCreatedTextures.insert({backEndID,{idx,texBackend}});
		return backEndID;
	}

	void ImGuiManagerPrivate::destroyImTexture(ImTextureData* tex)
	{
		auto backendID = (uint32_t)tex->BackendUserData;
		auto itor = mImCreatedTextures.find(backendID);
		if (itor == mImCreatedTextures.end()) {
			assert(false);
			return;
		}
		auto bindlessIdx = itor->second.first;
		auto globalBindless = RenderSystem::instance()->getGlobalBindlessData();
		RenderSystem::instance()->unbindGlobalBindlessDataTexture(globalBindless, bindlessIdx);
		mImCreatedTextures.erase(backendID);
		tex->SetStatus(ImTextureStatus_Destroyed);
		tex->SetTexID(ImTextureID_Invalid);
	}

	void ImGuiManagerPrivate::deinit()
	{
		isInited = false;
		delete imguiRender;
		RenderSystem::instance()->destroyBuffer(vertexBuffer);
		vertexBuffer = nullptr;
		RenderSystem::instance()->destroyBuffer(triangleAttributeIdxBuffer);
		triangleAttributeIdxBuffer = nullptr;
		RenderSystem::instance()->destroyBuffer(idxBuffer);
		idxBuffer = nullptr;
		RenderSystem::instance()->destroyBuffer(attributeBuffer);
		attributeBuffer = nullptr;
		ImGui::DestroyContext();
		mImCreatedTextures.clear();
	}

	IMGUIManager::IMGUIManager()
	{
		mDP = new ImGuiManagerPrivate;
		mDP->mLastFrameTime = std::chrono::steady_clock::now();
	}

	void IMGUIManager::draw()
	{
		auto RenderSys = RenderSystem::instance();
		auto io = ImGui::GetIO();
		//This act as a imgui engine backend, use engine api to render widgets
		ImGui::EndFrame();
		ImGui::Render();

		if (!mDP->isInited)return;

		auto imDrawData = ImGui::GetDrawData();
		auto globalBindless = RenderSystem::instance()->getGlobalBindlessData();
		//Update textures
		std::vector<std::pair<rs_image_view*, uint32_t>> viewBindlessIndexPairs{};
		for (ImTextureData* tex : *imDrawData->Textures) {
			bool needUpdateToBindless = false;
			if (tex->Status == ImTextureStatus_WantCreate) {
				needUpdateToBindless = true;
				mDP->createImTexture(tex);
			}
			if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames >= 5) {
				mDP->destroyImTexture(tex);
			}
			//update just a little bit
			if (tex->Status == ImTextureStatus_WantUpdates) {
				mDP->updateImTexture(tex);

			}
			rs_image_view* view = (rs_image_view*)tex->GetTexID();
			if (tex->Status == ImTextureStatus_OK) {
				if (view == nullptr) {

					assert(view != nullptr);
					continue;

				}
				if (view->viewKey.getUAVAccess() != UAVAccess::ReadOnly)
				{		
					assert(view->viewKey.getUAVAccess() == UAVAccess::ReadOnly &&
						"Only SRV CAN BE RENDER IN IMGUI!!!");
					continue;
				}

				if (needUpdateToBindless) {
					auto texIdx = RenderSystem::instance()->updateGlobalBindlessDataTexture(
						globalBindless, view
					);
					viewBindlessIndexPairs.push_back({ view,texIdx });
				}
				RenderSys->markGlobalBindlessDataTexture(globalBindless, view);

			}
		}

		//parse drawData, generate vertex buffer, index buffer, update attribute,update textures to global bindless data
		//And dispatch dc.
		auto vtxSize = imDrawData->TotalVtxCount;
		auto idxSize = imDrawData->TotalIdxCount;
		auto imDrawListCnt = imDrawData->CmdListsCount;
		int totalIndexWillDraw = 0;
		int totalDc = 0;
		for (auto drawList : imDrawData->CmdLists) {
			for (auto& cmd : drawList->CmdBuffer) {
				totalDc++;
				totalIndexWillDraw += cmd.ElemCount;
			}
		}
		int triangleCount = totalIndexWillDraw / 3.;
		int curVtxPos = 0;
		int curIdxPos = 0;
		int curTriPos = 0;
		this->mDP->onReSizeBuffer(vtxSize, idxSize, triangleCount, totalDc);

		//This is actually a triangle -> drawcall's attribute mapping
		//And per drawcall has it's own attribute
		//Like: scissor, textureid, viewport info
		std::vector<u32> triAttIdToFill(triangleCount);
		std::vector<ImGuiManagerPrivate::PerDrawcallAttribute> perDrawCallAttributes{};
		perDrawCallAttributes.reserve(totalDc);
		ImGuiManagerPrivate::PerDrawcallAttribute attr{};
		ImGuiManagerPrivate::ViewportInfo vpInfo{};

		for (int i = 0;i < imDrawListCnt; ++i) {
			//Fill viewport info, each drawlist shares one viewport
			ImVec2 clipOff = imDrawData->DisplayPos;         // (0,0) unless using multi-viewports / but we do not support that....
			ImVec2 clipScale = imDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
			vpInfo.scale = vec2(2.0f / imDrawData->DisplaySize.x, 2.0f / imDrawData->DisplaySize.y);
			vpInfo.offset = vec2(-1.0f - imDrawData->DisplayPos.x * vpInfo.scale[0],
				-1.0f - imDrawData->DisplayPos.y * vpInfo.scale[1]);
			attr.info = vpInfo;

			//UPDATE IDX,VTX BUFFER
			//They are shared inside drawlist
			//assert(imDrawListCnt == 1);
			auto vtxBuffer = mDP->vertexBuffer;
			auto idxBuffer = mDP->idxBuffer;
			auto attributeIndexBuffer = mDP->triangleAttributeIdxBuffer;
			const ImDrawList* imDrawList = imDrawData->CmdLists[i];
			//1. copy vtx data
			const auto& vtxToCpy = imDrawList->VtxBuffer;
			auto vtxByteSize = sizeof(ImDrawVert) * vtxToCpy.size();
			auto vtxOffset = sizeof(ImDrawVert) * curVtxPos;
			RenderSys->updateBufferData(
				vtxBuffer, vtxToCpy.Data, vtxByteSize, vtxOffset
			);
			//2. copy idx data
			const auto& idxToCpy = imDrawList->IdxBuffer;
			auto idxByteSize = sizeof(ImDrawIdx) * idxToCpy.size();
			auto idxOffset = sizeof(ImDrawIdx) * curIdxPos;
			if (curIdxPos > 0) {
				
				std::vector<ImDrawIdx>finalIndex(idxToCpy.size());
				
				for (int iid = 0;iid < idxToCpy.size();++iid) {
					finalIndex[iid] = idxToCpy.Data[iid] + curVtxPos;
				}
				
				RenderSys->updateBufferData(
					idxBuffer, finalIndex.data(), idxByteSize, idxOffset
				);
			}else {
				RenderSys->updateBufferData(
					idxBuffer, idxToCpy.Data, idxByteSize, idxOffset
				);
			}


			//3. construct attribute data, and setup triangle to drawcall's attribute mapping
			//Look into each im draw command 
			//In imgui
			//Each draw cmd is a drawcall 
			//and each drawcall shares one texture,one scissor
			//draw call only be generated when texture/scissor changed.
			const auto& imCmds = imDrawList->CmdBuffer;
			for (int cmdIdx = 0;cmdIdx < imCmds.size(); ++cmdIdx) {


				const auto& imCmd = imCmds[cmdIdx];
				auto idxBeg = imCmd.IdxOffset;
				auto idxSize = imCmd.ElemCount;
				auto triangleBegin = curTriPos;
				auto triangleCount = u32(idxSize / 3);
				rs_image_view* view = (rs_image_view*)imCmd.GetTexID();

				auto texIdx = RenderSystem::instance()->updateGlobalBindlessDataTexture(
					globalBindless, view
				);
				//This function will try translate view to shader read type
				//And must be called each frame.
				RenderSys->markGlobalBindlessDataTexture(globalBindless, view);
				viewBindlessIndexPairs.push_back({ view,texIdx });
				uint32_t isDepth = view->viewKey.getAspect() == ViewAspect::Depth ? 1 : 0;
				uint32_t drawcallIdToFill = perDrawCallAttributes.size();
				uint32_t texidToFill = view->bindlessIndex;
				assert(view != nullptr && view->viewKey.getUAVAccess() == UAVAccess::ReadOnly && view->bindlessIndex != INVALID_BINDLESS_INDEX);
				if (view == nullptr || view->viewKey.getUAVAccess() != UAVAccess::ReadOnly || view->bindlessIndex == INVALID_BINDLESS_INDEX) {
					assert(false && "This imgui render may be wrong because texture binding error");
					texidToFill = 0;
				}

				curTriPos += triangleCount;
				
				//Fill attribute index.
				std::fill(triAttIdToFill.begin() + triangleBegin, triAttIdToFill.begin() + triangleBegin + triangleCount,
					drawcallIdToFill
				);

				//Fill attribute...
				attr.texIdx = texidToFill;
				auto imDisplaySize = ImGui::GetIO().DisplaySize;
				vec2 displaySize(imDisplaySize.x, imDisplaySize.y);
				vec2 clipMin = (vec2(imCmd.ClipRect.x - clipOff.x, imCmd.ClipRect.y - clipOff.y) * vec2(clipScale.x,clipScale.y) / displaySize ) * 2 - vec2(1);
				vec2 clipMax = (vec2(imCmd.ClipRect.z - clipOff.x, imCmd.ClipRect.w - clipOff.y) * vec2(clipScale.x, clipScale.y) / displaySize ) * 2 - vec2(1);
				attr.scissorMin = clipMin;
				attr.scissorMax = clipMax;
				attr.isDepth = isDepth;
				perDrawCallAttributes.push_back(attr);
			}
			curVtxPos += vtxToCpy.size();
			curIdxPos += idxToCpy.size();

		}
		if (mDP->triangleAttributeIdxBuffer) {
			RenderSys->updateBufferData(mDP->triangleAttributeIdxBuffer, triAttIdToFill.data(), triAttIdToFill.size() * sizeof(uint32_t), 0);
		}

		if (mDP->attributeBuffer)
		{
			//Update per drawcall attribute data....
			RenderSys->updateBufferData(mDP->attributeBuffer, perDrawCallAttributes.data(), sizeof(attr) * perDrawCallAttributes.size(), 0);
		}
		//Update Render entity data.
		auto entity = mDP->imguiRender;
		auto& renderInfo = mDP->imguiRender->getRenderInfo();
		renderInfo.indexType = IndexType::Uint32;
		renderInfo.idxCount = idxSize;
		renderInfo.indexBuffer = nullptr;

		//We can't keep these data for long.....
		for (auto& [view, idx] : viewBindlessIndexPairs) {
			RenderSys->unbindGlobalBindlessDataTexture(globalBindless, idx);
		}

		ImGui::NewFrame();

		/////////////////////////////////////////////////////////
		if(idxSize > 0)
		{
			auto renderPass = (DebugOverlayPass*)RenderSys->getRenderPass(PassName::GUIOverlay);
			renderPass->addDrawEntity(entity);
		}
	}

	void IMGUIManager::update()
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//Display size
		int winx, winy;
		RenderSystem::instance()->getWindowSize(winx, winy);
		io.DisplaySize.x = winx;
		io.DisplaySize.y = winy;
		//Add input info 
		auto inputMgr = InputManager::instance();
		bool needConsume = io.WantCaptureKeyboard;

		//Key?
		for (int i = 0;i < int(KeyCode::Max);++i) {
			KeyCode code = KeyCode(i);
			ImGuiKey imKey = toImKey(code);
			if (imKey == ImGuiKey::ImGuiKey_None)continue;

			bool keyPressed = inputMgr->isKeyPressed(code);
			bool keyReleased = inputMgr->isKeyReleased(code);
			if (keyPressed) {
				io.AddKeyEvent(imKey, true);
			}
			if (keyReleased) {
				io.AddKeyEvent(imKey, false);
			}
			if (needConsume) {
				inputMgr->consumeKey(code);
			}
		}

		//Input text?
		needConsume = io.WantTextInput;
		while (inputMgr->peekChar() > 0) {
			io.AddInputCharacter(inputMgr->consumeChar());
		}

		//Mouse?
		needConsume = io.WantCaptureMouse;
		for (int i = 0;i < (int)MouseButton::Max;++i) {
			MouseButton btn = (MouseButton)i;
			ImGuiKey imBtn = toImKey(btn);
			ImGuiMouseButton_ imMsBtn = toImMouseBtn(btn);
			bool mousePressed = inputMgr->isMousePressed(btn);
			bool mouseRelease = inputMgr->isMouseReleased(btn);
			if (mousePressed) {
				//Mouse is somehow a kind of key in im
				//But it also has its own func....
				if (imMsBtn != ImGuiMouseButton_::ImGuiMouseButton_COUNT) {
					io.AddMouseButtonEvent(imMsBtn, true);
				}
				else {
					io.AddKeyEvent(imBtn, true);
				}
			}

			if (mouseRelease) {
				if (imMsBtn != ImGuiMouseButton_::ImGuiMouseButton_COUNT) {
					io.AddMouseButtonEvent(imMsBtn, false);
				}
				else {
					io.AddKeyEvent(imBtn, false);
				}
			}

			double mouseX,mouseY;
			double mouseScX, mouseScY;
			inputMgr->getCursorPos(mouseX,mouseY);
			inputMgr->getMouseScroll(mouseScX, mouseScY);
			vec2 scroll(mouseScX, mouseScY);
			scroll = scroll * 1. / 100 * mDP->scrollSens;
			io.AddMousePosEvent(mouseX, mouseY);
			io.AddMouseWheelEvent(scroll.x, scroll.y);

			if (needConsume)
			{
				inputMgr->consumeMouse(btn);
				inputMgr->consumeMouseMove();
			}
		}

		//Extra things


		auto curClock = std::chrono::steady_clock::now();
		auto dur = curClock - mDP->mLastFrameTime;
		float dt = std::chrono::duration_cast<std::chrono::microseconds>(dur).count() / 1000. / 1000.;
		if (dt <= 0. || dt > 100.) {
			volatile int break1 = 0;
		}
		mDP->mLastFrameTime = curClock;

		io.DeltaTime = dt;
	}

	void IMGUIManager::setImGuiScrollSensitivity(float x)
	{
		mDP->scrollSens = x;
	}

	void IMGUIManager::init()
	{
		mDP->init();
	}
	void IMGUIManager::deinit()
	{
		mDP->deinit();
	}
	IMGUIManager::~IMGUIManager()
	{
		delete mDP;
		mDP = NULL;
	}

	void IMGUIManager::setTextureSamplerFilter(Filter filter)
	{
		SamplerDesc desc{};
		desc.minFilter = filter;
		desc.magFilter = filter;
		mDP->mImSampler = SamplerResourceManager::instance()->getOrCreateSampler(desc);
		if (mDP->material) {
			mDP->material->bindParameter("u_sampler", mDP->mImSampler);
		}
	}

	ImTextureID toImTex(rs_image_view* view)
	{
		return ImTextureID(view);
	}

	ImGuiKey toImKey(KeyCode code)
	{
		return KeyCodeToImGuiMapInstance[code];
	}

	ImGuiKey toImKey(MouseButton btn)
	{
		return MouseButtonToImGuiKeyMap[(int)btn];
	}

}


