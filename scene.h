#pragma once
#include "material_manager.h"

// epsilon
#define EPSILON		0.00001f

namespace Tmpl8
{

	enum class SceneType
	{
		Snake,
		Pillars,
		Viking,
		Beach,
		Pinball,
		COUNT // Used to show all values of this enum inside of ImGui dropdown "Selected World"
	};

	static const char* SceneTypeNames[] =
	{
		"Snake",
		"Pillars",
		"Viking",
		"Beach",
		"Pinball"
	};

	class Scene
	{
	public:
		struct DDAState
		{
			int3 step;
			uint X, Y, Z;
			float t;
			float3 tdelta;
			float3 tmax;
		};
		Scene();
		~Scene();
	
		void GenerateScene(SceneType selectedWorld); // Loads in scene
		
		void LoadVox(const char* fileDirectory); // Loads in .vox files
		void PlaceVox(const char* fileDirectory, const int spawnPosX, const int spawnPosY, const int spawnPosZ); // Places .vox file inside of grid
		void ClearVox(const int startX, const int startY, const int startZ, const int sizeX, const int sizeY, const int sizeZ); // Deletes .vox file inside of grid

		void LoadBin(const char* fileDirectory); // Loads in .bin files

		void FindNearest( Ray& ray ) const;
		bool IsOccluded( Ray& ray ) const;
		void Set( const size_t x, const size_t y, const size_t z, const uint8_t v );
	
		// Returns albedo from pallete[256]
		float3 GetAlbedo(uint8_t index) const
		{
			uint c = palette[index].color;
			return float3(((c >> 16) & 255) / 255.0f,
						  ((c >> 8) & 255) / 255.0f,
						   (c & 255) / 255.0f);
		}

		// Returns palette index of given voxel (palette contains all matter values of material)
		Material GetMaterialData(uint8_t index) const
		{
			const VoxelInfo& v = palette[index];
			return { "voxel", v.roughness, v.ior, v.specular, v.metallic, v.transparency };
		}

		// Returns material type out of either MIRROR, GLASS or DIFFUSE
		int GetMaterialType(uint8_t index) const
		{
			if (palette[index].metallic > 0 && palette[index].transparency == 0) return MIRROR;
			if (palette[index].transparency > 0) return GLASS;
			return DIFFUSE;
		}

		// Cleaner way to show voxels with materials --- e.g. 0x02AAFFAA turns into Voxel(GLASS, 0xAAFFAA)
		uint8_t Voxel(MaterialType mat, uint color);

		// Coordinates of selected voxel with mousePos
		mutable uint3 hitVoxel = { 0, 0, 0 }; // Mutable because it needs to change a const in FindNearest();

		struct VoxelInfo
		{
			uint color;
			float roughness;
			float ior;
			float specular;
			float metallic;
			float transparency;
		};

		uint8_t* grid = nullptr; // uint8_t instead of unsigned int, so it will use 4x less processing memory
		VoxelInfo palette[256] = {}; // Contains color + materials

		uint worldSize = 0; // power of 2. Warning: max 512 for a 512x512x512x4 bytes = 512MB world!
		size_t worldSizeSquared = 0;
		size_t worldSizeCubed = 0;

		// Which scene is selected when running program
		int selected_world = 0;

	private:
		// Inline so that it saves a bit of performance: the compiler doesnt call Setup3DDDA() every time, but instead copies all the code of this function INSIDE of the functions that constantly call it
		inline bool Setup3DDDA( Ray& ray, DDAState& state ) const;
	};

}