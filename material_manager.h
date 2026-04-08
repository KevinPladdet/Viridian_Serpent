#pragma once
#include "tmpl8math.h"

enum MaterialType
{
	DIFFUSE = 0,
	MIRROR = 1,
	GLASS = 2,
	MAT_COUNT
};

struct Material
{
	const char* name;
	float roughness; // Adds blur
	float ior; // Index of Refraction (Air = 1.0f -- Water = 1.33f -- Glass = 1.46f)
	float specular; // Controls reflection strength
	float metallic; // Full metallic = mirror
	float transparency; // Controls how much refracted light goes through
};

namespace Tmpl8
{

	class Renderer;

	class Material_Manager
	{
	public:
		Material_Manager(Renderer* r);
		
		const Material& Get(int id) const
		{
			if (id < 0 || id >= materials.size()) return materials[DIFFUSE];
			return materials[id];
		}

		std::vector<Material> materials =
		{
			{ "Diffuse", 1.0f, 1.0f, 0.0f, 0.0f, 0.0f },
			{ "Mirror",  0.0f, 1.0f, 1.0f, 1.0f, 0.0f },
			{ "Glass",  0.0f, 1.46f, 1.0f, 0.0f, 1.0f }
		};

		// Copy of materials, because if values get modified through ImGui it can break sliders
		const std::vector<Material> defaults = materials;

	private:
		Renderer* m_renderer;
	};

}