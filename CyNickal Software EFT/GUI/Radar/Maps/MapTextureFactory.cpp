#include "pch.h"
#include "GUI/Radar/Maps/MapTextureFactory.h"
#include <fstream>
#include <regex>
#include <algorithm>

// NanoSVG includes
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "../../../Dependencies/nanosvg/nanosvg.h"
#include "../../../Dependencies/nanosvg/nanosvgrast.h"

MapLayerTexture MapTextureFactory::CreateFromSVG(const std::string& svgPath,
                                                 const EftMapLayer* layerConfig,
                                                 float rasterScale)
{
	MapLayerTexture result = {};

	// Read SVG content
	std::ifstream svgFile(svgPath);
	if (!svgFile.is_open())
	{
		printf("[MapTextureFactory] Failed to open SVG: %s\n", svgPath.c_str());
		return result;
	}

	std::string svgContent((std::istreambuf_iterator<char>(svgFile)), std::istreambuf_iterator<char>());
	svgFile.close();

	// Flatten styles for NanoSVG
	std::string flattenedSvg = FlattenSvgStyles(svgContent);

	// Parse SVG
	NSVGimage* svgImage = nsvgParse((char*)flattenedSvg.c_str(), "px", 96.0f);
	if (!svgImage)
	{
		printf("[MapTextureFactory] Failed to parse SVG: %s\n", svgPath.c_str());
		return result;
	}

	// Create rasterizer
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	if (!rast)
	{
		printf("[MapTextureFactory] Failed to create NanoSVG rasterizer\n");
		nsvgDelete(svgImage);
		return result;
	}

	// Rasterize at specified scale (default 4x for quality)
	int width = (int)(svgImage->width * rasterScale);
	int height = (int)(svgImage->height * rasterScale);

	// Limit max size
	if (width > 8192) width = 8192;
	if (height > 8192) height = 8192;

	std::vector<unsigned char> pixels(width * height * 4, 0);
	nsvgRasterize(rast, svgImage, 0, 0, rasterScale, pixels.data(), width, height, width * 4);

	unsigned char sampledBgR = pixels[0], sampledBgG = pixels[1], sampledBgB = pixels[2], sampledBgA = pixels[3];
	printf("[MapTextureFactory] Loaded SVG: %s (Size: %.2fx%.2f) - BG Color: %02x%02x%02x%02x\n",
	       svgPath.c_str(), svgImage->width, svgImage->height, sampledBgR, sampledBgG, sampledBgB, sampledBgA);

	// Apply visibility corrections
	bool isBaseLayer = layerConfig && !layerConfig->minHeight.has_value() && !layerConfig->maxHeight.has_value();
	ApplyVisibilityCorrections(pixels, width, height, isBaseLayer);

	// Create D3D11 texture
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA subRes = {};
	subRes.pSysMem = pixels.data();
	subRes.SysMemPitch = width * 4;

	ID3D11Texture2D* texture = nullptr;
	HRESULT hr = m_device->CreateTexture2D(&texDesc, &subRes, &texture);
	if (FAILED(hr))
	{
		printf("[MapTextureFactory] CreateTexture2D failed: 0x%08X\n", hr);
		nsvgDeleteRasterizer(rast);
		nsvgDelete(svgImage);
		return result;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* srv = nullptr;
	hr = m_device->CreateShaderResourceView(texture, &srvDesc, &srv);
	texture->Release();

	if (FAILED(hr))
	{
		printf("[MapTextureFactory] CreateShaderResourceView failed: 0x%08X\n", hr);
		nsvgDeleteRasterizer(rast);
		nsvgDelete(svgImage);
		return result;
	}

	// Populate result
	result.textureView = srv;
	result.width = width;
	result.height = height;
	result.rawSvgWidth = svgImage->width;
	result.rawSvgHeight = svgImage->height;
	result.layer = layerConfig;

	printf("[MapTextureFactory] Created texture: %dx%d\n", width, height);

	nsvgDeleteRasterizer(rast);
	nsvgDelete(svgImage);

	return result;
}

std::string MapTextureFactory::FlattenSvgStyles(const std::string& svg)
{
	std::string result = svg;

	// Find style block
	std::regex styleRegex("<style[^>]*>([\\s\\S]*?)</style>");
	std::smatch styleMatch;

	if (std::regex_search(svg, styleMatch, styleRegex))
	{
		std::string css = styleMatch[1].str();

		// Map of class name -> styles (e.g. "st0" -> "fill:#123;stroke:#000;")
		std::unordered_map<std::string, std::string> classStyles;

		// Regex to find class rules: .name { styles }
		std::regex ruleRegex("\\.([a-zA-Z0-9_-]+)\\s*\\{([^}]*)\\}");
		auto rulesBegin = std::sregex_iterator(css.begin(), css.end(), ruleRegex);
		auto rulesEnd = std::sregex_iterator();

		for (std::sregex_iterator i = rulesBegin; i != rulesEnd; ++i)
		{
			std::string className = (*i)[1].str();
			std::string styles = (*i)[2].str();

			// Clean up styles (remove newlines and extra spaces)
			styles.erase(std::remove(styles.begin(), styles.end(), '\n'), styles.end());
			styles.erase(std::remove(styles.begin(), styles.end(), '\r'), styles.end());

			classStyles[className] = styles;
		}

		// Now find all class="stX" in the SVG and replace with style="..." or direct attributes
		for (const auto& [className, styles] : classStyles)
		{
			std::string classAttr = "class=\"" + className + "\"";
			std::string styleAttr = "";

			// Simple parser for properties
			std::regex propRegex("([a-z-]+)\\s*:\\s*([^;]+);?");
			auto propsBegin = std::sregex_iterator(styles.begin(), styles.end(), propRegex);
			auto propsEnd = std::sregex_iterator();

			for (std::sregex_iterator j = propsBegin; j != propsEnd; ++j)
			{
				std::string key = (*j)[1].str();
				std::string val = (*j)[2].str();
				// Trim val
				val.erase(0, val.find_first_not_of(" \t"));
				val.erase(val.find_last_not_of(" \t") + 1);

				styleAttr += key + "=\"" + val + "\" ";
			}

			// Replace class="stX" with the attributes
			size_t pos = 0;
			while ((pos = result.find(classAttr, pos)) != std::string::npos)
			{
				result.replace(pos, classAttr.length(), styleAttr);
				pos += styleAttr.length();
			}
		}
	}

	return result;
}

void MapTextureFactory::ApplyVisibilityCorrections(std::vector<unsigned char>& pixels,
                                                   int width, int height,
                                                   bool isBaseLayer)
{
	// Sample background color from corner pixel
	unsigned char sampledBgR = pixels[0], sampledBgG = pixels[1], sampledBgB = pixels[2], sampledBgA = pixels[3];

	// Background removal (Chroma Key) for overlay layers
	// Overlay layers (non-base) often have a solid background color that hides the foundation
	bool doBackgroundRemoval = !isBaseLayer && (sampledBgA > 0);

	for (size_t p = 0; p < pixels.size(); p += 4)
	{
		if (pixels[p + 3] > 0) // Only process non-transparent pixels
		{
			// 1. Background Removal: If overlay, check if it matches the corner/bg color
			if (doBackgroundRemoval)
			{
				if (pixels[p] == sampledBgR && pixels[p + 1] == sampledBgG && pixels[p + 2] == sampledBgB)
				{
					pixels[p + 3] = 0; // Make it transparent
					continue;
				}
			}

			// 2. Brightness Boost: Boost RGB by 1.5x + higher fixed offset
			// Using +60 instead of +40 for better visibility of dark base maps
			for (int c = 0; c < 3; c++)
			{
				int val = (int)(pixels[p + c] * 1.5f) + 60;
				pixels[p + c] = (val < 0) ? 0 : (val > 255 ? 255 : (unsigned char)val);
			}

			// Ensure alpha is boosted too if it's low
			if (pixels[p + 3] < 200)
			{
				int aVal = (int)(pixels[p + 3] * 1.2f);
				pixels[p + 3] = (aVal > 255) ? 255 : (unsigned char)aVal;
			}
		}
	}
}
