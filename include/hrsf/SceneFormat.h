#pragma once
#include <string>
#include "../../dependencies/bmf/include/bmf/BinaryMesh.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "../../dependencies/json/single_include/nlohmann/json.hpp"
#include "Environment.h"
#include <filesystem>
#include "srgb.h"

namespace hrsf
{
	namespace fs = std::filesystem;

	class SceneFormat
	{
		using json = nlohmann::json;
	public:
		using MeshT = bmf::BinaryMesh16;

		SceneFormat() = default;
		SceneFormat(MeshT mesh, Camera cam, std::vector<Light> lights, std::vector<Material> materials, Environment env);
		SceneFormat(SceneFormat&&) = default;
		SceneFormat& operator=(SceneFormat&&) = default;

		const MeshT& getMesh() const;
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
		static Path loadPath(fs::path filename);
		/// \brief saves the scene.
		/// \param filename output files are: filename.json, filename.bmf
		void save(const fs::path& filename, bool singleFile) const;
		static void saveCamera(const fs::path& filename, const Camera& camera);
		static void saveMaterials(const fs::path& filename, const std::vector<Material>& materials);
		static void saveLights(const fs::path& filename, const std::vector<Light>& lights);
		static void saveEnvironment(const fs::path& filename, const Environment& env);
		static void savePath(const fs::path& filename, const Path& path);
	private:
		static json getMaterialsJson(const std::vector<Material>& materials, const fs::path& root);
		static json getLightsJson(const std::vector<Light>& lights);
		static json getCameraJson(const Camera& camera);
		static json getEnvironmentJson(const Environment& env, const fs::path& root);
		static json getPathJson(const Path& path);
		static json openFile(fs::path filename);
		static void saveFile(const json& j, fs::path filename);
		static Material loadMaterialJson(const json& j, const fs::path& root);
		static std::vector<Material> loadMaterialsJson(const json& j, const fs::path& root);
		static Camera loadCameraJson(const json& j, const fs::path& root);
		static Environment loadEnvironmentJson(const json& j, const fs::path& root);
		static std::vector<Light> loadLightsJson(const json& j, const fs::path& root);
		static Light loadLightJson(const json& j, const fs::path& root);
		static Path loadPathJson(const json& j, const fs::path& root);
		static PathSection loadPathSectionJson(const json& j);

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

		MeshT m_mesh;
		Camera m_camera;
		std::vector<Light> m_lights;
		std::vector<Material> m_materials;
		Environment m_environment;

		static constexpr size_t s_version = 4;
	};
}
