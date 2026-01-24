#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef  STB_IMAGE_IMPLEMENTATION
#include "Renderer/Texture.h"
#include "platform/FileSystem/FileSystem.h"
#include "Renderer/RenderSystem.h"
#include <iostream>
#define HDR_ELE_SIZE 4

namespace Render {
    namespace {

        ImageFormat getImageFormat(int channel,bool isHdr) {
            if (!isHdr) {
                switch (channel) {
                case 1:
                    return ImageFormat::R8_UNORM;
                case 2:
                    return ImageFormat::RG8_UNORM;
                case 3:
                    return ImageFormat::RGBA8_UNORM;
                case 4:
                    return ImageFormat::RGBA8_UNORM;
                default:
                    return ImageFormat::RGBA8_UNORM;
                }
            }
            else {
                switch (channel) {
                case 1:
                    return ImageFormat::R32_SFLOAT;
                case 2:
                    return ImageFormat::RG32_SFLOAT;
                case 3:
                    return ImageFormat::RGB32_SFLOAT;
                case 4:
                    return ImageFormat::RGBA32_SFLOAT;
                default:
                    return ImageFormat::RGBA32_SFLOAT;
                }
            }
            return ImageFormat::RGBA8_UNORM;
        }

        int stbReadCb(void* user, char* data, int size) {
            auto fileStream = (Platform::IFileStream*)user;
            return fileStream->read(data, size);
        }

        void stbSkipCb(void* user, int n) {
            auto fileStream = (Platform::IFileStream*)user;
            fileStream->seek(n, false);
        }

        int stbEofCb(void* user) {
            auto fileStream = (Platform::IFileStream*)user;
            return (fileStream->seek(0, false) >= fileStream->getSize()) ? 1 : 0;
        }
    }

	Texture* Texture::fromRsImage(rs_image* image)
	{
        Texture* texture = new Texture();
        texture->pImage = image;
        return texture;
	}

	const Name& Texture::typeName()
    {
        static const Name sTypeName = Name("Texture");
        return sTypeName;
    }
    const Name& Texture::getTypeName() const
    {
        return typeName();
    }
    ResourceMemory Texture::getMemory() const
    {
        ResourceMemory memory{};
        memory.cpuMemory = sizeof(this) + sizeof(pImage);
        memory.gpuMemory = RenderSystem::instance()->getImageSize(pImage);
        return memory;
    }
    rs_image* Texture::getRsImage()
    {
        return pImage;
    }
    ImageRaw* ImageRaw::createImageRaw(const std::string& path, int wantChannel)
    {
        return new ImageRaw(path,wantChannel);
    }
    ImageRaw* ImageRaw::createImageRaw(int width, int height, int channel, bool hdr)
    {
        return new ImageRaw(width, height,channel, hdr);
    }
    size_t ImageRaw::getByteSize() const
    {
        size_t perEleSize = isHdr() ? HDR_ELE_SIZE : 1;
        return perEleSize * getWidth() * getHeight() * getChannel();
    }
    rs_image* ImageRaw::updateToGPU() const
    {
        size_t perElementSize = isHdr() ? 4 : 1;
        ImageFormat fmt = getImageFormat(this->mChannel, this->mIsHdr);
        return RenderSystem::instance()->createImage2D(this->pImageData, perElementSize * mSizeX * mSizeY * mChannel,fmt, mSizeX,mSizeY,1,1,1);
    }
    Texture* ImageRaw::toTextureResource()
    {
        Texture* tex = new Texture();
        tex->pImage = this->updateToGPU();
        tex->mState = ResourceState::Loaded;
        return tex;
    }
    ImageRaw::~ImageRaw()
    {
        if (!mMannul) {
            stbi_image_free(pImageData);
        }
        else {
            delete[] pImageData;
        }
        pImageData = 0;
    }
    ImageRaw::ImageRaw(const std::string& path, int want_channels)
    {
        auto fileStream = Platform::FileSystem::instance()->openFileStream(path);
        int req_channel = 4;
        stbi_io_callbacks iocb{ .read = stbReadCb,.skip = stbSkipCb,.eof = stbEofCb };
        if (want_channels <= 0) {
            //4 is required to avoid none support format
        }
        else {
            req_channel = want_channels;
        }

        fileStream->seek(0, true);
        mIsHdr = stbi_is_hdr_from_callbacks(&iocb, fileStream.get());
        fileStream->seek(0, true);

        int _ignore = 0;
        if (mIsHdr) {
            pImageData = stbi_loadf_from_callbacks(&iocb, fileStream.get(), &mSizeX, &mSizeY, &_ignore, req_channel);
        }
        else {
            pImageData = stbi_load_from_callbacks(&iocb, fileStream.get(), &mSizeX, &mSizeY, &_ignore, req_channel);
        }
        mChannel = req_channel;
        if (pImageData == 0) {
            std::cerr<<"Load texture failed: reason: " << stbi_failure_reason();
        }
    }
    ImageRaw::ImageRaw(int width, int height, int channel, bool hdr):mSizeX(width),mSizeY(height),mChannel(channel),mIsHdr(hdr), mMannul(true)
    {
        assert(width > 0 && height > 0 && channel >= 1);
        size_t totalSize = size_t(width) * size_t(height) * size_t(channel) * size_t(hdr ? HDR_ELE_SIZE : 1);
        pImageData = new uint8_t[totalSize];
    }
}
