#include "GeometryUpdate.hpp"

#include "Renderer/Renderer.hpp"

#include "Graphics/Command/CommandBuffer.hpp"
#include "Graphics/GraphicsContext.hpp"
#include "Graphics/Profiler.hpp"

#include "Scene/Component/Renderable.hpp"
#include "Scene/Scene.hpp"

#include "tbb/tbb.h"

namespace Ilum::sym
{
void GeometryUpdate::run()
{
	GraphicsContext::instance()->getProfiler().beginSample("Geometry Update");
	auto &resource_cache       = Renderer::instance()->getResourceCache();

	// Update static mesh only when it needed
	auto &static_vertex_buffer = Renderer::instance()->Render_Buffer.Static_Vertex_Buffer;
	auto &static_index_buffer  = Renderer::instance()->Render_Buffer.Static_Index_Buffer;

	if (resource_cache.getVerticesCount() * sizeof(Vertex) != static_vertex_buffer.getSize() ||
	    resource_cache.getIndicesCount() * sizeof(uint32_t) != static_index_buffer.getSize())
	{
		cmpt::Renderable::update = true;

		GraphicsContext::instance()->getQueueSystem().waitAll();

		// Resize buffer
		static_vertex_buffer = Buffer(resource_cache.getVerticesCount() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		static_index_buffer  = Buffer(resource_cache.getIndicesCount() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		if (static_vertex_buffer.getSize() == 0 || static_index_buffer.getSize() == 0)
		{
			return;
		}

		// Staging buffer
		Buffer staging_vertex_buffer(static_vertex_buffer.getSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Buffer staging_index_buffer(static_index_buffer.getSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		auto * vertex_data = staging_vertex_buffer.map();
		auto * index_data  = staging_index_buffer.map();

		// CPU -> Staging
		for (auto &[name, index] : resource_cache.getModels())
		{
			auto &model = resource_cache.loadModel(name);

			std::memcpy(vertex_data + model.get().vertices_offset * sizeof(Vertex), model.get().mesh.vertices.data(), sizeof(Ilum::Vertex) * model.get().vertices_count);
			std::memcpy(index_data + model.get().indices_offset * sizeof(uint32_t), model.get().mesh.indices.data(), sizeof(uint32_t) * model.get().indices_count);
		}

		staging_vertex_buffer.unmap();
		staging_index_buffer.unmap();

		// Staging -> GPU
		CommandBuffer command_buffer(QueueUsage::Transfer);
		command_buffer.begin();
		command_buffer.copyBuffer(BufferInfo{staging_vertex_buffer}, BufferInfo{static_vertex_buffer}, static_vertex_buffer.getSize());
		command_buffer.copyBuffer(BufferInfo{staging_index_buffer}, BufferInfo{static_index_buffer}, static_index_buffer.getSize());
		command_buffer.end();
		command_buffer.submitIdle();
	}

	// Update dynamic mesh, update every frame
	auto view = Scene::instance()->getRegistry().view<cmpt::MeshRenderer>();
	tbb::parallel_for_each(view.begin(), view.end(), [](entt::entity entity) {
		auto &mesh_renderer = Entity(entity).getComponent<cmpt::MeshRenderer>();
		if (mesh_renderer.vertices.size() * sizeof(Vertex) != mesh_renderer.vertex_buffer.getSize())
		{
			GraphicsContext::instance()->getQueueSystem().waitAll();
			mesh_renderer.vertex_buffer = Buffer(mesh_renderer.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		}

		if (mesh_renderer.indices.size() * sizeof(Vertex) != mesh_renderer.index_buffer.getSize())
		{
			GraphicsContext::instance()->getQueueSystem().waitAll();
			mesh_renderer.index_buffer = Buffer(mesh_renderer.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		}

		if (mesh_renderer.vertices.size() == 0 || mesh_renderer.indices.size() == 0)
		{
			return;
		}

		std::memcpy(mesh_renderer.vertex_buffer.map(), mesh_renderer.vertices.data(), mesh_renderer.vertex_buffer.getSize());
		std::memcpy(mesh_renderer.index_buffer.map(), mesh_renderer.indices.data(), mesh_renderer.index_buffer.getSize());

		mesh_renderer.vertex_buffer.unmap();
		mesh_renderer.index_buffer.unmap();
	});

	GraphicsContext::instance()->getProfiler().endSample("Geometry Update");
}
}        // namespace Ilum::sym