#ifndef RENDER_RESOURCE_H_
#define RENDER_RESOURCE_H_

#include "render_resource_def.h"
namespace Render {

	struct rs_context{
	};

	struct rs_base {
		void* native = 0;
		void* delfunc = 0;
	};


	struct rs_shader : rs_base {
		ShaderType shaderType;
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

	struct rs_pipelineLayout : rs_base {};

	struct rs_renderpass : rs_base {};

	struct rs_fence : rs_base {};

	struct rs_semaphore : rs_base {};

	struct rs_event : rs_base {};

	struct rs_commandBuffer : rs_base { };

	struct rs_commandPool : rs_base { };

	struct rs_descriptorSetPool : rs_base { };

	struct rs_descriptorSet : rs_base { };

	struct rs_descriptorSetLayout : rs_base { };

	struct rs_queue : rs_base {
		uint8_t queueType;
	};
};

#endif