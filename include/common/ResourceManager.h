#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include "common/CoreDefs.h"
#include "common/Name.h"
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <atomic>

namespace Render {

	enum class ResourceLifetime : u8 {
		Transient,
		Persistent,     // Do not destroy when ref count == 0
		Manual          // Can only be destroyed manually, or when resource manager is destroyed
	};

	enum class ResourceLoadState
	{
		Invalid,
		Unloaded,
		Loading,
		Loaded,
		Failed,
		Reloading
	};

	struct ResourceMemory {
		u32 cpuMemory;
		u32 gpuMemory;
	};

	// ==========================================
	// Interface: IResource
	// ==========================================
	class IResource {
	public:
		inline ResourceLoadState GetState() const { return mState; }
		inline virtual bool IsReady() const { return mState == ResourceLoadState::Loaded; }

		virtual const Name& getTypeName() const = 0;
		virtual ResourceMemory getMemory() const = 0;

		virtual ~IResource();

		virtual void OnLoaded();
		virtual void OnReloadBegin();
		virtual void OnReloadEnd();
		virtual void OnUnload();

	protected:
		ResourceLoadState mState = ResourceLoadState::Invalid;
	};

	using UserDeletor = std::function<bool(const Name&, IResource*)>;

	struct ResourceEntry {
		Name resourceName;
		IResource* resource = nullptr;
		std::atomic<u32> refCount = 0;
		ResourceLifetime lifeTimeCtrl = ResourceLifetime::Transient;
		UserDeletor deletor = nullptr;
	};

	// ==========================================
	// Interface: IResourceManager (Base)
	// ==========================================
	class IResourceManager {
	public:
		virtual ~IResourceManager() = default;
		virtual const Name& typeName() const = 0;

		virtual ResourceEntry* registerResource(const Name& id, IResource* resource, ResourceLifetime lifeTimeCtr, UserDeletor deletor) = 0;
		virtual void createNecessaryPersistenceResources() = 0;
		virtual ResourceEntry* create(const Name& id, ResourceLifetime lifeTimeCtr) = 0;
		virtual void           destroy(const Name& id) = 0; // force delete.
		virtual ResourceEntry* acquireOrCreate(const Name& id) = 0;
		virtual ResourceEntry* acquire(const Name& id) = 0;
		virtual void release(const Name& id) = 0;
		virtual const Name& getDefaultResourceName()const = 0;
		virtual void clearAll() = 0;
		friend class ResourceSystem;
	};


	template<typename T>
	class ResourceManager : public IResourceManager {
	public:
		explicit ResourceManager();
		virtual ~ResourceManager() = default;

		virtual const Name& typeName() const override = 0;

		virtual ResourceEntry* registerResource(const Name& id, IResource* resource, ResourceLifetime lifeTimeCtr, UserDeletor deletor) override;
		virtual ResourceEntry* acquire(const Name& name) override;
		virtual ResourceEntry* create(const Name& name, ResourceLifetime lifeTimeCtr) override;
		virtual void destroy(const Name& name) override;
		virtual ResourceEntry* acquireOrCreate(const Name& name) override;
		virtual void release(const Name& id) override;
		virtual const Name& getDefaultResourceName()const override;
		virtual void clearAll() override;
		inline virtual void createNecessaryPersistenceResources() override {};

		// Debug helper
		size_t debugRefCount(const Name& id) const;

	protected:
		virtual T* loadImpl(const Name& id) = 0;
		virtual void unloadImpl(T* /*resource*/) = 0;

	protected:
		std::mutex m_mutex;
		std::map<Name, std::unique_ptr<ResourceEntry>> m_entries;
	};

} // namespace Render

#include "ResourceManager.inl"

#endif // _RESOURCE_MANAGER_H_