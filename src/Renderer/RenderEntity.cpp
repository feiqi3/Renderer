#include "Renderer/RenderEntity.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/MaterialInstance.h"
namespace Render {
	RenderEntity::~RenderEntity()
	{
		for (auto&& [passName, pass] : mPasses) {
			destroyPass(pass);
		}
		mPasses.clear();
	}
	Material* RenderEntity::getMaterial()
	{
		return nullptr;
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

	rs_drawdata* RenderEntity::getDrawData(const Name& passName)
	{
		auto itor = this->mPasses.find(passName);
		if (itor == mPasses.end()) {
			return nullptr;
		}
		auto pass = itor->second;
		return pass->mDrawData;
	}

	Pass* RenderEntity::createPass(const Name& passName)
	{
		if (this->mPasses.find(passName) != mPasses.end()) {
			return mPasses[passName];
		}
		auto material = this->getMaterial();
		if (material) {
			auto Variant = getMaterial()->getMaterialPass(passName);
			if (Variant) {
				auto pass = new Pass;
				pass->mDrawData = RenderSystem::instance()->createDrawData();
				pass->mMaterial = Variant;
				this->mPasses.insert({ passName,pass });
				return pass;
			}
		}
		return nullptr;
	}

	void RenderEntity::destroyPass(const Name& passName)
	{
		auto renderSys = RenderSystem::instance();
		auto pass = getPass(passName);
		if (pass) {
			destroyPass(pass);
			mPasses.erase(passName);
		}
		
	}

	Pass* RenderEntity::getPass(const Name& passName)
	{
		if (mPasses.size() < 10) {
			//fast path
			for (const auto& [name, pass] : mPasses) {
				if (name == passName) {
					return pass;
				}
			}
			return nullptr;
		}
		else {
			auto itor = this->mPasses.find(passName);
			if (itor == mPasses.end()) {
				return nullptr;
			}
			return itor->second;
		}
	}

	void RenderEntity::destroyPass(Pass* pass)
	{
		auto renderSys = RenderSystem::instance();
		renderSys->destroyDrawData(pass->mDrawData);
		delete pass;
		pass = 0;
	}

	void RenderEntity::clear() {
		RenderSystem::instance()->clearRenderEntity(this);
	}

	void RenderEntity::setModelMatrix(const mat4& mat)
	{
		mModelMatrix = mat;
	}

	const mat4& RenderEntity::getModelMatrix() const
	{
		return mModelMatrix;
	}

	vec3 RenderEntity::getWorldPos() const
	{
		glm::vec3 translation = glm::vec3(mModelMatrix[3]);
		return translation;
	}
	
}
