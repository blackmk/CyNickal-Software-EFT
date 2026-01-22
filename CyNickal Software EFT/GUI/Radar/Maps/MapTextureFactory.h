#pragma once
#include "GUI/Radar/Radar2D.h"  // For MapLayerTexture
#include "GUI/Radar/EftMapConfig.h"
#include <d3d11.h>
#include <string>

// Factory for creating D3D11 textures from SVG map files
//
// Purpose: Encapsulate texture creation logic
// Benefits:
//   - Separates rasterization from map management
//   - Reusable for different texture creation scenarios
//   - Testable independently
//   - Contains all the NanoSVG/D3D11 complexity
class MapTextureFactory
{
public:
	explicit MapTextureFactory(ID3D11Device* device)
		: m_device(device)
	{
	}

	// Create a texture from an SVG file
	// Returns empty MapLayerTexture on failure (textureView == nullptr)
	//
	// Parameters:
	//   svgPath - Path to SVG file
	//   layerConfig - Layer configuration (for height range, etc.)
	//   rasterScale - Scale factor for rasterization (4.0 for quality)
	//
	// Processing steps:
	//   1. Read and parse SVG with NanoSVG
	//   2. Rasterize at specified scale
	//   3. Apply visibility corrections (brighten, alpha boost)
	//   4. Background removal for overlay layers
	//   5. Create D3D11 texture and shader resource view
	MapLayerTexture CreateFromSVG(const std::string& svgPath,
	                              const EftMapLayer* layerConfig,
	                              float rasterScale = 4.0f);

private:
	// Flatten SVG styles for NanoSVG compatibility
	// Converts CSS styles to inline attributes
	std::string FlattenSvgStyles(const std::string& svgContent);

	// Apply visibility corrections to rasterized pixels
	// - Brighten dark maps (+60 brightness boost)
	// - Alpha enhancement for semi-transparent pixels
	// - Background removal for overlay layers (chroma key)
	void ApplyVisibilityCorrections(std::vector<unsigned char>& pixels,
	                               int width, int height,
	                               bool isBaseLayer);

	ID3D11Device* m_device = nullptr;
};
