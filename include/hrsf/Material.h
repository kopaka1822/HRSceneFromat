#pragma once
#include <string>
#include <array>

namespace hrsf
{
	struct MaterialTextures
	{
		std::string ambient;
		std::string diffuse;
		std::string specular;
		std::string occlusion; // extra map if transparency was not stored in diffuse map
	};

	// material data that is aligned to 16 byte for the graphics card
	struct MaterialData
	{
		enum Flags
		{
			None = 0,
			Reflection = 1,
		};

		std::array<float, 3> ambient;
		float roughness;
		std::array<float, 3> diffuse;
		float occlusion;
		std::array<float, 3> specular;
		float gloss; // glossiness for specular reflection
		std::array<float, 3> emission;
		int flags; // bitflags from Flags enum

		static const MaterialData& Default()
		{
			static const MaterialData d = {
				0.0f, 0.0f, 0.0f,
				0.0f,
				1.0f, 1.0f, 1.0f,
				1.0f,
				0.0f, 0.0f, 0.0f,
				50.0f,
				0.0f, 0.0f, 0.0f,
				None
				};
			return d;
		};
	};

	struct Material
	{
		std::string name;
		MaterialTextures textures;
		MaterialData data;
	};
}
