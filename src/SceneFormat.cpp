#include "../include/hrsf/SceneFormat.h"

namespace hrsf
{
	SceneFormat::SceneFormat(std::vector<Mesh> meshes, Camera cam, std::vector<Light> lights,
		std::vector<Material> materials, Environment env)
		:
		m_meshes(std::move(meshes)), m_camera(cam), m_lights(std::move(lights)),
		m_materials(std::move(materials)), m_environment(env)
	{}

	const std::vector<Mesh>& SceneFormat::getMeshes() const
	{
		return m_meshes;
	}

	const Camera& SceneFormat::getCamera() const
	{
		return m_camera;
	}

	const std::vector<Light>& SceneFormat::getLights() const
	{
		return m_lights;
	}

	const std::vector<Material>& SceneFormat::getMaterials() const
	{
		return m_materials;
	}

	std::vector<MaterialData> SceneFormat::getMaterialsData() const
	{
		std::vector<MaterialData> res;
		res.resize(m_materials.size());
		std::transform(m_materials.begin(), m_materials.end(), res.begin(), [](const Material& m)
			{
				return m.data;
			});
		return res;
	}

	const Environment& SceneFormat::getEnvironment() const
	{
		return m_environment;
	}

	void SceneFormat::removeUnusedMaterials()
	{
		std::vector<bool> isUsed(m_materials.size(), false);

		for(const auto& m : m_meshes)
		{
			if (m.type == Mesh::Triangle)
			{
				for (const auto& s : m.triangle.getShapes())
				{
					isUsed[s.materialId] = true;
				}
			}
			else if (m.type == Mesh::Billboard)
			{
				if(!(m.billboard.getAttributes() & bmf::Material)) continue;
				for (const auto& matId : m.billboard.getMaterialAttribBuffer())
				{
					isUsed[matId] = true;
				}
			}
			else assert(false);
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

		// adjust vertices
		for(auto& m : m_meshes)
		{
			if(m.type == Mesh::Triangle)
			{
				for (auto& s : m.triangle.getShapes())
				{
					s.materialId = materialLookup[s.materialId];
				}
			}
			else if(m.type == Mesh::Billboard) // this is more complicated...
			{
				if (!(m.billboard.getAttributes() & bmf::Material)) continue;
				auto& verts = m.billboard.getVertices();
				if(verts.empty()) continue;
				// change the material id of each vertex attribute
				auto it = verts.begin() + bmf::getAttributeElementOffset(m.billboard.getAttributes(), bmf::Material);
				const auto stride = bmf::getAttributeElementStride(m.billboard.getAttributes());
				const auto vertexCount = verts.size() / stride;
				for(uint32_t i = 0; i < vertexCount; ++i)
				{
					*it = bmf::asFloat(materialLookup[bmf::asInt(*it)]);
					it += stride;
				}
			}
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

	void SceneFormat::offsetMaterials(uint32_t offset)
	{
		if (offset == 0) return;
		for(auto& m : m_meshes)
		{
			if (m.type == Mesh::Billboard)
				m.billboard.offsetMaterial(offset);
			else if (m.type == Mesh::Triangle)
				m.triangle.offsetMaterial(offset);
			else assert(false);
		}
	}

	void SceneFormat::verify() const
	{
		// verify mesh
		for(const auto& m : m_meshes)
		{
			if (m.type == Mesh::Triangle)
			{
				m.triangle.verify();
				// test that materials are not out of bound
				for (const auto& s : m.triangle.getShapes())
				{
					if (s.materialId >= m_materials.size())
						throw std::runtime_error("material id out of bound: " + std::to_string(s.materialId));
				}
			}
			else if (m.type == Mesh::Billboard)
			{
				m.billboard.verify();
				if(m.billboard.getAttributes() & bmf::Material)
				{
					auto mats = m.billboard.getMaterialAttribBuffer();
					for(const auto& matId : mats)
					{
						if(size_t(matId) >= m_materials.size())
							throw std::runtime_error("material id out of bound: " + std::to_string(matId));
					}
				}
			}
			else throw std::runtime_error("invalid mesh type");
			m.position.verify();
			m.lookAt.verify();
		}

		

		// test that path have no negative times
		for (const auto& l : m_lights)
			l.path.verify();

		m_camera.lookAtPath.verify();
		m_camera.positionPath.verify();
	}

	SceneFormat SceneFormat::load(fs::path filename)
	{
		auto j = openFile(filename);

		const auto version = j["version"].get<size_t>();
		if (version != s_version)
			throw std::runtime_error(filename.string() + " invalid version");

		// get directory path from filename
		const auto directory = absolute(filename).parent_path();

		auto meshNames = j["meshes"].get<std::vector<std::string>>();
		std::vector<Mesh> meshes;
		meshes.reserve(meshNames.size());
		for(auto& file : meshNames)
		{
			meshes.emplace_back(loadMesh(directory / file));
		}

		// load camera etc.
		auto camera = loadCameraJson(j["camera"], directory);
		auto env = loadEnvironmentJson(j["environment"], directory);
		auto materials = loadMaterialsJson(j["materials"], directory);
		auto lights = loadLightsJson(j["lights"], directory);

		return SceneFormat(
			std::move(meshes),
			std::move(camera),
			std::move(lights),
			std::move(materials),
			std::move(env)
		);
	}

	Mesh SceneFormat::loadMesh(fs::path filename)
	{
		return loadMeshJson(openFile(filename), absolute(filename).parent_path());
	}

	Camera SceneFormat::loadCamera(fs::path filename)
	{
		return loadCameraJson(openFile(filename), absolute(filename).parent_path());
	}

	std::vector<Material> SceneFormat::loadMaterials(fs::path filename)
	{
		return loadMaterialsJson(openFile(filename), absolute(filename).parent_path());
	}

	std::vector<Light> SceneFormat::loadLights(fs::path filename)
	{
		return loadLightsJson(openFile(filename), absolute(filename).parent_path());
	}

	Environment SceneFormat::loadEnvironment(fs::path filename)
	{
		return loadEnvironmentJson(openFile(filename), filename.parent_path());
	}

	Path SceneFormat::loadPath(fs::path filename)
	{
		return loadPathJson(openFile(filename), absolute(filename).parent_path());
	}

	void SceneFormat::save(const fs::path& filename, bool singleFile, Component components) const
	{
		const fs::path binaryName = filename.string() + ".bmf";
		const fs::path rootDirectory = filename.parent_path();
		
		json j;
		j["version"] = s_version;

		if(components & Component::Mesh)
		{
			auto arr = json::array();

			// generate "smart" names for meshes
			std::unordered_map<std::string, size_t> usedSuffixMap;
			for(const auto& mesh : m_meshes)
			{
				auto suffix = generateMeshSuffix(mesh);
				if(usedSuffixMap.find(suffix) == usedSuffixMap.end())
				{ 
					// create new entry
					usedSuffixMap[suffix] = 0;
				}
				size_t id = ++usedSuffixMap[suffix];
				if (id > 1 || suffix.empty())
					suffix += std::to_string(id);

				auto meshFilename = fs::path(filename.string() + suffix);
				saveMesh(meshFilename, mesh);

				arr.push_back(meshFilename.filename().string() + ".json");
			}

			j["meshes"] = arr;
		}

		auto mats = getMaterialsJson(m_materials, rootDirectory);;
		auto lights = getLightsJson(m_lights);
		auto camera = getCameraJson(m_camera);
		auto env = getEnvironmentJson(m_environment, rootDirectory);

		if (singleFile)
		{
			if(components & Component::Material)
				j["materials"] = move(mats);
			if(components & Component::Lights)
				j["lights"] = move(lights);
			if(components & Component::Camera)
				j["camera"] = move(camera);
			if(components & Component::Environment)
				j["environment"] = move(env);
		}
		else
		{
			// safe components in files first
			if (components & Component::Material)
				saveFile(mats, filename.string() + "_material");
			if (components & Component::Lights)
				saveFile(lights, filename.string() + "_light");
			if (components & Component::Camera)
				saveFile(camera, filename.string() + "_camera");
			if (components & Component::Environment)
				saveFile(env, filename.string() + "_env");

			// redirect to files
			const auto bareName = filename.filename().string();
			j["materials"] = bareName + "_material.json";
			j["lights"] = bareName + "_light.json";
			j["camera"] = bareName + "_camera.json";
			j["environment"] = bareName + "_env.json";
		}

		saveFile(j, filename);
	}

	void SceneFormat::saveMesh(const fs::path& filename, const Mesh& mesh)
	{
		saveFile(getMeshJson(mesh, filename.parent_path(), fs::absolute(filename.string() + ".bmf")), filename);
	}

	void SceneFormat::saveCamera(const fs::path& filename, const Camera& camera)
	{
		saveFile(getCameraJson(camera), filename);
	}

	void SceneFormat::saveMaterials(const fs::path& filename, const std::vector<Material>& materials)
	{
		saveFile(getMaterialsJson(materials, filename.parent_path()), filename);
	}

	void SceneFormat::saveLights(const fs::path& filename, const std::vector<Light>& lights)
	{
		saveFile(getLightsJson(lights), filename);
	}

	void SceneFormat::saveEnvironment(const fs::path& filename, const Environment& env)
	{
		saveFile(getEnvironmentJson(env, filename.parent_path()), filename);
	}

	void SceneFormat::savePath(const fs::path& filename, const Path& path)
	{
		saveFile(getPathJson(path), filename);
	}

	SceneFormat::json SceneFormat::getMeshJson(const Mesh& mesh, const fs::path& root, const fs::path& bmfFilename)
	{
		json res;
		if(mesh.type == Mesh::Triangle)
		{
			res["type"] = "Triangle";
			res["file"] = getRelativePath(root, bmfFilename);
			mesh.triangle.saveToFile(bmfFilename.string());
		}
		else if(mesh.type == Mesh::Billboard)
		{
			res["type"] = "Billboard";
			res["file"] = getRelativePath(root, bmfFilename);
			mesh.triangle.saveToFile(bmfFilename.string());
		}

		if (!mesh.position.isStatic())
			res["position"] = getPathJson(mesh.position);
		if (!mesh.lookAt.isStatic())
			res["lookAt"] = getPathJson(mesh.lookAt);

		return res;
	}

	SceneFormat::json SceneFormat::getMaterialsJson(const std::vector<Material>& materials, const fs::path& root)
	{
		auto res = json::array();
		for (const auto& m : materials)
		{
			json j;
			j["name"] = m.name;

			// textures
			if (!m.textures.albedo.empty())
				j["albedoTex"] = getRelativePath(root, m.textures.albedo);
			if (!m.textures.specular.empty())
				j["specularTex"] = getRelativePath(root, m.textures.specular);
			if (!m.textures.occlusion.empty())
				j["occlusionTex"] = getRelativePath(root, m.textures.occlusion);

			// data
			// always write diffuse
			if(toSrgb(m.data.albedo) != MaterialData::Default().albedo)
				writeVec3(j["albedo"], toSrgb(m.data.albedo));
			if (m.data.roughness != MaterialData::Default().roughness)
				j["roughness"] = m.data.roughness;
			if (m.data.occlusion != MaterialData::Default().occlusion)
				j["occlusion"] = m.data.occlusion;
			if (m.data.specular != MaterialData::Default().specular)
				j["specular"] = m.data.specular;
			if (m.data.metalness != MaterialData::Default().metalness)
				j["metalness"] = m.data.metalness;
			if (toSrgb(m.data.emission) != MaterialData::Default().emission)
				writeVec3(j["emission"], toSrgb(m.data.emission));
			if (toSrgb(m.data.translucency) != MaterialData::Default().translucency)
				writeVec3(j["translucency"], toSrgb(m.data.translucency));

			// write flags as booleans
			const bool transparent = (m.data.flags & MaterialData::Transparent) != 0;
			if (((MaterialData::Default().flags & MaterialData::Transparent) != 0) != transparent)
				j["transparent"] = transparent;

			res.push_back(std::move(j));
		}
		return res;
	}

	SceneFormat::json SceneFormat::getLightsJson(const std::vector<Light>& lights)
	{
		auto res = json::array();
		for (const auto& l : lights)
		{
			json j;
			std::string strType;
			switch (l.data.type)
			{
			case LightData::Point:
				strType = "Point";
				writeVec3(j["position"], l.data.position);
				j["radius"] = l.data.radius;
				break;
			case LightData::Directional:
				strType = "Directional";
				writeVec3(j["direction"], l.data.direction);
				break;
			default:
				throw std::runtime_error("invalid light type");
			}

			j["type"] = strType;
			writeVec3(j["color"], toSrgb(l.data.color));

			if (!l.path.isStatic())
				j["path"] = getPathJson(l.path);

			res.push_back(std::move(j));
		}

		return res;
	}

	SceneFormat::json SceneFormat::getCameraJson(const Camera& camera)
	{
		json j;
		std::string strType;
		switch (camera.data.type)
		{
		case CameraData::Pinhole:
			strType = "Pinhole";
			break;
		default: throw std::runtime_error("invalid camera type");
		}

		j["type"] = strType;
		writeVec3(j["position"], camera.data.position);
		writeVec3(j["direction"], camera.data.direction);
		j["fov"] = camera.data.fov;
		if (camera.data.near != CameraData::Default().near)
			j["near"] = camera.data.near;
		if (camera.data.far != CameraData::Default().far)
			j["far"] = camera.data.far;
		if (camera.data.up != CameraData::Default().up)
			writeVec3(j["up"], camera.data.up);

		if (!camera.positionPath.isStatic())
			j["positionPath"] = getPathJson(camera.positionPath);
		if (!camera.lookAtPath.isStatic())
			j["lookAtPath"] = getPathJson(camera.lookAtPath);

		return j;
	}

	SceneFormat::json SceneFormat::getEnvironmentJson(const Environment& env, const fs::path& root)
	{
		json j;
		writeVec3(j["color"], toSrgb(env.color));

		if (env.ambientUp != Environment::Default().ambientUp)
			writeVec3(j["ambientUp"], toSrgb(env.ambientUp));
		if (env.ambientDown != Environment::Default().ambientDown)
			writeVec3(j["ambientDown"], toSrgb(env.ambientDown));

		if (!env.map.empty())
			j["map"] = getRelativePath(root, env.map);
		if (!env.ambient.empty())
			j["ambient"] = getRelativePath(root, env.ambient);

		return j;
	}

	SceneFormat::json SceneFormat::getPathJson(const Path& path)
	{
		json j;
		if (path.getScale() != 1.0f)
			j["scale"] = path.getScale();
		auto secj = json::array();
		for (const auto& s : path.getSections())
		{
			json js;
			js["time"] = s.time;
			writeVec3(js["pos"], s.position);
			secj.push_back(js);
		}

		j["sections"] = std::move(secj);
		return j;
	}

	SceneFormat::json SceneFormat::openFile(fs::path filename)
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

	void SceneFormat::saveFile(const json& j, fs::path filename)
	{
		filename = filename.replace_extension(".json");
		std::ofstream file(filename);
		if (!file.is_open())
			throw std::runtime_error("could not open " + filename.string());

		file << j.dump(3);
		file.close();
	}

	Mesh SceneFormat::loadMeshJson(const json& j, const fs::path& root)
	{
		Mesh m;

		// bmf filename
		auto mfile = j["file"].get<std::string>();
		auto meshFilePath = getAbsolutePath(root, mfile);

		// bmf file type
		auto strType = j["type"].get<std::string>();
		if (strType == "Triangle")
		{
			m.type = Mesh::Triangle;
			m.triangle.loadFromFile(meshFilePath.string());
		}
		else if (strType == "Billboard")
		{
			m.type = Mesh::Billboard;
			m.billboard.loadFromFile(meshFilePath.string());
		}
		else throw std::runtime_error("unknown mesh type " + strType);

		// load paths if present
		m.position = getPathOrDefault(j, "position", root);
		m.lookAt = getPathOrDefault(j, "lookAt", root);

		return m;
	}

	Material SceneFormat::loadMaterialJson(const json& j, const fs::path& root)
	{
		Material mat;
		mat.name = j["name"].get<std::string>();

		// textures
		mat.textures.albedo = getFilename(j, "albedoTex", root);
		mat.textures.specular = getFilename(j, "specularTex", root);
		mat.textures.occlusion = getFilename(j, "occlusionTex", root);

		// data
		mat.data.albedo = fromSrgb(getOrDefault(j, "albedo", MaterialData::Default().albedo));
		mat.data.roughness = getOrDefault(j, "roughness", MaterialData::Default().roughness);
		mat.data.occlusion = getOrDefault(j, "occlusion", MaterialData::Default().occlusion);
		mat.data.specular = getOrDefault(j, "specular", MaterialData::Default().specular);
		mat.data.emission = fromSrgb(getOrDefault(j, "emission", MaterialData::Default().emission));
		mat.data.metalness = getOrDefault(j, "metalness", 0.0f);
		mat.data.translucency = fromSrgb(getOrDefault(j, "translucency", glm::vec3(0.0f)));
		mat.data.flags = 0;

		if (getOrDefault(j, "transparent", (MaterialData::Default().flags & MaterialData::Transparent) != 0))
			mat.data.flags |= MaterialData::Transparent;

		return mat;
	}

	std::vector<Material> SceneFormat::loadMaterialsJson(const json& j, const fs::path& root)
	{
		// filename node?
		if (j.is_string())
			return loadMaterials(getAbsolutePath(root, j.get<std::string>()));

		std::vector<Material> materials;
		if (!j.is_array())
			throw std::runtime_error("materials must be an array");
		materials.reserve(j.size());
		for (const auto& m : j)
			materials.emplace_back(loadMaterialJson(m, root));

		return materials;
	}

	Camera SceneFormat::loadCameraJson(const json& j, const fs::path& root)
	{
		if (j.is_string())
			return loadCamera(getAbsolutePath(root, j.get<std::string>()));

		// data
		Camera cam;
		const auto strType = j["type"].get<std::string>();
		if (strType == "Pinhole")
			cam.data.type = CameraData::Pinhole;
		else throw std::runtime_error("unknown camera type " + strType);

		cam.data.position = getVec3(j["position"]);
		cam.data.direction = getVec3(j["direction"]);
		cam.data.fov = j["fov"].get<float>();
		cam.data.near = getOrDefault(j, "near", CameraData::Default().near);
		cam.data.far = getOrDefault(j, "far", CameraData::Default().far);
		cam.data.up = getOrDefault(j, "up", CameraData::Default().up);
		cam.data.speed = getOrDefault(j, "speed", 1.0f);

		// paths
		cam.positionPath = getPathOrDefault(j, "positionPath", root);
		cam.lookAtPath = getPathOrDefault(j, "lookAtPath", root);

		return cam;
	}

	Environment SceneFormat::loadEnvironmentJson(const json& j, const fs::path& root)
	{
		if (j.is_string())
			return loadEnvironment(getAbsolutePath(root, j.get<std::string>()));

		Environment env;
		env.color = fromSrgb(getVec3(j["color"]));
		env.ambientUp = fromSrgb(getOrDefault(j, "ambientUp", Environment::Default().ambientUp));
		env.ambientDown = fromSrgb(getOrDefault(j, "ambientDown", Environment::Default().ambientDown));

		env.map = getFilename(j, "map", root);
		env.ambient = getFilename(j, "ambient", root);

		return env;
	}

	std::vector<Light> SceneFormat::loadLightsJson(const json& j, const fs::path& root)
	{
		if (j.is_string())
			return loadLights(getAbsolutePath(root, j.get<std::string>()));

		std::vector<Light> lights;
		if (!j.is_array())
			throw std::runtime_error("lights must be an array");
		lights.reserve(j.size());
		for (const auto& l : j)
			lights.emplace_back(loadLightJson(l, root));

		return lights;
	}

	Light SceneFormat::loadLightJson(const json& j, const fs::path& root)
	{
		Light l;
		const auto strType = j["type"].get<std::string>();
		if (strType == "Point")
		{
			l.data.type = LightData::Point;
			l.data.position = getVec3(j["position"]);
			l.data.radius = j["radius"].get<float>();
		}
		else if (strType == "Directional")
		{
			l.data.type = LightData::Directional;
			l.data.direction = getVec3(j["direction"]);
		}
		else throw std::runtime_error("invalid light type " + strType);

		l.data.color = fromSrgb(getVec3(j["color"]));

		l.path = getPathOrDefault(j, "path", root);

		return l;
	}

	Path SceneFormat::loadPathJson(const json& j, const fs::path& root)
	{
		if (j.is_string())
			return loadPath(getAbsolutePath(root, j.get<std::string>()));

		auto scale = getOrDefault(j, "scale", 1.0f);
		std::vector<PathSection> sections;
		auto secs = j.find("sections");
		if (secs != j.end())
		{
			if (!secs->is_array())
				throw std::runtime_error("sections must be an array");
			sections.reserve(secs->size());
			for (auto& s : *secs)
			{
				sections.emplace_back(loadPathSectionJson(s));
			}
		}

		return Path(std::move(sections), scale);
	}

	PathSection SceneFormat::loadPathSectionJson(const json& j)
	{
		PathSection s;
		s.time = j["time"].get<float>();
		s.position = getVec3(j["pos"]);
		return s;
	}

	std::string SceneFormat::generateMeshSuffix(const Mesh& mesh) const
	{
		std::string suffix;

		if (!mesh.isStatic())
			suffix += "Moving";

		if(mesh.type == Mesh::Billboard)
		{
			return suffix + "Points";
		}

		if (mesh.isTransparent(m_materials))
			suffix = "Trans" + suffix;
		
		return suffix;
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
	glm::vec3 SceneFormat::getOrDefault<glm::vec3>(const json& j, const char* name,
		glm::vec3 defaultValue)
	{
		auto it = j.find(name);
		if (it == j.end()) return defaultValue;
		return getVec3(it.value());
	}

	Path SceneFormat::getPathOrDefault(const json& j, const char* name, const fs::path& root)
	{
		auto it = j.find(name);
		if (it == j.end()) return Path();
		return loadPathJson(*it, root);
	}

	fs::path SceneFormat::getFilename(const json& j, const char* name, const fs::path& root)
	{
		const auto it = j.find(name);
		if (it == j.end()) return fs::path();

		return getAbsolutePath(root, it.value().get<std::string>());
	}

	glm::vec3 SceneFormat::getVec3(const json& j)
	{
		if (j.is_array())
		{
			if (j.size() == 1)
				return glm::vec3(j[0].get<float>());
			if (j.size() == 3)
			{
				auto tmp = j.get<std::array<float, 3>>();
				return glm::vec3(tmp[0], tmp[1], tmp[2]);
			}
			throw std::runtime_error("expected array with 3 or 1 element but got " + std::to_string(j.size()));
		}

		return glm::vec3(j.get<float>());
	}

	void SceneFormat::writeVec3(json& j, const glm::vec3& vec)
	{
		// all values are equal?	
		if (vec[0] == vec[1] && vec[1] == vec[2])
			j = vec[0]; // write only single value
		else
			j = std::array<float, 3>{vec.x, vec.y, vec.z};
	}

	std::string SceneFormat::getRelativePath(const fs::path& root, const fs::path& p)
	{
		if (p.is_relative()) // is already relative to the json root?
			return p.string();

		std::error_code err;
		auto res = fs::relative(p, root, err);
		if (err)
			throw std::runtime_error("could not form relative path: " + err.message());

		return res.string();
	}

	fs::path SceneFormat::getAbsolutePath(const fs::path& root, const fs::path& p)
	{
		if (p.is_absolute())
			return p;

		return root / p;
	}
}