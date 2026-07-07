#ifndef RENDER_RESOURCE_FUNCTION
#define RENDER_RESOURCE_FUNCTION
#include "render_resource.h"
#include "render_resource_createinfo.h"

namespace Render {

	namespace Window {
		class rs_window;
	};
	uint32_t vertexFormatToSize(VertexFormat format);
	ImageFormat  fromVertexFormatToImageFormat(VertexFormat fmt);
	VertexFormat fromImaegFormatToVertexFormat(ImageFormat fmt);
	rs_context* initRsBackEnd(const BackEndInitDesc& desc);

	rs_buffer* createRsBuffer(rs_context* context,BufferDesc& desc);
	rs_buffer* destroyRsBuffer(rs_context* context,BufferDesc& desc);
	bool isRsBufferMappable(rs_context* context, rs_buffer* buffer);

	rs_image* createRsImage(rs_context* context, ImageDesc& desc);
	rs_image* destroyRsImage(rs_context* context, ImageDesc& desc);
	size_t getRsImageGPUSize(rs_image* image);

	rs_shader_module* createShader(rs_context* context, ShaderDesc& desc);
	rs_shader_module* destroyShader(rs_context* context, ShaderDesc& desc);
	bool queryImgFormatCaps(rs_context* ctx, ImageFormat fmt, FormatCapFlag flags);
	ImageFormat fromRtFormatToImageFormat(rs_context* ctx,RenderTextureFormat fmt);
	RenderTextureFormat fromImageFormatToRtFormat(rs_context* ctx, ImageFormat fmt);
	bool isHDRRtFormat(RenderTextureFormat fmt);

	struct RenderTargetSize {
		uint16_t width = 0;
		uint16_t height = 0;
		uint16_t arrayLayers = 0;
	};

	inline RenderTargetSize getRenderTargetSize(const rs_rendertarget* rt) {
		RenderTargetSize size;

		if (!rt) {
			return size;
		}

		const rs_image* targetImage = nullptr;

		if (!rt->m_attachments.empty() && rt->m_attachments[0] != nullptr) {
			targetImage = rt->m_attachments[0];
		}
		else if (rt->m_depthStencilAttachment != nullptr) {
			targetImage = rt->m_depthStencilAttachment;
		}

		if (targetImage) {
			size.width = targetImage->width;
			size.height = targetImage->height;
			size.arrayLayers = targetImage->arrayLayers; 
		}

		return size;
	}

	inline StageMacroPairs mergeStageMacroPairs(const StageMacroPairs& lhs, const StageMacroPairs& rhs)
	{
		StageMacroPairs result = lhs;

		for (const auto& rhsStagePair : rhs)
		{
			ShaderStage stage = rhsStagePair.first;
			const auto& rhsMacros = rhsStagePair.second;

			auto it = std::find_if(result.begin(), result.end(), [stage](const auto& pair) {
				return pair.first == stage;
				});

			if (it != result.end())
			{
				it->second.insert(it->second.end(), rhsMacros.begin(), rhsMacros.end());
			}
			else
			{
				result.push_back(rhsStagePair);
			}
		}
		return result;
	}

	ImageViewKey genViewKey(ImageType viewType, ViewAspect aspect, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt, UAVAccess uav);
};


#endif