#pragma once
#include <string>
#include <array>
#include <filesystem>
#include <glm/vec3.hpp>

namespace hrsf
{
	struct MaterialTextures
	{
		std::filesystem::path ambient;
		std::filesystem::path diffuse;
		std::filesystem::path specular;
		std::filesystem::path occlusion; // extra map if transparency was not stored in diffuse map
	};

	// material data that is aligned to 16 byte for the graphics card
	struct MaterialData
	{
		enum Flags
		{
			None = 0,
			Reflection = 1,
			Transparent = 2,
		};

		glm::vec3 ambient;
		float roughness;
		glm::vec3 diffuse;
		float occlusion;
		glm::vec3 specular;
		float gloss; // glossiness for specular reflection
		glm::vec3 emission;
		int flags; // bitflags from Flags enum

		static const MaterialData& Default()
		{
			static const MaterialData d = {
				{1.0f, 1.0f, 1.0f},
				0.0f,
				{1.0f, 1.0f, 1.0f},
				1.0f,
				{0.0f, 0.0f, 0.0f},
				50.0f,
				{0.0f, 0.0f, 0.0f},
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
