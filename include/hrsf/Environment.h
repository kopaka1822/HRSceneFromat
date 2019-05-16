#pragma once
#include <string>
#include <array>
#include <filesystem>
#include <glm/vec3.hpp>

namespace hrsf
{
	struct Environment
	{
		std::filesystem::path map; // environment map
		std::filesystem::path ambient; // ambient environment map
		glm::vec3 color; // either multiplied with the envmap or background color
	};
}
