#pragma once

#include "Renderer/RenderPass.hpp"

namespace Ilum
{
STRUCT(CUDATexture, Enable, RenderPass("CUDA Texture"), Category("CUDA-HLSL Test")) :
    public RenderPass
{
	virtual RenderPassDesc CreateDesc() override;

	virtual RenderGraph::RenderTask Create(const RenderPassDesc &desc, RenderGraphBuilder &builder, Renderer *renderer) override;
};
}        // namespace Ilum