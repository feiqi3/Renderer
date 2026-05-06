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
		inline explicit Name(const char* str, u32 strLength)
			: data_(StringPool::instance()->getOrGenStringData(str, strLength)),hash_(data_ != nullptr ? data_->mHash : 0) {
		}

		inline explicit Name(const char* str)
			: Name(str, str != nullptr ? u32(strlen(str)) : 0) {
		}

		inline explicit Name(const ::std::string& str)
			: Name(str.data(),u32(str.size())){
		}

		inline bool isEmpty()const { return data_ == nullptr; }

		inline Name(::std::string_view sv) : Name(sv.data(),u32(sv.size())) {}
		inline Name() :Name(nullptr, 0) {}
		~Name() {}
		Name(const Name& other) = default;
		Name(Name&& other) noexcept = default;
		Name& operator=(const Name& other) = default;
		Name& operator=(Name&& other) noexcept = default;
		inline bool operator<(const Name& other) const noexcept {
			return view() < other.view();
		}

		inline bool isEqual(const char* rhs) const noexcept {
			if (rhs == nullptr)return false;
			if (data_ == nullptr)return false;
			return (strcmp(this->c_str(), rhs) == 0);
		}

		inline bool isEqual(const ::std::string& rhs)const noexcept {
			if (data_ == nullptr)return false;
			return (strcmp(this->c_str(), rhs.c_str()) == 0);
		}

		bool operator==(const Name& other) const noexcept {
			if (!data_) {
				return other.data_ == nullptr;
			}
			else {
				return data_->unique_id == other.data_->unique_id;
			}
		}

		inline bool operator!=(const Name& other) const noexcept {
			return !(*this == other);
		}

		inline const char* c_str() const noexcept {
			if (data_)
				return data_->mStringVal;
			else
				return nullptr;
		}
		inline ::std::string_view view() const noexcept {
			if (data_ == nullptr) {
				return ::std::string_view();
			}
			else {
				return ::std::string_view(data_->mStringVal, data_->mStringLen);
			}
		}

		inline size_t hash() const noexcept { return hash_; }

		inline ::std::string str() const noexcept {
			if (data_ == nullptr) {
				return std::string();
			}
			else {
				return ::std::string(data_->mStringVal, data_->mStringLen);
			}
		}

		static const Name _emptyName;
		inline static Name Empty() { return _emptyName; }

	};

	inline const Name Name::_emptyName = Name();

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
