#pragma once
#include <string>
#include <array>

namespace hrsf
{
	struct Environment
	{
		std::string map; // environment map
		std::string ambient; // ambient environment map
		std::array<float, 3> color; // either multiplied with the envmap or background color
	};
}
