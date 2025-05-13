#ifndef BIT_HELPER_H
#define BIT_HELPER_H

namespace Render::Util {
	template<typename T>
	bool ContainsAll(T a, T b) {
		return (a & b) == b;
	}

	template<typename T>
	bool ContainsAny(T a, T b) {
		return (a & b)!=0;
	}

	template<typename T>
	inline void clearMask(T& value, T mask) {
		value &= ~mask;
	}

	template<typename T>
	void ReplaceBits(T& value, T mask, T newBits) {
		value = (value & ~mask) | (newBits & mask);
	}
};

#endif