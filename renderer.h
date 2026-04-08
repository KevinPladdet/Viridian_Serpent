#pragma once

#include "audio_manager.h"
#include "ui_manager.h"
#include "light_manager.h"
#include "material_manager.h"
#include "keys.h"
#include "sphere.h"
#include "snake.h"

namespace Tmpl8
{

	class Renderer : public TheApp
	{
	public:
		// game flow methods
		float3 Trace(Ray& ray, int frame = 0, int depth = 0, int x = 0, int y = 0);

		bool CalcShadows(const float3& origin, const float3& L, const float maxDistance);

		float3 SampleSkydome(const Ray& ray);

		// Depth of Field
		void CalcDoF(Ray& ray);
		void FocusDoF();

		// Generates a random direction away from the surface for stochastic sampling
		float3 StochasticSampling(const float3& N, float blueR1, float blueR2);

		void SnakeGameSetup();

		void Init();
		void Tick(float dt);
		void ResetAccumulator();
		void UI();
		void Shutdown();
		// input handling
		void MouseUp(int button) { button = 0; /* implement if you want to detect mouse button presses */ }
		void MouseDown(int button) { button = 0; /* implement if you want to detect mouse button presses */ }
		void MouseMove(int x, int y)
		{
#if defined(DOUBLESIZE) && !defined(FULLSCREEN)
			mousePos.x = x / 2, mousePos.y = y / 2;
#else
			mousePos.x = x, mousePos.y = y;
#endif
		}
		void MouseWheel(float y) { y = 0; /* implement if you want to handle the mouse wheel */ }
		void KeyUp(int /*key*/); // UNCOMMENT key if you want to implement KeyDown
		void KeyDown(int key);
		// data members
		int2 mousePos;
		Scene scene;
		Camera camera;

		// Is in public because ui_manager needs to use it
		std::vector<PointLight> pointLights;
		std::vector<DirectionalLight> directionalLights;
		std::vector<SpotLight> spotLights;
		std::vector<AreaLight> areaLights;
		std::vector<Sphere> spheres;

		// Is in public because ui_manager needs to use it
		float fps;
		float deltaTime;
		int spp = 1;
		bool cameraLocked = true;

		// Depth of Field
		bool dofEnabled = false; // "dof" means depth of field
		float focusDistance = 0.0f;
		float aperture = 0.001f;

		// Other Variables
		bool spherePhysics = true; // When disabled, all sphere physics pause
		bool gammaCorrection = true;
		bool skydomeLighting = true; // Stochastic Skydome Lighting

	private:

		// Audio Manager
		Audio_Manager* m_audio_manager = nullptr;

		// UI Manager
		bool UI_Enabled = true;
		UI_Manager* m_ui_manager = nullptr;

		// Light Manager
		Light_Manager* m_light_manager = nullptr;

		// Material Manager
		Material_Manager* m_material_manager = nullptr;
		
		// Snake
		Snake* m_snake = nullptr;

		// Anti Aliasing
		float3* accumulator = nullptr;

		// Skydome
		float* skyPixels = nullptr;
		int skyWidth = 0;
		int skyHeight = 0;
		int skyBpp = 0; // bpp is bytes per pixel

		// Noise
		Surface* blueNoise = nullptr;

		// Reprojection
		float3* history;
		static constexpr float REPROJECTION_ALPHA = 0.4f; // Weight % of new sample that gets blended together with history

		// Other Variables
		static constexpr int MAX_BOUNCES = 5; // Amount of times ray can bounce on mirrors

		// Used to calculate FPS
		float avg = 10;
		float alpha = 1;
};

} // namespace Tmpl8