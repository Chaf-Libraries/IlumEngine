#include "SceneView.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"

#include "ImGui/ImGuiTool.hpp"
#include "ImGui/ImGuiContext.hpp"

#include <imgui.h>


namespace Ilum::panel
{
SceneView::SceneView()
{
	m_name = "SceneView";
}

void SceneView::draw()
{
	auto render_graph = Renderer::instance()->getRenderGraph();
	ImGui::Begin("SceneView");

	auto region = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
	onResize(VkExtent2D{static_cast<uint32_t>(region.x), static_cast<uint32_t>(region.y)});

	if (render_graph->hasAttachment(render_graph->view()))
	{
		ImGui::Image(ImGuiContext::textureID(render_graph->getAttachment(render_graph->view()), Renderer::instance()->getSampler(Renderer::SamplerType::Trilinear_Clamp)), region);
		LOG_TRACE("View: {}", render_graph->view());
	}

	ImGui::End();
}

void SceneView::onResize(VkExtent2D extent)
{
	if (extent.width != Renderer::instance()->getRenderTargetExtent().width ||
		extent.height != Renderer::instance()->getRenderTargetExtent().height)
	{
		Renderer::instance()->resizeRenderTarget(extent);
	}
}
}        // namespace Ilum::panel