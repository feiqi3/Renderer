#include "platform/FileSystem/FileSystem.h"
#include "platform/FileSystem/WinFileSystem.h"
#include <set>
namespace Render::Platform {

    Render::Platform::FileSystem::FileSystem()
    {
    }

    FileSystem::~FileSystem()
    {
        mFileSystems.clear();
    }

    std::shared_ptr<IFileStream> FileSystem::openFileStream(const std::string& path)
    {
        std::shared_ptr<IFileStream> fileStream = nullptr;
        for (auto&& [priority, filesystem] : mFileSystems) {
            fileStream = filesystem->open(path);
            if(fileStream != nullptr)
            {
                return fileStream;
            }
        }
        return nullptr;
    }

    std::shared_ptr<IFileStream> FileSystem::createFileStream(const std::string& path)
    {
        std::shared_ptr<IFileStream> fileStream = nullptr;
        for (auto&& [priority, filesystem] : mFileSystems) {
            fileStream = filesystem->create(path);
            if (fileStream != nullptr && fileStream->getState() & FileAccess::WRITE)
            {
                return fileStream;
            }
        }
        return nullptr;
    }

    void FileSystem::registerFileSystem(IVirtualFileSystemImpl* fileSystem, u32 priority)
    {
        if (fileSystem == nullptr) {
            assert(0);
            return;
        }
        mFileSystems.insert({ priority,fileSystem });
    }

    void FileSystem::unregisterFileSystem(IVirtualFileSystemImpl* fileSystem)
    {
        for (auto itor = mFileSystems.begin(); itor != mFileSystems.end(); ++itor) {
            if (itor->second == fileSystem) {
                mFileSystems.erase(itor);
                return;
            }
        }
    }

    std::vector<std::string> FileSystem::listDirectory(const std::string& path) const {

        std::set<std::string> files;
        //Let file with high priority overwrite those files with low priority
        for (auto rItor = mFileSystems.rbegin(); rItor != mFileSystems.rend(); rItor++) {
            auto filesCurSys = rItor->second->listDirectory(path);
            for (auto&& name  : filesCurSys) {
                files.insert(name);
            }
        }
        std::vector<std::string> ret;
        for (auto&& file : files) {
            ret.push_back(file);
        }
        return ret;
    }
    bool FileSystem::exists(const std::string& path) const {
        for (auto&& [priority, filesystem] : mFileSystems) {
            if (filesystem->exists(path)) {
                return true;
            }
        }
        return false;
    }

}