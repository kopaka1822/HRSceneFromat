#pragma once
#include <string>
#include <array>
#include <filesystem>
#include <glm/vec3.hpp>

namespace hrsf
{
	struct MaterialTextures
	{
		std::filesystem::path albedo;
		std::filesystem::path specular;
		std::filesystem::path occlusion; // extra map if transparency was not stored in diffuse map
	};

	// material data that is aligned to 16 byte for the graphics card
	// note all colors will be in linear color space after loading and displayed in srgb when saved
	struct MaterialData
	{
		enum Flags
		{
			None = 0,
			Transparent = 1,
		};

		glm::vec3 albedo;
		float occlusion;
		glm::vec3 translucency;
		float specular; // intensity
		glm::vec3 emission;
		float metalness;
		float roughness;
		int flags; // bitflags from Flags enum
		glm::vec2 padding;

		static const MaterialData& Default()
		{
			static const MaterialData d = {
				{1.0f, 1.0f, 1.0f},
				1.0f,
				{0.0f, 0.0f, 0.0f},
				0.1f,
				{0.0f, 0.0f, 0.0f},
				0.0f,
				1.0f,
				None,
			{0.0f, 0.0f}
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
