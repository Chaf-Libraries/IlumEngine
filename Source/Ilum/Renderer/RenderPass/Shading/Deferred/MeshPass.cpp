#include "MeshPass.hpp"

#include "Renderer/Renderer.hpp"

namespace Ilum::pass
{
void MeshPass::setupPipeline(PipelineState &state)
{
	state.shader.load(std::string(PROJECT_SOURCE_DIR) + "Source/Shaders/Shading/Deferred/Mesh.hlsl", VK_SHADER_STAGE_TASK_BIT_NV, Shader::Type::HLSL, "ASmain");
	state.shader.load(std::string(PROJECT_SOURCE_DIR) + "Source/Shaders/Shading/Deferred/Mesh.hlsl", VK_SHADER_STAGE_MESH_BIT_NV, Shader::Type::HLSL, "MSmain");
	state.shader.load(std::string(PROJECT_SOURCE_DIR) + "Source/Shaders/Shading/Deferred/Mesh.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT, Shader::Type::HLSL, "PSmain");

	state.dynamic_state.dynamic_states = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	state.color_blend_attachment_states.resize(1);
	state.depth_stencil_state.stencil_test_enable = false;

	// Disable blending
	for (auto &color_blend_attachment_state : state.color_blend_attachment_states)
	{
		color_blend_attachment_state.blend_enable = false;
	}

	state.rasterization_state.polygon_mode = VK_POLYGON_MODE_FILL;

	/*StructuredBuffer<Instance> instances : register(t0);
StructuredBuffer<Meshlet> meshlets : register(t1);
ConstantBuffer<Camera> camera : register(b2);
StructuredBuffer<Vertex> vertices : register(t3);
StructuredBuffer<uint> indices : register(t4);
ConstantBuffer<CullingInfo> culling_info : register(b5);*/

	 state.descriptor_bindings.bind(0, 0, "PerInstanceBuffer", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 1, "PerMeshletBuffer", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 2, "Camera", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	state.descriptor_bindings.bind(0, 3, "Vertices", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 4, "Indices", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	state.descriptor_bindings.bind(0, 5, "CullingBuffer", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	// GBuffer 0: RGB - Albedo, A - Anisotropic
	// GBuffer 1: RGB - Normal, A - LinearDepth
	// GBuffer 2: R - Metallic, G - Roughness, B - Subsurface, A - EntityID
	// GBuffer 3: R - Sheen, G - Sheen Tint, B - Clearcoat, A - Clearcoat Gloss
	// GBuffer 4: RG - Velocity, B - Specular, A - Specular Tint
	// GBuffer 5: RGB - Emissive, A - Material Type

	state.declareAttachment("GBuffer0", VK_FORMAT_R16G16B16A16_SFLOAT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);
	// state.declareAttachment("GBuffer1", VK_FORMAT_R16G16B16A16_SFLOAT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);
	// state.declareAttachment("GBuffer2", VK_FORMAT_R16G16_SFLOAT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);
	// state.declareAttachment("GBuffer3", VK_FORMAT_R16G16B16A16_SFLOAT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);
	// state.declareAttachment("GBuffer4", VK_FORMAT_R16G16B16A16_SFLOAT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);
	// state.declareAttachment("GBuffer5", VK_FORMAT_R16G16B16A16_SFLOAT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);
	state.declareAttachment("DepthStencil", VK_FORMAT_D32_SFLOAT_S8_UINT, Renderer::instance()->getRenderTargetExtent().width, Renderer::instance()->getRenderTargetExtent().height);

	VkClearColorValue clear_color = {};
	clear_color.uint32[3]         = static_cast<uint32_t>(entt::null);
	clear_color.float32[3]        = std::numeric_limits<float>::max();

	state.addOutputAttachment("GBuffer0", AttachmentState::Clear_Color);
	// state.addOutputAttachment("GBuffer1", clear_color);
	// state.addOutputAttachment("GBuffer2", clear_color);
	// state.addOutputAttachment("GBuffer3", AttachmentState::Clear_Color);
	// state.addOutputAttachment("GBuffer4", AttachmentState::Clear_Color);
	// state.addOutputAttachment("GBuffer5", AttachmentState::Clear_Color);

	state.addOutputAttachment("DepthStencil", VkClearDepthStencilValue{1.f, 0u});
}

void MeshPass::resolveResources(ResolveState &resolve)
{
	 resolve.resolve("PerInstanceBuffer", Renderer::instance()->Render_Buffer.Instance_Buffer);
	resolve.resolve("PerMeshletBuffer", Renderer::instance()->Render_Buffer.Meshlet_Buffer);
	resolve.resolve("Camera", Renderer::instance()->Render_Buffer.Camera_Buffer);
	resolve.resolve("Vertices", Renderer::instance()->Render_Buffer.Static_Vertex_Buffer);
	resolve.resolve("Indices", Renderer::instance()->Render_Buffer.Static_Index_Buffer);
	resolve.resolve("CullingBuffer", Renderer::instance()->Render_Buffer.Culling_Buffer);
}

void MeshPass::render(RenderPassState &state)
{
	auto &cmd_buffer = state.command_buffer;

	VkRenderPassBeginInfo begin_info = {};
	begin_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.renderPass            = state.pass.render_pass;
	begin_info.renderArea            = state.pass.render_area;
	begin_info.framebuffer           = state.pass.frame_buffer;
	begin_info.clearValueCount       = static_cast<uint32_t>(state.pass.clear_values.size());
	begin_info.pClearValues          = state.pass.clear_values.data();

	vkCmdBeginRenderPass(cmd_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd_buffer, state.pass.bind_point, state.pass.pipeline);

	auto &extent = Renderer::instance()->getRenderTargetExtent();

	VkViewport viewport = {0, static_cast<float>(extent.height), static_cast<float>(extent.width), -static_cast<float>(extent.height), 0, 1};
	VkRect2D   scissor  = {0, 0, extent.width, extent.height};

	vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
	vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

	for (auto &descriptor_set : state.pass.descriptor_sets)
	{
		vkCmdBindDescriptorSets(cmd_buffer, state.pass.bind_point, state.pass.pipeline_layout, descriptor_set.index(), 1, &descriptor_set.getDescriptorSet(), 0, nullptr);
	}

	// vkCmdPushConstants(cmd_buffer, state.pass.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &m_frustum_culling_enable);
	// auto &cmd = Renderer::instance()->Render_Buffer.Command_Buffer;
	vkCmdDrawMeshTasksNV(cmd_buffer, (Renderer::instance()->Render_Stats.static_mesh_count.meshlet_count + 32 - 1) / 32, 0);

	vkCmdEndRenderPass(cmd_buffer);
}

void MeshPass::onImGui()
{
}
}        // namespace Ilum::pass