#include "vulkan/vulkan_render_function.h"
#include "render_resource_createInfo.h"
#include "vulkan/vulkan_pipeline.h"
#include "window/render_resource_window_glfw.h"
#include "vulkan/vulkan_command.h"
#include <iostream>
#include <fstream>

void renderLoop(Render::Vulkan::rs_context_vk* render_context,Render::Window::rs_window_glfw* window);
void initSomeThing(Render::Vulkan::rs_context_vk* render_context);

int main() {
	using namespace Render::Vulkan;
	using namespace Render;
	BackEndInitDesc backEndDesc;
	Window::rs_window_glfw* window = new Window::rs_window_glfw("Hello world", 800, 600);
	backEndDesc.appName = "Test";
	backEndDesc.engineName = "Test";
	backEndDesc.enableValidation = true;
	auto ctx = initVulkanBackEnd(backEndDesc, window);
	renderLoop(ctx, window);
	deinitVulkanBackEnd(ctx, window);
}

void renderLoop(Render::Vulkan::rs_context_vk* render_context, Render::Window::rs_window_glfw* window)
{
	using namespace Render::Vulkan;
	using namespace Render;

	using namespace Render::Vulkan;
	using namespace Render;

	const char* filenameVs = "../shader/test.vs";
	const char* filenamePs = "../shader/test.ps";
	std::string vsCode;
	std::string psCode;

	{
		std::ifstream fileVs(filenameVs, std::ios::in);
		fileVs.seekg(0, std::ios::beg);
		fileVs.seekg(0, std::ios::end);
		auto len = fileVs.tellg();
		fileVs.seekg(0, std::ios::beg);
		vsCode.resize(len);
		vsCode.resize(vsCode.size() + 1,'\0');
		fileVs.read(vsCode.data(), len);

		std::ifstream filePs(filenamePs, std::ios::in);
		filePs.seekg(0, std::ios::beg);
		filePs.seekg(0, std::ios::end);
		len = filePs.tellg();
		filePs.seekg(0, std::ios::beg);
		psCode.resize(len);
		psCode.resize(psCode.size() + 1,'\0');
		filePs.read(psCode.data(), len);
	}
	
	auto maxFif = render_context->maxFrameInFlight;

	ShaderCompileDesc compileDesc{};
	compileDesc.stage = ShaderStage::Vertex;
	compileDesc.langType = ShaderLang::GLSL;
	compileDesc.shaderName = "test.vs";
	compileDesc.shaderSrcCode = vsCode;
	compileDesc.generateDebugInfo = true;

	ShaderDesc shaderVtxDesc;
	shaderVtxDesc.entryPoint = "main";
	shaderVtxDesc.shaderCode = vsCode.data();
	shaderVtxDesc.codeSizeByte = vsCode.size();

	shaderVtxDesc.compileDesc = &compileDesc;
	shaderVtxDesc.isSpirv = false;
	auto shaderModuleVs = createRsShader(render_context, shaderVtxDesc);

	ShaderDesc shaderPsDesc;
	shaderPsDesc.entryPoint = "main";
	shaderPsDesc.compileDesc = &compileDesc;
	compileDesc.stage = ShaderStage::Fragment;
	compileDesc.shaderName = "test.ps";
	compileDesc.shaderSrcCode = psCode;

	shaderVtxDesc.compileDesc = &compileDesc;
	shaderVtxDesc.isSpirv = false;
	auto shaderModulePs = createRsShader(render_context, shaderPsDesc);
	PipelineDesc pipelineDesc;
	pipelineDesc.shaders = { shaderModuleVs ,shaderModulePs };
	pipelineDesc.type = PipelineType::Graphics;

	auto& renderState = pipelineDesc.renderState;
	renderState.depthTestEnable = false;

	auto& iaDesc = pipelineDesc.vertexInputDesc;
	InputBufferBinding bufferBinding{};
	InputAttribute inputAttr{};
	inputAttr.binding = 0;
	inputAttr.format = VertexFormat::RG32_SFLOAT;
	inputAttr.location = 0;
	inputAttr.offset = 0;
	bufferBinding.perInstance = false;
	bufferBinding.stride = 8;
	iaDesc.bindings.push_back(bufferBinding);
	iaDesc.attributes.push_back(inputAttr);
	
	std::vector<rs_rendertarget_vk*> mainRts;
	mainRts.resize(render_context->maxSwapChainImages);
	for (int i = 0; i < mainRts.size(); ++i) {
		mainRts[i] = createRsRenderTarget(render_context, { render_context->swapchain->swapchainImgs[i] }, 0);
	}
	PassDesc passDesc{};
	PassAttachment attachment{};
	attachment.isHDR = false;
	attachment.loadOp = StorageOp::Clear;
	attachment.storeOp = StorageOp::Cached;
	passDesc.attachments.push_back(attachment);
	auto renderPasses = std::vector <rs_renderpass_vk*>(mainRts.size());
	auto pipelines = std::vector < rs_pipeline_vk*>(mainRts.size());
	for (int i = 0; i < mainRts.size(); ++i) {
		renderPasses[i] = createRsRenderPass(render_context, mainRts[i], passDesc);
		pipelines[i] = createRsPipeline(render_context, renderPasses[i], pipelineDesc);
	}

	std::vector<float> floatVec = { 0.0f, -0.5f ,0.5f,  0.5f ,-0.5f,  0.5f };
	BufferDesc bufferDesc{};
	bufferDesc. byteSize = floatVec.size() * sizeof(float);
	bufferDesc. bufUsage = BufferType::BufferType_Vertex;
	bufferDesc. queueType = QueueType_Graphics;
	bufferDesc.	mappable = true;
	auto vsBuffer = createRsBuffer(render_context, bufferDesc);
	void* mappAddr = mapRsBuffer(render_context, vsBuffer);
	memcpy(mappAddr, floatVec.data(), sizeof(float) * floatVec.size());
	
	std::vector<uint32_t> indexVec = { 0,1,2};
	BufferDesc bufferIndexDesc{};
	bufferIndexDesc. byteSize = indexVec.size() * sizeof(uint32_t);
	bufferIndexDesc. bufUsage = BufferType::BufferType_Index;
	bufferIndexDesc. queueType = QueueType_Graphics;
	bufferIndexDesc. mappable = true;
	auto idxBuffer = createRsBuffer(render_context, bufferIndexDesc);
	mappAddr = mapRsBuffer(render_context, idxBuffer);
	memcpy(mappAddr, indexVec.data(), sizeof(uint32_t) * indexVec.size());
	auto descriptorSetMgr = render_context->descriptorSetMgr;
	auto cmdBufferMgr = render_context->cmdBufferMgr;
	float setData[4] = {0.f,1.f,1.f,1.f};
	RenderInfo renderInfo;
	renderInfo.bindingBuffers.push_back({0,vsBuffer });                      //in buffers
	renderInfo.indexBuffer = idxBuffer;
	renderInfo.idxCount = 3;
	renderInfo.instanceCount = 1;
	std::vector< ClearColor> clrCol = { {} };
	std::vector<rs_semaphore_vk*> semphoresToWait = std::vector<rs_semaphore_vk*>(maxFif);
	std::vector<rs_semaphore_vk*> semphoresToSignal = std::vector<rs_semaphore_vk*>(maxFif);
	std::vector<rs_fence_vk*> fenceToWait = std::vector<rs_fence_vk*>(maxFif);
	for (auto i = 0; i < semphoresToWait.size(); ++i) {
		semphoresToWait[i] = createRsSemaphore(render_context);
		semphoresToSignal[i] = createRsSemaphore(render_context);
		fenceToWait[i] = createRsFence(render_context);
		resetRsFence(render_context, fenceToWait[i]);
	}
	ClearDepthStencil clrDep{};
	Rect2D viewportSize{};
	viewportSize.l = 0.f;
	viewportSize.r = 1.f;
	viewportSize.t = 0.f;
	viewportSize.b = 1.f;
	while (!window->shouldClose()) {
		window->pollEvents();
		using namespace std::chrono;
		beginRsFrameVk(render_context);
		auto nxtRenderFrame = render_context->curRenderFrame;
		auto curFif = nxtRenderFrame % maxFif;
		auto curImg = nxtRenderFrame % render_context->maxSwapChainImages;
		auto nxtImg = waitForNextPresentImage(render_context, semphoresToWait[curFif], 0);
		auto descriptorSet = descriptorSetMgr->AllocateDescriptorSet(nxtRenderFrame, render_context, pipelines[curFif],0);
		descriptorSetMgr->updateBufferData(nxtRenderFrame, render_context, descriptorSet, 0, setData, sizeof(float) * 4, QueueType_Graphics);
		renderInfo.pipeline = pipelines[curFif];
		renderInfo.descriptors.resize(1);
		renderInfo.descriptors[0] = { 0,descriptorSet };
		auto cmdbuffer = cmdBufferMgr->getCmdBufferLocalThread(render_context, nxtRenderFrame, QueueType_Graphics);
		cmdBeginRecord(cmdbuffer);
		cmdBeginRenderPass(cmdbuffer, renderPasses[curImg], clrCol, clrDep);
		cmdSetViewport(cmdbuffer, viewportSize,0.f,1.f, 0);
		cmdSetScissor(cmdbuffer, viewportSize, 0);
		cmdDrawIndexed(cmdbuffer, renderInfo, false);
		cmdEndRenderPass(cmdbuffer);
		cmdEndRecord(cmdbuffer);
		cmdSubmitCmdBuffer(render_context, cmdbuffer, QueueType_Graphics, { semphoresToWait[curFif] }, { semphoresToSignal[curFif] }, render_context->mFences[curFif]);
		render_context->cmdBufferMgr->submitFrame(render_context, render_context->nextRenderFrame);
		submitToPresentImage(render_context, nxtImg, { semphoresToSignal[curFif] });
		descriptorSetMgr->ReturnDescriptorSet(nxtRenderFrame, render_context, descriptorSet);
		endRsFrameVk(render_context);
	};
}