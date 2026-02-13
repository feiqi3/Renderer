#ifndef _COMMON_MATH_H_
#define _COMMON_MATH_H_
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include "glm/matrix.hpp"
#include "glm/ext.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
namespace Render {
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using mat2 = glm::mat2;
	using mat3 = glm::mat3;
	using mat4 = glm::mat4;
	using quat = glm::quat;
	using ivec2 = glm::ivec2;
	using ivec3 = glm::ivec3;
	using ivec4 = glm::ivec4;

	inline auto inverse(const auto& m) { return glm::inverse(m); }
	inline auto transpose(const auto& m) { return glm::transpose(m); }
	inline auto determinant(const auto& m) { return glm::determinant(m); }

	inline auto dot(const auto& a, const auto& b) { return glm::dot(a, b); }
	inline auto cross(const vec3& a, const vec3& b) { return glm::cross(a, b); }
	inline auto normalize(const auto& v) { return glm::normalize(v); }
	inline auto length(const auto& v) { return glm::length(v); }
	inline auto distance(const auto& a, const auto& b) { return glm::distance(a, b); }
	inline auto reflect(const auto& I, const auto& N) { return glm::reflect(I, N); }
	inline auto refract(const auto& I, const auto& N, float eta) { return glm::refract(I, N, eta); }

	inline auto translate(const mat4& m, const vec3& v) { return glm::translate(m, v); }
	inline auto rotate(const mat4& m, float angle, const vec3& axis) { return glm::rotate(m, angle, axis); }
	inline auto rotate(const mat4& m, const vec3& v) { return glm::mat4_cast(glm::quat(v)); }
	inline auto rotate(const mat4& m, const quat& v) { return glm::mat4_cast(v); }
	inline auto scale(const mat4& m, const vec3& v) { return glm::scale(m, v); }
	inline auto getTRS(const vec3& t, const vec3& r, const vec3& s) {
		return scale(rotate(translate(mat4(1.0), t), r), s);
	}
	inline bool decompose(
		mat4 const& modelMatrix,
		vec3& scale,
		quat& rotation,
		vec3& translation,
		vec3& skew,
		vec4& perspective
	) {
		return glm::decompose(
			modelMatrix,
			scale,
			rotation,
			translation,
			skew,
			perspective
		);
	}
	inline auto getTRS(const vec3& t, const quat& r, const vec3& s) {
		return scale(rotate(translate(mat4(1.0), t), r), s);
	}
	inline auto lookAt(const vec3& eye, const vec3& center, const vec3& up) {
		return glm::lookAt(eye, center, up);
	}
	inline auto perspective(float fov, float aspect, float znear, float zfar) {
		return glm::perspective(fov, aspect, znear, zfar);
	}
	inline auto ortho(float left, float right, float bottom, float top, float znear, float zfar) {
		return glm::ortho(left, right, bottom, top, znear, zfar);
	}

	inline auto radians(float degrees) { return glm::radians(degrees); }
	inline auto degrees(float radians) { return glm::degrees(radians); }
	inline auto mix(const auto& a, const auto& b, float t) { return glm::mix(a, b, t); }
	inline auto clamp(const auto& v, const auto& min, const auto& max) { return glm::clamp(v, min, max); }
	inline auto smoothstep(float edge0, float edge1, float x) { return glm::smoothstep(edge0, edge1, x); }

	inline auto fromEulerAngles(const vec3& angles) {
		return glm::quat(angles);
	}
}

#endif