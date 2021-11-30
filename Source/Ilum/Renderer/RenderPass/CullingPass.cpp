#pragma once

#include "CullingPass.hpp"

#include "Renderer/Renderer.hpp"

#include "Graphics/GraphicsContext.hpp"

#include "Device/PhysicalDevice.hpp"

namespace Ilum::pass
{
CullingPass::CullingPass()
{
	m_count_buffer = createScope<Buffer>(sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
}

void CullingPass::setupPipeline(PipelineState &state)
{
	state.shader.load(std::string(PROJECT_SOURCE_DIR) + "Asset/Shader/GLSL/culling.comp", VK_SHADER_STAGE_COMPUTE_BIT, Shader::Type::GLSL);

	state.descriptor_bindings.bind(0, 0, "BoundingBox", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 1, "IndirectDrawCommand", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 2, "TransformData", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 3, "Camera", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	state.descriptor_bindings.bind(0, 4, "Meshlet", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 5, "hiz - buffer", Renderer::instance()->getSampler(Renderer::SamplerType::Trilinear_Clamp), ImageViewType::Native, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	state.descriptor_bindings.bind(0, 6, "meshlet_count", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

void CullingPass::resolveResources(ResolveState &resolve)
{
	resolve.resolve("BoundingBox", Renderer::instance()->getBuffer(Renderer::BufferType::BoundingBox));
	resolve.resolve("IndirectDrawCommand", Renderer::instance()->getBuffer(Renderer::BufferType::IndirectCommand));
	resolve.resolve("TransformData", Renderer::instance()->getBuffer(Renderer::BufferType::Transform));
	resolve.resolve("Camera", Renderer::instance()->getBuffer(Renderer::BufferType::MainCamera));
	resolve.resolve("Meshlet", Renderer::instance()->getBuffer(Renderer::BufferType::Meshlet));
	resolve.resolve("hiz - buffer", *Renderer::instance()->Last_Frame.hiz_buffer);
	resolve.resolve("meshlet_count", *m_count_buffer);
}

void CullingPass::render(RenderPassState &state)
{
	{
		std::memcpy(&Renderer::instance()->Meshlet_Visible, m_count_buffer->map(), sizeof(uint32_t));
		m_count_buffer->unmap();
	}

	// Update cull data
	{
		m_cull_data.last_view        = m_cull_data.view;
		m_cull_data.view             = Renderer::instance()->Main_Camera.view;
		m_cull_data.P00              = Renderer::instance()->Main_Camera.projection[0][0];
		m_cull_data.P11              = Renderer::instance()->Main_Camera.projection[1][1];
		m_cull_data.znear            = Renderer::instance()->Main_Camera.near_plane;
		m_cull_data.zfar             = Renderer::instance()->Main_Camera.far_plane;
		m_cull_data.draw_count       = Renderer::instance()->Meshlet_Count;
		m_cull_data.frustum_enable   = Renderer::instance()->Culling.frustum_culling;
		m_cull_data.backface_enable  = Renderer::instance()->Culling.backface_culling;
		m_cull_data.occlusion_enable = Renderer::instance()->Culling.occulsion_culling;
		m_cull_data.zbuffer_width    = Renderer::instance()->Last_Frame.hiz_buffer->getWidth();
		m_cull_data.zbuffer_height   = Renderer::instance()->Last_Frame.hiz_buffer->getHeight();
	}

	auto &cmd_buffer = state.command_buffer;

	vkCmdBindPipeline(cmd_buffer, state.pass.bind_point, state.pass.pipeline);

	for (auto &descriptor_set : state.pass.descriptor_sets)
	{
		vkCmdBindDescriptorSets(cmd_buffer, state.pass.bind_point, state.pass.pipeline_layout, descriptor_set.index(), 1, &descriptor_set.getDescriptorSet(), 0, nullptr);
	}

	vkCmdPushConstants(cmd_buffer, state.pass.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_cull_data), &m_cull_data);

	uint32_t group_count = (Renderer::instance()->Meshlet_Count + 1024 - 1) / 1024;
	vkCmdDispatch(cmd_buffer, group_count, 1, 1);
}
}        // namespace Ilum::pass