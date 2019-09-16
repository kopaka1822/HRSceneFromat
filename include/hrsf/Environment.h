#pragma once
#include <string>
#include <array>
#include <filesystem>
#include <glm/vec3.hpp>

namespace hrsf
{
	// colors will be in linear space after loading but displayed in srgb in the file
	struct Environment
	{
		std::filesystem::path map; // environment map
		std::filesystem::path ambient; // ambient environment map
		glm::vec3 ambientUp; // ambient color for normals facing upwards
		glm::vec3 ambientDown; // ambient color for normals facing downwards
		glm::vec3 color; // either multiplied with the envmap or background color
		// TODO add fogg https://docs.unrealengine.com/en-US/Engine/Components/Rendering/index.html

		static const Environment& Default()
		{
			static const Environment e =
			{
				"",
				"",
				glm::vec3(0.0f),
				glm::vec3(0.0f),
				glm::vec3(0.0f)
			};
			return e;
		}
	};
}
