#pragma once
#include "Game/Classes/Vector.h"
#include "Game/Classes/CUnityTransform/CUnityTransform.h"
#include <vector>

// Standalone fireport transform class that reads all data synchronously
// This matches exactly how the reference implementation works
class CFireportTransform
{
private:
	uintptr_t m_TransformAddress{ 0 };
	uintptr_t m_HierarchyAddress{ 0 };
	uintptr_t m_IndicesAddress{ 0 };
	uintptr_t m_VerticesAddress{ 0 };
	int32_t m_Index{ 0 };
	
	std::vector<int32_t> m_Indices{};     // Read once in constructor (never changes)
	std::vector<VertexEntry> m_Vertices{};
	
	bool m_bValid{ false };

public:
	CFireportTransform(uintptr_t transformInternal);
	
	bool IsValid() const { return m_bValid; }
	
	// Update vertices (call each frame before GetPosition/GetRotation)
	bool UpdateVertices();
	
	// Get world position
	Vector3 GetPosition() const;
	
	// Get world rotation
	Quaternion GetRotation() const;
};
