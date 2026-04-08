#include "template.h"
#include "spline.h"

namespace Tmpl8
{

	Camera::Camera()
	{
		// Set camPos and camTarget
		camPos = float3(0.5f, 0.5f, -0.2f);
		camTarget = float3(0.5f, 0.5f, 0.0f);

		// Setup a basic view frustum looking straight forward (without being rotated)
		float3 ahead = normalize(camTarget - camPos);
		float3 tmpUp(0, 1, 0);
		right = normalize(cross(tmpUp, ahead));
		up = normalize(cross(ahead, right));

		float fovScale = tanf((FOV * 0.5f) * PI / 180.0f);
		float3 center = camPos + ahead;
		topLeft = center - aspect * fovScale * right + fovScale * up;
		topRight = center + aspect * fovScale * right + fovScale * up;
		bottomLeft = center - aspect * fovScale * right - fovScale * up;
	}

	Camera::~Camera()
	{
		delete m_snake;
	}

	Ray Camera::GetPrimaryRay(const float x, const float y)
	{
		// calculate pixel position on virtual screen plane
		const float u = (float)x * (1.0f / SCRWIDTH);
		const float v = (float)y * (1.0f / SCRHEIGHT);
		const float3 P = topLeft + u * (topRight - topLeft) + v * (bottomLeft - topLeft);
		// return Ray( camPos, normalize( P - camPos ) );
		return Ray(camPos, P - camPos);
		// Note: no need to normalize primary rays in a pure voxel world
		// TODO: 
		// - if we have other primitives as well, we *do* need to normalize!
		// - there are far cooler camera models, e.g. try 'Panini projection'.
	}

	bool Camera::HandleInput(const float t)
	{
		if (!WindowHasFocus()) return false;
		float speed = camSpeed * t;
		float3 ahead = normalize(camTarget - camPos);
		float3 tmpUp(0, 1, 0);
		right = normalize(cross(tmpUp, ahead));
		up = normalize(cross(ahead, right));
		bool changed = false;
		if (IsKeyDown(GLFW_KEY_UP)) camTarget -= speed * up, changed = true;
		if (IsKeyDown(GLFW_KEY_DOWN)) camTarget += speed * up, changed = true;
		if (IsKeyDown(GLFW_KEY_LEFT)) camTarget -= speed * right, changed = true;
		if (IsKeyDown(GLFW_KEY_RIGHT)) camTarget += speed * right, changed = true;
		ahead = normalize(camTarget - camPos);
		right = normalize(cross(tmpUp, ahead));
		up = normalize(cross(ahead, right));
		if (IsKeyDown(GLFW_KEY_A)) camPos -= speed * right, changed = true;
		if (IsKeyDown(GLFW_KEY_D)) camPos += speed * right, changed = true;
		if (GetAsyncKeyState('W')) camPos += speed * ahead, changed = true;
		if (IsKeyDown(GLFW_KEY_S)) camPos -= speed * ahead, changed = true;
		if (IsKeyDown(GLFW_KEY_R)) camPos += speed * up, changed = true;
		if (IsKeyDown(GLFW_KEY_F)) camPos -= speed * up, changed = true;

		camTarget = camPos + ahead;
		ahead = normalize(camTarget - camPos); // Direction that camera is looking at
		up = normalize(cross(ahead, right));
		right = normalize(cross(up, ahead));

		float fovScale = tanf((FOV * 0.5f) * PI / 180.0f); // Convert half of FOV to radians (FOV is halved because a triangle is 180 degrees)

		float3 center = camPos + ahead;

		topLeft = center - aspect * fovScale * right + fovScale * up;
		topRight = center + aspect * fovScale * right + fovScale * up;
		bottomLeft = center - aspect * fovScale * right - fovScale * up;

		if (!changed) return false;
		return true;
	}


	Frustum Camera::BuildFrustum()
	{
		Frustum f;

		f.plane[0] = cross(topLeft - bottomLeft, topLeft - camPos);
		f.plane[1] = cross(topRight - camPos, topLeft - bottomLeft);
		f.plane[2] = cross(topRight - topLeft, topLeft - camPos);
		f.plane[3] = cross(bottomLeft - camPos, topRight - topLeft);

		for (int i = 0; i < 4; i++)
		{
			f.plane[i].w = distance(f.plane[i], camPos);
		}

		return f;
	}

	
	void Camera::StartSpline(float duration)
	{
		splineTime = 0.0f;
		splineDuration = duration;
		splineActive = true;
	}

	void Camera::UpdateSpline(float deltaTime)
	{
		// Safety check: if spline isn't active or there are less than 4 points, return;
		if (!splineActive || splinePoints.size() < 4) return;

		// Keep track of how long spline has been active (in seconds / deltaTime)
		splineTime += deltaTime;
		float t = splineTime / splineDuration;

		// If spline reached the end, end it by with "splineActive = false"
		if (t >= 1.0f)
		{
			t = 1.0f;
			splineActive = false;
			m_snake->snakePaused = false;
		}
		
		// Spline travel order: P3 to P0 to P1 to P2 to P3 (Starts and ends with P3)
		float3 waypoints[4] =
		{
			splinePoints[3], // P3 - Front (Starting position)
			splinePoints[0], // P0 - Right
			splinePoints[1], // P1 - Back
			splinePoints[2], // P2 - Left
		};

		// splinePoints contains 4 points, so each one will take 0.25 of total splineTime
		float segmentFloat = t * 4.0f;
		int segment = min((int)segmentFloat, 3); // Turn float segment into int segment (segment is the index)
		float localT = segmentFloat - (float)segment; // Time in between segments

		// Catmull-Rom Spline needs minimum of 4 points to create a closed loop
		float3 p0 = waypoints[(segment - 1 + 4) % 4]; // p0 = previous point
		float3 p1 = waypoints[segment];               // p1 = front
		float3 p2 = waypoints[(segment + 1) % 4];     // p2 = right
		float3 p3 = waypoints[(segment + 2) % 4];     // p3 = back

		camPos = Spline::CatmullRomSpline(p0, p1, p2, p3, localT);

		// Always look at center of Snake game (0.5f, 0.5f, 0.5f)
		camTarget = float3(0.5f, 0.5f, 0.5f);
		
		// Rebuild view frustum to look at updateed camTarget (also scales correctly with FOV)
		float3 ahead = normalize(camTarget - camPos);
		float3 tmpUp(0, 1, 0);
		right = normalize(cross(tmpUp, ahead));
		up = normalize(cross(ahead, right));

		float fovScale = tanf((FOV * 0.5f) * PI / 180.0f);
		float3 center = camPos + ahead;

		topLeft = center - aspect * fovScale * right + fovScale * up;
		topRight = center + aspect * fovScale * right + fovScale * up;
		bottomLeft = center - aspect * fovScale * right - fovScale * up;
	}

}