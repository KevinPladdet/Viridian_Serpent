#include "template.h"
#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"
#include <unordered_map>

namespace Tmpl8
{

	inline float intersect_cube(Ray& ray)
	{
		// branchless slab method by Tavian
		const float tx1 = -ray.O.x * ray.rD.x, tx2 = (1 - ray.O.x) * ray.rD.x;
		float ty, tz, tmin = min(tx1, tx2), tmax = max(tx1, tx2);
		const float ty1 = -ray.O.y * ray.rD.y, ty2 = (1 - ray.O.y) * ray.rD.y;
		ty = min(ty1, ty2), tmin = max(tmin, ty), tmax = min(tmax, max(ty1, ty2));
		const float tz1 = -ray.O.z * ray.rD.z, tz2 = (1 - ray.O.z) * ray.rD.z;
		tz = min(tz1, tz2), tmin = max(tmin, tz), tmax = min(tmax, max(tz1, tz2));
		if (tmin == tz) ray.axis = 2; else if (tmin == ty) ray.axis = 1;
		return tmax >= tmin && tmin > 0 ? tmin : 1e34f;
	}

	inline bool point_in_cube(const float3& pos)
	{
		// test if pos is inside the cube
		return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 &&
			pos.x <= 1 && pos.y <= 1 && pos.z <= 1;
	}

	Scene::Scene()
	{
		grid = nullptr;
	}

	Scene::~Scene()
	{
		FREE64(grid); // Fixes memory leak for VLD
	}

	void Scene::GenerateScene(SceneType selectedWorld)
	{
		// Clear grid when switching scenes
		if (grid)
		{
			FREE64(grid);
			grid = nullptr;
		}

		// Clear palette when switching scenes
		memset(palette, 0, sizeof(palette));

		// Viking house
		if (selectedWorld == SceneType::Viking)
		{
			LoadBin("assets/viking.bin");
			return;
		}

		// Beach.vox
		if (selectedWorld == SceneType::Beach)
		{
			LoadVox("assets/vox/beach.vox");
			return;
		}

		// Pinball.vox
		if (selectedWorld == SceneType::Pinball)
		{
			LoadVox("assets/vox/pinballmachine_118x254x236_141_colors.vox");
			return;
		}

		// Snake
		if (selectedWorld == SceneType::Snake)
		{
			worldSize = 80; // Voxel World = 80x80x80 (lower worldSize = more FPS)
			worldSizeSquared = (size_t)worldSize * worldSize;
			worldSizeCubed = (size_t)worldSize * worldSize * worldSize;

			grid = (uint8_t*)MALLOC64(worldSizeCubed * sizeof(uint8_t));
			memset(grid, 0, worldSizeCubed * sizeof(uint8_t));

			#pragma region Borders
			const int GRID_SIZE = 15; // Same as GRID_SIZE in snake.h (15x15 grid)
			const float CELL_SIZE = 0.025f * 2.0f; // Same as CELL_SIZE in snake.h
			const float gridStart = 0.5f - (GRID_SIZE / 2.0f) * CELL_SIZE; // gridPos in world space of bottom-left corner of grid
			const float gridEnd = gridStart + GRID_SIZE * CELL_SIZE; // gridPos in world space of top-right corner of grid

			// Game Borders
			const int minX = (int)(gridStart * worldSize); // Left border
			const int maxX = (int)(gridEnd * worldSize); // Right border
			const int minY = (int)(gridStart * worldSize); // Bottom border
			const int maxY = (int)(gridEnd * worldSize); // Top border
			const int borderZ = worldSize / 2; // Z position of border (middle of world)

			for (int x = minX; x <= maxX; x++)
			{
				for (int y = minY; y <= maxY; y++)
				{
					// Only place voxels on the borders, not on the inside
					bool isBorder = (x == minX || x == maxX || y == minY || y == maxY);
					if (isBorder)
					{
						// borderZ layer 0 is black voxels
						Set(x, y, borderZ, Voxel(DIFFUSE, 0x010101));

						// borderZ layer 1 to 4 is white voxels
						for (int layersAmount = 1; layersAmount <= 4; layersAmount++)
						{
							Set(x, y, borderZ + layersAmount, Voxel(DIFFUSE, 0xFFFFFF));
						}
					}
				}
			}
			#pragma endregion
			
			#pragma region Background
			const int bgZ = worldSize - 38; // Z position of checkers background (voxelZ = 42, because 80 - 38)
			const float bgStart = 0.5f - (GRID_SIZE / 2.0f) * CELL_SIZE; // World space pos of checkers background bottom-left corner of grid

			// Loop over all grid cells to create black checkers background
			for (int gridX = 0; gridX < GRID_SIZE; gridX++)
			{
				for (int gridY = 0; gridY < GRID_SIZE; gridY++)
				{
					// Skip every other cell to create a checkers pattern in the background
					if ((gridX + gridY) % 2 != 0) continue;

					// Calculate voxel bounds of this grid cell
					float worldLeft = bgStart + gridX * CELL_SIZE; // World space pos of left edge of cell
					float worldRight = bgStart + (gridX + 1) * CELL_SIZE; // World space pos of right edge of cell
					float worldBottom = bgStart + gridY * CELL_SIZE; // World space pos of bottom edge of cell
					float worldTop = bgStart + (gridY + 1) * CELL_SIZE; // World space pos of top edge of cell

					int vxMin = (int)roundf(worldLeft * worldSize); // Left voxel
					int vxMax = (int)roundf(worldRight * worldSize); // Right voxel
					int vyMin = (int)roundf(worldBottom * worldSize); // Bottom voxel
					int vyMax = (int)roundf(worldTop * worldSize); // Top voxel

					// Fill cell with black voxels
					for (int x = vxMin; x < vxMax; x++)
					{
						for (int y = vyMin; y < vyMax; y++)
						{
							if (x >= 0 && x < (int)worldSize && y >= 0 && y < (int)worldSize)
							{
								Set(x, y, bgZ, Voxel(DIFFUSE, 0x080808));
							}
						}
					}
				}
			}
			#pragma endregion

			return;
		}

		// Plane with 3 pillars on it, used to test noise from shadows
		if (selectedWorld == SceneType::Pillars)
		{
			worldSize = 128;
			worldSizeSquared = (size_t)worldSize * worldSize;
			worldSizeCubed = (size_t)worldSize * worldSize * worldSize;

			grid = (uint8_t*)MALLOC64(worldSizeCubed * sizeof(uint8_t));
			memset(grid, 0, worldSizeCubed * sizeof(uint8_t));

			for (int i = 0; i < 128 * 128 * 128; i++)
			{
				const int x = i & 127, y = (i >> 7) & 127, z = i >> 14;
				bool v = y < 10, p = z > 90 && z < 94;
				p &= (x & 31) > 10 && (x & 31) < 18 && y < 80;
				Set(x, y, z, p || v ? Voxel(DIFFUSE, 0xffffff) : 0);
			}

			return;
		}
	}

	void Scene::LoadVox(const char* fileDirectory)
	{
		// Read file into memory
		FILE* fp = fopen(fileDirectory, "rb");
		if (!fp)
		{
			printf("ERROR - FILE %s WAS NOT FOUND!\n", fileDirectory);
			return;
		}

		fseek(fp, 0, SEEK_END);
		size_t fileSize = ftell(fp);
		rewind(fp);

		uint8_t* buffer = (uint8_t*)malloc(fileSize);
		fread(buffer, 1, fileSize, fp);
		fclose(fp);

		// sceneData contains models, palette, materials, etc
		const ogt_vox_scene* sceneData = ogt_vox_read_scene(buffer, static_cast<uint32_t>(fileSize));
		free(buffer);
		if (!sceneData)
		{
			printf("ERROR - sceneData was empty or not found!\n");
			return;
		}

		// Out of all instances, set the MIN and MAX for each axis, will later be used to create 1 big bounding box around all instances
		int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
		int maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

		// Loop over all instances of .vox file to set worldSize and offset (which is the center in between worldSize and bounding box)
		for (uint32_t i = 0; i < sceneData->num_instances; i++)
		{
			const ogt_vox_instance& inst = sceneData->instances[i];
			const ogt_vox_model* model = sceneData->models[inst.model_index];
			const ogt_vox_transform& t = inst.transform;

			// Center of model
			float centerX = (float)model->size_x / 2.0f;
			float centerY = (float)model->size_y / 2.0f;
			float centerZ = (float)model->size_z / 2.0f;

			// All 8 corners of bounding box in local model space
			float corners[8][3] =
			{
				{-centerX, -centerY, -centerZ}, { centerX, -centerY, -centerZ}, {-centerX,  centerY, -centerZ}, { centerX,  centerY, -centerZ},
				{-centerX, -centerY,  centerZ}, { centerX, -centerY,  centerZ}, {-centerX,  centerY,  centerZ}, { centerX,  centerY,  centerZ}
			};

			for (int localCorner = 0; localCorner < 8; localCorner++)
			{
				// Transform each local model space into MagicaVoxel's world space
				float worldX = t.m00 * corners[localCorner][0] + t.m10 * corners[localCorner][1] + t.m20 * corners[localCorner][2] + t.m30;
				float worldY = t.m01 * corners[localCorner][0] + t.m11 * corners[localCorner][1] + t.m21 * corners[localCorner][2] + t.m31;
				float worldZ = t.m02 * corners[localCorner][0] + t.m12 * corners[localCorner][1] + t.m22 * corners[localCorner][2] + t.m32;

				// Swap Y and Z axis because MagicaVoxel is weird and has them switched up
				int x = (int)floorf(worldX);
				int y = (int)floorf(worldZ);
				int z = (int)floorf(worldY);

				// Find and sets the smallest values between min and world space corners
				minX = min(minX, x);
				minY = min(minY, y);
				minZ = min(minZ, z);
				// Find and sets the biggest values between max and world space corners
				maxX = max(maxX, x + 1);
				maxY = max(maxY, y + 1);
				maxZ = max(maxZ, z + 1);
			}
		}

		// Set scene size with min and max
		int sceneWidth = maxX - minX;
		int sceneHeight = maxY - minY;
		int sceneDepth = maxZ - minZ;

		// Set worldSize with scene size so everything fits
		worldSize = max(sceneWidth, max(sceneHeight, sceneDepth));
		worldSizeSquared = (size_t)worldSize * worldSize;
		worldSizeCubed = (size_t)worldSize * worldSize * worldSize;

		// Clear grid by setting every voxel to 0
		grid = (uint8_t*)MALLOC64(worldSizeCubed * sizeof(uint8_t));
		memset(grid, 0, worldSizeCubed * sizeof(uint8_t));

		// Offset to position in between center of grid (worldSize / 2) and center of bounding box ((maxX + minX) / 2)
		int offsetX = worldSize / 2 - (maxX + minX) / 2;
		int offsetY = worldSize / 2 - (maxY + minY) / 2;
		int offsetZ = worldSize / 2 - (maxZ + minZ) / 2;

		// Fill palette with sceneData's colors and materials
		for (int i = 1; i < 256; i++)
		{
			const ogt_vox_rgba& color = sceneData->palette.color[i];
			const ogt_vox_matl* mat = &sceneData->materials.matl[i];

			palette[i].color = (color.r << 16) | (color.g << 8) | color.b;

			if (mat)
			{
				palette[i].roughness = mat->rough;
				palette[i].ior = mat->ior > 0.0f ? mat->ior : 1.0f;
				palette[i].specular = mat->sp;
				palette[i].metallic = mat->metal;
				palette[i].transparency = mat->trans;
			}
			else
			{
				// Default diffuse
				palette[i].roughness = 1.0f;
				palette[i].ior = 1.0f;
				palette[i].specular = 0.0f;
				palette[i].metallic = 0.0f;
				palette[i].transparency = 0.0f;
			}
		}

		// Loop over all instances to set every voxel
		for (uint32_t i = 0; i < sceneData->num_instances; i++)
		{
			const ogt_vox_instance& inst = sceneData->instances[i];
			const ogt_vox_model* model = sceneData->models[inst.model_index];
			const ogt_vox_transform& t = inst.transform;

			// Center of model
			float centerX = (float)model->size_x / 2.0f;
			float centerY = (float)model->size_y / 2.0f;
			float centerZ = (float)model->size_z / 2.0f;

			for (uint32_t z = 0; z < model->size_z; z++)
				for (uint32_t y = 0; y < model->size_y; y++)
					for (uint32_t x = 0; x < model->size_x; x++)
					{
						uint32_t index = x + y * model->size_x + z * model->size_x * model->size_y;

						uint8_t colorIndex = model->voxel_data[index];

						// If voxel is empty, skip it
						if (colorIndex == 0) continue;

						// Local position centered around origin (MagicaVoxel coords)
						float localX = (float)x - centerX + 0.5f;
						float localY = (float)y - centerY + 0.5f;
						float localZ = (float)z - centerZ + 0.5f;

						// Apply full 3x3 rotation + translation (MagicaVoxel coords)
						float worldX = t.m00 * localX + t.m10 * localY + t.m20 * localZ + t.m30;
						float worldY = t.m01 * localX + t.m11 * localY + t.m21 * localZ + t.m31;
						float worldZ = t.m02 * localX + t.m12 * localY + t.m22 * localZ + t.m32;

						// Swap Y and Z axis because MagicaVoxel is weird and has them switched up
						int voxelX = (int)floorf(worldX) + offsetX;
						int voxelY = (int)floorf(worldZ) + offsetY;
						int voxelZ = (int)floorf(worldY) + offsetZ;

						// Check if voxel is inside of grid, if yes: set voxel
						if (voxelX >= 0 && voxelX < (int)worldSize &&
							voxelY >= 0 && voxelY < (int)worldSize &&
							voxelZ >= 0 && voxelZ < (int)worldSize)
						{
							Set(voxelX, voxelY, voxelZ, colorIndex);
						}
					}
		}

		// Saving sceneDate so I don't get an error about trying to access destroyed data
		uint32_t numInstances = sceneData->num_instances;
		uint32_t numModels = sceneData->num_models;

		// Prevent memory leaks by destroying sceneData
		ogt_vox_destroy_scene(sceneData);

		printf("\nLoaded %s ::: worldSize = %u (%u models, %u instances)\n", fileDirectory, worldSize, numModels, numInstances);
	}

	void Scene::PlaceVox(const char* fileDirectory, const int spawnPosX, const int spawnPosY, const int spawnPosZ)
	{
		// Read file into memory
		FILE* fp = fopen(fileDirectory, "rb");
		if (!fp)
		{
			printf("ERROR - FILE %s WAS NOT FOUND!\n", fileDirectory);
			return;
		}

		fseek(fp, 0, SEEK_END);
		size_t fileSize = ftell(fp);
		rewind(fp);

		uint8_t* buffer = (uint8_t*)malloc(fileSize);
		fread(buffer, 1, fileSize, fp);
		fclose(fp);

		// sceneData contains models, palette, materials, etc
		const ogt_vox_scene* sceneData = ogt_vox_read_scene(buffer, static_cast<uint32_t>(fileSize));
		free(buffer);
		if (!sceneData)
		{
			printf("ERROR - sceneData was empty or not found!\n");
			return;
		}

		// Fill palette with sceneData's colors and materials
		for (int i = 1; i < 256; i++)
		{
			// Only fill empty palette slots
			if (palette[i].color != 0 || palette[i].metallic != 0 || palette[i].transparency != 0) continue;

			const ogt_vox_rgba& color = sceneData->palette.color[i];
			const ogt_vox_matl* mat = &sceneData->materials.matl[i];

			palette[i].color = (color.r << 16) | (color.g << 8) | color.b;

			if (mat)
			{
				palette[i].roughness = mat->rough;
				palette[i].ior = mat->ior > 0.0f ? mat->ior : 1.0f;
				palette[i].specular = mat->sp;
				palette[i].metallic = mat->metal;
				palette[i].transparency = mat->trans;
			}
			else
			{
				// Default diffuse
				palette[i].roughness = 1.0f;
				palette[i].ior = 1.0f;
				palette[i].specular = 0.0f;
				palette[i].metallic = 0.0f;
				palette[i].transparency = 0.0f;
			}
		}

		// Get first model
		const ogt_vox_model* model = sceneData->models[0];

		// Loop over all voxels
		for (uint32_t z = 0; z < model->size_z; z++)
			for (uint32_t y = 0; y < model->size_y; y++)
				for (uint32_t x = 0; x < model->size_x; x++)
				{
					uint32_t index = x + y * model->size_x + z * model->size_x * model->size_y;
					uint8_t colorIndex = model->voxel_data[index];

					// If voxel is empty, skip it
					if (colorIndex == 0) continue;

					// Swap Y and Z axis because MagicaVoxel is weird and has them switched up
					int voxelX = spawnPosX + (int)x;
					int voxelY = spawnPosY + (int)z;
					int voxelZ = spawnPosZ + (int)y;

					// Check if voxel is inside of grid, if yes: set voxel
					if (voxelX >= 0 && voxelX < (int)worldSize &&
						voxelY >= 0 && voxelY < (int)worldSize &&
						voxelZ >= 0 && voxelZ < (int)worldSize)
					{
						Set(voxelX, voxelY, voxelZ, colorIndex);
					}
				}

		// Prevent memory leaks by destroying sceneData
		ogt_vox_destroy_scene(sceneData);

		//printf("\nLoaded %s at (%d, %d, %d)\n", fileDirectory, spawnPosX, spawnPosY, spawnPosZ);
	}

	void Scene::ClearVox(const int startX, const int startY, const int startZ, const int sizeX, const int sizeY, const int sizeZ)
	{
		// Loop over all voxels
		for (int z = 0; z < sizeZ; z++)
			for (int y = 0; y < sizeY; y++)
				for (int x = 0; x < sizeX; x++)
				{
					// Swap Y and Z axis because MagicaVoxel is weird and has them switched up
					int voxelX = startX + (int)x;
					int voxelY = startY + (int)z;
					int voxelZ = startZ + (int)y;

					// Check if voxel is inside of grid, if yes: set voxel
					if (voxelX >= 0 && voxelX < (int)worldSize &&
						voxelY >= 0 && voxelY < (int)worldSize &&
						voxelZ >= 0 && voxelZ < (int)worldSize)
					{
						Set(voxelX, voxelY, voxelZ, 0); // 0 = empty voxel
					}
				}
	}

	void Scene::LoadBin(const char* fileDirectory)
	{
		gzFile f = gzopen(fileDirectory, "rb");
		if (!f)
		{
			printf("ERROR - FILE %s WAS NOT FOUND!\n", fileDirectory);
			return;
		}

		int3 size = 0;
		gzread(f, &size, sizeof(int3));

		worldSize = size.x;
		worldSizeSquared = (size_t)worldSize * worldSize;
		worldSizeCubed = (size_t)worldSize * worldSize * worldSize;

		// Convert to uint so gzread can read it without errors
		uint* tempGrid = (uint*)MALLOC64(worldSizeCubed * sizeof(uint));
		gzread(f, tempGrid, (unsigned int)worldSizeCubed * sizeof(uint));
		gzclose(f);

		grid = (uint8_t*)MALLOC64(worldSizeCubed * sizeof(uint8_t));
		memset(grid, 0, worldSizeCubed * sizeof(uint8_t));

		// Fill colorMap with colors from .bin file
		std::unordered_map<uint, uint8_t> colorMap;
		uint8_t nextIndex = 1; // 0 is reserved for empty color

		// Loop through every voxel of world
		for (size_t i = 0; i < worldSizeCubed; i++)
		{
			uint voxel = tempGrid[i];
			if (voxel == 0) continue;

			// If color doesn't already exist in colorMap, add color + material of voxel to palette
			if (colorMap.find(voxel) == colorMap.end())
			{
				// If color went past 255 aka palette is full: printf warning
				if (nextIndex == 0)
				{
					printf("WARNING - %s HAS MORE THAN 256 UNIQUE COLORS, EVERYTHING PAST 256 WILL BE LOST!\n", fileDirectory);
					break;
				}

				colorMap[voxel] = nextIndex;

				// Extract color and material from voxel
				const uint color = voxel & 0x00FFFFFF;
				const uint8_t matID = voxel >> 24;

				// Fill palette with colors and materials from voxel
				palette[nextIndex].color = color;

				// Check if mat is MIRROR or GLASS --- if its none: make it DIFFUSE (values of default in switch)
				switch (matID)
				{
				case MIRROR: palette[nextIndex] = { color, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f }; break;
				case GLASS:  palette[nextIndex] = { color, 0.0f, 1.46f, 1.0f, 0.0f, 1.0f }; break;
				default:     palette[nextIndex] = { color, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
				}

				// nextIndex++ to get next available palette slot
				nextIndex++;
			}

			grid[i] = colorMap[voxel];
		}

		// Free memory after .bin file loaded
		FREE64(tempGrid);

		printf("\nLoaded %s ::: worldSize = %u, unique colors = %u\n", fileDirectory, worldSize, nextIndex - 1);
		return;
	}

	void Scene::Set(const size_t x, const size_t y, const size_t z, const uint8_t v)
	{
		size_t index = x + y * worldSize + z * worldSizeSquared;
		grid[index] = v;
	}

	uint8_t Scene::Voxel(MaterialType mat, uint color)
	{
		// Default values of matters for DIFFUSE
		float roughness = 1.0f, ior = 1.0f, specular = 0.0f, metallic = 0.0f, transparency = 0.0f;

		// Check if mat is MIRROR or GLASS
		switch (mat)
		{
		case MIRROR: roughness = 0.0f; ior = 1.0f; specular = 1.0f; metallic = 1.0f; break;
		case GLASS:  roughness = 0.0f; ior = 1.46f; specular = 1.0f; transparency = 1.0f; break;
		default: break; // DIFFUSE
		}

		// Check if the color and material combo already exists in palette[256]
		for (int i = 1; i < 256; i++)
		{
			if (palette[i].color == color &&
				palette[i].roughness == roughness &&
				palette[i].metallic == metallic &&
				palette[i].transparency == transparency)
				return (uint8_t)i;
		}

		// Find first palette[i] that is empty, if found: fill it with new color and material combo
		for (int i = 1; i < 256; i++)
		{
			if (palette[i].color == 0 && palette[i].metallic == 0 && palette[i].transparency == 0)
			{
				palette[i] = { color, roughness, ior, specular, metallic, transparency };
				return (uint8_t)i;
			}
		}

		// It shouldn't ever get to here, but just in case it does: warning printf
		printf("WARNING - PALETTE IS FULL WITH 256 COLORS, EVERYTHING PAST 256 WILL BE LOST!\n");
		return 1;
	}

	bool Scene::Setup3DDDA(Ray& ray, DDAState& state) const
	{
		// if ray is not inside the world: advance until it is
		state.t = 0;

		bool startedInGrid = point_in_cube(ray.O);
		if (!startedInGrid)
		{
			state.t = intersect_cube(ray);
			if (state.t > 1e33f) return false; // ray misses voxel data entirely
		}

		// setup amanatides & woo - assume world is 1x1x1, from (0,0,0) to (1,1,1)
		float cellSize = 1.0f / worldSize;
		state.step = make_int3(1.0f - ray.Dsign * 2.0f);
		const float3 posInGrid = (float)worldSize * (ray.O + (state.t + 0.00005f) * ray.D);
		const float3 gridPlanes = (ceilf(posInGrid) - ray.Dsign) * cellSize;
		const int3 P = clamp(make_int3(posInGrid), 0, worldSize - 1);
		state.X = P.x, state.Y = P.y, state.Z = P.z;
		state.tdelta = cellSize * float3(state.step) * ray.rD;
		state.tmax = (gridPlanes - ray.O) * ray.rD;

		// detect rays that start inside a voxel
		size_t index = P.x + P.y * worldSize + P.z * worldSizeSquared;
		uint8_t cell = grid[index];

		ray.inside = (cell != 0) && startedInGrid;

		// proceed with traversal
		return true;
	}

	void Scene::FindNearest(Ray& ray) const
	{
		// nudge origin
		ray.O += EPSILON * ray.D;

		// setup Amanatides & Woo grid traversal
		DDAState s;
		if (!Setup3DDDA(ray, s)) return;

		// Less multiplications = more performance
		size_t index = s.X + s.Y * worldSize + s.Z * worldSizeSquared;
		const size_t stepX = (size_t)s.step.x;
		const size_t stepY = (size_t)s.step.y * worldSize;
		const size_t stepZ = (size_t)s.step.z * worldSizeSquared;

		uint8_t cell, lastCell = 0;
		int axis = ray.axis;

		bool inside = ray.inside;

		// start stepping until we find an empty voxel
		while (1)
		{
			cell = grid[index];

			if (inside)
			{
				if (!cell) break;

				// Check if material of lastCell is different than material of current cell, if yes: break;
				if (lastCell && (palette[cell].metallic != palette[lastCell].metallic ||
					palette[cell].transparency != palette[lastCell].transparency)) break;

				lastCell = cell;
			}
			else
			{
				if (cell)
				{
					lastCell = cell;
					break;
				}
			}

			if (s.tmax.x < s.tmax.y)
			{
				if (s.tmax.x < s.tmax.z)
				{
					s.t = s.tmax.x;
					s.X += s.step.x;
					index += stepX;
					axis = 0;

					if (s.X >= worldSize) break;
					s.tmax.x += s.tdelta.x;
				}
				else
				{
					s.t = s.tmax.z;
					s.Z += s.step.z;
					index += stepZ;
					axis = 2;

					if (s.Z >= worldSize) break;
					s.tmax.z += s.tdelta.z;
				}
			}
			else
			{
				if (s.tmax.y < s.tmax.z)
				{
					s.t = s.tmax.y;
					s.Y += s.step.y;
					index += stepY;
					axis = 1;

					if (s.Y >= worldSize) break;
					s.tmax.y += s.tdelta.y;
				}
				else
				{
					s.t = s.tmax.z;
					s.Z += s.step.z;
					index += stepZ;
					axis = 2;

					if (s.Z >= worldSize) break;
					s.tmax.z += s.tdelta.z;
				}
			}
		}

		if (inside)
		{
			if (!cell)
			{
				ray.voxel = lastCell; // Exited into air
			}
			else if (lastCell && (palette[cell].metallic != palette[lastCell].metallic ||
				palette[cell].transparency != palette[lastCell].transparency))
			{
				ray.voxel = cell; // Hit a different material
			}
			else
			{
				ray.voxel = lastCell; // we store the voxel we just left
			}
		}
		else
		{
			ray.voxel = cell;
		}

		ray.t = s.t;
		ray.axis = axis;

		hitVoxel = { s.X, s.Y, s.Z };
	}

	bool Scene::IsOccluded(Ray& ray) const
	{
		// nudge origin
		ray.O += EPSILON * ray.D;
		ray.t -= EPSILON * 2.0f;
		// setup Amanatides & Woo grid traversal
		DDAState s;
		if (!Setup3DDDA(ray, s)) return false;
		// start stepping
		while (s.t < ray.t)
		{
			size_t index = s.X + s.Y * worldSize + s.Z * worldSizeSquared;
			const uint8_t cell = grid[index];
			if (cell) /* we hit a solid voxel */ return s.t < ray.t;
			if (s.tmax.x < s.tmax.y)
			{
				if (s.tmax.x < s.tmax.z) { if ((s.X += s.step.x) >= worldSize) return false; s.t = s.tmax.x, s.tmax.x += s.tdelta.x; }
				else { if ((s.Z += s.step.z) >= worldSize) return false; s.t = s.tmax.z, s.tmax.z += s.tdelta.z; }
			}
			else
			{
				if (s.tmax.y < s.tmax.z) { if ((s.Y += s.step.y) >= worldSize) return false; s.t = s.tmax.y, s.tmax.y += s.tdelta.y; }
				else { if ((s.Z += s.step.z) >= worldSize) return false; s.t = s.tmax.z, s.tmax.z += s.tdelta.z; }
			}
		}
		return false;
	}

}