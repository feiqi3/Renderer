#ifndef NAME_H_
#define NAME_H_

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>



class Name;

class StringPool {
public:
	struct StringData {
		const std::string value;
		const size_t hash;

		StringData(std::string str, size_t h)
			: value(std::move(str)), hash(h) {
		}
	};

private:


	std::unordered_map<size_t, std::vector<std::unique_ptr<StringData>>> pool_;
	mutable std::mutex mutex_;
	std::hash<std::string> hasher_;

	StringPool() = default;

public:
	static StringPool& instance() {
		static StringPool pool;  
		return pool;
	}

	const StringData* getOrCreate(const std::string& str) {
		size_t hash_val = hasher_(str);

		std::lock_guard<std::mutex> lock(mutex_);

		auto& bucket = pool_[hash_val];

		for (const auto& data : bucket) {
			if (data->value == str) {
				return data.get(); 
			}
		}

		auto new_data = std::make_unique<StringData>(str, hash_val);
		const StringData* ptr = new_data.get();
		bucket.push_back(std::move(new_data));
		return ptr;
	}

	StringPool(const StringPool&) = delete;
	StringPool& operator=(const StringPool&) = delete;
};

class Name {
private:
	const StringPool::StringData* data_;  
	size_t hash_;  

public:
	explicit Name(const std::string& str)
		: data_(StringPool::instance().getOrCreate(str)), hash_(data_->hash) {
	}

	explicit Name(const char* str)
		: data_(StringPool::instance().getOrCreate(std::string(str))), hash_(data_->hash) {
	}

	Name(std::string_view sv) : Name(std::string(sv)) {}
	Name() : Name("") {}

	Name(const Name& other) = default;
	Name(Name&& other) noexcept = default;
	Name& operator=(const Name& other) = default;
	Name& operator=(Name&& other) noexcept = default;

	bool operator==(const Name& other) const noexcept {
		if (this == &other || data_ == other.data_) return true;

		if (hash_ != other.hash_) return false;

		return data_->value == other.data_->value;
	}

	bool operator!=(const Name& other) const noexcept {
		return !(*this == other);
	}

	bool operator<(const Name& other) const noexcept {
		return hash_ < other.hash_ ||
			(hash_ == other.hash_ && data_->value < other.data_->value);
	}

	const char* c_str() const noexcept { return data_->value.c_str(); }
	std::string_view view() const noexcept { return std::string_view(data_->value); }
	size_t hash() const noexcept { return hash_; }
	const std::string& str() const noexcept { return data_->value; }

	explicit operator std::string() const { return data_->value; }

	const void* address() const noexcept { return static_cast<const void*>(data_); }
};

namespace std {
	template<>
	struct hash<Name> {
		size_t operator()(const Name& name) const noexcept {
			return name.hash();
		}
	};
}

#endif
