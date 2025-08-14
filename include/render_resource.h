#ifndef RENDER_RESOURCE_H_
#define RENDER_RESOURCE_H_
#include <string>
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
namespace Render {

	struct rs_context{
		BackEndInitDesc initDesc;
		std::vector<std::string> physicalDevices;
		uint64_t nextRenderFrame = 0;
		uint64_t curRenderFrame = 0;
		uint32_t maxFrameInFlight = 2;
		std::vector<struct rs_fence*> mFences;
	};

	struct rs_base {
		void* native = 0;
		void* delfunc = 0;
	};


	struct rs_shader_module : rs_base {
		ShaderStage shaderStage;
		std::string entryPoint = "main";
		std::string shaderName = "shader";
		uint64_t shaderHash = 0;
		std::vector<ShaderModuleDescriptorsInfo> reflectInfo;
		std::vector<InputAttribute> inputAttributes;
		std::string shaderCode;
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
	};

	struct rs_buffer : rs_base {
		uint32_t bufferType = BufferType::BufferType_None;
		uint32_t byteSize = 0;
		uint8_t queueType;
		void* mappedPtr = 0;
	};

	struct rs_sampler : rs_base {
	};

	struct rs_binding_info {
		std::vector<BindingInfo> mInfo;
	};

	struct rs_pipeline : rs_base {
		VertexInputDescription vtxInput;
		RenderState renderState;
	};

	struct rs_renderpass : rs_base {
		std::string passName;
		PassDesc passDesc;
		uint32_t height = 0;
		uint32_t width = 0;
		bool haveDepth = false;
		bool writeDepth = false;
		struct rs_rendertarget* renderTarget = 0;
	};

	struct rs_fence : rs_base {};

	struct rs_semaphore : rs_base {};

	struct rs_event : rs_base {};

	struct rs_commandbuffer : rs_base {
		uint8_t queueType;
		rs_renderpass* currentRenderPass = 0;;
		bool isSecondary = false;
		bool isTransitent = false;
		bool recording = false;
		uint32_t lastActiveFrames = 0;
	};

	struct rs_descriptorSetPool : rs_base { };

	struct rs_descriptorSet : rs_base { };

	struct rs_descriptorSetLayout : rs_base { };

	struct rs_swapchain : rs_base {};

	struct rs_queue : rs_base {
		uint8_t queueType;
	};

	struct rs_rendertarget : rs_base {
		std::vector<rs_image*> m_attachments;
		rs_image* m_depthStencilAttachment;
	};

	using rs_descriptor = BindingInfo;
};

#endif