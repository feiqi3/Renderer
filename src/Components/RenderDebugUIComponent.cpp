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
		{
			auto curScene = Scene::getCurrentScene();
			if (!curScene)return nullptr;
			auto shadowTex =  curScene->getShadowMgr().getDirShadowTexture();
			if (!shadowTex)return nullptr;
			return shadowTex->getRsImage()->defaultView;
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
