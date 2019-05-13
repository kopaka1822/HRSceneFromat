#pragma once
#include <string>
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "../../dependencies/json/single_include/nlohmann/json.hpp"
#include "Environment.h"
#include <filesystem>

namespace hrsf
{
	namespace fs = std::filesystem;

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
		static SceneFormat load(fs::path filename);
		/// \brief loads the camera from the filesystem
		/// \param filename filename without extension
		static Camera loadCamera(fs::path filename);
		/// \brief loads the materials from the filesystem
		/// \param filename filename without extension
		static std::vector<Material> loadMaterials(fs::path filename);
		/// \brief loads the lights from the filesystem
		/// \param filename filename without extension
		static std::vector<Light> loadLights(fs::path filename);
		/// \brief loads the environment from the filesystem
		/// \param filename filename without extension
		static Environment loadEnvironment(fs::path filename);
		/// \brief saves the scene.
		/// \param filename output files are: filename.json, filename.bmf
		void save(const fs::path& filename) const;
		static void saveCamera(const fs::path& filename, const Camera& camera);
		static void saveMaterials(const fs::path& filename, const std::vector<Material>& materials);
		static void saveLights(const fs::path& filename, const std::vector<Light>& lights);
		static void saveEnvironment(const fs::path& filename, const Environment& env);
	private:
		static json getMaterialsJson(const std::vector<Material>& materials, const fs::path& root);
		static json getLightsJson(const std::vector<Light>& lights);
		static json getCameraJson(const Camera& camera);
		static json getEnvironmentJson(const Environment& env, const fs::path& root);
		static json openFile(fs::path filename);
		static void saveFile(const json& j, fs::path filename);
		static Material loadMaterialJson(const json& j, const fs::path& root);
		static std::vector<Material> loadMaterialsJson(const json& j, const fs::path& root);
		static Camera loadCameraJson(const json& j);
		static Environment loadEnvironmentJson(const json& j, const fs::path& root);
		static std::vector<Light> loadLightsJson(const json& j);
		static Light loadLightJson(const json& j);

		/// \brief retrieves the value from the json. if the json does not contain the value
		/// the default value is returned instead
		template<class T>
		static T getOrDefault(const json& j, const char* name, T defaultValue);
		template<>
		static std::array<float, 3> getOrDefault(const json& j, const char* name, std::array<float, 3> defaultValue);
		static fs::path getFilename(const json& j, const char* name, const fs::path& root);

		static std::array<float, 3> getVec3(const json& j);
		static void writeVec3(json& j, const std::array<float, 3>& vec);

		static std::string getRelativePath(const fs::path& root, const fs::path& p);
		static fs::path getAbsolutePath(const fs::path& root, const fs::path& p);

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

		for (const auto& s : m_mesh.getShapes())
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
		for (uint32_t i = 0; i < uint32_t(m_materials.size()); ++i)
		{
			if (isUsed[i])
			{
				materialLookup[i] = curIndex++;
			}
		}

		for (auto& s : m_mesh.getShapes())
		{
			s.materialId = materialLookup[s.materialId];
		}

		// remove unused materials
		decltype(m_materials) newMaterials;
		newMaterials.reserve(m_materials.size());

		for (size_t i = 0; i < isUsed.size(); ++i)
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
		for (const auto& s : m_mesh.getShapes())
		{
			if (s.materialId >= m_materials.size())
				throw std::runtime_error("material id out of bound: " + std::to_string(s.materialId));
		}
	}

	inline SceneFormat SceneFormat::load(fs::path filename)
	{
		auto j = openFile(filename);

		const auto version = j["version"].get<size_t>();
		if (version != s_version)
			throw std::runtime_error(filename.string() + " invalid version");

		// get directory path from filename
		const auto directory = absolute(filename).parent_path();
		const fs::path binaryName = j["scene"].get<std::string>();

		auto bmf = bmf::BinaryMesh::loadFromFile(getAbsolutePath(directory, binaryName).string());

		// load camera etc.
		auto camera = loadCameraJson(j["camera"]);
		auto env = loadEnvironmentJson(j["environment"], directory);
		auto materials = loadMaterialsJson(j["materials"], directory);
		auto lights = loadLightsJson(j["lights"]);

		return SceneFormat(
			std::move(bmf),
			std::move(camera),
			std::move(lights),
			std::move(materials),
			std::move(env)
		);
	}

	inline Camera SceneFormat::loadCamera(fs::path filename)
	{
		return loadCameraJson(openFile(filename));
	}

	inline std::vector<Material> SceneFormat::loadMaterials(fs::path filename)
	{
		return loadMaterialsJson(openFile(filename), absolute(filename).parent_path());
	}

	inline std::vector<Light> SceneFormat::loadLights(fs::path filename)
	{
		return loadLightsJson(openFile(filename));
	}

	inline Environment SceneFormat::loadEnvironment(fs::path filename)
	{
		return loadEnvironmentJson(openFile(filename), filename.parent_path());
	}

	inline void SceneFormat::save(const fs::path& filename) const
	{
		const fs::path binaryName = filename.string() + ".bmf";
		const fs::path rootDirectory = filename.parent_path();
		m_mesh.saveToFile(binaryName.string());

		json j;
		j["version"] = s_version;
		j["scene"] = binaryName.filename().string();
		j["materials"] = getMaterialsJson(m_materials, rootDirectory);
		j["lights"] = getLightsJson(m_lights);
		j["camera"] = getCameraJson(m_camera);
		j["environment"] = getEnvironmentJson(m_environment, rootDirectory);

		saveFile(j, filename);
	}

	inline void SceneFormat::saveCamera(const fs::path& filename, const Camera& camera)
	{
		saveFile(getCameraJson(camera), filename);
	}

	inline void SceneFormat::saveMaterials(const fs::path& filename, const std::vector<Material>& materials)
	{
		saveFile(getMaterialsJson(materials, filename.parent_path()), filename);
	}

	inline void SceneFormat::saveLights(const fs::path& filename, const std::vector<Light>& lights)
	{
		saveFile(getLightsJson(lights), filename);
	}

	inline void SceneFormat::saveEnvironment(const fs::path& filename, const Environment& env)
	{
		saveFile(getEnvironmentJson(env, filename.parent_path()), filename);
	}

	inline SceneFormat::json SceneFormat::getMaterialsJson(const std::vector<Material>& materials, const fs::path& root)
	{
		auto res = json::array();
		for (const auto& m : materials)
		{
			json j;
			j["name"] = m.name;

			// textures
			if (!m.textures.diffuse.empty())
				j["diffuseTex"] = getRelativePath(root, m.textures.diffuse);
			if (!m.textures.ambient.empty())
				j["ambientTex"] = getRelativePath(root, m.textures.ambient);
			if (!m.textures.specular.empty())
				j["specularTex"] = getRelativePath(root, m.textures.specular);
			if (!m.textures.occlusion.empty())
				j["occlusionTex"] = getRelativePath(root, m.textures.occlusion);

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

	inline SceneFormat::json SceneFormat::getLightsJson(const std::vector<Light>& lights)
	{
		auto res = json::array();
		for (const auto& l : lights)
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

	inline SceneFormat::json SceneFormat::getCameraJson(const Camera& camera)
	{
		json j;
		std::string strType;
		switch (camera.type)
		{
		case Camera::Pinhole:
			strType = "Pinhole";
			break;
		default: throw std::runtime_error("invalid camera type");
		}

		j["type"] = strType;
		j["position"] = camera.position;
		j["direction"] = camera.direction;
		j["fov"] = camera.fov;
		if (camera.near != Camera::Default().near)
			j["near"] = camera.near;
		if (camera.far != Camera::Default().far)
			j["far"] = camera.far;
		if (camera.up != Camera::Default().up)
			j["up"] = camera.up;

		return j;
	}

	inline SceneFormat::json SceneFormat::getEnvironmentJson(const Environment& env, const fs::path& root)
	{
		json j;
		writeVec3(j["color"], env.color);

		if (!env.map.empty())
			j["map"] = getRelativePath(root, env.map);
		if (!env.ambient.empty())
			j["ambient"] = getRelativePath(root, env.ambient);

		return j;
	}

	inline SceneFormat::json SceneFormat::openFile(fs::path filename)
	{
		// absolute path of the json
		auto jsonName = fs::absolute(filename.replace_extension(".json"));

		// load from file
		std::ifstream file(jsonName);
		if (!file.is_open())
			throw std::runtime_error("could not open " + jsonName.string());

		json j;
		file >> j;
		file.close();

		return j;
	}

	inline void SceneFormat::saveFile(const json& j, fs::path filename)
	{
		filename = filename.replace_extension(".json");
		std::ofstream file(filename);
		if (!file.is_open())
			throw std::runtime_error("could not open " + filename.string());

		file << j.dump(3);
		file.close();
	}

	inline Material SceneFormat::loadMaterialJson(const json& j, const fs::path& root)
	{
		Material mat;
		mat.name = j["name"].get<std::string>();

		// textures
		mat.textures.diffuse = getFilename(j, "diffuseTex", root);
		mat.textures.ambient = getFilename(j, "ambientTex", root);
		mat.textures.specular = getFilename(j, "specularTex", root);
		mat.textures.occlusion = getFilename(j, "occlusionTex", root);
		
		// data
		mat.data.diffuse = getVec3(j["diffuse"]);
		mat.data.roughness = getOrDefault(j, "roughness", MaterialData::Default().roughness);
		mat.data.occlusion = getOrDefault(j, "occlusion", MaterialData::Default().occlusion);
		mat.data.specular = getOrDefault(j, "specular", MaterialData::Default().specular);
		mat.data.gloss = getOrDefault(j, "gloss", MaterialData::Default().gloss);
		mat.data.emission = getOrDefault(j, "emission", MaterialData::Default().emission);

		mat.data.flags = 0;

		// reflection?
		if (getOrDefault(j, "reflection", (MaterialData::Default().flags & MaterialData::Reflection) != 0))
			mat.data.flags |= MaterialData::Reflection;

		return mat;
	}

	inline std::vector<Material> SceneFormat::loadMaterialsJson(const json& j, const fs::path& root)
	{
		std::vector<Material> materials;
		if (!j.is_array())
			throw std::runtime_error("materials must be an array");
		materials.reserve(j.size());
		for (const auto& m : j)
			materials.emplace_back(loadMaterialJson(m, root));

		return materials;
	}

	inline Camera SceneFormat::loadCameraJson(const json& j)
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

	inline Environment SceneFormat::loadEnvironmentJson(const json& j, const fs::path& root)
	{
		Environment env;
		env.color = getVec3(j["color"]);

		env.map = getFilename(j, "map", root);
		env.ambient = getFilename(j, "ambient", root);

		return env;
	}

	inline std::vector<Light> SceneFormat::loadLightsJson(const json& j)
	{
		std::vector<Light> lights;
		if (!j.is_array())
			throw std::runtime_error("lights must be an array");
		lights.reserve(j.size());
		for (const auto& l : j)
			lights.emplace_back(loadLightJson(l));

		return lights;
	}

	inline Light SceneFormat::loadLightJson(const json& j)
	{
		Light l;
		const auto strType = j["type"].get<std::string>();
		if (strType == "Point")
		{
			l.type = Light::Point;
			l.position = getVec3(j["position"]);
			l.linearFalloff = j["linearFalloff"].get<float>();
			l.quadFalloff = j["quadFalloff"].get<float>();
		}
		else if (strType == "Directional")
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

	inline fs::path SceneFormat::getFilename(const json& j, const char* name, const fs::path& root)
	{
		const auto it = j.find(name);
		if (it == j.end()) return fs::path();

		return getAbsolutePath(root, it.value().get<std::string>());
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

	inline std::string SceneFormat::getRelativePath(const fs::path& root, const fs::path& p)
	{
		if (p.is_relative()) // is already relative to the json root?
			return p.string();

		std::error_code err;
		auto res = fs::relative(p, root, err);
		if (err)
			throw std::runtime_error("could not form relative path: " + err.message());

		return res.string();
	}

	inline fs::path SceneFormat::getAbsolutePath(const fs::path& root, const fs::path& p)
	{
		if (p.is_absolute())
			return p;

		return root/p;
	}
}
