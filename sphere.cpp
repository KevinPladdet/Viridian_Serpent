#include "template.h"
#include "sphere.h"

namespace Tmpl8
{

	Sphere::Sphere(float3 pos, float radius, float3 color, int material, bool hasPhysics) :
		pos(pos),
		radius(radius),
		color(color),
		material(material),
		hasPhysics(hasPhysics)
	{

	}

	bool Sphere::Intersect(Ray& ray) const
	{
		// Origin to center
		const float3 oc = ray.O - pos;

		// Squared length of ray direction
		const float a = dot(ray.D, ray.D);

		// Projection of oc on ray direction
		float p = dot(ray.D, oc);

		// Squared distance from origin to center minus r^2
		const float c = dot(oc, oc) - radius * radius;

		const float d = p * p - a * c;
		if (d < 0) return false;

		// sd = squared distance
		const float sd = sqrtf(d);

		// Entry point of sphere
		float t = (-p - sd) / a;

		// If inside of sphere, set exit point
		if (t < 0)
		{
			t = (-p + sd) / a;
		}

		// Keep hit if its the closest one so far
		if (t > 0 && t < ray.t)
		{
			ray.t = t;
			return true;
		}

		return false;
	}

	void Sphere::Physics(float deltaTime, Scene& scene)
	{
		const float gravity = -9.81f * radius;
		const float bounceMultiplier = 0.5f; // Used to reduce velocity when bouncing back upwards

		// Apply gravity and clamp it to a maximum of -50.0f
		if (!grounded)
		{
			velocity.y += gravity * deltaTime;
			velocity.y = max(velocity.y, -50.0f);
		}

		float3 nextPos = pos + velocity * deltaTime;

		// Raycast
		const float3 rayOrigin = nextPos;
		const float3 rayDir = float3(0, -1, 0);
		const float rayDistance = radius + 0.1f;
		Ray ray(rayOrigin, rayDir, rayDistance);
		scene.FindNearest(ray);

		if (ray.voxel != 0)
		{
			// Length of ray
			float hitDistance = ray.t;

			// If hitDistance is smaller than radius, it means sphere went through voxel
			if (hitDistance <= radius)
			{
				// posY of where ray hit the ground / voxel
				const float groundY = rayOrigin.y - hitDistance;

				// Snap sphere to surface
				nextPos.y = groundY + radius;

				if (velocity.y < 0)
				{
					// Reduce velocity by bounceLoss
					velocity.y = -velocity.y * bounceMultiplier;

					// To prevent infinite small bounces
					if (fabsf(velocity.y) < 0.05f)
					{
						velocity.y = 0.0f;
						grounded = true;
					}
				}
			}
		}
		else
		{
			grounded = false;
		}

		pos = nextPos;
	}

	float3 Sphere::GetNormal(const float3& hitPoint) const
	{
		return normalize(hitPoint - pos);
	}

}