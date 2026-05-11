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

		void write(const void* data,u32 size, u32 offset);

		unsigned char*  get() const;
		void			setDirty(bool v);
		bool			getDirty( )const ;
		u32 size() const;

	private:
		u32   _size = 0;
		bool  _Dirty = true;
		unsigned char* _data = nullptr;
	};

	struct BufferPair {
		rs_buffer* buffer;
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	struct TextureViewPair {
		TexturePtr		tex;
		rs_image_view* view;
	};

	class RenderResourceVariant {
	public:
		RenderResourceVariant();
		explicit RenderResourceVariant(TexturePtr tex);
		explicit RenderResourceVariant(TexturePtr tex,ImageViewKey viewKey);
		explicit RenderResourceVariant(TexturePtr tex, rs_image_view* view);
		explicit RenderResourceVariant(SamplerPtr sampler);
		explicit RenderResourceVariant(rs_buffer* buffer);
		explicit RenderResourceVariant(u32	size);



		template<typename T>
		void set(const T& data);

		void set(const TexturePtr& tex, ImageViewKey key);
		void set(const TexturePtr& tex, rs_image_view* view);

		bool isTextureView()   const;
		bool isTexture()       const;
		bool isSampler()       const;
		bool isUniformBuffer() const;
		bool isBuffer()		   const; 
		bool isValid()         const;
		bool hasResource()     const;

		TexturePtr				getTexture() const;
		const TextureViewPair*  getTextureView()const;
		SamplerPtr				getSampler() const;
		const BufferPair*		getBufferPair() const;
		void					getData(void** data, uint32_t* size)const;
		void					setUniformBuffer(const void* data, u32 size);
		ShaderScopeDataPtr*		getUniformDataPtr();
		void					writeData(const void* data, u32 size, u32 offset = 0);


		~RenderResourceVariant() = default;
		RenderResourceVariant(RenderResourceVariant&&) = default;
		RenderResourceVariant& operator=(RenderResourceVariant&&) = default;
	private:
		std::variant<std::monostate, TextureViewPair, TexturePtr, SamplerPtr, BufferPair, ShaderScopeDataPtr> mData;
	};

	template<typename T>
	inline void RenderResourceVariant::set(const T& data)
	{
		this->mData = data;
	}

} // namespace Render

#endif // RESOURCE_VARIANT_H_