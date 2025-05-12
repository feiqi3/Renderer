#include "volk.h"

namespace Render {

	struct rs_context{
	};

	struct rs_base {
		void* native = 0;
	};


	struct rs_shader : rs_base {};

	struct rs_image : rs_base {};

	struct rs_imageview : rs_base {};

	struct rs_buffer : rs_base {};

	struct rs_sampler : rs_base {};
	
	struct rs_memory : rs_base {};

	struct rs_pipeline : rs_base {};

	struct rs_pipelineLayout : rs_base {};

	struct rs_renderpass : rs_base {};

	struct rs_fence : rs_base {};

	struct rs_semaphore : rs_base {};

	struct rs_event : rs_base {};

	struct rs_commandBuffer : rs_base { };

	class rs_commandPool : rs_base { };

	class rs_descriptorSetPool : rs_base { };

	class rs_descriptorSet : rs_base { };

	class rs_descriptorSetLayout : rs_base { };

	class rs_queue : rs_base { };
};