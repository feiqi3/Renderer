#ifndef RENDER_RESOURCE_H_
#define RENDER_RESOURCE_H_
#include <string>
#include <atomic>
#include <array>
#include <set>
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
#include "common/SmallVector.h"
#include <vector>
namespace Render {

	struct rs_context{
		BackEndInitDesc initDesc;
		std::vector<std::string> physicalDevices;
		uint64_t nextRenderFrame = -1;
		uint64_t curRenderFrame = -1;
		uint32_t maxFrameInFlight = 2;
		bool needWireFramePipeline = true;

		uint32_t LogicFrameFif = 0;
		uint32_t RenderFrameFif = 0;
		std::atomic<bool> canRenderNextFrame = false;
		std::array<FormatCapFlag, int(ImageFormat::Invalid)> ImageFormatCaps;
		std::array<ImageFormat, int(RenderTextureFormat::Invalid)> rtFormatMap;
	};

	struct rs_base {
		void* native = 0;
	};

	struct BindlessInfo {
		Name 		bindlessItemName;
		uint32_t 	offset;
	};

	struct rs_shader_reflect_info {
		SmallVector<BindlessInfo,1> bindlessInfo;
		std::vector<BindingInfo>	bindingInfo;
	};

	struct rs_shader_module : rs_base {
		ShaderStage shaderStage;
		std::string entryPoint = "main";
		std::string shaderName = "shader";
		uint64_t shaderHash = 0;
		std::vector<BindingInfo> reflectInfo;
		std::vector<InputAttribute> inputAttributes;
		std::string shaderCode;
	};

	//Actually this is created JIT and is hidden behind API specific functions
	struct rs_image_view : rs_base {
		ImageViewKey viewKey;
		struct rs_image*	 image;
	};

	struct rs_image : rs_base {
		ImageFormat format = ImageFormat::Invalid;
		ImageType type = ImageType::V2D;
		uint16_t width;
		uint16_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t arrayLayers;
		uint32_t usage;
		SampleCount sampleCount = SampleCount::Count1;
		std::vector<ResourceState> subresourceStates;
		//---------------------------//
		rs_image_view defaultView;
		//create in runtime.
		std::list<rs_image_view> imageViews;
	};

	struct rs_buffer : rs_base {
		uint32_t bufferType = BufferType::BufferType_None;
		uint32_t byteSize = 0;
		uint8_t queueType;
		void* mappedPtr = 0;
		ResourceState state = ResourceState::Common;
	};

	struct rs_sampler : rs_base {
	};

	struct rs_pipeline_layout : rs_base {
		std::vector<BindingInfo> bindingInfo;
	};

	struct rs_pipeline : rs_base {
		rs_pipeline_layout* pipelineLayout;
		PipelineType type{};
	};

	struct rs_graphic_pipeline : rs_pipeline {
		RenderState renderState;
		VertexInputDescription vtxInput;
	};

	struct rs_compute_pipeline : rs_pipeline {
	};

	struct rs_renderpass : rs_base {
		std::string passName;
		PassDesc passDesc;
		bool haveDepth = false;
		bool writeDepth = false;
	};

	struct rs_fence : rs_base {};

	struct rs_semaphore : rs_base {};

	struct rs_event : rs_base {};
	struct rs_rendertarget;
	struct rs_commandbuffer : rs_base {
		uint8_t queueType;
		rs_renderpass* currentRenderPass = 0;
		rs_rendertarget* currentRenderTarget = 0;
		std::vector<ClearColor> currentClearColor;
		ClearDepthStencil currentClearDepthStencil;
		bool isSecondary = false;
		bool isTransitent = false;
		bool recording = false;
		uint32_t lastActiveFrames = 0;
		bool hasCommands = false;
		std::vector<std::set<void*>> resourceToBeTransit;
	};

	struct rs_binding_slot{
		UniformType type = UniformType::Count;
		uint16_t fifDirtyFlag = 0xFFFF;
		uint32_t bufferOffset = 0;
		uint32_t bufferSize = 0;
		uint32_t uboDyOffset = 0;
		SmallVector<void*, 1> rsData = {};
	};

	struct rs_descriptorSet : rs_base { 
	};

	struct rs_swapchain : rs_base {
		ImageFormat SwapchainImageFormat;
	};

	struct rs_queue : rs_base {
		uint8_t queueType;
	};

	struct rs_rendertarget : rs_base {
		std::vector<rs_image*> m_attachments;
		std::vector<rs_image_view*> m_views;
		rs_image* m_depthStencilAttachment;
		rs_image_view* m_dsView;
	};



	struct rs_drawdata {
		uint32_t FiFDirtyFlag = 0xFFFFFFFF;
		bool isOneShot = false; //This will lead to some optimize
	};

	using rs_descriptor = BindingInfo;
};

#endif