#pragma once

#include "Renderer/RenderPass.hpp"

namespace Ilum
{
STRUCT(VisibilityBufferPass, Enable, RenderPass("Visibility Buffer Pass"), Category("Base Pass")) :
    public RenderPass
{
	virtual RenderPassDesc CreateDesc() override;

	virtual RenderGraph::RenderTask Create(const RenderPassDesc &desc, RenderGraphBuilder &builder, Renderer *renderer) override;

	struct Config
	{
		[[editor("slider"), min(0), max(100)]] float a = 100.f;
		std::string                                  m = "fuck you";
	};
};
}        // namespace Ilum