#include "pch.h"
#include "GUI/Radar/Maps/EftMap.h"

EftMap::EftMap(const std::string& id, const EftMapConfig& config)
	: m_id(id), m_config(config)
{
}

EftMap::~EftMap()
{
	Unload();
}

std::vector<const MapLayerTexture*> EftMap::GetVisibleLayers(float playerHeight) const
{
	std::vector<const MapLayerTexture*> visible;

	for (const auto& layer : m_layers)
	{
		// Base layers (terrain/foundation) are always included
		if (!layer.layer || (!layer.layer->minHeight.has_value() && !layer.layer->maxHeight.has_value()))
		{
			visible.push_back(&layer);
			continue;
		}

		// Non-base layers must be in range
		if (layer.layer->IsHeightInRange(playerHeight))
		{
			visible.push_back(&layer);
		}
	}

	return visible;
}

const MapLayerTexture* EftMap::GetBaseLayer() const
{
	if (m_layers.empty())
		return nullptr;
	return &m_layers[0];
}

float EftMap::GetMapWidth() const
{
	const auto* baseLayer = GetBaseLayer();
	if (!baseLayer)
		return 0.0f;

	return baseLayer->rawSvgWidth * m_config.svgScale;
}

float EftMap::GetMapHeight() const
{
	const auto* baseLayer = GetBaseLayer();
	if (!baseLayer)
		return 0.0f;

	return baseLayer->rawSvgHeight * m_config.svgScale;
}

void EftMap::Unload()
{
	// Release D3D11 resources
	for (auto& layer : m_layers)
	{
		if (layer.textureView)
		{
			layer.textureView->Release();
			layer.textureView = nullptr;
		}
	}
	m_layers.clear();
}
