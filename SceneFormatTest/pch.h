//
// pch.h
// Header for standard system include files.
//

#pragma once

#include "gtest/gtest.h"
#include "../include/hrsf/SceneFormat.h"
#include <glm/vec3.hpp>
using namespace hrsf;

inline void EXPECT_VEC3_EQUAL(glm::vec3 a, glm::vec3 b)
{
	EXPECT_LE(std::abs(a.x - b.x), 0.0001f);
	EXPECT_LE(std::abs(a.y - b.y), 0.0001f);
	EXPECT_LE(std::abs(a.z - b.z), 0.0001f);
}