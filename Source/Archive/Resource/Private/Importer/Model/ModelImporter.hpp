#pragma once

#include "Resource/ModelResource.hpp"
#include "Importer/Texture/TextureImporter.hpp"

#include <Geometry/Vertex.hpp>

#include <vector>

namespace Ilum
{
struct ModelImportInfo
{
	std::string name;

	std::vector<Submesh> submeshes;

	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;

	std::vector<Meshlet>  meshlets;
	std::vector<uint32_t> meshlet_vertices;
	std::vector<uint32_t> meshlet_primitives;

	AABB aabb;

	 std::unordered_map<size_t, TextureImportInfo> textures;
};

class ModelImporter
{
  public:
	virtual ModelImportInfo ImportImpl(const std::string &filename) = 0;

	static ModelImportInfo Import(const std::string &filename);

  protected:
	uint32_t PackTriangle(uint8_t v0, uint8_t v1, uint8_t v2);
};

}        // namespace Ilum