#pragma once

namespace Tmpl8
{

	class Renderer;
	class Material_Manager;
	class Audio_Manager;
	class Snake;

	class UI_Manager
	{
	public:
		UI_Manager(Renderer* r, Material_Manager* mat_manager, Audio_Manager* audio_manager, Snake* snake);
		void GUI();
		void ImGuiCategory(const char* category, bool spacing);
		void ShowFPS();
		uint ReturnVoxel();

		uint3 lockedHitVoxel = { 0, 0, 0 }; // Voxel coordinates of the voxel as voxelLocked turns true
		// Used to show selected voxel with mousePos
		uint latestVoxel = 0;
		bool voxelLocked = false;

		bool developerDebug = false;

	private:
		static constexpr char* materialNames[] = { "Diffuse", "Mirror", "Glass", "Water" };

		Renderer* m_renderer;
		Material_Manager* m_material_manager;
		Audio_Manager* m_audio_manager;
		Snake* m_snake;
	};

}