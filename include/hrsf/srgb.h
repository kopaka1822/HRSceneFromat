#pragma once
#include <algorithm>
#include <glm/glm.hpp>

namespace hrsf
{
	inline constexpr float toSrgb(float value)
	{
		if (value >= 1.0f) return 1.0f;
		if (value <= 0.0) return 0.0f;
		if (value <= 0.0031308f) return 12.92f * value;
		return 1.055f * std::pow(value, 0.41666f) - 0.055f;
	}

	inline glm::vec3 toSrgb(glm::vec3 value)
	{
		value[0] = toSrgb(value[0]);
		value[1] = toSrgb(value[1]);
		value[2] = toSrgb(value[2]);
		return value;
	}

	inline constexpr float fromSrgb(float value)
	{
		//if (value >= 1.0f) return 1.0f;
		if (value <= 0.0f) return 0.0f;
		if (value <= 0.04045f) return value / 12.92f;
		return pow((value + 0.055f) / 1.055f, 2.4f);
	}

	inline glm::vec3 fromSrgb(glm::vec3 value)
	{
		value[0] = fromSrgb(value[0]);
		value[1] = fromSrgb(value[1]);
		value[2] = fromSrgb(value[2]);
		return value;
	}
}