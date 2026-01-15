#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>
#include "Game/Classes/Vector.h"

// Structs ported from C#
struct BallisticsInfo
{
	float BulletSpeed{ 0.f };           // Muzzle Velocity
	float BulletMassGrams{ 0.f };
	float BulletDiameterMillimeters{ 0.f };
	float BallisticCoefficient{ 0.f };

	bool IsAmmoValid() const
	{
		return BulletMassGrams > 0.f && BulletMassGrams < 2000.f &&
			BulletSpeed > 1.f && BulletSpeed < 2500.f &&
			BallisticCoefficient >= 0.f && BallisticCoefficient <= 3.f &&
			BulletDiameterMillimeters > 0.f && BulletDiameterMillimeters <= 100.f;
	}
};

struct BallisticSimulationOutput
{
	float BulletDrop{ 0.f };
	float TravelTime{ 0.f };
};

struct G1DragModel
{
	float Mach;
	float Ballist;
};

class G1
{
private:
	static constexpr G1DragModel _g1Coeffs[] = {
		{0.f, 0.2629f}, {0.05f, 0.2558f}, {0.1f, 0.2487f}, {0.15f, 0.2413f},
		{0.2f, 0.2344f}, {0.25f, 0.2278f}, {0.3f, 0.2214f}, {0.35f, 0.2155f},
		{0.4f, 0.2104f}, {0.45f, 0.2061f}, {0.5f, 0.2032f}, {0.55f, 0.202f},
		{0.6f, 0.2034f}, {0.7f, 0.2165f}, {0.725f, 0.223f}, {0.75f, 0.2313f},
		{0.775f, 0.2417f}, {0.8f, 0.2546f}, {0.825f, 0.2706f}, {0.85f, 0.2901f},
		{0.875f, 0.3136f}, {0.9f, 0.3415f}, {0.925f, 0.3734f}, {0.95f, 0.4084f},
		{0.975f, 0.4448f}, {1.f, 0.4805f}, {1.025f, 0.5136f}, {1.05f, 0.5427f},
		{1.075f, 0.5677f}, {1.1f, 0.5883f}, {1.125f, 0.6053f}, {1.15f, 0.6191f},
		{1.2f, 0.6393f}, {1.25f, 0.6518f}, {1.3f, 0.6589f}, {1.35f, 0.6621f},
		{1.4f, 0.6625f}, {1.45f, 0.6607f}, {1.5f, 0.6573f}, {1.55f, 0.6528f},
		{1.6f, 0.6474f}, {1.65f, 0.6413f}, {1.7f, 0.6347f}, {1.75f, 0.628f},
		{1.8f, 0.621f}, {1.85f, 0.6141f}, {1.9f, 0.6072f}, {1.95f, 0.6003f},
		{2.f, 0.5934f}, {2.05f, 0.5867f}, {2.1f, 0.5804f}, {2.15f, 0.5743f},
		{2.2f, 0.5685f}, {2.25f, 0.563f}, {2.3f, 0.5577f}, {2.35f, 0.5527f},
		{2.4f, 0.5481f}, {2.45f, 0.5438f}, {2.5f, 0.5397f}, {2.6f, 0.5325f},
		{2.7f, 0.5264f}, {2.8f, 0.5211f}, {2.9f, 0.5168f}, {3.f, 0.5133f},
		{3.1f, 0.5105f}, {3.2f, 0.5084f}, {3.3f, 0.5067f}, {3.4f, 0.5054f},
		{3.5f, 0.504f}, {3.6f, 0.503f}, {3.7f, 0.5022f}, {3.8f, 0.5016f},
		{3.9f, 0.501f}, {4.f, 0.5006f}, {4.2f, 0.4998f}, {4.4f, 0.4995f},
		{4.6f, 0.4992f}, {4.8f, 0.499f}, {5.f, 0.4988f}
	};

public:
	static float CalculateDragCoefficient(float velocity)
	{
		int num = static_cast<int>(std::round(std::floor(velocity / 343.f / 0.05f)));

		if (num <= 0)
			return 0.f;

		if (num > std::size(_g1Coeffs) - 1)
			return _g1Coeffs[std::size(_g1Coeffs) - 1].Ballist;

		float num2 = _g1Coeffs[num - 1].Mach * 343.f;
		float num3 = _g1Coeffs[num].Mach * 343.f;
		float ballist = _g1Coeffs[num - 1].Ballist;

		return (_g1Coeffs[num].Ballist - ballist) / (num3 - num2) * (velocity - num2) + ballist;
	}
};

class BallisticsSimulation
{
	static constexpr int _maxIterations = 1300;
	static constexpr float _simTimeStep = 0.01f;
	static constexpr float _optimalLerpTolerance = 0.001f;
	static constexpr float PI = 3.14159265358979323846f;

	// Use functions for static vectors to avoid initialization order issues or warnings
	static Vector3 Gravity() { return Vector3::Zero() + Vector3{ 0.f, -9.81f, 0.f }; }
	static Vector3 Forward() { return Vector3::Forward(); }

	static float FindOptimalLerp(Vector3 posBefore, Vector3 posAfter, float shotDistance)
	{
		float lerpMin = 0.f;
		float lerpMax = 1.f;
		float lerpMid = 0.5f;

		while (lerpMax - lerpMin > _optimalLerpTolerance)
		{
			Vector3 lerped = Vector3::Lerp(posBefore, posAfter, lerpMid);

			if (lerped.z < shotDistance)
				lerpMin = lerpMid;
			else
				lerpMax = lerpMid;

			lerpMid = (lerpMin + lerpMax) / 2.f;
		}

		return lerpMid;
	}

	static float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

public:
	// Returns trajectory points relative to start (0,0,0) assuming flat fire.
	// Z = Distance, Y = Drop (negative).
	static std::vector<Vector3> SimulateTrajectory(const BallisticsInfo& ballistics, float maxTime = 1.5f)
	{
		std::vector<Vector3> points;
		points.reserve((size_t)(maxTime / _simTimeStep) + 1);
		points.push_back(Vector3{ 0.f, 0.f, 0.f });

		float uE002 = ballistics.BulletMassGrams / 1000.f;
		float uE004 = uE002 * 2.f;
		float uE003 = ballistics.BulletDiameterMillimeters / 1000.f;
		float uE005 = uE002 * 0.0014223f / (uE003 * uE003 * ballistics.BallisticCoefficient);
		float uE006 = uE003 * uE003 * PI / 4.f;
		float uE007 = 1.2f * uE006;

		Vector3 lastPosition{ 0.f, 0.f, 0.f };
		Vector3 lastVelocity = Forward() * ballistics.BulletSpeed;

		int steps = (int)(maxTime / _simTimeStep);
		if (steps > _maxIterations) steps = _maxIterations;

		for (int i = 1; i < steps; i++)
		{
			float magnitude = lastVelocity.Length();
			float dragCoefficient = G1::CalculateDragCoefficient(magnitude) * uE005;

			Vector3 translationOffset = Gravity() + (lastVelocity.Normalize() * (uE007 * -dragCoefficient * magnitude * magnitude / uE004));

			Vector3 currentPosition = lastPosition + lastVelocity * _simTimeStep + translationOffset * 5E-05f;
			Vector3 currentVelocity = lastVelocity + translationOffset * _simTimeStep;

			points.push_back(currentPosition);

			lastPosition = currentPosition;
			lastVelocity = currentVelocity;
		}

		return points;
	}

	static BallisticSimulationOutput Run(Vector3 startPosition, Vector3 endPosition, BallisticsInfo ballistics)
	{
		float shotDistance = startPosition.DistanceTo(endPosition);

		float uE002 = ballistics.BulletMassGrams / 1000.f;
		float uE004 = uE002 * 2.f;
		float uE003 = ballistics.BulletDiameterMillimeters / 1000.f;
		float uE005 = uE002 * 0.0014223f / (uE003 * uE003 * ballistics.BallisticCoefficient);
		float uE006 = uE003 * uE003 * PI / 4.f;
		float uE007 = 1.2f * uE006;

		float time = 0.f;
		float lastTravelTime = 0.f;
		Vector3 lastPosition{ 0.f, 0.f, 0.f };
		Vector3 lastVelocity = Forward() * ballistics.BulletSpeed;

		float bulletDrop = 0.f;
		float travelTime = 0.f;

		for (int i = 1; i < _maxIterations; i++)
		{
			float magnitude = lastVelocity.Length();
			float dragCoefficient = G1::CalculateDragCoefficient(magnitude) * uE005;
			
			// Replicating C# logic:
			// translationOffset = _gravity + (lastVelocity.Normalize() * (uE007 * -dragCoefficient * magnitude * magnitude / uE004));
			Vector3 translationOffset = Gravity() + (lastVelocity.Normalize() * (uE007 * -dragCoefficient * magnitude * magnitude / uE004));

			Vector3 currentPosition = lastPosition + lastVelocity * _simTimeStep + translationOffset * 5E-05f;
			Vector3 currentVelocity = lastVelocity + translationOffset * _simTimeStep;

			float currentDistance = currentPosition.DistanceTo(Vector3::Zero());
			if (currentDistance >= shotDistance)
			{
				float optimalLerp = FindOptimalLerp(lastPosition, currentPosition, shotDistance);
				
				Vector3 lerpedPos = Vector3::Lerp(lastPosition, currentPosition, optimalLerp);
				bulletDrop = std::abs(lerpedPos.y);
				travelTime = Lerp(lastTravelTime, time + _simTimeStep, optimalLerp);

				break;
			}
			else
			{
				time += _simTimeStep;
				lastTravelTime = time;
				lastPosition = currentPosition;
				lastVelocity = currentVelocity;
			}
		}

		return BallisticSimulationOutput{ bulletDrop, travelTime };
	}
};