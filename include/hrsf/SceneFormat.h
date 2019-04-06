#pragma once
#include <string>
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"

namespace hrsf
{
	class SceneFormat
	{
	public:
		SceneFormat() = default;
		SceneFormat(bmf::BinaryMesh mesh, Camera cam, std::vector<Light> lights, std::vector<Material> materials);
		SceneFormat(SceneFormat&&) = default;
		SceneFormat& operator=(SceneFormat&&) = default;
		
		const bmf::BinaryMesh& getMesh() const;
		const Camera& getCamera() const;
		const std::vector<Light>& getLights() const;
		const std::vector<Material>& getMaterials() const;
		std::vector<MaterialData> getMaterialsData() const;

		/// \brief loads the scene from the filesystem
		/// \param filename filename without extension
		static SceneFormat load(const std::string& filename);
		/// \brief saves the scene.
		/// \param filename output files are: filename.json, filename.bmf
		void save(const std::string& filename);

	private:
		bmf::BinaryMesh m_mesh;
		Camera m_camera;
		std::vector<Light> m_lights;
		std::vector<Material> m_materials;
	};

	inline SceneFormat::SceneFormat(bmf::BinaryMesh mesh, Camera cam, std::vector<Light> lights,
		std::vector<Material> materials)
		:
	m_mesh(std::move(mesh)), m_camera(cam), m_lights(std::move(lights)), m_materials(std::move(materials))
	{}

	inline const bmf::BinaryMesh& SceneFormat::getMesh() const
	{
		return m_mesh;
	}

	inline const Camera& SceneFormat::getCamera() const
	{
		return m_camera;
	}

	inline const std::vector<Light>& SceneFormat::getLights() const
	{
		return m_lights;
	}

	inline const std::vector<Material>& SceneFormat::getMaterials() const
	{
		return m_materials;
	}

	inline std::vector<MaterialData> SceneFormat::getMaterialsData() const
	{
		std::vector<MaterialData> res;
		res.resize(m_materials.size());
		std::transform(m_materials.begin(), m_materials.end(), res.begin(), [](const Material& m)
		{
			return m.data;
		});
		return res;
	}
}
