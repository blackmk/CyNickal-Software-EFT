#pragma once
#include "GUI/Radar/Core/IRadarMap.h"
#include "GUI/Radar/EftMapConfig.h"
#include "GUI/Radar/Radar2D.h"  // For MapLayerTexture
#include <string>
#include <vector>

// Concrete implementation of IRadarMap for Tarkov maps
// Wraps the existing LoadedMap structure
//
// Purpose: Adapter between existing map loading code and new interface
// Benefits:
//   - Implements IRadarMap contract
//   - Maintains backward compatibility with existing structures
//   - Encapsulates layer management logic
class EftMap : public IRadarMap
{
public:
	EftMap(const std::string& id, const EftMapConfig& config);
	~EftMap() override;

	// IRadarMap interface
	const std::string& GetId() const override { return m_id; }
	const EftMapConfig& GetConfig() const override { return m_config; }

	std::vector<const MapLayerTexture*> GetVisibleLayers(float playerHeight) const override;
	const MapLayerTexture* GetBaseLayer() const override;

	float GetMapWidth() const override;
	float GetMapHeight() const override;

	bool IsLoaded() const override { return !m_layers.empty(); }
	void Unload() override;

	// Internal methods for loading (called by RadarMapManager)
	void AddLayer(MapLayerTexture&& layer) { m_layers.push_back(std::move(layer)); }
	std::vector<MapLayerTexture>& GetLayers() { return m_layers; }

private:
	std::string m_id;
	EftMapConfig m_config;
	std::vector<MapLayerTexture> m_layers;
};
