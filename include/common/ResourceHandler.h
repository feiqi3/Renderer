#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_

#include "common/CoreDefs.h"
#include "common/Name.h"
#include "common/ResourceManager.h"
#include <atomic>
#include <cassert>

namespace Render {

    template<typename T>
    class ResourceHandle {
    private:
        struct ControlBlock {
            ResourceEntry* entry;
            IResourceManager* owner;
            std::atomic<uint32_t> refCount;

            ControlBlock(IResourceManager* mgr, ResourceEntry* e)
                : entry(e), owner(mgr), refCount(1) {
            }
        };

        ControlBlock* m_block = nullptr;

    public:
        ResourceHandle(nullptr_t) : m_block(nullptr) {}
        ResourceHandle() : m_block(nullptr) {}

        ResourceHandle(IResourceManager* mgr, ResourceEntry* e)
        {
            if (e && mgr) {
                m_block = new ControlBlock(mgr, e);
            }
        }

        ResourceHandle(const ResourceHandle& rhs)
            : m_block(rhs.m_block)
        {
            addRef();
        }

        ResourceHandle(ResourceHandle&& rhs) noexcept
            : m_block(rhs.m_block)
        {
            rhs.m_block = nullptr;
        }

        ResourceHandle& operator=(const ResourceHandle& rhs) {
            if (this != &rhs) {
                release(); 
                m_block = rhs.m_block; 
                addRef();  
            }
            return *this;
        }

        ResourceHandle& operator=(ResourceHandle&& rhs) noexcept {
            if (this != &rhs) {
                release();
                m_block = rhs.m_block;
                rhs.m_block = nullptr;
            }
            return *this;
        }

        ~ResourceHandle() {
            release();
        }

        T* operator->() const { return isValid() ? (T*)m_block->entry->resource : nullptr; }
        T& operator*() const { assert(isValid()); return *((T*)m_block->entry->resource); }
        T* get() const { return isValid() ? (T*)m_block->entry->resource : nullptr; }

        bool isValid() const { return m_block != nullptr && m_block->entry != nullptr; }
        bool valid() const { return isValid(); }

    private:
        void addRef() {
            if (m_block) {
                m_block->refCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        void release() {
            if (!m_block) return;

            uint32_t old = m_block->refCount.fetch_sub(1, std::memory_order_acq_rel);

            if (old == 1) {
                if (m_block->owner && m_block->entry) {
                    m_block->owner->release(m_block->entry->resourceName);
                }

                delete m_block;
            }

            m_block = nullptr;
        }
    };
}
#endif