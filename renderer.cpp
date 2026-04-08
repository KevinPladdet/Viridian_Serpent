#include "template.h"
#include "stb_image.h"

#ifdef _DEBUG
#include <./lib/vld/include/vld.h>
#endif

namespace Tmpl8
{

	// Used for Reprojection
	Frustum previous;
	float3 prevCamPos;

	// -----------------------------------------------------------
	// Calculate light transport via a ray
	// -----------------------------------------------------------
	float3 Renderer::Trace(Ray& ray, int frame, int depth, int x, int y)
	{
		// Blue Noise
		float blueR1 = RandomFloat();
		float blueR2 = RandomFloat();
		if (spp == 1)
		{
			int pixel = blueNoise->pixels[(x & 63) + (y & 63) * 64];
			int red = pixel >> 16, green = (pixel >> 8) & 255;
			blueR1 = fracf(red / 256.0f + frame * 0.61803399f);
			blueR2 = fracf(green / 256.0f + frame * 0.61803399f);
		}

		float3 accumulatedColor = float3(1); // Color gets multiplied from each bounce (depth is max amount of bounces it can do from mirror to mirror)

		for (; depth < MAX_BOUNCES; depth++)
		{
			scene.FindNearest(ray);

			Sphere* hitSphere = nullptr;
			for (auto& sphere : spheres)
			{
				if (sphere.Intersect(ray))
				{
					hitSphere = &sphere;
				}
			}

			// HDR Skydome
			if (ray.voxel == 0 && !hitSphere)
			{
				// Sample Sky
				return accumulatedColor * SampleSkydome(ray);
			}

			float3 N = 0;
			float3 I = 0;
			float3 albedo = 0;
			Material mat = {};

			if (hitSphere) // If sphere is hit by ray
			{
				I = ray.O + ray.D * ray.t;
				N = hitSphere->GetNormal(I);
				albedo = hitSphere->color;
				mat = m_material_manager->Get(hitSphere->material);

				// Check if ray is inside of sphere
				if (dot(N, ray.D) > 0)
				{
					N = -N; // Flip normal to show glass from the inside
					ray.inside = true;
				}
				else
				{
					ray.inside = false;
				}
			}
			else
			{
				N = ray.GetNormal();
				I = ray.IntersectionPoint();
				uint8_t voxelIndex = (uint8_t)ray.voxel;
				albedo = scene.GetAlbedo(voxelIndex);
				mat = scene.GetMaterialData(voxelIndex);
			}

			if (mat.metallic > 0 && mat.transparency == 0)
			{
				//----------------
				// Mirror Material
				// ---------------
				float3 R = ray.D - 2.0f * N * dot(ray.D, N);

				// Trace multiple reflection rays for rough mirrors to reduce noise
				if (mat.roughness > 0)
				{
					const int samplesAmount = 2;
					float3 totalReflection(0);

					for (int s = 0; s < samplesAmount; s++)
					{
						// Use new random values instead of blueR1 and blueR2 because blue noise is only meant for 1 spp
						float r1 = RandomFloat();
						float r2 = RandomFloat();

						// Adds random offset to reflection based on mat.roughness (0 = perfect mirror, 1 = fully blurred)
						float3 sampledR = normalize(R + mat.roughness * StochasticSampling(N, r1, r2));

						// If random offset pushed ray below the surface, set it back to what it was above
						if (dot(sampledR, N) < 0)
						{
							sampledR = R;
						}

						Ray reflRay(I, sampledR);
						totalReflection += Trace(reflRay, frame, depth + 1, x, y);
					}

					return accumulatedColor * albedo * mat.metallic * (totalReflection / (float)samplesAmount);
				}
				else
				{
					// Perfect mirror, roughness = 0
					accumulatedColor *= albedo * mat.metallic;
					ray = Ray(I, R);
					continue;
				}
			}
			else if (mat.transparency > 0)
			{
				//----------------
				// Glass Material
				//----------------
				float n1 = 1.0f; // Air
				float n2 = mat.ior; // Glass

				// Swap indices if ray is inside of object
				if (ray.inside) swap(n1, n2);

				const float d = dot(N, -ray.D);
				const float eta = n1 / n2;
				const float k = 1 - eta * eta * (1 - pow2f(d));

				float3 refract = eta * ray.D + (eta * d - sqrtf(k)) * N;
				float3 reflect = ray.D + 2.0f * N * d;

				// Fresnel
				const float R0 = pow2f((n1 - n2) / (n1 + n2));
				float fresnel = k > 0 ? R0 + (1 - R0) * pow5f(1 - d) : 1;

				// Specular controls reflection strength
				fresnel *= mat.specular;

				const int samplesAmount = (depth == 0) ? 2 : 1;
				float3 blendedColor(0);

				for (int s = 0; s < samplesAmount; s++)
				{
					float3 sampledRefracted(0), sampledReflected(0);

					// Refraction (Snell's Law)
					if (k > 0)
					{
						float3 R = refract;

						if (mat.roughness > 0)
						{
							// Use new random values instead of blueR1 and blueR2 because blue noise is only meant for 1 spp
							float r1 = RandomFloat();
							float r2 = RandomFloat();

							// Adds random offset to surface to blur refracted ray (the higher mat.roughness is, the more blur will be applied)
							R = normalize(R + mat.roughness * StochasticSampling(-N, r1, r2));
						}

						Ray refractRay(I, R); // Passing new Ray into Trace() to prevent warning
						sampledRefracted = Trace(refractRay, frame, depth + 1, x, y);
					}

					// Reflection
					float3 R = reflect;

					if (mat.roughness > 0)
					{
						// Use new random values instead of blueR1 and blueR2 because blue noise is only meant for 1 spp
						float r1 = RandomFloat();
						float r2 = RandomFloat();

						// Adds random offset to reflection to blur reflected ray
						R = normalize(R + mat.roughness * StochasticSampling(N, r1, r2));

						// If random offset pushed ray below the surface, set it back to what it was above
						if (dot(R, N) < 0)
						{
							R = reflect;
						}
					}

					Ray reflectRay(I, R); // Passing new Ray into Trace() to prevent warning
					sampledReflected = Trace(reflectRay, frame, depth + 1, x, y);

					// Transparency controls how much refracted light goes through
					sampledRefracted *= mat.transparency;

					// Blend sampledReflected and sampledRefracted
					blendedColor += fresnel * sampledReflected + (1 - fresnel) * sampledRefracted;

					// Returns after all 4 samples are done
					return accumulatedColor * albedo * (blendedColor / (float)samplesAmount);
				}
			}
			else
			{
				//----------------
				// Diffuse Material
				//----------------
				float3 result = float3(0);

				result += m_light_manager->CalcPointLights(N, I, albedo);
				result += m_light_manager->CalcDirectionalLights(N, I, albedo);
				result += m_light_manager->CalcSpotLights(N, I, albedo);
				result += m_light_manager->CalcAreaLights(N, I, albedo, blueR1, blueR2);

				// Stochastic Skydome Lighting
				if (skydomeLighting)
				{
					float3 skyDir = StochasticSampling(N, blueR1, blueR2);
					Ray skyRay(I + N * 0.001f, skyDir);

					float3 skyLight = Trace(skyRay, frame, depth + 1);

					result += skyLight * albedo;
				}

				return accumulatedColor * result;
			}
		}

		// If ray bounced 5 times and hit nothing, return black color
		return float3(0);
	}

	bool Renderer::CalcShadows(const float3& origin, const float3& L, const float maxDistance)
	{
		Ray shadowRay(origin, L, maxDistance);
		return scene.IsOccluded(shadowRay);
	}


	float3 Renderer::SampleSkydome(const Ray& ray)
	{
		const uint u = (uint)(skyWidth * atan2f(ray.D.z, ray.D.x) * INV2PI - 0.5f);
		const uint v = (uint)(skyHeight * acosf(ray.D.y) * INVPI - 0.5f);
		const uint skyIdx = (u + v * skyWidth) % (skyWidth * skyHeight);
		return 0.65f * float3(skyPixels[skyIdx * 3], skyPixels[skyIdx * 3 + 1], skyPixels[skyIdx * 3 + 2]);
	}


	void Renderer::CalcDoF(Ray& ray)
	{
		const float3 forward = normalize(camera.camTarget - camera.camPos);
		const float rayFocal = focusDistance / dot(ray.D, forward);

		// Exact point in world space where the ray hits the voxel
		const float3 focalPoint = ray.O + ray.D * rayFocal;

		const float radius = sqrt(RandomFloat()) * aperture;
		const float theta = RandomFloat() * 2.0f * PI;

		// Convert polar coordinates to cartesian coordinates (guess the math lectures did help a bit)
		const float lensX = radius * cosf(theta);
		const float lensY = radius * sinf(theta);

		const float3 lensOffset = camera.right * lensX + camera.up * lensY;

		// Create a new ray that shoots with a random offset outside of the focalPoint
		const float3 newOrigin = ray.O + lensOffset;
		const float3 newDir = focalPoint - newOrigin;
		ray = Ray(newOrigin, newDir);
	}

	void Renderer::FocusDoF()
	{
		// Shoots ray from camera to pixel that mousePos is on, then finds nearest voxel
		Ray r = camera.GetPrimaryRay((float)mousePos.x, (float)mousePos.y);
		scene.FindNearest(r);

		if (r.voxel != 0)
		{
			// Position where the ray hit a voxel
			const float3 hitPos = r.O + r.D * r.t;

			// Camera forward direction
			const float3 forward = normalize(camera.camTarget - camera.camPos);

			// Sets focusDistance by calculating how far hitPos is from the camera
			focusDistance = dot(hitPos - camera.camPos, forward);

			dofEnabled = true;
			ResetAccumulator();
			printf("Focused Depth of Field: %f\n", focusDistance);
		}
	}

	float3 Renderer::StochasticSampling(const float3& N, float blueR1, float blueR2)
	{
		// Generate random number from 0-1 and then convert it to 360 degrees in radians with 2*PI 
		float a = blueR1 * 2 * PI;

		// Generate random number from 0-1 to be the vertical point on the sphere, then does "* 2 - 1" to normalize to -1 to +1
		float v = blueR2 * 2 - 1;

		// Calculates how wide sphere is at v
		float r = sqrtf(1 - v * v);

		// Convert spherical coordinates to point in world space
		float3 p = float3(r * cosf(a), r * sinf(a), v);

		// Returns random direction away from surface
		return dot(p, N) < 0 ? -p : p;
	}

	void Renderer::SnakeGameSetup()
	{
		// Create Red Area Light
		AreaLight a;
		a.pos = { 0.25f, 0.5f, -0.1f };
		a.color = float3(1.0f, 0.0f, 0.0f);
		a.intensity = 5.0f;
		a.size = 1.0f;
		areaLights.push_back(a);

		// Create Blue Area Light
		a.pos = { 0.75f, 0.5f, -0.1f };
		a.color = float3(0.0f, 0.0f, 1.0f);
		areaLights.push_back(a);
	}
	
	

	// -----------------------------------------------------------
	// Application initialization - Executed once, at app start
	// -----------------------------------------------------------
	void Renderer::Init()
	{
		// Initialize the scene with selected world 
		scene.GenerateScene(static_cast<SceneType>(scene.selected_world));

		// Load HDR Sky
		skyPixels = stbi_loadf("assets/skydome/puresky_4k.hdr", &skyWidth, &skyHeight, &skyBpp, 0);
		for (int i = 0; i < skyWidth * skyHeight * 3; i++)
			skyPixels[i] = sqrtf(skyPixels[i]);

		// Load Blue Noise
		blueNoise = new Surface("assets/BlueNoise.png");

		// Creates Colored Area Lights
		SnakeGameSetup();

		// Set accumulator
		accumulator = new float3[SCRWIDTH * SCRHEIGHT];
		history = new float3[SCRWIDTH * SCRHEIGHT];
		ResetAccumulator();

		m_audio_manager = new Audio_Manager();
		m_material_manager = new Material_Manager(this);
		m_snake = new Snake(this, m_audio_manager);
		camera.SetSnake(m_snake);
		m_ui_manager = new UI_Manager(this, m_material_manager, m_audio_manager, m_snake);
		m_light_manager = new Light_Manager(this);
		
		// Start Camera Catmull-Rom Spline
		camera.StartSpline(5.0f);
	}

	// -----------------------------------------------------------
	// Main application tick function - Executed every frame
	// -----------------------------------------------------------
	void Renderer::Tick(float dt)
	{
		// For Reprojection: Save current frustum and camPos before camera moves
		previous = camera.BuildFrustum();
		prevCamPos = camera.camPos;

		// Increment samples per pixel because its a new frame
		spp++;

		// high-resolution timer, see template.h
		Timer t;

		float dtConverted = dt * 0.001f; // Convert deltaTime to seconds (its ~11 without doing this, way too much)

		// Sphere Physics
		if (spherePhysics)
		{
			for (auto& sphere : spheres)
			{
				if (sphere.hasPhysics)
				{
					sphere.Physics(dtConverted, scene);
					if (sphere.velocity.y != 0)
					{
						ResetAccumulator();
					}
				}
			}
		}

		// Snake Movement
		if (!m_snake->snakePaused)
		{
			m_snake->Update(dtConverted);
		}

		// Frame Counter (Used for Blue Noise)
		static int frame = 0;
		frame++;

		// pixel loop: lines are executed as OpenMP parallel tasks (disabled in DEBUG)
#pragma omp parallel for schedule(dynamic)
		for (int y = 0; y < SCRHEIGHT; y++)
		{
			// trace a primary ray for each pixel on the line
			for (int x = 0; x < SCRWIDTH; x++)
			{
				Ray r = camera.GetPrimaryRay((float)x, (float)y);

				// Depth of Field
				if (dofEnabled) { CalcDoF(r); }

				// Anti-Aliasing
				scene.FindNearest(r);
				float stableT = r.t;
				uint stableVoxel = r.voxel;

				// RandomFloat() to get offset, so that it can be used to fix jittered voxels with anti-aliasing
				Ray jitteredRay = camera.GetPrimaryRay(x + RandomFloat(), y + RandomFloat());
				if (dofEnabled) CalcDoF(jitteredRay);

				// Create sample with jitteredRay
				float3 sample = Trace(jitteredRay, frame, 0, x, y);

				float3 historySample(0);
				float historyWeight = 0;
				bool reprojectionFailed = true;

				// If pixel hit a sphere or missed everything, call break;
				bool hitSphere = false;
				if (stableVoxel > 0 || stableT < 1e33f)
				{
					float3 P = r.O + stableT * r.D;
					for (auto& sphere : spheres)
					{
						// If the hit point is very close to any sphere surface, this pixel hit a sphere
						float dist = length(P - sphere.pos) - sphere.radius;
						if (fabsf(dist) < 0.01f)
						{
							hitSphere = true;
							break;
						}
					}
				}

				if (stableVoxel > 0 && !hitSphere)
				{
					float3 P = r.O + stableT * r.D; // P = point

					// Calculate the distance of P, outcome will be the location of P in the previous frame
					float dLeft = distance(previous.plane[0], P);
					float dRight = distance(previous.plane[1], P);
					float dUp = distance(previous.plane[2], P);
					float dDown = distance(previous.plane[3], P);

					// Calcuulate pixel location for P in previous frame
					float prev_x = (SCRWIDTH * dLeft) / (dLeft + dRight);
					float prev_y = (SCRHEIGHT * dUp) / (dUp + dDown);

					// Check if the pixel coordinates are actually on the screen
					if (prev_x >= 0 && prev_x < SCRWIDTH - 2 && prev_y >= 0 && prev_y < SCRHEIGHT - 2)
					{
						// Verify Reprojection
						Ray verifyRay(prevCamPos, P - prevCamPos);
						scene.FindNearest(verifyRay);
						float3 Q = prevCamPos + verifyRay.t * verifyRay.D;

						// Reprojection is succesful
						if (sqrLength(P - Q) < 0.001f)
						{
							historyWeight = 1.0f - REPROJECTION_ALPHA;
							reprojectionFailed = false;

							// Calculate fractional part of the pixel coordinate
							float fx = fracf(prev_x);
							float fy = fracf(prev_y);

							// Calculate weights for all 4 pixels
							float w0 = (1 - fx) * (1 - fy);
							float w1 = fx * (1 - fy);
							float w2 = (1 - fx) * fy;
							float w3 = fx * fy;

							// Read 2x2 block of pixels from history
							int a = (int)prev_x + (int)prev_y * SCRWIDTH;
							float3 p0 = history[a]; // (x, y)
							float3 p1 = history[a + 1]; // (x+1, y)
							float3 p2 = history[a + SCRWIDTH]; // (x, y+1)
							float3 p3 = history[a + SCRWIDTH + 1]; // (x+1, y+1)

							// Blend all 4 pixels by their weights
							historySample = p0 * w0 + p1 * w1 + p2 * w2 + p3 * w3;
						}
					}
				}

				if (reprojectionFailed)
				{
					// Trace 3 more samples because this pixel failed
					for (int sampleCount = 0; sampleCount < 3; sampleCount++)
					{
						Ray extraRay = camera.GetPrimaryRay((float)x, (float)y);

						if (dofEnabled)
						{
							CalcDoF(extraRay);
						}

						sample += Trace(extraRay, frame, 0, x, y);
					}

					// Average all 4 samples
					sample *= 0.25f;
				}

				// Blend history (95%) with new sample (5%) to create pixel
				float3 pixel = historyWeight * historySample + (1 - historyWeight) * sample;

				// Draw pixel to screen
				int pos = x + y * SCRWIDTH;
				accumulator[pos] = pixel;

				// Gamma Correction (Brightens colors)
				if (gammaCorrection)
				{
					screen->pixels[pos] = RGBF32_to_RGB8(float3(sqrtf(pixel.x), sqrtf(pixel.y), sqrtf(pixel.z)));
				}
				else
				{
					screen->pixels[pos] = RGBF32_to_RGB8(pixel);
				}
			}
		}

		// performance report - running average - ms, MRays/s
		deltaTime = dt;
		avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
		if (alpha > 0.05f) alpha *= 0.5f;
		fps = 1000.0f / avg;

		// Camera Spline
		camera.UpdateSpline(dtConverted);

		// Camera Movement
		if (!cameraLocked)
		{
			camera.HandleInput(deltaTime);
		}
		
		// Copy accumulator to history
		swap(history, accumulator);
	}

	void Renderer::ResetAccumulator()
	{
		spp = 1;
		memset(accumulator, 0, SCRWIDTH * SCRHEIGHT * sizeof(float3)); // Reset every pixel color back to float3(0) (black)
	}

	// -----------------------------------------------------------
	// Update user interface (imgui)
	// -----------------------------------------------------------
	void Renderer::UI()
	{
		if (UI_Enabled)
		{
			m_ui_manager->GUI();
		}
	}

	void Renderer::Shutdown()
	{
		delete[] accumulator;
		delete[] history;

		delete m_audio_manager;
		delete m_ui_manager;
		delete m_light_manager;
		delete m_material_manager;

		delete blueNoise;

		if (skyPixels)
		{
			stbi_image_free(skyPixels);
		}
	}

	void Renderer::KeyUp(int /*key*/)
	{
		/*if (key == KEY_F3)
		{
			printf("F3 pressed\n");
		}*/
	}

	void Renderer::KeyDown(int key)
	{
		//printf("KeyDown: %d\n", key); // Used to get virtual keycode of keys

		switch (key)
		{
			// Toggle Debug Window
			case KEY_F3:
				UI_Enabled = !UI_Enabled;
				printf("\nUI Enabled: %s\n", UI_Enabled ? "True" : "False");
				break;

			// Toggle Developer Debug
			case KEY_F5:
				m_ui_manager->developerDebug = !m_ui_manager->developerDebug;
				printf("\nDeveloper Debug Enabled: %s\n", m_ui_manager->developerDebug ? "True" : "False");
				break;

			// Focus Depth of Field
			case KEY_SPACEBAR:
				FocusDoF();
				break;

			// Lock Selected Voxel
			case KEY_L:
				if (!m_ui_manager->voxelLocked)
				{
					m_ui_manager->latestVoxel = m_ui_manager->ReturnVoxel();
					m_ui_manager->lockedHitVoxel = scene.hitVoxel;
				}
				m_ui_manager->voxelLocked = !m_ui_manager->voxelLocked;
				break;

			#pragma region Snake Inputs
			// Snake Movement, can only move around when snake is not paused
			case KEY_W:
				if (!m_snake->snakePaused)
					m_snake->Input(m_snake->UP); break;

			case KEY_S:
				if (!m_snake->snakePaused) 
					m_snake->Input(m_snake->DOWN); break;

			case KEY_A:
				if (!m_snake->snakePaused)
					m_snake->Input(m_snake->LEFT); break;

			case KEY_D:
				if (!m_snake->snakePaused) 
					m_snake->Input(m_snake->RIGHT); break;

			// Pause Snake
			case KEY_P:
				m_snake->snakePaused = !m_snake->snakePaused;
				break;
			#pragma endregion

			default:
				break;
		}
	}

}