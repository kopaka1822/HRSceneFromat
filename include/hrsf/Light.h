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
		
		glm::vec3 color; // linear color space
		float radius; // for point light
	};

	struct Light
	{
		LightData data;
		Path path;
	};
}
