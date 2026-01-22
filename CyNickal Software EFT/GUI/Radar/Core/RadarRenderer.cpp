#include "pch.h"
#include "GUI/Radar/Core/RadarRenderer.h"
#include "GUI/Radar/Radar2D.h"

void RadarRenderer::RenderRadar(ImDrawList* drawList,
                               IRadarMap* map,
                               const RadarSettings& settings,
                               const MapParams& params,
                               const ImVec2& canvasMin,
                               const ImVec2& canvasMax,
                               const Vector3& centerPos)
{
	if (!drawList || !map || !params.config)
		return;

	// FPS capping (optional - can be disabled by setting target to 0)
	if (m_targetFPS > 0 && !ShouldRenderFrame())
		return;

	// Render map layers if enabled
	if (settings.bShowMapImage)
	{
		bool enableDimming = !map->GetConfig().disableDimming;
		RenderMapLayers(drawList, map, centerPos.y, params, canvasMin, canvasMax, enableDimming);
	}
}

void RadarRenderer::RenderMapLayers(ImDrawList* drawList,
                                   IRadarMap* map,
                                   float playerHeight,
                                   const MapParams& params,
                                   const ImVec2& canvasMin,
                                   const ImVec2& canvasMax,
                                   bool enableDimming)
{
	if (!drawList || !map || !params.config)
		return;

	// Get visible layers for current height
	auto visibleLayers = map->GetVisibleLayers(playerHeight);
	if (visibleLayers.empty())
		return;

	// Determine front (topmost) layer for dimming
	const MapLayerTexture* frontLayer = visibleLayers.back();
	bool cannotDimLower = frontLayer->layer && frontLayer->layer->cannotDimLowerLayers;

	// Render all visible layers with dimming strategy (from Lum0s)
	for (size_t i = 0; i < visibleLayers.size(); i++)
	{
		const auto* layer = visibleLayers[i];
		if (!layer || !layer->textureView)
			continue;

		// Dimming logic:
		// - Front layer: Full opacity
		// - Lower layers: 50% dimmed (unless dimming disabled or cannotDimLowerLayers)
		bool isFrontLayer = (i == visibleLayers.size() - 1);
		bool shouldDim = enableDimming && !isFrontLayer && !cannotDimLower;

		ImU32 tintColor = shouldDim ? TINT_DIMMED : TINT_FRONT;

		RenderLayer(drawList, layer, params, canvasMin, canvasMax, tintColor);
	}
}

void RadarRenderer::RenderEntities(ImDrawList* drawList,
                                  const std::vector<IRadarEntity*>& entities,
                                  const MapParams& params,
                                  const ImVec2& canvasMin,
                                  const ImVec2& canvasMax,
                                  const Vector3& localPlayerPos)
{
	if (!drawList || !params.config)
		return;

	for (auto* entity : entities)
	{
		if (!entity || !entity->IsValid())
			continue;

		entity->Draw(drawList, params, canvasMin, canvasMax, localPlayerPos);
	}
}

bool RadarRenderer::ShouldRenderFrame()
{
	if (m_targetFPS <= 0)
		return true; // No FPS cap

	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFrameTime);

	// Calculate frame time for target FPS
	int frameTimeMs = 1000 / m_targetFPS;

	if (elapsed.count() < frameTimeMs)
		return false; // Skip frame

	m_lastFrameTime = now;
	return true;
}

void RadarRenderer::RenderLayer(ImDrawList* drawList,
                               const MapLayerTexture* layer,
                               const MapParams& params,
                               const ImVec2& canvasMin,
                               const ImVec2& canvasMax,
                               ImU32 tintColor)
{
	if (!layer || !layer->textureView || !params.config)
		return;

	// Calculate UV coordinates for the visible map bounds
	float mapWidth = layer->rawSvgWidth * params.config->svgScale;
	float mapHeight = layer->rawSvgHeight * params.config->svgScale;

	float uvLeft = params.boundsLeft / mapWidth;
	float uvTop = params.boundsTop / mapHeight;
	float uvRight = params.boundsRight / mapWidth;
	float uvBottom = params.boundsBottom / mapHeight;

	// Clamp UVs to valid range [0, 1]
	uvLeft = std::max(0.0f, std::min(1.0f, uvLeft));
	uvTop = std::max(0.0f, std::min(1.0f, uvTop));
	uvRight = std::max(0.0f, std::min(1.0f, uvRight));
	uvBottom = std::max(0.0f, std::min(1.0f, uvBottom));

	// Calculate screen-space draw area (with centering offsets)
	ImVec2 drawMin = ImVec2(canvasMin.x + params.xOffset, canvasMin.y + params.yOffset);
	ImVec2 drawMax = ImVec2(
		drawMin.x + params.GetBoundsWidth() * params.xScale,
		drawMin.y + params.GetBoundsHeight() * params.yScale
	);

	// Draw textured quad
	drawList->AddImage(
		layer->textureView,
		drawMin,
		drawMax,
		ImVec2(uvLeft, uvTop),
		ImVec2(uvRight, uvBottom),
		tintColor
	);
}
