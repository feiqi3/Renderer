#ifndef WIN_FILE_SYSTEM_H_
#define WIN_FILE_SYSTEM_H_
#include "common/CoreDefs.h"
#include "platform/FileSystem/FileSystem.h"
namespace Render::Platform::Win {
	class WindowsFileStream : public IFileStream {
	public:
		WindowsFileStream(const std::string& path, bool createMode);
		~WindowsFileStream() override;

		size_t read(void* buffer, size_t size) override;
		size_t write(const void* buffer, size_t size) override;
		size_t getSize() const override;
		bool seek(size_t offset, bool fromBeginning) override;
		bool isGood() const override { return mIsGood && mFile != nullptr; }
		u32 getState() const override { return mState; }

	private:
		void* mFile;
		bool mIsGood;
		u32 mState;
	};

	class WinFileSystem : public IVirtualFileSystemImpl {
	public:
		WinFileSystem() = default;
		~WinFileSystem() override = default;

		std::shared_ptr<IFileStream> open(const std::string& path) const override;
		std::shared_ptr<IFileStream> create(const std::string& path) const override;
		void remove(std::string& path) const override;

		bool exists(const std::string& path) const override;

		std::vector<std::string> listDirectory(const std::string& path) const override;
	};
}

#endif