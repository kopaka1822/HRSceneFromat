#pragma once

namespace hrsf
{
	// light structure that is aligned to 16 byte for the graphics card
	struct Light
	{
		enum Type
		{
			Point,
			Directional
		};

		int type; // light type
		union
		{
			std::array<float, 3> position; // for point light
			std::array<float, 3> direction; // fir directional light
		};
		
		std::array<float, 3> color;
		float linearFalloff;

		float quadFalloff;
	};
}