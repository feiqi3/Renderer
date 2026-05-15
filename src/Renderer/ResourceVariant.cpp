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

	void ShaderScopeDataPtr::write(const void* data, u32 size,u32 offset)
	{
		assert(offset + size <= _size);
		memcpy(this->_data + offset,data, std::min((u32)size, _size));
		_Dirty = true;
	}

	unsigned char* ShaderScopeDataPtr::get() const {
		return _data;
	}

	void ShaderScopeDataPtr::setDirty(bool v)
	{
		_Dirty = v;
	}

	bool ShaderScopeDataPtr::getDirty() const
	{
		return _Dirty;
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

	RenderResourceVariant::RenderResourceVariant(rs_buffer* buffer){
		BufferPair bufferPair{.buffer = buffer};
		mData = bufferPair;
	}

	RenderResourceVariant::RenderResourceVariant(u32 size)
	{
		this->mData = ShaderScopeDataPtr(size);
	}

	RenderResourceVariant::RenderResourceVariant(TexturePtr tex, rs_image_view* view)
	{
		if (view) {
			TextureViewPair pair{};
			pair.tex = tex;
			pair.view = view;
			mData = std::move(pair);
		}
		else {
			return;
		}
	}

	void RenderResourceVariant::set(const TexturePtr& tex, ImageViewKey key)
	{
		auto view = RenderSystem::instance()->getViewFromImage(tex->getRsImage(), key);
		if (view) {
			TextureViewPair pair{};
			pair.tex = tex;
			pair.view = view;
			mData = std::move(pair);
		}
		else {
			mData = tex;
		}
	}

	void RenderResourceVariant::set(const TexturePtr& tex, rs_image_view* view)
	{
		if (!tex || !view) {
			//In bindless mode we can bind null parameter 
			//But in Non-Bindless mode, it's not ok.
			return;
		}
		assert(view->image == tex->getRsImage());
		TextureViewPair viewPair{};
		viewPair.tex = tex;
		viewPair.view = view;
		mData = viewPair;
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

	bool RenderResourceVariant::isBuffer() const {
		return std::holds_alternative<BufferPair>(mData);
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
		else if (isBuffer()) {
			return getBufferPair() != nullptr;
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

	const BufferPair* RenderResourceVariant::getBufferPair() const {
		if (isBuffer()) {
			return &(std::get<BufferPair>(mData));
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
			scopedShaderData.write((void*)data, size, 0);
		}
		else {
			ShaderScopeDataPtr ubo(size);
			if (data) {
				ubo.write((void*)data, size,0);
			}
			mData = std::move(ubo);
		}
	}

	ShaderScopeDataPtr* RenderResourceVariant::getUniformDataPtr()
	{
		if (this->isUniformBuffer()) {
			auto& scopedShaderData = std::get<ShaderScopeDataPtr>(mData);
			return &scopedShaderData;
		}
		return nullptr;
	}

	void RenderResourceVariant::writeData(const void* data, u32 size, u32 offset)
	{
		auto& dataPtr = *this->getUniformDataPtr();
		dataPtr.write(data, size,offset);
	}

} // namespace Render