#ifndef TEXTURE_H_
#define TEXTURE_H_
#pragma once

#include "common/ResourceManager.h"
#include "common/ResourceHandler.h"
#include "platform/FileSystem/FileSystem.h"
#include "RenderSystem.h"
namespace Render {
	
	struct rs_image;

	class ImageRaw {
	public:
		static ImageRaw* createImageRaw(const std::string& path, int wantChannel = -1);
		static ImageRaw* createImageRaw(int width, int height, int channel, bool hdr = false);
		inline void* getImageRaw() { return pImageData; }
		inline int getWidth()const { return mSizeX; }
		inline int getHeight()const { return mSizeY; }
		inline int getChannel()const { return mChannel; }
		inline bool isHdr()const { return mIsHdr; }
		size_t getByteSize()const;
		rs_image* updateToGPU()const;
		class Texture* toTextureResource();
		~ImageRaw();
		ImageFormat getFormat()const;
	protected:
		ImageRaw(const std::string& path,int want_channels = -1);
		ImageRaw(int width, int height, int channel, bool hdr);
	private:
		void* pImageData = nullptr;
		int mSizeX = 0;
		int mSizeY = 0;
		int mChannel = 0;
		bool mIsHdr = false;
		bool mMannul = false;

	};
	class TextureResourceManager;
	class Texture : public IResource {
	public:

		uint32_t getWidth()const ;
		uint32_t getHeight()const ;
		uint32_t getDepth()const ;
		uint32_t getMips()const ;
		uint32_t getArrayLayers()const;
		static const Name& typeName();
		virtual const Name& getTypeName() const override;
		virtual ResourceMemory getMemory() const override;
		rs_image* getRsImage();
		void setRsImage(rs_image* image, bool destroyOld = true);
		static Texture* fromRsImage(rs_image* image);
	protected:
		//Hand over image's lifetime control! 
		rs_image* pImage = nullptr;
		friend class TextureResourceManager;
		friend class ImageRaw;
	};

	using TexturePtr = ResourceHandle<Texture>;
}

#endif