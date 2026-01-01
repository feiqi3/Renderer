#ifndef NAME_H_
#define NAME_H_

#include <vector>
#include <string>
#include <string_view>
#include "common/StringPool.h"


namespace Render {
	class Name {
	private:
		const _StringData* data_;
		size_t hash_;

	public:
		explicit Name(const char* str, u32 strLength)
			: data_(StringPool::instance()->getOrGenStringData(str, strLength)),hash_(data_ != nullptr ? data_->mHash : 0) {
		}

		explicit Name(const char* str)
			: Name(str, str != nullptr ? u32(strlen(str)) : 0) {
		}

		explicit Name(const ::std::string& str)
			: Name(str.data(),str.size()){
		}

		bool isEmpty()const { return data_ == nullptr; }

		Name(::std::string_view sv) : Name(sv.data(),sv.size()) {}
		Name() :Name(nullptr, 0) {}
		~Name() {}
		Name(const Name& other) = default;
		Name(Name&& other) noexcept = default;
		Name& operator=(const Name& other) = default;
		Name& operator=(Name&& other) noexcept = default;
		bool operator<(const Name& other) const noexcept {
			return view() < other.view();
		}
		bool operator==(const Name& other) const noexcept {
			return data_->unique_id == other.data_->unique_id;
		}

		bool operator!=(const Name& other) const noexcept {
			return !(*this == other);
		}

		const char* c_str() const noexcept { return data_->mStringVal; }
		::std::string_view view() const noexcept { return ::std::string_view(data_->mStringVal,data_->mStringLen); }
		size_t hash() const noexcept { return hash_; }
		::std::string str() const noexcept { return ::std::string(data_->mStringVal,data_->mStringLen); }
	};


}

namespace std {
	template<>
	struct hash<Render::Name> {
		size_t operator()(const Render::Name& n) const noexcept {
			return n.hash();
		}
	};
}
#endif
