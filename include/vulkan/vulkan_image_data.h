#include "vulkan/vulkan_command.h"
#include <vector>
#include <list>
namespace Render::Vulkan {
	class ImageDataManager {
	public:
		ImageDataManager(uint32_t maxFrameInFlight);
		void beginRenderFrame(uint32_t fif, rs_context_vk* ctx);
		void updateImageData(uint32_t fif,rs_context_vk* ctx,rs_image_vk* image,void* data, size_t byteSize,int x,int y,int z,int width,int height,int depth, int layeroff, int layerSize,int mip,bool imm);
		void updateBufferData(uint32_t fif, rs_context_vk* ctx, rs_buffer_vk* buffer, void* data, size_t byteSize,size_t offsetDst,bool imm);
		void clearAll(rs_context_vk* ctx);
		~ImageDataManager();

	private:
		struct PendingBufferUpdateInfo {
			rs_buffer_vk* buffer;
			rs_buffer_vk* pendingBuffer;
			size_t size;
			size_t offsetDst;
		};

		struct PendingUpdateInfo{
			rs_image_vk* target;
			rs_buffer_vk* buffer; size_t datasize;
			int x; int width;
			int y; int height;
			int z; int depth;
			int layeroff;int layersize;
			int mip;
		};

		void recordUpdateCmd(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, PendingUpdateInfo& pendingInfo);
		void recordUpdateCmd(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, PendingBufferUpdateInfo& pendingInfo);
		uint32_t mMaxFrameInFlight;
		std::vector<
			std::list< PendingUpdateInfo>
		> mPendingDataInfo;
		std::vector<
			std::list< PendingBufferUpdateInfo>
		> mPendingBufferDataInfo;
	};
}