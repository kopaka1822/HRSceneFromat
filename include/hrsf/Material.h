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
		std::filesystem::path coverage; // extra map if transparency was not stored in diffuse map
	};

	// material data that is aligned to 16 byte for the graphics card
	// note all colors will be in linear color space after loading and displayed in srgb when saved
	struct MaterialData
	{
		enum Flags
		{
			None = 0,
			Transparent = 1,

			// surface describes a volume:
			// the insides are described through the normals:
			// *outside* n <-| *inside* |-> n (n is the normal of the surface | )
			Volume = 1 << 1,

			// indicates that this surface has no normals (likely because it is volumetric)
			IgnoreNormals = 1 << 2,
			
			// indicates y-axis orientation for billboards
			YOrientation =  1 << 3, 
			
			// indicates that the texture sampler should clamp instead of wrap
			TextureClamp = 1 << 4
		};

		glm::vec3 albedo;
		float coverage; // coverage of the material

		glm::vec3 emission;
		float metalness;

		float roughness;
		int flags; // bitflags from Flags enum
		float translucency; // amount of light that is transmitted through the object
		float specular; // reflective intensity

		float ior; // index of refraction
		glm::vec3 padding; // padding for 16byte shader alignment

		static const MaterialData& Default()
		{
			static const MaterialData d = {
				{1.0f, 1.0f, 1.0f},
				1.0f,
				{0.0f, 0.0f, 0.0f},
				0.0f,
				1.0f,
				None,
				0.0f,
				0.1f,
				1.0f,
				{0.0f, 0.0f, 0.0f}
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
