#include <cstring> // for std::memcpy
#include "Renderer/ResourceVariant.h"
namespace Render {

	ShaderScopeDataPtr::ShaderScopeDataPtr(u32 size) : _size(size) {
		if (size > 0) {
			_data = new unsigned char[size];
		}
	}

	ShaderScopeDataPtr::~ShaderScopeDataPtr() {
		delete[] _data;
		_data = nullptr;
		_size = 0;
	}

	ShaderScopeDataPtr::ShaderScopeDataPtr(ShaderScopeDataPtr&& other) noexcept
		: _size(other._size), _data(other._data)
	{
		other._size = 0;
		other._data = nullptr;
	}

	ShaderScopeDataPtr& ShaderScopeDataPtr::operator=(ShaderScopeDataPtr&& other) noexcept {
		if (this != &other) {
			delete[] _data;
			_size = other._size;
			_data = other._data;

			other._size = 0;
			other._data = nullptr;
		}
		return *this;
	}

	unsigned char* ShaderScopeDataPtr::get() const {
		return _data;
	}

	u32 ShaderScopeDataPtr::size() const {
		return _size;
	}

	RenderResourceVariant::RenderResourceVariant() : mData(std::monostate{}) {}

	RenderResourceVariant::RenderResourceVariant(TexturePtr tex){
		mData = std::move(tex);
	}

	RenderResourceVariant::RenderResourceVariant(TexturePtr tex, ImageViewKey viewKey)
	{
		auto view = RenderSystem::instance()->getViewFromImage(tex->getRsImage(), viewKey);
		if (view) {
			TextureViewPair pair{};
			pair.tex = std::move(tex);
			pair.view = view;
			mData = std::move(pair);
		}
		else {
			mData = std::move(tex);
		}
	}

	RenderResourceVariant::RenderResourceVariant(SamplerPtr sampler) : mData(std::move(sampler)) {}

	RenderResourceVariant::RenderResourceVariant(rs_buffer* buffer) : mData(buffer) {}

	RenderResourceVariant::RenderResourceVariant(u32 size)
	{
		this->mData = ShaderScopeDataPtr(size);
	}

	bool RenderResourceVariant::isTextureView() const
	{
		return std::holds_alternative<TextureViewPair>(mData);
	}

	bool RenderResourceVariant::isTexture() const {
		return std::holds_alternative<TexturePtr>(mData);
	}

	bool RenderResourceVariant::isSampler() const {
		return std::holds_alternative<SamplerPtr>(mData);
	}

	bool RenderResourceVariant::isUniformBuffer() const {
		return std::holds_alternative<ShaderScopeDataPtr>(mData);
	}

	bool RenderResourceVariant::isRsBuffer() const {
		return std::holds_alternative<rs_buffer*>(mData);
	}

	bool RenderResourceVariant::isValid() const {
		return !std::holds_alternative<std::monostate>(mData);
	}

	bool RenderResourceVariant::hasResource() const {
		if (isTexture()) {
			return getTexture() != nullptr;
		}
		else if (isSampler()) {
			return getSampler() != nullptr;
		}
		else if (isRsBuffer()) {
			return getRsBuffer() != nullptr;
		}
		else if (isUniformBuffer()) {
			return std::get<ShaderScopeDataPtr>(mData).get() != nullptr;
		}
		return false;
	}

	TexturePtr RenderResourceVariant::getTexture() const {
		if (isTexture()) {
			return std::get<TexturePtr>(mData);
		}
		return nullptr;
	}

	const TextureViewPair* RenderResourceVariant::getTextureView() const
	{
		if (isTextureView()) {
			return &(std::get<TextureViewPair>(mData));
		}
		return nullptr;
	}

	SamplerPtr RenderResourceVariant::getSampler() const {
		if (isSampler()) {
			return std::get<SamplerPtr>(mData);
		}
		return nullptr;
	}

	rs_buffer* RenderResourceVariant::getRsBuffer() const {
		if (isRsBuffer()) {
			return std::get<rs_buffer*>(mData);
		}
		return nullptr;
	}

	void RenderResourceVariant::getData(void** data, uint32_t* size) const
	{
		if (isUniformBuffer()) {
			auto& scopedShaderData = std::get<ShaderScopeDataPtr>(mData);
			*data = scopedShaderData.get();
			*size = scopedShaderData.size();
		}
	}

	void RenderResourceVariant::setUniformBuffer(const void* data, u32 size) {
		if (size == 0) return;
		if (this->isUniformBuffer())
		{
			if (!data)return;
			auto& scopedShaderData = std::get<ShaderScopeDataPtr>(mData);
			auto sizeMax = scopedShaderData.size();
			auto sizeToCp = std::min(size, sizeMax);
			std::memcpy(scopedShaderData.get(), data, sizeToCp);
		}
		else {
			ShaderScopeDataPtr ubo(size);
			if (data) {
				std::memcpy(ubo.get(), data, size);
			}
			mData = std::move(ubo);
		}
	}

} // namespace Render