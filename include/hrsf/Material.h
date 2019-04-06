#pragma once
#include <string>

namespace hrsf
{
	struct MaterialTextures
	{
		std::string ambient;
		std::string diffuse;
		std::string specular;
		std::string transparency; // extra map if transparency was not stored in diffuse map
	};

	// material data that is aligned to 16 byte for the graphics card
	struct MaterialData
	{
		enum Flags
		{
			None = 0,
			Reflection = 1,
		};

		float ambient[3];
		float roughness;
		float diffuse[3];
		float transparency;
		float specular[3];
		float gloss; // glossiness for specular reflection
		float emission[3];
		int flags; // bitflags from Flags enum
	};

	struct Material
	{
		std::string name;
		MaterialTextures textures;
		MaterialData data;
	};
}
