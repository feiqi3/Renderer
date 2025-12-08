#include "platform/FileSystem/WinFileSystem.h"
#include "Windows.h"

namespace Render::Platform::Win{
	WindowsFileStream::WindowsFileStream(const std::string & path, bool createMode):mFile(nullptr),mIsGood(false),mState(FileAccess::INVALID)
	{
		DWORD access = GENERIC_WRITE | GENERIC_READ;
		DWORD creation = createMode ? CREATE_ALWAYS : OPEN_EXISTING;
        mFile = CreateFileA(
            path.c_str(),
            access,
            FILE_SHARE_READ,
            nullptr,
            creation,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (mFile == INVALID_HANDLE_VALUE) {
            mIsGood = false;
            return;
        }

        mState = FileAccess::READ | FileAccess::WRITE;
        mIsGood = true;
	}

    WindowsFileStream::~WindowsFileStream()
    {
        if (mFile != INVALID_HANDLE_VALUE)
            CloseHandle(mFile);
    }

    size_t WindowsFileStream::read(void* buffer, size_t size)
    {
        if (!isGood() || !(mState & FileAccess::READ))
            return 0;

        DWORD bytesRead = 0;
        if (!ReadFile(mFile, buffer, (DWORD)size, &bytesRead, nullptr))
            return 0;

        return bytesRead;
    }

    size_t WindowsFileStream::write(const void* buffer, size_t size)
    {
        if (!isGood() || !(mState & FileAccess::WRITE))
            return 0;

        DWORD bytesWritten = 0;
        if (!WriteFile(mFile, buffer, (DWORD)size, &bytesWritten, nullptr))
            return 0;

        return bytesWritten;
    }

    size_t WindowsFileStream::getSize() const
    {
        if (!isGood())
            return 0;

        LARGE_INTEGER sz;
        if (!GetFileSizeEx(mFile, &sz))
            return 0;

        return (size_t)sz.QuadPart;
    }

    bool WindowsFileStream::seek(size_t offset, bool fromBeginning)
    {
        if (!isGood())
            return false;

        LARGE_INTEGER li;
        li.QuadPart = (LONGLONG)offset;

        DWORD method = fromBeginning ? FILE_BEGIN : FILE_CURRENT;

        LARGE_INTEGER newPos;
        return SetFilePointerEx(mFile, li, &newPos, method);
    }


    std::shared_ptr<IFileStream> WinFileSystem::open(const std::string& path) const
    {
        auto fs = std::make_shared<WindowsFileStream>(path, false);
        if (!fs->isGood()) return nullptr;
        return fs;
    }

    std::shared_ptr<IFileStream> WinFileSystem::create(const std::string& path) const
    {
        auto fs = std::make_shared<WindowsFileStream>(path, true);
        if (!fs->isGood()) return nullptr;
        return fs;
    }

    void WinFileSystem::remove(std::string& path) const
    {
        DeleteFileA(path.c_str());
    }

    bool WinFileSystem::exists(const std::string& path) const
    {
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES);
    }

    std::vector<std::string> WinFileSystem::listDirectory(const std::string& path) const
    {
        std::vector<std::string> result;
        std::string search = path + "\\*";

        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(search.c_str(), &fd);

        if (hFind == INVALID_HANDLE_VALUE)
            return result;

        do {
            const char* name = fd.cFileName;

            // Skip . and ..
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            result.emplace_back(name);

        } while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
        return result;
    }

}