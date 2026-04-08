#pragma once

namespace Tmpl8
{

	struct Sphere
	{
		// Sphere Values
		float3 pos;
		float radius;
		float3 color;
		int material; // 0 = diffuse, 1 = mirror, 2 = glass

		// Physics
		float3 velocity = 0;
		bool grounded = false;
		bool hasPhysics = false;

		Sphere(float3 pos, float radius, float3 color, int material = 0, bool hasPhysics = false);
		bool Intersect(Ray& ray) const;
		void Physics(float deltaTime, Scene& scene);
		float3 GetNormal(const float3& hitPoint) const;
	};

};