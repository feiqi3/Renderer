#ifndef TEXTURE_H_
#define TEXTURE_H_

#include "common/ResourceManager.h"
#include "common/ResourceHandler.h"
#include "platform/FileSystem/FileSystem.h"
#include "RenderSystem.h"
namespace Render {
	
	struct rs_image;

	class ImageRaw {
	public:
		static ImageRaw* createImageRaw(const std::string& path, int wantChannel = -1);

		inline void* getImageRaw() { return pImageData; }
		inline int getWidth()const { return mSizeX; }
		inline int getHeight()const { return mSizeY; }
		inline int getChannel()const { return mChannel; }
		inline bool isHdr()const { return mIsHdr; }
		size_t getByteSize()const;
		rs_image* updateToGPU()const;
		~ImageRaw();
	protected:
		ImageRaw(const std::string& path,int want_channels = -1);
	private:
		void* pImageData = nullptr;
		int mSizeX = 0;
		int mSizeY = 0;
		int mChannel = 0;
		bool mIsHdr = false;
	};

	using TextureResourceManager = ResourceManager<class Texture>;
	class Texture : public IResource {
	public:
		virtual const char* GetTypeName() const override;
		virtual ResourceMemory GetMemory() const override;
		rs_image* getRsImage();
	protected:
		rs_image* pImage = nullptr;
		ResourceMemory mMemory = {};
		friend class ResourceManager<class Texture>;
	};

	using TexturePtr = ResourceHandle<Texture>;
}

#endif