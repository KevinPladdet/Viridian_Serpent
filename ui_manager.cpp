#include "template.h"
#include "ui_manager.h"
#include "renderer.h"
#include "material_manager.h"

namespace Tmpl8
{

	UI_Manager::UI_Manager(Renderer* r, Material_Manager* mat_manager, Audio_Manager* audio_manager, Snake* snake)
	{
		m_renderer = r;
		m_material_manager = mat_manager;
		m_audio_manager = audio_manager;
		m_snake = snake;
	}

	void UI_Manager::GUI()
	{
		// Snake Game Over Menu
		if (m_snake->gameOver)
		{
			// Get display size
			ImVec2 displaySize = ImGui::GetIO().DisplaySize;

			// Set window size
			ImVec2 windowSize = ImVec2(265, 185);
			ImGui::SetNextWindowSize(windowSize);

			// Put window in center of screen
			ImGui::SetNextWindowPos(ImVec2((displaySize.x - windowSize.x) * 0.5f, (displaySize.y - windowSize.y) * 0.5f), ImGuiCond_Always);

			// Change window background opacity
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.75f)); // 0.5 = 50% opacity

			// Begin window
			ImGui::Begin("Game Over Menu", nullptr,
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoScrollbar);

			// Game Over Text
			ImGui::SetWindowFontScale(2.0f);
			ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "Game Over!");

			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); // Spacing downwards

			// Scores
			ImGui::SetWindowFontScale(1.5f);
			ImGui::Text("Score: %i", m_snake->score);
			ImGui::Text("High Score: %i", m_snake->highscore);
			
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); // Spacing downwards

			// Retry Button
			if (ImGui::Button("Retry", ImVec2(120, 0)))
			{
				m_audio_manager->PlaySFX("assets/sfx/ui_button_sfx.mp3", 1.0f);
				m_snake->Reset();
			}

			ImGui::SameLine();

			// Quit Button
			if (ImGui::Button("Quit", ImVec2(120, 0)))
			{
				m_audio_manager->PlaySFX("assets/sfx/ui_button_sfx.mp3", 1.0f);
				glfwSetWindowShouldClose(m_renderer->m_window, GLFW_TRUE);
			}

			ImGui::End();
		}

		bool spacing = true;

		// Create ImGui window in top left
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);

		ImGui::Begin("Debug", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar);

		if (ImGui::BeginTabBar("Tabs"))
		{
			// Snake Tab
			if (ImGui::BeginTabItem("Snake"))
			{
				// Display FPS and FrameTime
				ImGui::Text("FPS: %.0f", m_renderer->fps);
				float frameTime = m_renderer->deltaTime;
				ImGui::Text("FrameTime: %.2f ms", frameTime);

				ImGuiCategory("Audio", spacing);

				ImGui::PushItemWidth(150); // Slider width to 150px
				// SFX Slider
				if (ImGui::SliderFloat("SFX", &m_audio_manager->sfxVolume, 0.0f, 100.0f, "%.0f%%"))
				{
					m_audio_manager->ChangeVolumes();
				}
				// Music Slider
				if (ImGui::SliderFloat("Music", &m_audio_manager->musicVolume, 0.0f, 100.0f, "%.0f%%"))
				{
					m_audio_manager->ChangeVolumes();
				}
				ImGui::PopItemWidth(); // Reset Slider width to default

				ImGui::EndTabItem();
			}
			
			if (developerDebug)
			{
				// Debugging Tab
				if (ImGui::BeginTabItem("Debug"))
				{
					ShowFPS();

					ImGui::Text("spp: %i", m_renderer->spp);

					if (ImGui::DragFloat3("camPos", &m_renderer->camera.camPos.x, 0.001f))
					{
						m_renderer->ResetAccumulator();
					}

					ImGui::DragFloat("camSpeed", &m_renderer->camera.camSpeed, 0.00005f, 0.0005f, 0.005f, "%.4f");

					ImGui::Checkbox("Camera Locked", &m_renderer->cameraLocked);

					// Lights Category
					ImGuiCategory("Lights", spacing);
					if (ImGui::Button("Create Point Light"))
					{
						const float3 color = float3(1.0f, 1.0f, 1.0f);
						m_renderer->pointLights.push_back({ m_renderer->camera.camPos, color });
						m_renderer->ResetAccumulator();
					}

					if (ImGui::Button("Create Spot Light"))
					{
						SpotLight s;
						s.pos = m_renderer->camera.camPos;
						s.dir = normalize(m_renderer->camera.camTarget - m_renderer->camera.camPos);
						s.color = float3(1.0f, 1.0f, 1.0f);
						m_renderer->spotLights.push_back(s);
						m_renderer->ResetAccumulator();
					}

					if (ImGui::Button("Create Area Light"))
					{
						AreaLight a;
						a.pos = m_renderer->camera.camPos;
						a.color = float3(1.0f, 1.0f, 1.0f);
						a.intensity = 2.0f;
						a.size = 1.0f;
						m_renderer->areaLights.push_back(a);
						m_renderer->ResetAccumulator();
					}

					if (m_renderer->directionalLights.size() == 0)
					{
						if (ImGui::Button("Create Directional Light"))
						{
							const float3 color = float3(1.0f, 1.0f, 1.0f);
							const float3 dir = normalize(float3(-0.25f, -0.25f, 1.0f)); // Will shine light on the right face and a quarter on both the back and bottom face
							m_renderer->directionalLights.push_back({ dir, color });
							m_renderer->ResetAccumulator();
						}
					}
					else
					{
						if (ImGui::Button("Delete Directional Light"))
						{
							m_renderer->directionalLights.erase(m_renderer->directionalLights.begin() + 0);
							m_renderer->ResetAccumulator();
						}
					}

					// Spheres Category
					ImGuiCategory("Spheres", spacing);

					static int sphereMaterial = DIFFUSE;
					if (ImGui::Button("Create Sphere With Material:"))
					{
						const float3 pos = m_renderer->camera.camPos;
						const float radius = 0.05f;
						float3 color = float3(1);
						// Randomize color if material is not glass
						if (sphereMaterial != GLASS)
						{
							color = float3(RandomFloat(), RandomFloat(), RandomFloat());
						}
						const bool hasPhysics = true;
						m_renderer->spheres.push_back(Sphere(pos, radius, color, sphereMaterial, hasPhysics));
						m_renderer->ResetAccumulator();
					}
					ImGui::SameLine();
					ImGui::Combo("##sphereMaterial", &sphereMaterial, materialNames, 3);

					if (ImGui::Button("Create 1000 Spheres"))
					{
						float3 color = float3(1);
						const float radius = 0.05f;

						const int gridSize = 32; // 32 x 32 = 1024 spheres
						const float gridSpacing = 0.2f;

						for (int x = 0; x < gridSize; x++)
						{
							for (int y = 0; y < gridSize; y++)
							{
								// Randomize color if material is not glass
								if (sphereMaterial != GLASS)
								{
									color = float3(RandomFloat(), RandomFloat(), RandomFloat());
								}

								float3 pos = { 0.0f, 0.0f, 0.0f };

								pos.x = x * gridSpacing;
								pos.y = y * gridSpacing;

								const bool hasPhysics = false;

								m_renderer->spheres.push_back(Sphere(pos, radius, color, sphereMaterial, hasPhysics));
							}
						}
						m_renderer->ResetAccumulator();
					}
					if (!m_renderer->spheres.empty())
					{
						ImGui::SameLine();
						if (ImGui::Button("Delete All Spheres"))
						{
							m_renderer->spheres.clear();
							m_renderer->ResetAccumulator();
						}
					}

					if (ImGui::Checkbox("Sphere Physics", &m_renderer->spherePhysics))
					{
						m_renderer->ResetAccumulator();
					}

					ImGui::Text("Total Spheres: %i", m_renderer->spheres.size());


					// Other Category
					ImGuiCategory("Other", spacing);

					// FOV
					if (ImGui::DragFloat("FOV", &m_renderer->camera.FOV, 0.1f, 30.0f, 110.0f, "%.0f"))
					{
						m_renderer->ResetAccumulator();
					}

					// Selected world
					if (ImGui::Combo("Selected World", &m_renderer->scene.selected_world, SceneTypeNames, static_cast<int>(SceneType::COUNT)))
					{
						m_renderer->scene.GenerateScene(static_cast<SceneType>(m_renderer->scene.selected_world));
						m_renderer->ResetAccumulator();
					}

					// Gamma Correction
					if (ImGui::Checkbox("Gamma Correction", &m_renderer->gammaCorrection))
					{
						m_renderer->ResetAccumulator();
					}

					// Stochastic Skydome Lighting
					if (ImGui::Checkbox("Stochastic Skydome Lighting", &m_renderer->skydomeLighting))
					{
						m_renderer->ResetAccumulator();
					}

					// Selected Voxel
					if (!voxelLocked)
					{
						latestVoxel = ReturnVoxel();
					}
					ImGui::Text("voxel: %i %s", latestVoxel, voxelLocked ? "(Locked)" : "");

					if (latestVoxel != 0)
					{
						ImGui::SameLine();
						const uint8_t voxelIndex = (uint8_t)latestVoxel;
						int currentMaterial = m_renderer->scene.GetMaterialType(voxelIndex);

						if (ImGui::Combo("##voxelMaterial", &currentMaterial, materialNames, 3))
						{
							const uint color = m_renderer->scene.palette[voxelIndex].color;
							const uint8_t newIndex = m_renderer->scene.Voxel((MaterialType)currentMaterial, color);
							m_renderer->scene.Set(lockedHitVoxel.x, lockedHitVoxel.y, lockedHitVoxel.z, newIndex);
							latestVoxel = newIndex;
							m_renderer->ResetAccumulator();
						}
					}


					// Depth of Field Category
					ImGuiCategory("Depth of Field", spacing);

					if (ImGui::DragFloat("Aperture", &m_renderer->aperture, 0.0003f, 0.0f, 0.5f))
					{
						m_renderer->ResetAccumulator();
					}

					ImGui::BeginDisabled(!m_renderer->dofEnabled);
					if (ImGui::Button("Clear DoF"))
					{
						m_renderer->dofEnabled = false;
						m_renderer->ResetAccumulator();
					}
					ImGui::EndDisabled();

					ImGui::SameLine();

					ImGui::BeginDisabled();
					ImGui::Checkbox("Depth of Field", &m_renderer->dofEnabled);
					ImGui::EndDisabled();

					ImGui::EndTabItem();
				}


				// Show Lights tab only if a light is created
				if (!m_renderer->directionalLights.empty() || !m_renderer->pointLights.empty() || !m_renderer->spotLights.empty() || !m_renderer->areaLights.empty())
				{
					if (ImGui::BeginTabItem("Lights"))
					{
						#pragma region Directional Lights
						for (int i = 0; i < m_renderer->directionalLights.size(); i++)
						{
							ImGuiCategory("Directional Light (Sun)", spacing);

							if (ImGui::DragFloat3("Direction", &m_renderer->directionalLights[i].dir.x, 0.02f, -1.0f, 1.0f))
							{
								m_renderer->directionalLights[i].dir = normalize(m_renderer->directionalLights[i].dir);
								m_renderer->ResetAccumulator();
							}

							if (ImGui::ColorEdit3("Color", &m_renderer->directionalLights[i].color.x))
							{
								m_renderer->directionalLights[i].color = clamp(m_renderer->directionalLights[i].color, 0.0f, 1.0f);
								m_renderer->ResetAccumulator();
							}
						}
						#pragma endregion

						#pragma region Point Lights
						// Spacing
						if (!m_renderer->pointLights.empty()) { ImGuiCategory("", spacing); }

						ImGui::PushID("PointLights");

						for (int i = 0; i < m_renderer->pointLights.size(); i++)
						{
							// Using PushID and PopID because otherwise when changing a value, it will change it for every point light
							ImGui::PushID(i);

							if (i != 0) ImGui::Separator();
							ImGui::Text("Point Light %d", (i + 1));

							// Position
							if (ImGui::DragFloat3("Pos", &m_renderer->pointLights[i].pos.x, 0.1f, 0.0f, 0.0f, "%.1f"))
							{
								m_renderer->ResetAccumulator();
							}

							// Color
							if (ImGui::ColorEdit3("Color", &m_renderer->pointLights[i].color.x))
							{
								m_renderer->pointLights[i].color = clamp(m_renderer->pointLights[i].color, 0.0f, 1.0f);
								m_renderer->ResetAccumulator();
							}

							// Delete Button
							if (ImGui::Button("Delete"))
							{
								m_renderer->pointLights.erase(m_renderer->pointLights.begin() + i);
								m_renderer->ResetAccumulator();
								ImGui::PopID();
								break;
							}
							ImGui::PopID();

							ImGui::Spacing(); ImGui::Spacing();
						}
						ImGui::PopID();
						#pragma endregion

						#pragma region Spot Lights
						// Spacing
						if (!m_renderer->spotLights.empty()) { ImGuiCategory("", spacing); }

						ImGui::PushID("SpotLights");

						for (int i = 0; i < m_renderer->spotLights.size(); i++)
						{
							// Using PushID and PopID because otherwise when changing a value, it will change it for every spot light
							ImGui::PushID(i);

							if (i != 0) ImGui::Separator();
							ImGui::Text("Spot Light %d", (i + 1));
							// Position
							if (ImGui::DragFloat3("Pos", &m_renderer->spotLights[i].pos.x, 0.1f, 0.0f, 0.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}
							// Direction
							if (ImGui::DragFloat3("Direction", &m_renderer->spotLights[i].dir.x, 0.02f, -1.0f, 1.0f, "%.2f"))
							{
								m_renderer->spotLights[i].dir = normalize(m_renderer->spotLights[i].dir);
								m_renderer->ResetAccumulator();
							}
							// Color
							if (ImGui::ColorEdit3("Color", &m_renderer->spotLights[i].color.x))
							{
								m_renderer->spotLights[i].color = clamp(m_renderer->spotLights[i].color, 0.0f, 1.0f);
								m_renderer->ResetAccumulator();
							}

							// Delete Button
							if (ImGui::Button("Delete"))
							{
								m_renderer->spotLights.erase(m_renderer->spotLights.begin() + i);
								m_renderer->ResetAccumulator();
								ImGui::PopID();
								break;
							}
							ImGui::PopID();

							ImGui::Spacing(); ImGui::Spacing();
						}
						ImGui::PopID();
						#pragma endregion

						#pragma region Area Lights
						// Spacing
						if (!m_renderer->areaLights.empty()) { ImGuiCategory("", spacing); }

						ImGui::PushID("AreaLights");

						for (int i = 0; i < m_renderer->areaLights.size(); i++)
						{
							// Using PushID and PopID because otherwise when changing a value, it will change it for every area light
							ImGui::PushID(i);

							if (i != 0) ImGui::Separator();
							ImGui::Text("Area Light %d", (i + 1));

							// Position
							if (ImGui::DragFloat3("Pos", &m_renderer->areaLights[i].pos.x, 0.1f, 0.0f, 0.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}

							// Color
							if (ImGui::ColorEdit3("Color", &m_renderer->areaLights[i].color.x))
							{
								m_renderer->areaLights[i].color = clamp(m_renderer->areaLights[i].color, 0.0f, 1.0f);
								m_renderer->ResetAccumulator();
							}

							// Intensity
							if (ImGui::DragFloat("Intensity", &m_renderer->areaLights[i].intensity, 0.1f, 0.0f, 5.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}

							// Delete Button
							if (ImGui::Button("Delete"))
							{
								m_renderer->areaLights.erase(m_renderer->areaLights.begin() + i);
								m_renderer->ResetAccumulator();
								ImGui::PopID();
								break;
							}
							ImGui::PopID();

							ImGui::Spacing(); ImGui::Spacing();
						}
						ImGui::PopID();
						#pragma endregion

						ImGui::EndTabItem();
					}
				}

				#pragma region Spheres Tab
				if (!m_renderer->spheres.empty())
				{
					if (ImGui::BeginTabItem("Spheres"))
					{
						ImGui::PushID("Spheres");
						// Each spot light gets a ColorEdit3 and delete button
						for (int i = 0; i < m_renderer->spheres.size(); i++)
						{
							// Using PushID and PopID because otherwise when changing a ColorEdit3 value, it will change it for every sphere
							ImGui::PushID(i);

							if (i != 0) ImGui::Separator();
							ImGui::Text("Sphere %d", (i + 1));

							// Position
							if (ImGui::DragFloat3("Pos", &m_renderer->spheres[i].pos.x, 0.01f))
							{
								m_renderer->ResetAccumulator();
							}

							// Radius
							if (ImGui::DragFloat("Radius", &m_renderer->spheres[i].radius, 0.01f, 0.01f, 10.0f))
							{
								m_renderer->ResetAccumulator();
							}

							// Color
							if (ImGui::ColorEdit3("Color", &m_renderer->spheres[i].color.x))
							{
								m_renderer->ResetAccumulator();
							}

							// Material
							if (ImGui::Combo("Material", &m_renderer->spheres[i].material, materialNames, 3))
							{
								m_renderer->ResetAccumulator();
							}

							// Ray Physics
							if (ImGui::Checkbox("Physics", &m_renderer->spheres[i].hasPhysics))
							{
								m_renderer->ResetAccumulator();
								ImGui::PopID();
							}

							// Delete Button
							if (ImGui::Button("Delete"))
							{
								m_renderer->spheres.erase(m_renderer->spheres.begin() + i);
								m_renderer->ResetAccumulator();
								ImGui::PopID();
								break;
							}
							ImGui::PopID();
							ImGui::Spacing(); ImGui::Spacing();
						}
						ImGui::PopID();
						ImGui::EndTabItem();
					}
				}
				#pragma endregion

				#pragma region Materials Tab
				if (ImGui::BeginTabItem("Materials"))
				{
					for (int i = 1; i < MAT_COUNT; i++) // int = 1, so DIFFUSE gets skipped
					{
						ImGui::PushID(i);
						Material& mat = m_material_manager->materials[i];
						const Material& original = m_material_manager->defaults[i];

						ImGuiCategory(mat.name, spacing);

						if (original.metallic > 0)
						{
							if (ImGui::SliderFloat("Metallic", &mat.metallic, 0.0f, 1.0f, "%.2f"))
								m_renderer->ResetAccumulator();
						}

						if (original.transparency > 0)
						{
							if (ImGui::SliderFloat("Transparency", &mat.transparency, 0.0f, 1.0f, "%.2f"))
								m_renderer->ResetAccumulator();
						}

						if (ImGui::SliderFloat("IOR", &mat.ior, 1.0f, 3.0f, "%.2f"))
							m_renderer->ResetAccumulator();

						if (ImGui::SliderFloat("Specular", &mat.specular, 0.0f, 1.0f, "%.2f"))
							m_renderer->ResetAccumulator();

						if (ImGui::SliderFloat("Roughness", &mat.roughness, 0.0f, 1.0f, "%.2f"))
							m_renderer->ResetAccumulator();

						ImGui::PopID();
					}

					ImGuiCategory("Palette Materials", spacing);

					// Unique materials from voxel palette indexes
					for (int i = 1; i < 256; i++)
					{
						Scene::VoxelInfo& palette = m_renderer->scene.palette[i];

						// Skip empty slots
						if (palette.color == 0 && palette.metallic == 0 && palette.transparency == 0
							&& palette.specular == 0 && palette.roughness == 0) continue;
						// Skip diffuse slots
						if (palette.metallic == 0 && palette.transparency == 0
							&& palette.specular == 0 && palette.roughness <= 1.0f && palette.ior <= 1.0f) continue;

						ImGui::PushID(i);

						// Make a char that says which type the material is (GLASS, MIRROR or DIFFUSE)
						const char* type = palette.transparency > 0 ? "Glass" : (palette.metallic > 0 ? "Mirror" : "Diffuse");

						// Color Preview (Shows left of "Palette %d (%s)" text)
						float r = ((palette.color >> 16) & 255) / 255.0f;
						float g = ((palette.color >> 8) & 255) / 255.0f;
						float b = (palette.color & 255) / 255.0f;
						ImVec4 col(r, g, b, 1.0f);
						ImGui::ColorButton("##color", col, 0, ImVec2(14, 14));
						ImGui::SameLine();

						// Palette name text (snprintf is a clickable dropdown menu)
						char label[64];
						snprintf(label, sizeof(label), "Palette %d (%s)", i, type);

						if (ImGui::TreeNode(label))
						{
							if (ImGui::SliderFloat("Metallic", &palette.metallic, 0.0f, 1.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}
							if (ImGui::SliderFloat("Transparency", &palette.transparency, 0.0f, 1.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}
							if (ImGui::SliderFloat("IOR", &palette.ior, 1.0f, 3.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}
							if (ImGui::SliderFloat("Specular", &palette.specular, 0.0f, 2.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}
							if (ImGui::SliderFloat("Roughness", &palette.roughness, 0.0f, 1.0f, "%.2f"))
							{
								m_renderer->ResetAccumulator();
							}

							ImGui::TreePop();
						}

						ImGui::PopID();
					}

					ImGui::EndTabItem();
				}
				#pragma endregion
			}
		}
	}

	void UI_Manager::ImGuiCategory(const char* category, bool spacing)
	{
		if (spacing)
		{
			ImGui::Separator(); // Puts a grey line in debug window
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); // Spacing downwards
		}

		// Create new category
		ImGui::Text(category);
		ImGui::Separator();
	}

	void UI_Manager::ShowFPS()
	{
		static float fpsGraph[200] = {};
		static int fpsIndex = 0;

		fpsGraph[fpsIndex] = m_renderer->fps;
		fpsIndex = (fpsIndex + 1) % 200;

		static const float maxFPS = 240.0f;
		static const float padding = 130.0f;

		ImGui::Text("FPS: %.0f", m_renderer->fps);

		float frameTime = m_renderer->deltaTime;
		ImGui::Text("FrameTime: %.2f ms", frameTime);

		ImGui::PlotLines("##FPS Graph", fpsGraph, 200, fpsIndex, "FPS Graph", 0.0f, maxFPS + padding, ImVec2(0, 50));
	}

	uint UI_Manager::ReturnVoxel()
	{
		Ray r = m_renderer->camera.GetPrimaryRay((float)m_renderer->mousePos.x, (float)m_renderer->mousePos.y);
		m_renderer->scene.FindNearest(r);
		return uint(r.voxel);
	}

}