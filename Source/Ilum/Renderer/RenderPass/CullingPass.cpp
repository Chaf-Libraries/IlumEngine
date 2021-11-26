#pragma once

#include "CullingPass.hpp"

#include "Renderer/Renderer.hpp"

#include "Graphics/GraphicsContext.hpp"

#include "Device/PhysicalDevice.hpp"

namespace Ilum::pass
{
void CullingPass::setupPipeline(PipelineState &state)
{
	state.shader.load(std::string(PROJECT_SOURCE_DIR) + "Asset/Shader/GLSL/culling.comp", VK_SHADER_STAGE_COMPUTE_BIT, Shader::Type::GLSL);

	state.descriptor_bindings.bind(0, 0, "BoundingBox", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 1, "IndirectDrawCommand", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 2, "TransformData", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 3, "Camera", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	state.descriptor_bindings.bind(0, 4, "Meshlet", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

void CullingPass::resolveResources(ResolveState &resolve)
{
	resolve.resolve("BoundingBox", Renderer::instance()->getBuffer(Renderer::BufferType::BoundingBox));
	resolve.resolve("IndirectDrawCommand", Renderer::instance()->getBuffer(Renderer::BufferType::IndirectCommand));
	resolve.resolve("TransformData", Renderer::instance()->getBuffer(Renderer::BufferType::Transform));
	resolve.resolve("Camera", Renderer::instance()->getBuffer(Renderer::BufferType::MainCamera));
	resolve.resolve("Meshlet", Renderer::instance()->getBuffer(Renderer::BufferType::Meshlet));
}

void CullingPass::render(RenderPassState &state)
{
	auto &cmd_buffer = state.command_buffer;

	vkCmdBindPipeline(cmd_buffer, state.pass.bind_point, state.pass.pipeline);

	for (auto &descriptor_set : state.pass.descriptor_sets)
	{
		vkCmdBindDescriptorSets(cmd_buffer, state.pass.bind_point, state.pass.pipeline_layout, descriptor_set.index(), 1, &descriptor_set.getDescriptorSet(), 0, nullptr);
	}

	uint32_t group_count = Renderer::instance()->Meshlet_Count / 256 + 1;
	vkCmdDispatch(cmd_buffer, group_count, 1, 1);
}
}        // namespace Ilum::pass