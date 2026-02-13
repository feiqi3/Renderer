#ifndef SAMPLER_RESOURCE_MANAGER_H_
#define SAMPLER_RESOURCE_MANAGER_H_   
#include "common/Singleton.h"
#include "common/ResourceHandler.h"
#include "common/ResourceManager.h"
#include "render_resource_createinfo.h"
namespace Render {
    struct rs_sampler;
    class Sampler : public IResource {
    public:
        Sampler(const SamplerDesc& desc);
        virtual ~Sampler();

        // IResource 接口
        static const Name& typeName();
        virtual const Name& getTypeName() const override { return typeName(); }
        virtual ResourceMemory getMemory() const override;
        virtual void OnUnload() override;

        rs_sampler* getRsSampler() const { return mRsSampler; }
        const SamplerDesc& getDesc() const { return mDesc; }

    private:
        SamplerDesc mDesc;
        rs_sampler* mRsSampler = nullptr;
    };

	using SamplerPtr = ResourceHandle<Sampler>;

    class SamplerResourceManager :
        public ResourceManager<Sampler>,
        public Singleton<SamplerResourceManager>
    {
    public:
        SamplerResourceManager();
        virtual ~SamplerResourceManager();
        inline virtual const Name& typeName() const override { return Sampler::typeName(); }

        SamplerPtr getOrCreateSampler(const SamplerDesc& desc);

    protected:
        virtual Sampler* loadImpl(const Name& id) override;
        virtual void unloadImpl(Sampler* res) override;
    
    private:
        static inline void hash_combine(std::size_t& seed, size_t value) {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        struct SamplerDescHash {
            std::size_t operator()(const SamplerDesc& d) const {
                std::size_t seed = 0;
                hash_combine(seed, (size_t)d.addressU);
                hash_combine(seed, (size_t)d.addressV);
                hash_combine(seed, (size_t)d.addressW);
                hash_combine(seed, (size_t)d.minFilter);
                hash_combine(seed, (size_t)d.magFilter);
                hash_combine(seed, (size_t)d.mipmapMode);
                hash_combine(seed, (size_t)d.enableAnisotropy);
                hash_combine(seed, (size_t)(d.maxAnisotropy * 100));
                hash_combine(seed, (size_t)d.compareOp);
                hash_combine(seed, (size_t)d.unnormalizedCoords);
                hash_combine(seed, (size_t)d.borderColor);
                hash_combine(seed, (size_t)d.enableCompare);
                return seed;
            }
        };

        struct SamplerDescEqual {
            bool operator()(const SamplerDesc& a, const SamplerDesc& b) const {
                return a.addressU == b.addressU &&
                    a.addressV == b.addressV &&
                    a.addressW == b.addressW &&
                    a.minFilter == b.minFilter &&
                    a.magFilter == b.magFilter &&
                    a.mipmapMode == b.mipmapMode &&
                    a.enableAnisotropy == b.enableAnisotropy &&
                    a.maxAnisotropy == b.maxAnisotropy &&
                    a.compareOp == b.compareOp &&
                    a.unnormalizedCoords == b.unnormalizedCoords &&
                    a.borderColor == b.borderColor &&
                    a.enableCompare == b.enableCompare;
            }
        };
    private:
        std::unordered_map<SamplerDesc, uint32_t, SamplerDescHash, SamplerDescEqual> mDescToIdMap;

        uint32_t mNextSamplerId = 0;

        std::mutex mCacheMutex;
    };
}

#endif