#include "pch.h"

#define TestSuite SceneFormatIOTest

TEST(TestSuite, SaveLoad) 
{
	// dummy mesh
	const std::vector<float> vertices = {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // vertex 1
		1.0f, 0.0f, 1.0f, 0.1f, 0.2f, // vertex 2
		0.0f, 1.0f, 0.0f, 0.5f, 0.6f, // vertex 3
		1.0f, 0.0f, 2.0f, 0.7f, 0.9f, // vertex 4
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, // triangle 1
		1, 2, 3, // triangle 2
	};
	const std::vector<bmf::BinaryMesh::Shape> shapes = {
		bmf::BinaryMesh::Shape{0, 3, 0}, // shape
		bmf::BinaryMesh::Shape{3, 3, 1}, // shape
	};

	bmf::BinaryMesh mesh(bmf::Position | bmf::Texcoord0, vertices, indices, shapes);
	EXPECT_NO_THROW(mesh.verify());

	Camera cam = Camera::Default();
	cam.fov = 1.4f;
	cam.far = 1.0f;
	cam.position = { 10.f, 20.0f, 30.0f };

	std::vector<Light> lights;
	// point light
	lights.emplace_back(Light{ Light::Point });
	lights.back().position = { 0.0f, 30.0f, 0.0f };
	lights.back().linearFalloff = 1.0f;
	lights.back().quadFalloff = 10.0f;
	lights.back().color = { 1.0f, 0.0f, 0.0f };

	// directional light
	lights.emplace_back(Light{ Light::Directional });
	lights.back().direction = { 0.1f, -1.0f, 0.0f };
	lights.back().color = { 1.0f, 0.8f, 1.0f };

	std::vector<Material> materials;
	// mat 1 (default material)
	materials.emplace_back();
	materials.back().name = "default";
	materials.back().data = MaterialData::Default();

	materials.emplace_back();
	materials.back().name = "spec";
	materials.back().data = MaterialData::Default();
	materials.back().textures.diffuse = "myTexture";
	materials.back().data.flags = MaterialData::Reflection;
	materials.back().data.specular = { 1.0f, 0.0f, 1.0f };
	
	Environment env;
	env.color = { 0.4f, 0.6f, 1.0f };
	env.map = "envmap.hdr";

	SceneFormat f(std::move(mesh), cam, lights, materials, env);

	// save file
	f.save("test");

	// load file 
	SceneFormat res;
	ASSERT_NO_THROW(res = SceneFormat::load("test"));

	// test some material properties
	EXPECT_EQ(res.getMaterials().size(), f.getMaterials().size());
	EXPECT_EQ(res.getMaterials()[0].name, f.getMaterials()[0].name);
	EXPECT_EQ(res.getMaterials()[1].name, f.getMaterials()[1].name);
	
	EXPECT_EQ(res.getMaterials()[1].textures.diffuse, f.getMaterials()[1].textures.diffuse);
	EXPECT_EQ(res.getMaterials()[1].data.specular, f.getMaterials()[1].data.specular);
	EXPECT_EQ(res.getMaterials()[1].data.flags, f.getMaterials()[1].data.flags);

	// test some camera stuff
	EXPECT_EQ(res.getCamera().position, f.getCamera().position);
	EXPECT_EQ(res.getCamera().far, f.getCamera().far);
	EXPECT_EQ(res.getCamera().fov, f.getCamera().fov);

	// test some light stuff
	EXPECT_EQ(res.getLights().size(), f.getLights().size());
	EXPECT_EQ(res.getLights()[0].type, f.getLights()[0].type);
	EXPECT_EQ(res.getLights()[1].type, f.getLights()[1].type);

	// test some env stuff
	EXPECT_EQ(res.getEnvironment().color, f.getEnvironment().color);
	EXPECT_EQ(res.getEnvironment().map, f.getEnvironment().map);

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
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, // triangle 1
	};
	const std::vector<bmf::BinaryMesh::Shape> shapes = {
		bmf::BinaryMesh::Shape{0, 3, 0},
		bmf::BinaryMesh::Shape{0, 3, 1},
		bmf::BinaryMesh::Shape{0, 3, 3},
	};

	bmf::BinaryMesh mesh(bmf::Position, vertices, indices, shapes);
	EXPECT_NO_THROW(mesh.verify());

	Camera cam = Camera::Default();

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

	Environment env;
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
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, // triangle 1
	};
	const std::vector<bmf::BinaryMesh::Shape> shapes = {
		bmf::BinaryMesh::Shape{0, 3, 0},
		bmf::BinaryMesh::Shape{0, 3, 1}, // out of bound material
	};

	bmf::BinaryMesh mesh(bmf::Position, vertices, indices, shapes);
	EXPECT_NO_THROW(mesh.verify());

	Camera cam = Camera::Default();

	std::vector<Light> lights;

	std::vector<Material> materials;
	// mat 0 (default material)
	materials.emplace_back();
	materials.back().name = "mat0";
	materials.back().data = MaterialData::Default();

	Environment env;
	env.color = { 0.4f, 0.6f, 1.0f };

	SceneFormat f(std::move(mesh), cam, lights, materials, env);

	EXPECT_THROW(f.verify(), std::runtime_error);
}