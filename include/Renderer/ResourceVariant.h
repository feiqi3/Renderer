#ifndef RESOURCE_VARIANT_H_
#define RESOURCE_VARIANT_H_

#include <variant>
#include "Renderer/Texture.h" 
#include "Renderer/SamplerResourceManager.h"

struct rs_buffer;

namespace Render {

	class ShaderScopeDataPtr {
	public:
		ShaderScopeDataPtr() = default;
		explicit ShaderScopeDataPtr(u32 size);
		~ShaderScopeDataPtr();

		ShaderScopeDataPtr(const ShaderScopeDataPtr&) = delete;
		ShaderScopeDataPtr& operator=(const ShaderScopeDataPtr&) = delete;

		ShaderScopeDataPtr(ShaderScopeDataPtr&& other) noexcept;
		ShaderScopeDataPtr& operator=(ShaderScopeDataPtr&& other) noexcept;

		unsigned char* get() const;
		u32 size() const;

	private:
		u32   _size = 0;
		unsigned char* _data = nullptr;
	};

	class RenderResourceVariant {
	public:
		RenderResourceVariant();
		explicit RenderResourceVariant(TexturePtr tex);
		explicit RenderResourceVariant(SamplerPtr sampler);
		explicit RenderResourceVariant(rs_buffer* buffer);

		template<typename T>
		void set(const T& data);

		bool isTexture()       const;
		bool isSampler()       const;
		bool isUniformBuffer() const;
		bool isRsBuffer()      const; 
		bool isValid()         const;
		bool hasResource()     const;

		TexturePtr getTexture() const;
		SamplerPtr getSampler() const;
		rs_buffer* getRsBuffer() const; 

		void setUniformBuffer(const void* data, u32 size);

		~RenderResourceVariant() = default;
		RenderResourceVariant(RenderResourceVariant&&) = default;
		RenderResourceVariant& operator=(RenderResourceVariant&&) = default;
	private:
		std::variant<std::monostate, TexturePtr, SamplerPtr, rs_buffer*, ShaderScopeDataPtr> mData;
	};

	template<typename T>
	inline void RenderResourceVariant::set(const T& data)
	{
		this->mData = data;
	}

} // namespace Render

#endif // RESOURCE_VARIANT_H_