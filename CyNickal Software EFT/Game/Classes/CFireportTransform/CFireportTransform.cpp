#include "pch.h"
#include "CFireportTransform.h"
#include "Game/EFT.h"
#include "Game/Offsets/Offsets.h"

CFireportTransform::CFireportTransform(uintptr_t transformInternal)
	: m_TransformAddress(transformInternal)
{
	if (!m_TransformAddress)
		return;

	auto Conn = DMA_Connection::GetInstance();
	auto PID = EFT::GetProcess().GetPID();
	
	// Step 1: Read hierarchy address and index (synchronously)
	VMMDLL_MemReadEx(Conn->GetHandle(), PID,
		m_TransformAddress + Offsets::CUnityTransform::pTransformHierarchy,
		reinterpret_cast<PBYTE>(&m_HierarchyAddress), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
	
	VMMDLL_MemReadEx(Conn->GetHandle(), PID,
		m_TransformAddress + Offsets::CUnityTransform::Index,
		reinterpret_cast<PBYTE>(&m_Index), sizeof(int32_t), nullptr, VMMDLL_FLAG_NOCACHE);
	
	// Validate (same sanity checks as reference: index < 128000)
	if (!m_HierarchyAddress || m_Index < 0 || m_Index > 128000)
		return;
	
	// Step 2: Read indices and vertices addresses from hierarchy
	VMMDLL_MemReadEx(Conn->GetHandle(), PID,
		m_HierarchyAddress + Offsets::CTransformHierarchy::pIndices,
		reinterpret_cast<PBYTE>(&m_IndicesAddress), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
	
	VMMDLL_MemReadEx(Conn->GetHandle(), PID,
		m_HierarchyAddress + Offsets::CTransformHierarchy::pVertices,
		reinterpret_cast<PBYTE>(&m_VerticesAddress), sizeof(uintptr_t), nullptr, VMMDLL_FLAG_NOCACHE);
	
	if (!m_IndicesAddress || !m_VerticesAddress)
		return;
	
	// Step 3: Read indices array ONCE (indices never change for a transform)
	m_Indices.resize(static_cast<size_t>(m_Index) + 1);
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, m_IndicesAddress,
		reinterpret_cast<PBYTE>(m_Indices.data()), sizeof(int32_t) * m_Indices.size(),
		nullptr, VMMDLL_FLAG_NOCACHE);
	
	// Validate indices aren't garbage
	if (m_Indices.empty())
		return;
	
	// Check the parent index is reasonable
	int32_t parentIdx = m_Indices[m_Index];
	if (parentIdx > 128000) // Unreasonable value, likely garbage
		return;
	
	// Step 4: Initial vertices read
	if (!UpdateVertices())
		return;
	
	m_bValid = true;
}

bool CFireportTransform::UpdateVertices()
{
	if (!m_VerticesAddress || m_Indices.empty() || m_Index < 0)
		return false;
	
	auto Conn = DMA_Connection::GetInstance();
	auto PID = EFT::GetProcess().GetPID();
	
	// Determine required size - need to walk the hierarchy to find max index
	int32_t maxIndex = m_Index;
	int32_t currentIdx = m_Index;
	int iterations = 0;
	
	while (currentIdx >= 0 && currentIdx < static_cast<int32_t>(m_Indices.size()) && iterations < 128)
	{
		if (currentIdx > maxIndex)
			maxIndex = currentIdx;
		
		int32_t parentIdx = m_Indices[currentIdx];
		if (parentIdx < 0 || parentIdx >= static_cast<int32_t>(m_Indices.size()))
			break;
		
		if (parentIdx > maxIndex)
			maxIndex = parentIdx;
		
		currentIdx = parentIdx;
		iterations++;
	}
	
	size_t requiredSize = static_cast<size_t>(maxIndex) + 1;
	if (m_Vertices.size() < requiredSize)
		m_Vertices.resize(requiredSize);
	
	DWORD bytesRead = 0;
	VMMDLL_MemReadEx(Conn->GetHandle(), PID, m_VerticesAddress,
		reinterpret_cast<PBYTE>(m_Vertices.data()), sizeof(VertexEntry) * requiredSize,
		&bytesRead, VMMDLL_FLAG_NOCACHE);
	
	return bytesRead == sizeof(VertexEntry) * requiredSize;
}

// Helper to extract quaternion from VertexEntry
static Quaternion ExtractQuaternion(const VertexEntry& v)
{
	float qx = _mm_cvtss_f32(_mm_castsi128_ps(v.Quaternion));
	float qy = _mm_cvtss_f32(_mm_castsi128_ps(_mm_shuffle_epi32(v.Quaternion, 0x55)));
	float qz = _mm_cvtss_f32(_mm_castsi128_ps(_mm_shuffle_epi32(v.Quaternion, 0xAA)));
	float qw = _mm_cvtss_f32(_mm_castsi128_ps(_mm_shuffle_epi32(v.Quaternion, 0xFF)));
	return Quaternion{ qx, qy, qz, qw };
}

// Helper to extract position from VertexEntry
static Vector3 ExtractPosition(const VertexEntry& v)
{
	return *reinterpret_cast<const Vector3*>(&v.Translation);
}

// Helper to extract scale from VertexEntry
static Vector3 ExtractScale(const VertexEntry& v)
{
	return *reinterpret_cast<const Vector3*>(&v.Scale);
}

/* THANK YOU https://www.unknowncheats.me/forum/4527852-post13624.html */
Vector3 CFireportTransform::GetPosition() const
{
	if (!m_bValid || m_Vertices.empty() || m_Indices.empty())
		return Vector3{ 0, 0, 0 };
	
	if (m_Index < 0 || m_Index >= static_cast<int32_t>(m_Vertices.size()))
		return Vector3{ 0, 0, 0 };

	static constexpr __m128 MulVec1 = { -2.f, 2.f, -2.f, 0.f };
	static constexpr __m128 MulVec2 = { 2.f, -2.f, -2.f, 0.f };
	static constexpr __m128 MulVec3 = { -2.f, -2.f, 2.f, 0.f };

	__m128 Intermediate = m_Vertices[m_Index].Translation;
	int32_t DependencyIndex = m_Indices[m_Index];
	static constexpr size_t MaxIterations = 128;
	size_t IterationCount = 0;

	while (DependencyIndex >= 0)
	{
		if (static_cast<size_t>(DependencyIndex) >= m_Vertices.size() || static_cast<size_t>(DependencyIndex) >= m_Indices.size())
			break;

		IterationCount++;
		if (IterationCount >= MaxIterations)
			break;

		VertexEntry m = m_Vertices[DependencyIndex];

		__m128 v10 = _mm_mul_ps(m.Scale, Intermediate);
		__m128 v11 = _mm_castsi128_ps(_mm_shuffle_epi32(m.Quaternion, 0));
		__m128 v12 = _mm_castsi128_ps(_mm_shuffle_epi32(m.Quaternion, 85));
		__m128 v13 = _mm_castsi128_ps(_mm_shuffle_epi32(m.Quaternion, -114));
		__m128 v14 = _mm_castsi128_ps(_mm_shuffle_epi32(m.Quaternion, -37));
		__m128 v15 = _mm_castsi128_ps(_mm_shuffle_epi32(m.Quaternion, -86));
		__m128 v16 = _mm_castsi128_ps(_mm_shuffle_epi32(m.Quaternion, 113));
		__m128 v17 = _mm_add_ps(
			_mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(
						_mm_sub_ps(
							_mm_mul_ps(_mm_mul_ps(v11, MulVec2), v13),
							_mm_mul_ps(_mm_mul_ps(v12, MulVec3), v14)),
						_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v10), -86))),
					_mm_mul_ps(
						_mm_sub_ps(
							_mm_mul_ps(_mm_mul_ps(v15, MulVec3), v14),
							_mm_mul_ps(_mm_mul_ps(v11, MulVec1), v16)),
						_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v10), 85)))),
				_mm_add_ps(
					_mm_mul_ps(
						_mm_sub_ps(
							_mm_mul_ps(_mm_mul_ps(v12, MulVec1), v16),
							_mm_mul_ps(_mm_mul_ps(v15, MulVec2), v13)),
						_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v10), 0))),
					v10)),
			m.Translation);

		Intermediate = v17;

		DependencyIndex = m_Indices[DependencyIndex];
	}

	return *reinterpret_cast<Vector3*>(&Intermediate);
}

Quaternion CFireportTransform::GetRotation() const
{
	if (!m_bValid || m_Vertices.empty() || m_Indices.empty())
		return Quaternion::Identity();
	
	if (m_Index < 0 || m_Index >= static_cast<int32_t>(m_Vertices.size()))
		return Quaternion::Identity();
	
	// Start with local rotation
	Quaternion worldRot = ExtractQuaternion(m_Vertices[m_Index]);
	int32_t index = m_Indices[m_Index];
	int iterations = 0;
	
	// Walk up the hierarchy combining rotations: parent * child
	while (index >= 0 && index < static_cast<int32_t>(m_Vertices.size()) && iterations < 128)
	{
		Quaternion parentRot = ExtractQuaternion(m_Vertices[index]);
		worldRot = parentRot * worldRot;
		
		if (index >= static_cast<int32_t>(m_Indices.size()))
			break;
		
		index = m_Indices[index];
		iterations++;
	}
	
	return worldRot;
}
