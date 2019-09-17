#pragma once
#include "Path.h"
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"
#include "Material.h"

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

		/// indicates if all of the movement paths are static
		bool isStatic() const
		{
			return position.isStatic() && lookAt.isStatic();
		}

		/// indicates if the mesh contains any transparent material
		bool isTransparent(const std::vector<Material>& materials) const
		{
			if(type == Triangle)
			{
				for (const auto& s : triangle.getShapes())
					if (materials[s.materialId].data.flags & MaterialData::Transparent)
						return true;
			}
			else if(type == Billboard)
			{
				for (const auto& matId : billboard.getMaterialAttribBuffer())
					if (materials[matId].data.flags & MaterialData::Transparent)
						return true;
			}
			return false;
		}
	};
}