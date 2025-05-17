#ifndef RENDER_RESOURCE_H_
#define RENDER_RESOURCE_H_

#include "render_resource_def.h"
#include "render_resource_createinfo.h"
namespace Render {

	struct rs_context{
	};

	struct rs_base {
		void* native = 0;
		void* delfunc = 0;
	};


	struct rs_shader_module : rs_base {
		ShaderStage shaderStage;
		const char* entryPoint = "main";
	};

	struct rs_image : rs_base {
		ImageFormat format = ImageFormat::Invalid;
		ImageType type = ImageType::V2D;
		uint16_t width;
		uint16_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint16_t arrayLayers;
	};

	struct rs_buffer : rs_base {
		uint32_t bufferType = BufferType::BufferType_None;
		uint32_t byteSize = 0;
	};

	struct rs_sampler : rs_base {
	};

	struct rs_pipeline : rs_base {};

	struct rs_renderpass : rs_base {
		PassDesc passDesc;
	};

	struct rs_fence : rs_base {};

	struct rs_semaphore : rs_base {};

	struct rs_event : rs_base {};

	struct rs_commandbuffer : rs_base {
		QueueType queueType;
		bool isSecondary = false;
		bool isTransitent = false;
		bool recording = false;
	};

	struct rs_descriptorSetPool : rs_base { };

	struct rs_descriptorSet : rs_base { };

	struct rs_descriptorSetLayout : rs_base { };

	struct rs_queue : rs_base {
		uint8_t queueType;
	};
};

#endif