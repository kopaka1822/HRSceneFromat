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

		void removeUnusedMaterials();
		/// \brief throws an exception if something seems wrong
		void verify() const;

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

		/// \brief retrieves the value from the json. if the json does not contain the value
		/// the default value is returned instead
		template<class T>
		static T getOrDefault(const json& j, const char* name, T defaultValue);
		template<>
		static std::array<float, 3> getOrDefault(const json& j, const char* name, std::array<float, 3> defaultValue);

		static std::array<float, 3> getVec3(const json& j);
		static void writeVec3(json& j, const std::array<float, 3>& vec);

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

	inline void SceneFormat::removeUnusedMaterials()
	{
		std::vector<bool> isUsed(m_materials.size(), false);

		for(const auto& s : m_mesh.getShapes())
		{
			isUsed[s.materialId] = true;
		}

		if (std::all_of(isUsed.begin(), isUsed.end(), [](bool used) {return used; }))
			return; // all materials are used

		// remove unused materials
		// lookup table: materialLookup[a] = b the material id a will be changed to b
		std::vector<uint32_t> materialLookup;
		materialLookup.resize(m_materials.size());

		uint32_t curIndex = 0;
		for(uint32_t i = 0; i < uint32_t(m_materials.size()); ++i)
		{
			if(isUsed[i])
			{
				materialLookup[i] = curIndex++;
			}
		}

		for(auto& s : m_mesh.getShapes())
		{
			s.materialId = materialLookup[s.materialId];
		}

		// remove unused materials
		decltype(m_materials) newMaterials;
		newMaterials.reserve(m_materials.size());

		for(size_t i = 0; i < isUsed.size(); ++i)
		{
			if (isUsed[i])
				newMaterials.push_back(m_materials[i]);
		}

		m_materials = std::move(newMaterials);
	}

	inline void SceneFormat::verify() const
	{
		// verify mesh
		m_mesh.verify();

		// test that materials are not out of bound
		for(const auto& s : m_mesh.getShapes())
		{
			if (s.materialId >= m_materials.size())
				throw std::runtime_error("material id out of bound: " + std::to_string(s.materialId));
		}
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
		materials.reserve(marray.size());
		for (const auto& m : marray)
			materials.emplace_back(loadMaterial(m));

		std::vector<Light> lights;
		const auto& larray = j["lights"];
		if (!larray.is_array())
			throw std::runtime_error("lights must be an array");
		lights.reserve(larray.size());
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
			writeVec3(j["diffuse"], m.data.diffuse);
			if (m.data.ambient != MaterialData::Default().ambient)
				writeVec3(j["ambient"], m.data.ambient);
			if (m.data.roughness != MaterialData::Default().roughness)
				j["roughness"] = m.data.roughness;
			if (m.data.occlusion != MaterialData::Default().occlusion)
				j["occlusion"] = m.data.occlusion;
			if (m.data.specular != MaterialData::Default().specular)
				writeVec3(j["specular"], m.data.specular);
			if (m.data.gloss != MaterialData::Default().gloss)
				j["gloss"] = m.data.gloss;
			if (m.data.emission != MaterialData::Default().emission)
				writeVec3(j["emission"], m.data.emission);
			
			// write flags as booleans
			const bool reflection = (m.data.flags & MaterialData::Reflection) != 0;
			if (((MaterialData::Default().flags & MaterialData::Reflection) != 0) != reflection)
				j["reflection"] = reflection;
			
				

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
			writeVec3(j["color"], l.color);

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
		writeVec3(j["color"], m_environment.color);

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
		mat.textures.diffuse = getOrDefault(j, "diffuseTex", std::string());
		mat.textures.ambient = getOrDefault(j,"ambientTex", std::string());
		mat.textures.specular = getOrDefault(j,"specularTex", std::string());
		mat.textures.occlusion = getOrDefault(j,"occlusionTex", std::string());

		// data
		mat.data.diffuse = getVec3(j["diffuse"]);
		mat.data.roughness = getOrDefault(j,"roughness", MaterialData::Default().roughness);
		mat.data.occlusion = getOrDefault(j,"occlusion", MaterialData::Default().occlusion);
		mat.data.specular = getOrDefault(j,"specular", MaterialData::Default().specular);
		mat.data.gloss = getOrDefault(j,"gloss", MaterialData::Default().gloss);
		mat.data.emission = getOrDefault(j,"emission", MaterialData::Default().emission);
		
		mat.data.flags = 0;

		// reflection?
		if(getOrDefault(j, "reflection", (MaterialData::Default().flags & MaterialData::Reflection) != 0))
			mat.data.flags |= MaterialData::Reflection;

		return mat;
	}

	inline Camera SceneFormat::loadCamera(const json& j)
	{
		Camera cam;
		const auto strType = j["type"].get<std::string>();
		if (strType == "Pinhole")
			cam.type = Camera::Pinhole;
		else throw std::runtime_error("unknown camera type " + strType);

		cam.position = getVec3(j["position"]);
		cam.direction = getVec3(j["direction"]);
		cam.fov = j["fov"].get<float>();
		cam.near = getOrDefault(j, "near", Camera::Default().near);
		cam.far = getOrDefault(j, "far", Camera::Default().far);
		cam.up = getOrDefault(j, "up", Camera::Default().up);

		return cam;
	}

	inline Environment SceneFormat::loadEnvironment(const json& j)
	{
		Environment env;
		env.color = getVec3(j["color"]);

		env.map = getOrDefault(j, "map", std::string());
		env.ambient = getOrDefault(j, "ambient", std::string());

		return env;
	}

	inline Light SceneFormat::loadLight(const json& j)
	{
		Light l;
		const auto strType = j["type"].get<std::string>();
		if(strType == "Point")
		{
			l.type = Light::Point;
			l.position = getVec3(j["position"]);
			l.linearFalloff = j["linearFalloff"].get<float>();
			l.quadFalloff = j["quadFalloff"].get<float>();
		}
		else if(strType == "Directional")
		{
			l.type = Light::Directional;
			l.direction = getVec3(j["direction"]);
		}
		else throw std::runtime_error("invalid light type " + strType);

		l.color = getVec3(j["color"]);

		return l;
	}

	template <class T>
	T SceneFormat::getOrDefault(const json& j, const char* name, T defaultValue)
	{
		auto it = j.find(name);
		if (it == j.end()) return defaultValue;
		//if (it.value().empty()) return defaultValue;
		return it.value().get<T>();
	}

	template <>
	inline std::array<float, 3> SceneFormat::getOrDefault<std::array<float, 3>>(const json& j, const char* name,
		std::array<float, 3> defaultValue)
	{
		auto it = j.find(name);
		if (it == j.end()) return defaultValue;
		return getVec3(it.value());
	}

	inline std::array<float, 3> SceneFormat::getVec3(const json& j)
	{
		std::array<float, 3> res;
		if (j.is_array())
		{
			if (j.size() == 1)
				res.fill(j[0].get<float>());
			else if (j.size() == 3)
				return j.get<std::array<float, 3>>();
			else throw std::runtime_error("expected array with 3 or 1 element but got " + std::to_string(j.size()));
		}
		else
			res.fill(j.get<float>());

		return res;
	}

	inline void SceneFormat::writeVec3(json& j, const std::array<float, 3>& vec)
	{
		// all values are equal?	
		if (vec[0] == vec[1] && vec[1] == vec[2])
			j = vec[0]; // write only single value
		else
			j = vec;
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
