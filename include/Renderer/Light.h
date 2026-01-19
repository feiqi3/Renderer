#ifndef LIGHT_H_
#define LIGHT_H_

#include "common/CommonMath.h"
namespace Render {
	enum class LightType {
		Directional = 0,
		Point = 1,
		Spot = 2
	};

	class Light {
	public:
		inline Light(LightType type, const vec3& color = glm::vec3(1.0f), float intensity = 1.0f)
			: m_type(type), m_color(color), m_intensity(intensity) {
		}

		virtual ~Light() = default;

		inline void setPosition(const glm::vec3& pos) { m_position = pos; }
		inline void setDirection(const glm::vec3& dir) { m_direction = glm::normalize(dir); }
		inline void setColor(const glm::vec3& col) { m_color = col; }
		inline void setIntensity(float i) { m_intensity = i; }
		inline void setRange(float r) { m_range = r; }
		inline void setSpotAngles(float inner, float outer) { m_innerCone = inner; m_outerCone = outer; }

		inline GPULightData toGPUData() const {
			GPULightData data;

			// 1. Position + Type
			data.positionType = glm::vec4(m_position, static_cast<float>(m_type));

			// 2. Direction + Range
			data.directionRange = glm::vec4(m_direction, m_range);

			// 3. Color + Intensity
			data.colorIntensity = glm::vec4(m_color, m_intensity);

			float innerCos = cos(glm::radians(m_innerCone));
			float outerCos = cos(glm::radians(m_outerCone));
			data.spotParams = glm::vec4(innerCos, outerCos, 0.0f, 0.0f);

			return data;
		}

	public: 
		LightType m_type;
		glm::vec3 m_position = glm::vec3(0.0f);
		glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f); 
		glm::vec3 m_color;
		float m_intensity;
		float m_range = 10.0f; 
		float m_innerCone = 20.0f;
		float m_outerCone = 30.0f;
	};
}

#endif