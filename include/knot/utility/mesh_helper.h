// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/mesh.h>

namespace knot {
class Mesh;
class Shader;
class Model;

/** @brief Calculates orthogonalized vertex tangents for indexed triangles.
 *  @pre @p indices is a multiple of three and all indices are valid. */
void calculateMeshTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
/** @brief Creates a centered unit cube mesh. */
std::shared_ptr<Mesh> createCube();
/** @brief Creates a UV sphere with radius 0.5.
 *  @param sectors Number of longitudinal segments.
 *  @param stacks Number of latitudinal segments. */
std::shared_ptr<Mesh> createSphere(int sectors, int stacks);
/** @brief Creates an XY-plane quad centered at the origin. */
std::shared_ptr<Mesh> createPlane(float width, float height);
/** @brief Creates a centered regular polygon in the XY plane.
 *  @return nullptr if @p sectors is less than three. */
std::shared_ptr<Mesh> createRegularPolygon(int sectors, float radius);
/** @brief Triangulates an ordered polygon boundary as a fan.
 *  @return nullptr if fewer than three positions are supplied. */
std::shared_ptr<Mesh> createMeshFromVertices(const std::vector<glm::vec3>& positions);
/** @brief Loads geometry from an OBJ file and uploads it to the GPU.
 *  @return The loaded mesh, or nullptr if the file cannot be parsed. */
std::shared_ptr<Mesh> loadModelOBJ(const std::string& filePath);
/** @brief Loads an OBJ file with its referenced MTL materials.
 *
 *  The OBJ geometry is split into one model per material group. Each group
 *  gets a PbrMaterial built from the matching MTL entry: map_Kd/Kd become the
 *  albedo map and fallback color, map_Pr/Pr or Ns the roughness, map_Pm/Pm the
 *  metallic factor, and norm/bump/map_bump the normal map. Texture paths are
 *  resolved relative to the OBJ file.
 *  @param filePath Path to the OBJ file.
 *  @param pbrShader Shader assigned to every created material.
 *  @return One model per material group, or an empty vector on failure. */
std::vector<std::shared_ptr<Model>> loadModelOBJWithMTL(const std::string& filePath, std::shared_ptr<Shader> pbrShader);
} // namespace knot
