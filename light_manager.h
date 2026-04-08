#pragma once
#include "tmpl8math.h"

struct PointLight
{
	float3 pos =   { 1.0f, 1.0f, 1.0f };
	float3 color = { 1.0f, 1.0f, 1.0f };
};

struct DirectionalLight
{
	float3 dir =   { 1.0f, 0.0f, 0.0f };
	float3 color = { 1.0f, 1.0f, 1.0f };
};

struct SpotLight
{
	float3 pos =   { 1.0f, 1.0f, 1.0f };
	float3 dir =   { 1.0f, 0.0f, 0.0f };
	float3 color = { 1.0f, 1.0f, 1.0f };

	float innerCone = 0.9f;
	float outerCone = 0.8f;
};

struct AreaLight
{
	float3 pos = { 1.0f, 1.0f, 1.0f };
	float3 color = { 1.0f, 1.0f, 1.0f };
	float size = 1.0f; // Increasing this will increase amount of samples needed until all light is sampled
	float intensity = 2.0f;
};

namespace Tmpl8
{

	class Renderer;

	class Light_Manager
	{
	public:
		Light_Manager(Renderer* r);

		float3 CalcPointLights(const float3& N, const float3& I, const float3& albedo);
		float3 CalcDirectionalLights(const float3& N, const float3& I, const float3& albedo);
		float3 CalcSpotLights(const float3& N, const float3& I, const float3& albedo);
		
		float3 CalcAreaLights(const float3& N, const float3& I, const float3& albedo, float blueR1, float blueR2);
		float3 RandomPosAreaLight(const AreaLight& areaLight, float blueR1, float blueR2);

	private:
		Renderer* m_renderer;
	};

}