#pragma once
#include <glm/vec3.hpp>
#include "Path.h"

namespace hrsf
{
	// light structure that is aligned to 16 byte for the graphics card
	struct LightData
	{
		enum Type
		{
			Point,
			Directional
		};

		int type; // light type
		union
		{
			glm::vec3 position; // for point light
			glm::vec3 direction; // fir directional light
		};
		
		glm::vec3 color;
		float linearFalloff;

		float quadFalloff;
		// padding to keep structure 16 byte aligned
		glm::vec3 padding;
	};

	struct Light
	{
		LightData data;
		Path path;
	};
}
