#include "Components/RenderDebugUIComponent.h"
#include "function/TimeSystem.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/RenderPass.h"
#include "imgui.h"
#include "Renderer/UI/ImGuiUtils.h"
#include "function/Scene.h"
#include "Renderer/ShadowManager.h"
#include "common/ResourceSystem.h"
#include "function/Object.h"
#include "function/Scene.h"
namespace Render {
	void Render::RenderDebugUIComponent::onUpdate(float dt)
	{

		if (ImGui::Begin("RenderDebugUI##UI")) {
			ImGui::BeginChild("RenderDebugINFO##UIDEBUG",ImVec2(500,0),ImGuiChildFlags_None,ImGuiWindowFlags_NoDecoration);
			float logicFrameTime	= TimeSystem::instance()->getLogicFrameTime();
			float renderFrameTime	= RenderSystem::instance()->getLastRenderFrameTime();
			ImGui::Text("Logic frame time: %f ms | Render frame time: %f ms", logicFrameTime, renderFrameTime);
			ImGui::Text("Delta time: %f ms", TimeSystem::instance()->getDeltaTime());
			writePassTable();
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("RenderDebugImage##UIDEBUG", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoDecoration);
			
			static int comboSelect = (int)DebugView::Depth;
			ImGui::Combo("Debug view select##DEBUG UI",&comboSelect, DebugViewName, (int)DebugView::Max + 1);
			auto texView = this->getDebugTextureView((DebugView)comboSelect);
			if (texView) {
				auto imgSizeToShow = ImGui::GetContentRegionAvail();
				ImGui::Image(toImTex(texView), imgSizeToShow);
			}
			ImGui::EndChild();

			ImGui::BeginChild("RenderDebugShadow##UIDEBUG", ImVec2(0, 0));
			ImGui::BulletText("Shadow Settings");
			auto& shadowMgr = owner()->scene()->getShadowMgr();
			static float shadowDist = 10.;
			bool shadowDistChanged = ImGui::SliderFloat("Directional light shadow distance##DEBUGUI", &shadowDist, 0.5, 100, "%.1f");
			if (shadowDistChanged) {
				//shadowMgr.setDirLightShadowFarZ(shadowDist);
			}
			static int shadowMapResolution = 1024;
			bool shadowMapResChanged = ImGui::InputInt("Directional light shadow texture resolution##DEBUGUI", &shadowMapResolution);
			if (shadowMapResChanged) {
				shadowMgr.setShadowTexSize(shadowMapResolution);
			}

			static const char* ShadowTechniques[] = {
				"Normal",
				"PCF",
			};

			auto techniques = sizeof(ShadowTechniques) / sizeof(const char*);
			static int selectedTechnique = 0; //Normal
			ImGui::Combo("Shadow techniques", &selectedTechnique, ShadowTechniques, sizeof(ShadowTechniques) / sizeof(const char*));
			shadowMgr.setShadowTechnique((ShadowManager::ShadowTechnique)selectedTechnique);


			auto CSM = shadowMgr.getCascadedShadow();
			if(ImGui::CollapsingHeader("CSM Layer Distance##DEBUGUI"))
			{
				auto curCsmL0Dist = CSM->getCascadedLayerDistance(0);
				auto curCsmL1Dist = CSM->getCascadedLayerDistance(1);
				auto curCsmL2Dist = CSM->getCascadedLayerDistance(2);

				curCsmL0Dist = std::max(0.f, curCsmL0Dist);
				curCsmL1Dist = std::max(curCsmL0Dist, curCsmL1Dist);
				curCsmL2Dist = std::max(curCsmL1Dist, curCsmL2Dist);

				bool changed0 = ImGui::InputFloat("CSM_LAYER0_dist", &curCsmL0Dist);
				bool changed1 = ImGui::InputFloat("CSM_LAYER1_dist", &curCsmL1Dist);
				bool changed2 = ImGui::InputFloat("CSM_LAYER2_dist", &curCsmL2Dist);



				if (changed0) CSM->setCascadedLayerDistance(0, curCsmL0Dist);
				if (changed1) CSM->setCascadedLayerDistance(1, curCsmL1Dist);
				if (changed2) CSM->setCascadedLayerDistance(2, curCsmL2Dist);
			}
			
			if (ImGui::CollapsingHeader("CSM Cull Size##DEBUGUI"))
			{
				auto curCsmL0CullSize = CSM->getCascadedProjectionCullSize(0);
				auto curCsmL1CullSize = CSM->getCascadedProjectionCullSize(1);
				auto curCsmL2CullSize = CSM->getCascadedProjectionCullSize(2);

				curCsmL0CullSize = std::max(0.f, curCsmL0CullSize);
				curCsmL1CullSize = std::max(curCsmL0CullSize, curCsmL1CullSize);
				curCsmL2CullSize = std::max(curCsmL1CullSize, curCsmL2CullSize);

				bool changed0 = ImGui::InputFloat("CSM_LAYER0_CullSize", &curCsmL0CullSize);
				bool changed1 = ImGui::InputFloat("CSM_LAYER1_CullSize", &curCsmL1CullSize);
				bool changed2 = ImGui::InputFloat("CSM_LAYER2_CullSize", &curCsmL2CullSize);


				if (changed0) CSM->setCascadedProjectionCullSize(0, curCsmL0CullSize);
				if (changed1) CSM->setCascadedProjectionCullSize(1, curCsmL1CullSize);
				if (changed2) CSM->setCascadedProjectionCullSize(2, curCsmL2CullSize);
			}
			
			ImGui::EndChild();
		}
		ImGui::End();
	
	}

	void RenderDebugUIComponent::writePassTable()
	{
		auto passMgr = RenderSystem::instance()->getRenderPassManager();
		std::map<RenderPass*, std::vector<Name>> logicPasses;
		passMgr->traversalAllPasses(
			[&logicPasses](const Name& name, RenderPass* pass) {
				logicPasses[pass].push_back(name);
			}
		);

		if (ImGui::BeginTable("Pass Time## RENDER DEBUG UI",2, ImGuiTableFlags_Resizable)) {
			//Header defines
			ImGui::TableSetupColumn("Logic Passes");
			ImGui::TableSetupColumn("RenderPass Total Time");
			
			ImGui::TableHeadersRow();
			for (auto& [pass, passNames] : logicPasses) {
				if (pass->getPassTime() < 0)continue;
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Separator();
				ImGui::TableSetColumnIndex(1);
				ImGui::Separator();
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				for (auto& name : passNames) {
					ImGui::Text("%s | %f", name.c_str(),pass->getLogicPassTimes(name));
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%f",pass->getPassTime());
			}

			ImGui::EndTable();
		}
	}

	rs_image_view* RenderDebugUIComponent::getDebugTextureView(DebugView view)
	{
		switch (view)
		{
		case Render::RenderDebugUIComponent::DebugView::Depth:
		{
			if (!mainDepth) {
				mainDepth = ResourceSystem::instance()->getResource<Texture>(Texture::typeName(), Name("MainDepthTexture"));
			}
			if (!mainDepth)return nullptr;
			ImageViewKey key;
			key.setAspect(ViewAspect::Depth).setUAVAccess(UAVAccess::ReadOnly);
			return RenderSystem::instance()->getViewFromImage(mainDepth->getRsImage(), key);
		}
			break;
		case Render::RenderDebugUIComponent::DebugView::DirShadow:
		case Render::RenderDebugUIComponent::DebugView::DirShadow_CSM_1:
		case Render::RenderDebugUIComponent::DebugView::DirShadow_CSM_2:
		case Render::RenderDebugUIComponent::DebugView::DirShadow_CSM_3:
		{
			int csmLayer = (int)view - (int)Render::RenderDebugUIComponent::DebugView::DirShadow;
			auto curScene = Scene::getCurrentScene();
			if (!curScene)return nullptr;
			auto shadowTex =  curScene->getShadowMgr().getDirShadowTexture();
			if (!shadowTex)return nullptr;
			auto img = shadowTex->getRsImage();
			if (img->arrayLayers <= csmLayer) {
				return nullptr;
			}
			ImageViewKey key{};
			key.setBaseLayer(csmLayer).setAspect(ViewAspect::Depth);
			return RenderSystem::instance()->getViewFromImage(img,key);
		}
		break;
		case Render::RenderDebugUIComponent::DebugView::Max:
			break;
		default:
			break;
		}
		return nullptr;
	}

}
