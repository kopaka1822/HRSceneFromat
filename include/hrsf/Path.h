#pragma once
#include <array>

namespace hrsf
{
	struct PathSection
	{
		// time in seconds to go from the previous sections position to this sections position
		float time;
		std::array<float, 3> position;
	};

	
}
