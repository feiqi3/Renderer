#include "glm/matrix.hpp"
#include "glm/ext.hpp"

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

	auto inverse(const auto& m) { return glm::inverse(m); }
	auto transpose(const auto& m) { return glm::transpose(m); }
	auto determinant(const auto& m) { return glm::determinant(m); }

	auto dot(const auto& a, const auto& b) { return glm::dot(a, b); }
	auto cross(const vec3& a, const vec3& b) { return glm::cross(a, b); }
	auto normalize(const auto& v) { return glm::normalize(v); }
	auto length(const auto& v) { return glm::length(v); }
	auto distance(const auto& a, const auto& b) { return glm::distance(a, b); }
	auto reflect(const auto& I, const auto& N) { return glm::reflect(I, N); }
	auto refract(const auto& I, const auto& N, float eta) { return glm::refract(I, N, eta); }

	auto translate(const mat4& m, const vec3& v) { return glm::translate(m, v); }
	auto rotate(const mat4& m, float angle, const vec3& axis) { return glm::rotate(m, angle, axis); }
	auto scale(const mat4& m, const vec3& v) { return glm::scale(m, v); }
	auto lookAt(const vec3& eye, const vec3& center, const vec3& up) {
		return glm::lookAt(eye, center, up);
	}
	auto perspective(float fov, float aspect, float near, float far) {
		return glm::perspective(fov, aspect, near, far);
	}
	auto ortho(float left, float right, float bottom, float top, float near, float far) {
		return glm::ortho(left, right, bottom, top, near, far);
	}

	auto radians(float degrees) { return glm::radians(degrees); }
	auto degrees(float radians) { return glm::degrees(radians); }
	auto mix(const auto& a, const auto& b, float t) { return glm::mix(a, b, t); }
	auto clamp(const auto& v, const auto& min, const auto& max) { return glm::clamp(v, min, max); }
	auto smoothstep(float edge0, float edge1, float x) { return glm::smoothstep(edge0, edge1, x); }
}