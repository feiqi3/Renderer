#ifndef RENDER_ENTITY_H
#define RENDER_ENTITY_H
#include "MaterialVarient.h"
#include <map>
namespace Render {
	struct RenderPendingData {
		ResourceType type;
		union {
			void* dataPtr;		//Data of Per frame update
			void* resourcePtr;  //Ptr of Resource, Like Texture/Storage buffer
		};
		uint32_t size = 0;
	};

	class RenderEntity : Common::NonCopyable{
	public:
		void updateUniformBufferData(rs_binding_pos pos, void* data, uint32_t size);
		void updateBindingBuffer(rs_binding_pos pos, rs_buffer* buffer);
		void updateBindingImage(rs_binding_pos pos, rs_image* image);
		virtual bool isRenderReady() { return true; }
		Material* getMaterial();
		void setMaterial(Material* material);
		rs_buffer* getIndexBuffer();
		IndexType getIndexType()const;
		void setIndexBuffer(rs_buffer* buffer,IndexType type);

		std::vector<VertexBindingInfo >& getBindingBuffers();
	private:
		//TODO: Write every thing into descriptor
		Material* mMaterial = 0;
		RenderInfo mRenderInfo;
		std::map<rs_binding_pos, RenderPendingData> mPendingData;
	};
};

#endif