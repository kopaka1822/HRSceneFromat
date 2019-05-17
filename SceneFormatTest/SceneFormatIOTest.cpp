#include "pch.h"
#include <glm/vec3.hpp>

#define TestSuite SceneFormatIOTest

TEST(TestSuite, SaveLoad) 
{
	// dummy mesh
	const std::vector<float> vertices = {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // vertex 0
		1.0f, 0.0f, 1.0f, 0.1f, 0.2f, // vertex 1
		0.0f, 1.0f, 0.0f, 0.5f, 0.6f, // vertex 2
		1.0f, 0.0f, 2.0f, 0.7f, 0.9f, // vertex 3
		1.0f, 0.0f, 2.0f, 0.7f, 0.9f, // vertex 4
		1.0f, 0.0f, 2.0f, 0.7f, 0.9f, // vertex 5
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, // triangle 1
		2, 1, 0, // triangle 2
	};
	const std::vector<bmf::BinaryMesh::Shape> shapes = {
		bmf::BinaryMesh::Shape{0, 3, 0, 3, /*0, 1,*/ 0}, // shape
		bmf::BinaryMesh::Shape{3, 3, 3, 3, /*1, 1,*/ 1}, // shape
	};

	bmf::BinaryMesh mesh(bmf::Position | bmf::Texcoord0, vertices, indices, shapes);// , { glm::vec3(1.0f), glm::vec3(1.0f) });
	mesh.generateBoundingBoxes();
	EXPECT_NO_THROW(mesh.verify());

	Camera cam;
	cam.data = CameraData::Default();
	cam.data.fov = 1.4f;
	cam.data.far = 1.0f;
	cam.data.position = { 10.f, 20.0f, 30.0f };

	std::vector<Light> lights;
	// point light
	lights.emplace_back(Light{ LightData::Point });
	lights.back().data.position = { 0.0f, 30.0f, 0.0f };
	lights.back().data.radius = 1.0f;
	lights.back().data.color = { 1.0f, 0.0f, 0.0f };

	// directional light
	lights.emplace_back(Light{ LightData::Directional });
	lights.back().data.direction = { 0.1f, -1.0f, 0.0f };
	lights.back().data.color = { 1.0f, 0.8f, 1.0f };

	std::vector<Material> materials;
	// mat 1 (default material)
	materials.emplace_back();
	materials.back().name = "default";
	materials.back().data = MaterialData::Default();

	materials.emplace_back();
	materials.back().name = "spec";
	materials.back().data = MaterialData::Default();
	materials.back().textures.diffuse = "myTexture"; // save relative path
	materials.back().data.flags = MaterialData::Reflection;
	materials.back().data.specular = { 1.0f, 0.0f, 1.0f };
	
	Environment env = Environment::Default();
	env.color = { 0.4f, 0.6f, 1.0f };
	env.map = "envmap.hdr";

	SceneFormat f(std::move(mesh), cam, lights, materials, env);

	// save file
	f.save("subfolder/test");

	// load file 
	SceneFormat res;
	ASSERT_NO_THROW(res = SceneFormat::load("subfolder/test"));

	// test some material properties
	EXPECT_EQ(res.getMaterials().size(), f.getMaterials().size());
	EXPECT_EQ(res.getMaterials()[0].name, f.getMaterials()[0].name);
	EXPECT_EQ(res.getMaterials()[1].name, f.getMaterials()[1].name);

	// expect absolute path for texture
	EXPECT_EQ(res.getMaterials()[1].textures.diffuse, 
		std::filesystem::absolute(fs::path("subfolder/" + f.getMaterials()[1].textures.diffuse.string())));
	EXPECT_VEC3_EQUAL(res.getMaterials()[1].data.specular, f.getMaterials()[1].data.specular);
	EXPECT_EQ(res.getMaterials()[1].data.flags, f.getMaterials()[1].data.flags);

	// test some camera stuff
	EXPECT_EQ(res.getCamera().data.position, f.getCamera().data.position);
	EXPECT_EQ(res.getCamera().data.far, f.getCamera().data.far);
	EXPECT_EQ(res.getCamera().data.fov, f.getCamera().data.fov);

	// test some light stuff
	EXPECT_EQ(res.getLights().size(), f.getLights().size());
	EXPECT_EQ(res.getLights()[0].data.type, f.getLights()[0].data.type);
	EXPECT_EQ(res.getLights()[1].data.type, f.getLights()[1].data.type);

	// test some env stuff
	EXPECT_VEC3_EQUAL(res.getEnvironment().color, f.getEnvironment().color);
	EXPECT_EQ(res.getEnvironment().map, 
		std::filesystem::absolute(fs::path("subfolder/" + f.getEnvironment().map.string())));

	// test some binary mesh stuff
	EXPECT_NO_THROW(res.getMesh().verify());
	EXPECT_EQ(res.getMesh().getAttributes(), f.getMesh().getAttributes());
	EXPECT_EQ(res.getMesh().getIndices(), f.getMesh().getIndices());
	EXPECT_EQ(res.getMesh().getVertices(), f.getMesh().getVertices());
	EXPECT_EQ(res.getMesh().getShapes().size(), f.getMesh().getShapes().size());
}

TEST(TestSuite, UnusedMaterials)
{
	// dummy mesh
	const std::vector<float> vertices = {
		0.0f, 0.0f, 0.0f, // vertex 1
		1.0f, 0.0f, 1.0f, // vertex 2
		0.0f, 1.0f, 0.0f, // vertex 3
		0.0f, 0.0f, 0.0f, // vertex 1
		1.0f, 0.0f, 1.0f, // vertex 2
		0.0f, 1.0f, 0.0f, // vertex 3
		0.0f, 0.0f, 0.0f, // vertex 1
		1.0f, 0.0f, 1.0f, // vertex 2
		0.0f, 1.0f, 0.0f, // vertex 3
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, // triangle 1
		0, 1, 2, // triangle 1
		0, 1, 2, // triangle 1
	};
	const std::vector<bmf::BinaryMesh::Shape> shapes = {
		bmf::BinaryMesh::Shape{0, 3, 0, 3, /*0,1,*/0},
		bmf::BinaryMesh::Shape{0, 3, 3, 3, /*1,1,*/1},
		bmf::BinaryMesh::Shape{0, 3, 6, 3, /*2,1,*/3},
	};

	bmf::BinaryMesh mesh(bmf::Position, vertices, indices, shapes);//, {glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) });
	mesh.generateBoundingBoxes();
	EXPECT_NO_THROW(mesh.verify());

	Camera cam;
	cam.data = CameraData::Default();

	std::vector<Light> lights;

	std::vector<Material> materials;
	// mat 0 (default material)
	materials.emplace_back();
	materials.back().name = "mat0";
	materials.back().data = MaterialData::Default();

	// mat 1
	materials.emplace_back();
	materials.back().name = "mat1";
	materials.back().data = MaterialData::Default();

	// mat 2
	materials.emplace_back();
	materials.back().name = "mat2";
	materials.back().data = MaterialData::Default();

	// mat 3
	materials.emplace_back();
	materials.back().name = "mat3";
	materials.back().data = MaterialData::Default();

	// mat 4
	materials.emplace_back();
	materials.back().name = "mat4";
	materials.back().data = MaterialData::Default();

	Environment env = Environment::Default();
	env.color = { 0.4f, 0.6f, 1.0f };

	SceneFormat f(std::move(mesh), cam, lights, materials, env);

	EXPECT_NO_THROW(f.verify());
	f.removeUnusedMaterials();

	// kept the cor
	EXPECT_EQ(f.getMaterials().size(), 3);
	EXPECT_EQ(f.getMaterials()[0].name, std::string("mat0"));
	EXPECT_EQ(f.getMaterials()[1].name, std::string("mat1"));
	EXPECT_EQ(f.getMaterials()[2].name, std::string("mat3"));

	// new material ids for shapes
	EXPECT_EQ(f.getMesh().getShapes()[0].materialId, 0);
	EXPECT_EQ(f.getMesh().getShapes()[1].materialId, 1);
	EXPECT_EQ(f.getMesh().getShapes()[2].materialId, 2);
}

TEST(TestSuite, VerifyFail)
{
	// dummy mesh
	const std::vector<float> vertices = {
		0.0f, 0.0f, 0.0f, // vertex 1
		1.0f, 0.0f, 1.0f, // vertex 2
		0.0f, 1.0f, 0.0f, // vertex 3
		0.0f, 0.0f, 0.0f, // vertex 1
		1.0f, 0.0f, 1.0f, // vertex 2
		0.0f, 1.0f, 0.0f, // vertex 3
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, // triangle 1
		0, 1, 2, // triangle 2
	};
	const std::vector<bmf::BinaryMesh::Shape> shapes = {
		bmf::BinaryMesh::Shape{0, 3, 0, 3, /*0,1,*/0},
		bmf::BinaryMesh::Shape{0, 3, 3, 3, /*1,1,*/1}, // out of bound material
	};

	bmf::BinaryMesh mesh(bmf::Position, vertices, indices, shapes);// , { glm::vec3(1.0f) , glm::vec3(1.0f) });
	mesh.generateBoundingBoxes();
	EXPECT_NO_THROW(mesh.verify());

	Camera cam;
	cam.data = CameraData::Default();

	std::vector<Light> lights;

	std::vector<Material> materials;
	// mat 0 (default material)
	materials.emplace_back();
	materials.back().name = "mat0";
	materials.back().data = MaterialData::Default();

	Environment env = Environment::Default();
	env.color = { 0.4f, 0.6f, 1.0f };

	SceneFormat f(std::move(mesh), cam, lights, materials, env);

	EXPECT_THROW(f.verify(), std::runtime_error);
}

TEST(TestSuite, PathSectionLoad)
{
	std::vector<PathSection> sections;
	sections.push_back({ 2.0f, glm::vec3(1.0f) });
	sections.push_back({ 7.0f, glm::vec3(5.0f) });
	sections.push_back({ 1.0f, glm::vec3(2.0f) });
	Path origPath(sections, 1.0f);
	EXPECT_NO_THROW(origPath.verify());

	hrsf::SceneFormat::savePath("testpath", origPath);
	auto loadPath = hrsf::SceneFormat::loadPath("testpath");
	EXPECT_NO_THROW(loadPath.verify());

	EXPECT_EQ(origPath.getSections().size(), loadPath.getSections().size());
	EXPECT_EQ(origPath.getSections()[0].time, loadPath.getSections()[0].time);
	EXPECT_EQ(origPath.getSections()[2].position, loadPath.getSections()[2].position);
}