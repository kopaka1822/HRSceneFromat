#pragma once
#include <string>
#include <array>
#include <filesystem>

namespace hrsf
{
	struct Environment
	{
		std::filesystem::path map; // environment map
		std::filesystem::path ambient; // ambient environment map
		std::array<float, 3> color; // either multiplied with the envmap or background color
	};
}
