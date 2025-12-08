#include "common/CoreDefs.h"
#include "common/FileSystem.h"
namespace Render::Common::Win {
	class WindowsFileStream : public IFileStream {
	public:
		WindowsFileStream(const std::string& path);
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
}