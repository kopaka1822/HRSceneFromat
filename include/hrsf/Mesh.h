#pragma once
#include "Path.h"
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"

namespace hrsf
{
	struct Mesh
	{
		enum Type
		{
			Triangle,
			Billboard
		};

		Type type;
		bmf::BinaryMesh billboard;
		bmf::BinaryMesh16 triangle;

		Path position;
		Path lookAt;

		Mesh() = default;
		explicit Mesh(bmf::BinaryMesh16 mesh)
		{
			type = Triangle;
			triangle = std::move(mesh);
		}
		explicit Mesh(bmf::BinaryMesh mesh)
		{
			type = Billboard;
			billboard = std::move(mesh);
		}
	};
}