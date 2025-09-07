#include "Renderer/RenderEntity.h"
#include "Renderer/RenderSystem.h"
namespace Render {
	void RenderEntity::updateUniformBufferData(rs_binding_pos pos, void* data, uint32_t size)
	{
		auto& pendingData = mPendingData[pos];
		pendingData.type = ResourceType::UniformBuffer;
		pendingData.dataPtr = RenderSystem::instance()->placeFramePendingData(data, size);
		pendingData.size = size;
	}
	void RenderEntity::updateBindingBuffer(rs_binding_pos pos, rs_buffer* buffer)
	{
		auto& pendingData = mPendingData[pos];
		pendingData.type = ResourceType::StorageBuffer;
		pendingData.resourcePtr = buffer;
	}
	void RenderEntity::updateBindingImage(rs_binding_pos pos, rs_image* image)
	{
		auto& pendingData = mPendingData[pos];
		pendingData.type = ResourceType::Texture;
		pendingData.resourcePtr = image;
	}
	Material* RenderEntity::getMaterial()
	{
		return mMaterial;
	}
	void RenderEntity::setMaterial(Material* material)
	{
		mMaterial = material;
		mPendingData.clear();
	}
	rs_buffer* RenderEntity::getIndexBuffer()
	{
		return mRenderInfo.indexBuffer;
	}
	IndexType RenderEntity::getIndexType() const
	{
		return mRenderInfo.indexType;
	}
	void RenderEntity::setIndexBuffer(rs_buffer* buffer, IndexType type)
	{
		mRenderInfo.indexBuffer = buffer;
		mRenderInfo.indexType = type;
	}
	std::vector<VertexBindingInfo>& RenderEntity::getBindingBuffers()
	{
		return mRenderInfo.bindingBuffers;
	}
}
