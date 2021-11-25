#pragma once

#include "Graphics/Model/Model.hpp"

#include "Material/Material.h"

namespace Ilum::cmpt
{
struct MeshRenderer
{
	std::string model;

	std::vector<scope<IMaterial>> materials;

	inline static bool update = false;
};
}        // namespace Ilum::Cmpt