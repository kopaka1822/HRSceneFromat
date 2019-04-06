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
			float position[3]; // for point light
			float direction[3]; // fir directional light
		};
		
		float color[3];
		float linearFalloff;

		float quadFalloff;
	};
}