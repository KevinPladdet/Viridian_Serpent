#pragma once

namespace Tmpl8
{

	class Renderer;
	class Audio_Manager;

	class Snake
	{
	public:

		enum SnakeMovement
		{
			UP,
			DOWN,
			LEFT,
			RIGHT
		};

		Snake(Renderer* r, Audio_Manager* audio_manager);
		
		void Update(float deltaTime); // Handles movement of snake among other things
		void Input(SnakeMovement input); // Inputs to change snakeDir
		
		void Grow(); // Increases snakeSize by +1
		void SpawnFood();
		void EatFood(int index);

		void GameOver(); // Gets called when Snake hits itself
		void Reset(); // Gets called when "Retry" button is pressed
		void CreateStartingSnake(); // Gets called in constructor and in Reset()

		float GetGreen(int step);
		void SnakeEyes();

		// Checks if gridPos from parameters is in a cell that's already occupied by snake segment or food
		bool InsideSnake(int gridX, int gridY);
		bool InsideFood(int gridX, int gridY);

		// Variables used by m_renderer
		bool gameOver = false;
		bool snakePaused = true; // When true, Update() isn't called anymore

		// Variables used by m_ui_manager
		int score = 0;
		int highscore = 0;

	private:
		
		// Death Flash (Snake turns red when dying)
		bool deathFlashActive = false;
		float deathFlashTimer = 0.0f;
		static constexpr float DEATH_FLASH_DURATION = 0.3f;
		std::vector<float3> originalColors;

		// Snake Body
		std::vector<int2> gridBody; // Grid pos: X Y
		std::vector<int2> prevGridBody; // Previous frame grid positions
		std::vector<float3> worldBody; // World pos for spheres: X Y Z

		// Snake Direction
		int2 snakeDir = { 1, 0 }; // Current movement direction
		int2 inputQueue[2]; // Remembers next 2 upcoming inputs
		int inputCount = 0; // How many inputs are queued

		// Snake Movement
		float moveTimer = 0.0f; // Time since last moved
		float moveInterval = 0.2f; // 0.2 seconds between each move, 5 cells per second

		// Snake Size
		static constexpr float RADIUS = 0.025f; // How big each body sphere is
		int snakeSize = 0; // Total size of snake (in spheres)
		static constexpr int SNAKE_STARTING_SIZE = 4; // How big the snake is at the start

		// Grid
		static constexpr int GRID_SIZE = 15; // 15 x 15 grid
		static constexpr float CELL_SIZE = RADIUS * 2.0; // Size of each cell in grid
		float3 gridPos = { 0.5f - GRID_SIZE / 2.0f * CELL_SIZE, 0.5f - GRID_SIZE / 2.0f * CELL_SIZE, 0.5f }; // Bottom left corner of cell is 0 0 in world space

		// Converts Grid pos (X Y) to World pos (X Y Z)
		float3 GridToWorld(int gridX, int gridY)
		{
			return gridPos + float3((gridX + 0.5f) * CELL_SIZE, (gridY + 0.5f) * CELL_SIZE, 0.0f);
		}

		// Food
		std::vector<int2> foodPositions; // Stores grid positions of active food
		int activeFood = 0; // Amount of food that is currently on grid
		static constexpr int MAX_FOOD = 1; // Max amount of food that is on grid at once
		
		// Snake Eyes
		int leftEyeIndex = 4; // Index of left eye sphere in renderer->spheres
		int rightEyeIndex = 4; // Index of right eye sphere in renderer->spheres
		static constexpr float EYES_RADIUS = 0.01f; // How big the eyes are
		static constexpr float EYES_OFFSET = 0.015f; // How far apart the snake eyes are from eachother

		Renderer* m_renderer;
		Audio_Manager* m_audio_manager;
	};

}