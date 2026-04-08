#include "template.h"
#include "light_manager.h"
#include "renderer.h"

namespace Tmpl8
{

	Light_Manager::Light_Manager(Renderer* r)
	{
		m_renderer = r;
	}

	float3 Light_Manager::CalcPointLights(const float3& N, const float3& I, const float3& albedo)
	{
		float3 result(0);

		for (auto& pointLight : m_renderer->pointLights)
		{
			const float3 L = pointLight.pos - I;
			const float distanceSquared = dot(L, L);
			const float inverseDistance = rsqrtf(distanceSquared);
			const float3 Ldir = L * inverseDistance;
			const float distance = distanceSquared * inverseDistance;

			const float cosAngle = max(0.0f, dot(N, Ldir));

			// If light is behind face, don't check it (saves performance by not checking shadows)
			if (cosAngle <= 0.0f) continue;

			// Calculate shadows
			if (m_renderer->CalcShadows(I, Ldir, distance)) continue;

			result += pointLight.color * albedo * (inverseDistance * inverseDistance) * cosAngle;
		}

		return result;
	}

	float3 Light_Manager::CalcDirectionalLights(const float3& N, const float3& I, const float3& albedo)
	{
		float3 result(0);

		for (auto& directionalLight : m_renderer->directionalLights)
		{
			const float3 Ldir = -directionalLight.dir;
			const float cosAngle = max(0.0f, dot(N, Ldir));

			// If light is behind face, don't check it (saves performance by not checking shadows)
			if (cosAngle <= 0.0f) continue;

			// Calculate shadows
			const float distance = 10000.0f; // Distance should be infinite for shadows because then everything should be checked
			if (m_renderer->CalcShadows(I, Ldir, distance)) continue;

			result += directionalLight.color * albedo * cosAngle;
		}

		return result;
	}

	float3 Light_Manager::CalcSpotLights(const float3& N, const float3& I, const float3& albedo)
	{
		float3 result(0);

		for (auto& spotLight : m_renderer->spotLights)
		{
			const float3 L = spotLight.pos - I;
			const float distanceSquared = dot(L, L);
			const float inverseDistance = rsqrtf(distanceSquared);
			const float3 Ldir = L * inverseDistance;
			const float distance = distanceSquared * inverseDistance;

			const float cosAngle = dot(-Ldir, spotLight.dir);

			// No light when outside outerCone
			if (cosAngle < spotLight.outerCone) continue;

			// Diffuse lighting
			const float NdotL = max(0.0f, dot(N, Ldir)); // NdotL is dot product of Normal and Light (can't get negative values because of max(0.0f))
			if (NdotL <= 0.0f) continue;

			// Smooth fall off
			float spotIntensity = (cosAngle - spotLight.outerCone) / (spotLight.innerCone - spotLight.outerCone);
			spotIntensity = clamp(spotIntensity, 0.0f, 1.0f);

			// Calculate shadows
			if (m_renderer->CalcShadows(I, Ldir, distance)) continue;

			result += spotLight.color * albedo * (inverseDistance * inverseDistance) * NdotL * spotIntensity;
		}

		return result;
	}

	float3 Light_Manager::CalcAreaLights(const float3& N, const float3& I, const float3& albedo, float blueR1, float blueR2)
	{
		float3 result(0);

		for (auto& areaLight : m_renderer->areaLights)
		{
			const float3 samplePos = RandomPosAreaLight(areaLight, blueR1, blueR2);
			const float3 L = samplePos - I;

			const float distanceSquared = dot(L, L);
			const float inverseDistance = rsqrtf(distanceSquared);
			const float3 Ldir = L * inverseDistance;
			const float distance = distanceSquared * inverseDistance;

			const float cosAngle = max(0.0f, dot(N, Ldir));

			// If light is behind face, don't check it
			if (cosAngle <= 0.0f) continue;

			// Calculate shadows
			if (m_renderer->CalcShadows(I, Ldir, distance)) continue;

			result += areaLight.color * areaLight.intensity * albedo * cosAngle / distanceSquared;
		}

		return result;
	}

	float3 Light_Manager::RandomPosAreaLight(const AreaLight& areaLight, float blueR1, float blueR2)
	{
		// Used to sample area light by getting random position inside of area light
		float x = (blueR1 - 0.5f) * areaLight.size;
		float z = (blueR2 - 0.5f) * areaLight.size;

		return areaLight.pos + float3(x, 0.0f, z);
	}

}