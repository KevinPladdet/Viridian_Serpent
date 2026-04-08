#pragma once

// default screen resolution
#define SCRWIDTH	512 // 512
#define SCRHEIGHT	384 // 384
// #define FULLSCREEN
#define DOUBLESIZE

namespace Tmpl8 {

	typedef float4 Plane;
	struct Frustum { Plane plane[4]; };
	inline float distance(Plane& plane, float3& pos)
	{
		return dot(float3(plane), pos) - plane.w;
	}

	class Snake;

	class Camera
	{
	public:
		Camera();
		~Camera();
		Ray GetPrimaryRay( const float x, const float y );
		bool HandleInput( const float t );

		Frustum BuildFrustum();
		
		void StartSpline(float duration);
		void UpdateSpline(float deltaTime);

		// Used to access m_snake->snakePaused
		void SetSnake(Snake* s) { m_snake = s; }

		float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
		float3 camPos, camTarget;
		float3 topLeft, topRight, bottomLeft;
		float3 up, right;

		float camSpeed = 0.0015f;
		float FOV = 70.0f;

		// Public because Renderer needs to use it
		bool splineActive = false;
	
	private:
		float splineTime = 0.0f;
		float splineDuration = 5.0f;

		std::vector<float3> splinePoints
		{
			float3(1.5f, 1.25f, 0.25f),  // P0 (Right side of Snake)
			float3(1.0f, 0.7f, 0.5f),    // P1 (Back side of Snake)
			float3(0.5f, 0.9f, 1.4f),    // P2 (Left side of Snake)
			float3(0.5f, 0.5f, -0.2f)    // P3 (Front side of Snake)
		};

		Snake* m_snake = nullptr;
	};

}