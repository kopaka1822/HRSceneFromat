#pragma once
#include <string>
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Mesh.h"
#include "../../dependencies/json/single_include/nlohmann/json.hpp"
#include "Environment.h"
#include <filesystem>
#include "srgb.h"

namespace hrsf
{
	namespace fs = std::filesystem;

	enum class Component : uint32_t
	{
		None = 0,
		Mesh = 1,
		Camera = 1 << 1,
		Lights = 1 << 2,
		Material = 1 << 3,
		Environment = 1 << 4,
		All = 0xFFFFFFFF
	};

	class SceneFormat
	{
		using json = nlohmann::json;
	public:
		SceneFormat() = default;
		SceneFormat(std::vector<Mesh> meshes, Camera cam, std::vector<Light> lights, std::vector<Material> materials, Environment env);
		SceneFormat(SceneFormat&&) = default;
		SceneFormat& operator=(SceneFormat&&) = default;

		const std::vector<Mesh>& getMeshes() const;
		const Camera& getCamera() const;
		const std::vector<Light>& getLights() const;
		const std::vector<Material>& getMaterials() const;
		std::vector<MaterialData> getMaterialsData() const;
		const Environment& getEnvironment() const;

		void removeUnusedMaterials();
		// adds the offset to each material index
		void offsetMaterials(uint32_t offset);
		/// \brief throws an exception if something seems wrong
		void verify() const;

		/// \brief loads the scene from the filesystem
		/// \param filename filename without extension
		static SceneFormat load(fs::path filename);
		/// \brief loads the camera from the filesystem
		/// \param filename filename without extension
		static Mesh loadMesh(fs::path filename);
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

		
		static Path loadPath(fs::path filename);
		/// \brief saves the scene.
		/// \param filename output files are: filename.json, filename.bmf
		/// \param singleFile if false, the json for each component will be put in a different file
		/// \param components components that will be written into the file.
		///        If a component is missing and singleFile is false, the filename reference will be written but not the component file itself.
		void save(const fs::path& filename, bool singleFile, Component components = Component::All) const;
		static void saveMesh(const fs::path& filename, const Mesh& mesh);
		static void saveCamera(const fs::path& filename, const Camera& camera);
		static void saveMaterials(const fs::path& filename, const std::vector<Material>& materials);
		static void saveLights(const fs::path& filename, const std::vector<Light>& lights);
		static void saveEnvironment(const fs::path& filename, const Environment& env);
		static void savePath(const fs::path& filename, const Path& path);
	private:
		static json getMeshJson(const Mesh& mesh, const fs::path& root, const fs::path& bmfFilename);
		static json getMaterialsJson(const std::vector<Material>& materials, const fs::path& root);
		static json getLightsJson(const std::vector<Light>& lights);
		static json getCameraJson(const Camera& camera);
		static json getEnvironmentJson(const Environment& env, const fs::path& root);
		static json getPathJson(const Path& path);
		static json openFile(fs::path filename);
		static void saveFile(const json& j, fs::path filename);
		static Mesh loadMeshJson(const json& j, const fs::path& root);
		static Material loadMaterialJson(const json& j, const fs::path& root);
		static std::vector<Material> loadMaterialsJson(const json& j, const fs::path& root);
		static Camera loadCameraJson(const json& j, const fs::path& root);
		static Environment loadEnvironmentJson(const json& j, const fs::path& root);
		static std::vector<Light> loadLightsJson(const json& j, const fs::path& root);
		static Light loadLightJson(const json& j, const fs::path& root);
		static Path loadPathJson(const json& j, const fs::path& root);
		static PathSection loadPathSectionJson(const json& j);

		/// generates a mesh suffix based on the mesh properties
		std::string generateMeshSuffix(const Mesh& mesh) const;

		/// \brief retrieves the value from the json. if the json does not contain the value
		/// the default value is returned instead
		template<class T>
		static T getOrDefault(const json& j, const char* name, T defaultValue);
		template<>
		static glm::vec3 getOrDefault(const json& j, const char* name, glm::vec3 defaultValue);
		static Path getPathOrDefault(const json& j, const char* name, const fs::path& root);

		static fs::path getFilename(const json& j, const char* name, const fs::path& root);

		static glm::vec3 getVec3(const json& j);
		static void writeVec3(json& j, const glm::vec3& vec);

		static std::string getRelativePath(const fs::path& root, const fs::path& p);
		static fs::path getAbsolutePath(const fs::path& root, const fs::path& p);

		std::vector<Mesh> m_meshes;
		Camera m_camera;
		std::vector<Light> m_lights;
		std::vector<Material> m_materials;
		Environment m_environment;

		static constexpr size_t s_version = 5;
	};

	inline Component operator|(Component a, Component b)
	{
		return Component(uint32_t(a) | uint32_t(b));
	}

	inline bool operator&(Component a, Component b)
	{
		return (uint32_t(a) & uint32_t(b)) != 0;
	}
}
