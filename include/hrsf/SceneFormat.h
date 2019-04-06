#pragma once
#include <string>
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "../../dependencies/json/single_include/nlohmann/json.hpp"
#include "Environment.h"

namespace hrsf
{
	class SceneFormat
	{
		using json = nlohmann::json;
	public:
		SceneFormat() = default;
		SceneFormat(bmf::BinaryMesh mesh, Camera cam, std::vector<Light> lights, std::vector<Material> materials, Environment env);
		SceneFormat(SceneFormat&&) = default;
		SceneFormat& operator=(SceneFormat&&) = default;
		
		const bmf::BinaryMesh& getMesh() const;
		const Camera& getCamera() const;
		const std::vector<Light>& getLights() const;
		const std::vector<Material>& getMaterials() const;
		std::vector<MaterialData> getMaterialsData() const;
		const Environment& getEnvironment() const;

		/// \brief loads the scene from the filesystem
		/// \param filename filename without extension
		static SceneFormat load(const std::string& filename);
		/// \brief saves the scene.
		/// \param filename output files are: filename.json, filename.bmf
		void save(const std::string& filename) const;

	private:
		json getMaterialsJson() const;
		json getLightsJson() const;
		json getCameraJson() const;
		json getEnvironmentJson() const;
		static Material loadMaterial(const json& j);
		static Camera loadCamera(const json& j);
		static Environment loadEnvironment(const json& j);
		static Light loadLight(const json& j);

		/// \brief retrieves directory of the specified filename. works with / and \.
		/// data/filename.txt => data/
		static std::string getDirectory(const std::string& filename);

		bmf::BinaryMesh m_mesh;
		Camera m_camera;
		std::vector<Light> m_lights;
		std::vector<Material> m_materials;
		Environment m_environment;

		static constexpr size_t s_version = 1;
	};

	inline SceneFormat::SceneFormat(bmf::BinaryMesh mesh, Camera cam, std::vector<Light> lights,
		std::vector<Material> materials, Environment env)
		:
	m_mesh(std::move(mesh)), m_camera(cam), m_lights(std::move(lights)), 
	m_materials(std::move(materials)), m_environment(env)
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

	inline const Environment& SceneFormat::getEnvironment() const
	{
		return m_environment;
	}

	inline SceneFormat SceneFormat::load(const std::string& filename)
	{
		// load from file
		std::ifstream file(filename + ".json");
		json j;
		file >> j;
		file.close();

		const auto version = j["version"].get<size_t>();
		if (version != s_version)
			throw std::runtime_error(filename + " invalid version");

		// get directory path from filename
		const auto directory = getDirectory(filename);
		const auto binaryName = j["scene"].get<std::string>();
		auto bmf = bmf::BinaryMesh::loadFromFile(directory + binaryName);

		// load camera etc.
		auto camera = loadCamera(j["camera"]);
		auto env = loadEnvironment(j["environment"]);

		// material
		std::vector<Material> materials;
		const auto& marray = j["materials"];
		if (!marray.is_array())
			throw std::runtime_error("materials must be an array");
		materials.resize(marray.size());
		for (const auto& m : marray)
			materials.emplace_back(loadMaterial(m));

		std::vector<Light> lights;
		const auto& larray = j["lights"];
		if (!larray.is_array())
			throw std::runtime_error("lights must be an array");
		lights.resize(larray.size());
		for (const auto& l : larray)
			lights.emplace_back(loadLight(l));

		return SceneFormat(
			std::move(bmf), 
			std::move(camera), 
			std::move(lights), 
			std::move(materials), 
			std::move(env)
		);
	}

	inline void SceneFormat::save(const std::string& filename) const
	{
		const auto binaryName = filename + ".bmf";
		m_mesh.saveToFile(binaryName);

		json j;
		j["version"] = s_version;
		j["scene"] = binaryName;
		j["materials"] = getMaterialsJson();
		j["lights"] = getLightsJson();
		j["camera"] = getCameraJson();
		j["environment"] = getEnvironmentJson();

		std::ofstream file(filename + ".json");
		file << j.dump(3);
		file.close();
	}

	inline SceneFormat::json SceneFormat::getMaterialsJson() const
	{
		auto res = json::array();
		for(const auto& m : m_materials)
		{
			json j;
			j["name"] = m.name;

			// textures
			if(!m.textures.diffuse.empty())
				j["diffuseTex"] = m.textures.diffuse;
			if(!m.textures.ambient.empty())
				j["ambientTex"] = m.textures.ambient;
			if (!m.textures.specular.empty())
				j["specularTex"] = m.textures.specular;
			if (!m.textures.occlusion.empty())
				j["occlusionTex"] = m.textures.occlusion;

			// data
			// always write diffuse
			j["diffuse"] = m.data.diffuse;
			if (m.data.ambient != MaterialData::Default().ambient)
				j["ambient"] = m.data.ambient;
			if (m.data.roughness != MaterialData::Default().roughness)
				j["roughness"] = m.data.roughness;
			if (m.data.occlusion != MaterialData::Default().occlusion)
				j["occlusion"] = m.data.occlusion;
			if (m.data.specular != MaterialData::Default().specular)
				j["specular"] = m.data.specular;
			if (m.data.gloss != MaterialData::Default().gloss)
				j["gloss"] = m.data.gloss;
			if (m.data.emission != MaterialData::Default().emission)
				j["emission"] = m.data.emission;
			if (m.data.flags != MaterialData::Default().flags)
				j["flags"] = m.data.flags;

			res.push_back(std::move(j));
		}
		return res;
	}

	inline SceneFormat::json SceneFormat::getLightsJson() const
	{
		auto res = json::array();
		for(const auto& l : m_lights)
		{
			json j;
			std::string strType;
			switch (l.type)
			{
			case Light::Point:
				strType = "Point";
				j["position"] = l.position;
				j["linearFalloff"] = l.linearFalloff;
				j["quadFalloff"] = l.quadFalloff;
				break;
			case Light::Directional:
				strType = "Directional";
				j["direction"] = l.direction;
				break;
			default: 
				throw std::runtime_error("invalid light type");
			}

			j["type"] = strType;
			j["color"] = l.color;

			res.push_back(std::move(j));
		}

		return res;
	}

	inline SceneFormat::json SceneFormat::getCameraJson() const
	{
		json j;
		std::string strType;
		switch (m_camera.type) 
		{ 
		case Camera::Pinhole: 
			strType = "Pinhole";
			break;
		default: throw std::runtime_error("invalid camera type");
		}

		j["type"] = strType;
		j["position"] = m_camera.position;
		j["direction"] = m_camera.direction;
		j["fov"] = m_camera.fov;
		if (m_camera.near != Camera::Default().near)
			j["near"] = m_camera.near;
		if (m_camera.far != Camera::Default().far)
			j["far"] = m_camera.far;
		if (m_camera.up != Camera::Default().up)
			j["up"] = m_camera.up;

		return j;
	}

	inline SceneFormat::json SceneFormat::getEnvironmentJson() const
	{
		json j;
		j["color"] = m_environment.color;

		if(!m_environment.map.empty())
			j["map"] = m_environment.map;
		if (!m_environment.ambient.empty())
			j["ambient"] = m_environment.ambient;

		return j;
	}

	inline Material SceneFormat::loadMaterial(const json& j)
	{
		Material mat;
		mat.name = j["name"].get<std::string>();

		// textures
		return mat;
	}

	inline Camera SceneFormat::loadCamera(const json& j)
	{
		Camera cam;
		auto strType = j["type"].get<std::string>();
		if (strType == "Pinhole")
			cam.type = Camera::Pinhole;
		else throw std::runtime_error("unknown camera type " + strType);

		cam.position = j["position"].get<std::array<float, 3>>();

		return cam;
	}

	inline Environment SceneFormat::loadEnvironment(const json& j)
	{
		Environment env;

		return env;
	}

	inline Light SceneFormat::loadLight(const json& j)
	{
		Light l;

		return l;
	}

	inline std::string SceneFormat::getDirectory(const std::string& filename)
	{
		const auto fSlashPos = filename.find_last_of('/');
		const auto bSlashPos = filename.find_last_of('\\');
		if (bSlashPos == std::string::npos && fSlashPos == std::string::npos)
			return ""; // no directory
		if (fSlashPos == std::string::npos)
			return filename.substr(0, bSlashPos + 1);
		if (bSlashPos == std::string::npos)
			return filename.substr(0, fSlashPos + 1);
		return filename.substr(0, std::max(bSlashPos, fSlashPos) + 1);
	}
}
