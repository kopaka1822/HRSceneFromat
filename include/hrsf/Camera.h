#pragma once
#include <array>

namespace hrsf
{
	struct Camera
	{
		enum Type
		{
			Pinhole
		};

		Type type;
		std::array<float, 3> position;

		std::array<float, 3> direction;
		float fov; // field of vision in radians

		float near; // near plane distance
		float far; // far plane distance
		float speed; // units per second for manual movement

		std::array<float, 3> up; // up vector

		static const Camera& Default()
		{
			static const Camera c = {
				Pinhole,
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
				1.57f, // ~90 degrees
				0.01f,
				100000.0f,
				1.0f,
				0.0f, 1.0f, 0.0f,
				};
			return c;
		}
	};
}