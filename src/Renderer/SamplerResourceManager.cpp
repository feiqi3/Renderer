#include "Renderer/SamplerResourceManager.h"
#include "Renderer/RenderSystem.h"
namespace Render {
    const Name& Sampler::typeName() {
        static const Name name("Sampler");
        return name;
    }

    Sampler::Sampler(const SamplerDesc& desc) : mDesc(desc) {
        mRsSampler = RenderSystem::instance()->createSampler(desc);
        mState = ResourceState::Loaded;
    }

    Sampler::~Sampler() {
    }

    ResourceMemory Sampler::getMemory() const {
        return { (uint32_t)sizeof(*this), 0 };
    }

    void Sampler::OnUnload() {
    }
    SamplerResourceManager::SamplerResourceManager()
    {
    }
    SamplerResourceManager::~SamplerResourceManager()
    {
        clearAll();
    }
    SamplerPtr SamplerResourceManager::getOrCreateSampler(const SamplerDesc& desc)
    {
        //1. find in cache
        Name samplerName;
        {
            std::lock_guard<std::mutex> lock(mCacheMutex);
			auto it = mDescToIdMap.find(desc);
            if (it == mDescToIdMap.end()) {
                mDescToIdMap[desc] = mNextSamplerId;
                samplerName = Name("Samp_" + std::to_string(mNextSamplerId));
                mNextSamplerId++;

            }
            else {
                samplerName = Name("Samp_" + std::to_string(it->second));

            }

			auto entry = this->acquire(samplerName);
            if (entry) {
                return ResourceHandle<Sampler>(this, entry);
            }
            else {
                //2. create a new one
                auto res = new Sampler(desc);
                auto entry = this->registerResource(samplerName, res, ResourceLifetime::Transient, nullptr);
                return ResourceHandle<Sampler>(this, entry);
            }
		}
        
    }
    Sampler* SamplerResourceManager::loadImpl(const Name& id)
    {
		throw std::runtime_error("SamplerResourceManager::loadImpl should not be called directly.");
        return nullptr;
    }
    void SamplerResourceManager::unloadImpl(Sampler* res)
    {
		RenderSystem::instance()->destroyRsSampler(res->getRsSampler());
        delete res;
    }
}
