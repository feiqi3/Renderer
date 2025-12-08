#ifndef __FILE_SYSTEM_H_
#define __FILE_SYSTEM_H_
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "common/CoreDefs.h"
#include "common/Singleton.h"
namespace Render::Platform {
	namespace FileAccess {
		inline u32 INVALID			= 0;
		inline u32 READ				= 1 << 0;
		inline u32 WRITE			= 1 << 1;
	}
	class IFileStream;
	using FileStreamPtr = std::shared_ptr<IFileStream>;

	class IVirtualFileSystemImpl {
	public:
		virtual ~IVirtualFileSystemImpl() = default;

		virtual std::shared_ptr<IFileStream>	open(const std::string& path) const = 0;
		virtual std::shared_ptr<IFileStream>	create (const std::string& path)const = 0;
		virtual void							remove (std::string& path)const = 0;

		virtual bool exists(const std::string& path) const = 0;

		virtual std::vector<std::string> listDirectory(const std::string& path) const = 0;
	};

	class IFileStream {
	public:
	public:
		virtual ~IFileStream() = default;
		virtual size_t read(void* buffer, size_t size) = 0;
		virtual size_t write(const void* buffer, size_t size) = 0;
		virtual size_t getSize() const = 0;
		virtual bool seek(size_t offset, bool fromBeginning) = 0;
		virtual bool isGood() const = 0;
		virtual u32 getState()const = 0;
	};



	class FileSystem : public Singleton< FileSystem> {
	public:
		FileSystem();
		~FileSystem();
		bool exists(const std::string& path) const;
		std::vector<std::string> listDirectory(const std::string& path) const;
		std::shared_ptr<IFileStream> openFileStream(const std::string& path);
		std::shared_ptr<IFileStream> createFileStream(const std::string& path);
		void registerFileSystem(IVirtualFileSystemImpl* fileSystem,u32 priority);
		void unregisterFileSystem(IVirtualFileSystemImpl* fileSystem);
	private:
		//0 is the largest  priority
		std::multimap<u32,IVirtualFileSystemImpl*> mFileSystems;
	};
}

#endif