#include "pch.h"
#include "GUI/Radar/Maps/EftMap.h"
#include <algorithm>
#include <limits>

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

	// Sort layers: base layers first, then by min height (low to high)
	// This ensures consistent z-ordering and prevents flickering
	// Using stable_sort with pointer tiebreaker for deterministic ordering
	std::stable_sort(visible.begin(), visible.end(), [](const MapLayerTexture* a, const MapLayerTexture* b) {
		// Base layers (no height restrictions) always go first
		bool aBase = !a->layer || (!a->layer->minHeight.has_value() && !a->layer->maxHeight.has_value());
		bool bBase = !b->layer || (!b->layer->minHeight.has_value() && !b->layer->maxHeight.has_value());

		if (aBase && !bBase) return true;  // a is base, b is not -> a first
		if (!aBase && bBase) return false; // b is base, a is not -> b first

		// Both base or both non-base: compare by min height
		if (!a->layer || !b->layer)
			return a < b;  // Pointer tiebreaker for null layers

		float aHeight = a->layer->minHeight.value_or(std::numeric_limits<float>::lowest());
		float bHeight = b->layer->minHeight.value_or(std::numeric_limits<float>::lowest());

		if (aHeight != bHeight)
			return aHeight < bHeight; // Lower layers first

		// Tiebreaker for identical heights - ensures deterministic ordering
		return a < b;
	});

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
