#pragma once
#include <array>
#include <glm/vec3.hpp>
#include "Path.h"

namespace hrsf
{
	struct CameraData
	{
		enum Type
		{
			Pinhole
		};

		Type type;
		glm::vec3 position;

		glm::vec3 direction;
		float fov; // field of vision in radians

		float near; // near plane distance
		float far; // far plane distance
		float speed; // units per second for manual movement

		glm::vec3 up; // up vector

		static const CameraData& Default()
		{
			static const CameraData c = {
				Pinhole,
				{0.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, 1.0f},
				1.57f, // ~90 degrees
				0.01f,
				100000.0f,
				1.0f,
				{0.0f, 1.0f, 0.0f},
				};
			return c;
		}
	};

	struct Camera
	{
		CameraData data;
		Path positionPath; // automated camera movement
		Path lookAtPath;
	};
}
