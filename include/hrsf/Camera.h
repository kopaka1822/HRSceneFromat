#pragma once

namespace hrsf
{
	struct Camera
	{
		enum Type
		{
			Pinhole
		};

		Type type;
		float position[3];
		float direction[3];
		float fov; // field of vision in radians
		float near; // near plane distance
		float far; // far plane distance
		float up[3]; // up vector
	};
}