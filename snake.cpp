#include "template.h"
#include "snake.h"
#include "renderer.h"
#include "mat4.h"

namespace Tmpl8
{

Snake::Snake(Renderer* r, Audio_Manager* audio_manager)
{
	m_renderer = r;
	m_audio_manager = audio_manager;

	CreateStartingSnake();
}

void Snake::Update(float deltaTime)
{
	// Death Flash (Snake turns red when dying)
	if (deathFlashActive)
	{
		deathFlashTimer += deltaTime;
		if (deathFlashTimer >= DEATH_FLASH_DURATION)
		{
			for (size_t i = 0; i < leftEyeIndex; i++) // Don't include the 2 eyes of the snake
			{
				m_renderer->spheres[i].color = originalColors[i]; // Set each sphere back to the original color
			}
			deathFlashActive = false;
		}
	}

	moveTimer += deltaTime;

	// 0.0 = moved, 1.0 = about to move
	float t = moveTimer / moveInterval;
	if (t > 1.0f) t = 1.0f;

	if (moveTimer >= moveInterval)
	{
		moveTimer -= moveInterval;

		// Next queued input
		if (inputCount > 0)
		{
			// Set new direction with queued input
			snakeDir = inputQueue[0];

			// Remove queued input from inputQueue
			inputQueue[0] = inputQueue[1];
			inputCount--;
		}

		// Save current positions to prevGridBody
		prevGridBody = gridBody;

		// Move snake body forwards, each segment sets their position to the one in front
		for (int i = snakeSize - 1; i > 0; i--)
		{
			gridBody[i] = gridBody[i - 1];
		}

		// Move snake head forwards by one cell
		gridBody[0].x += snakeDir.x;
		gridBody[0].y += snakeDir.y;

		// Wrap around grid (teleport to the other side if going past GRID_SIZE)
		gridBody[0].x = (gridBody[0].x + GRID_SIZE) % GRID_SIZE;
		gridBody[0].y = (gridBody[0].y + GRID_SIZE) % GRID_SIZE;

		// gridPos of snake head
		int2 head = gridBody[0];

		// Check if head collides with snake segments
		for (int i = 1; i < snakeSize; i++) // "int = 1;" so head doesn't check collision with itself
		{
			if (gridBody[i].x == head.x && gridBody[i].y == head.y)
			{
				GameOver();
				return; // Stop updating, Game Over!
			}
		}

		// Check if head is inside grid cell that contains food
		for (int i = 0; i < (int)foodPositions.size(); i++)
		{
			if (foodPositions[i].x == head.x && foodPositions[i].y == head.y)
			{
				EatFood(i);
				break;
			}
		}

		// Reset for next input
		t = 0.0f;
	}
	
	// Update world positions between prevGridBody and gridBody
	for (int i = 0; i < snakeSize; i++)
	{
		// Calculate worldPos of last step and next step
		float3 from = GridToWorld(prevGridBody[i].x, prevGridBody[i].y); // Last step
		float3 to = GridToWorld(gridBody[i].x, gridBody[i].y); // Next step

		// Calculate how many cells segment has moved
		int dx = gridBody[i].x - prevGridBody[i].x;
		int dy = gridBody[i].y - prevGridBody[i].y;

		// If segment has moved more than 1 cell, wrap around grid
		if (abs(dx) > 1 || abs(dy) > 1)
		{
			// Get movement direction
			int dirX = (abs(dx) > 1) ? -((dx > 0) - (dx < 0)) : ((dx > 0) - (dx < 0));
			int dirY = (abs(dy) > 1) ? -((dy > 0) - (dy < 0)) : ((dy > 0) - (dy < 0));
			
			// t < 0.5 so that it perfectly divides teleporting and movement into 2 halves (the before and after)
			if (t < 0.5f)
			{
				// Teleport to the other side of grid
				from = GridToWorld(prevGridBody[i].x, prevGridBody[i].y);
				to = from + float3(dirX * CELL_SIZE * 0.5f, dirY * CELL_SIZE * 0.5f, 0);
				worldBody[i] = from + (to - from) * (t * 2.0f);
			}
			else 
			{
				// Move into new cell after teleporting
				to = GridToWorld(gridBody[i].x, gridBody[i].y);
				from = to - float3(dirX * CELL_SIZE * 0.5f, dirY * CELL_SIZE * 0.5f, 0);
				worldBody[i] = from + (to - from) * ((t - 0.5f) * 2.0f);
			}
			
			m_renderer->spheres[i].pos = worldBody[i];
			continue;
		}
		
		worldBody[i] = from + (to - from) * t;
		m_renderer->spheres[i].pos = worldBody[i];
	}

	// Cute little snake eyes :>
	SnakeEyes();

	if (activeFood < MAX_FOOD)
	{
		SpawnFood();
	}
}

void Snake::Input(SnakeMovement input)
{
	int2 newDir = { 0, 0 };
	int2 lastDir = { 0, 0 };

	switch (input)
	{
		case UP:
			newDir = { 0, 1 };
			break;

		case DOWN:
			newDir = { 0, -1 };
			break;

		case LEFT:
			newDir = { -1, 0 };
			break;

		case RIGHT:
			newDir = { 1, 0 };
			break;
	}
	
	// Check last queued direction
	if (inputCount > 0)
	{
		lastDir = inputQueue[inputCount - 1];
	}
	else
	{
		lastDir = snakeDir;
	}
	
	// Prevent reversing into yourself
	if (newDir.x + lastDir.x != 0 || newDir.y + lastDir.y != 0)
	{
		if (inputCount < 2)
		{
			inputQueue[inputCount] = newDir;
			inputCount++;
		}
	}
}

void Snake::Grow()
{
	// Last sphere of snake body
	const int2 tail = gridBody[snakeSize - 1];
	const float3 tailWorld = worldBody[snakeSize - 1];

	// Add sphere to snake body
	gridBody.push_back(tail);
	prevGridBody.push_back(tail);
	worldBody.push_back(tailWorld);
	snakeSize++;
	score++;

	// Green gradient
	int step = snakeSize - 1;
	float green = GetGreen(step);
	float3 color = { 0.0f, green, 0.0f };

	// Create Sphere
	const int material = MIRROR;

	// Insert sphere before the eyes so it doesn't mess up the order
	std::vector<Sphere>::iterator insertPos = m_renderer->spheres.begin() + leftEyeIndex;

	// Spawn in sphere
	m_renderer->spheres.insert(insertPos, Sphere(tailWorld, RADIUS, color, material, false));
	m_renderer->ResetAccumulator();

	// Move both eye indexes to the right by 1 in index because of newly created sphere
	leftEyeIndex++;
	rightEyeIndex++;
}

void Snake::SpawnFood()
{
	// Grid position of where food will spawn
	int gridX = rand() % GRID_SIZE;
	int gridY = rand() % GRID_SIZE;

	// If random gridPos is inside of snake or food, randomize gridPos again
	while (InsideSnake(gridX, gridY) || InsideFood(gridX, gridY))
	{
		gridX = rand() % GRID_SIZE;
		gridY = rand() % GRID_SIZE;
	}

	// Save grid position of food inside of foodPositions
	foodPositions.push_back({ gridX, gridY });

	// Convert grid space to world space
	float3 worldPos = GridToWorld(gridX, gridY);

	// redCube_2x2x2.vox is 2 2 2 worldSize in MagicaVoxel
	int modelSizeX = 2;
	int modelSizeY = 2;
	int modelSizeZ = 2;

	// Convert world space to voxel space (worldPos * worldSize)
	int voxelX = (int)roundf(worldPos.x * m_renderer->scene.worldSize) - modelSizeX / 2;
	int voxelY = (int)roundf(worldPos.y * m_renderer->scene.worldSize) - modelSizeY / 2;
	int voxelZ = (int)roundf(worldPos.z * m_renderer->scene.worldSize) - modelSizeZ / 2;

	m_renderer->scene.PlaceVox("assets/vox/redCube_2x2x2.vox", voxelX, voxelY, voxelZ);
	m_renderer->ResetAccumulator();

	activeFood++;
}

void Snake::EatFood(int index)
{
	m_audio_manager->PlaySFX("assets/sfx/pickup_food_sfx.mp3", 1.0f);

	int2 food = foodPositions[index];

	float3 worldPos = GridToWorld(food.x, food.y);

	// redCube_2x2x2.vox is 2 2 2 worldSize in MagicaVoxel
	int modelSizeX = 2;
	int modelSizeY = 2;
	int modelSizeZ = 2;

	int voxelX = (int)roundf(worldPos.x * m_renderer->scene.worldSize) - modelSizeX / 2;
	int voxelY = (int)roundf(worldPos.y * m_renderer->scene.worldSize) - modelSizeY / 2;
	int voxelZ = (int)roundf(worldPos.z * m_renderer->scene.worldSize) - modelSizeZ / 2;

	// Remove voxel model
	m_renderer->scene.ClearVox(voxelX, voxelY, voxelZ, modelSizeX, modelSizeY, modelSizeZ);

	// Remove grid position of that food from foodPositions
	foodPositions.erase(foodPositions.begin() + index);

	activeFood--;
	
	Grow();
}

void Snake::GameOver()
{
	m_audio_manager->PlaySFX("assets/sfx/game_over_sfx.mp3", 1.0f);

	gameOver = true;
	snakePaused = true;

	if (score > highscore)
	{
		highscore = score;
	}

	// Store original colors of snake for when game resets
	originalColors.clear();
	for (size_t i = 0; i < leftEyeIndex; i++) // Don't include the 2 eyes of the snake
	{
		originalColors.push_back(m_renderer->spheres[i].color);
		m_renderer->spheres[i].color = float3(1.0f, 0.0f, 0.0f); // Set all spheres to red
	}

	deathFlashActive = true;
	deathFlashTimer = 0.0f;
	
	printf("\n---------------");
	printf("\nGame Over!\n");
	printf("\nScore: %d", score);
	printf("\nHigh Score: %d", highscore);
	printf("\n---------------\n");
}

void Snake::Reset()
{
	m_renderer->spheres.clear();
	gridBody.clear();
	prevGridBody.clear();
	worldBody.clear();

	// Call ClearVox() on active food to remove vox models
	for (int2& food : foodPositions)
	{
		float3 worldPos = GridToWorld(food.x, food.y);

		int modelSizeX = 2;
		int modelSizeY = 2;
		int modelSizeZ = 2;

		int voxelX = (int)roundf(worldPos.x * m_renderer->scene.worldSize) - modelSizeX / 2;
		int voxelY = (int)roundf(worldPos.y * m_renderer->scene.worldSize) - modelSizeY / 2;
		int voxelZ = (int)roundf(worldPos.z * m_renderer->scene.worldSize) - modelSizeZ / 2;

		m_renderer->scene.ClearVox(voxelX, voxelY, voxelZ, modelSizeX, modelSizeY, modelSizeZ);
	}
	foodPositions.clear();

	snakeSize = 0;
	score = 0;
	activeFood = 0;

	snakeDir = { 1, 0 };
	inputCount = 0;

	snakePaused = false;
	gameOver = false;

	CreateStartingSnake();
}

void Snake::CreateStartingSnake()
{
	// Snake starts in center of grid
	int startX = GRID_SIZE / 2;
	int startY = GRID_SIZE / 2;

	// Create snake body
	for (int i = 0; i < SNAKE_STARTING_SIZE; i++)
	{
		gridBody.push_back({ startX - i, startY });
		worldBody.push_back(GridToWorld(startX - i, startY));
		snakeSize++;
	}

	// Save current positions to prevGridBody
	prevGridBody = gridBody;

	// Sphere Variables
	float3 color = { 1.0f, 1.0f, 1.0f };
	int material = MIRROR;

	// Create Spheres
	for (int i = 0; i < SNAKE_STARTING_SIZE; i++)
	{
		if (i == 0) // Head of snake
		{
			color = { 0.0f, 1.0f, 0.0f };
		}
		else // Rest of the snake body will be a green gradient
		{
			// Green gradient (each sphere will be 0.05 less green than the last sphere)
			float green = GetGreen(i);
			color = { 0.0f, green, 0.0f };
		}

		// Create Sphere
		m_renderer->spheres.push_back(Sphere(worldBody[i], RADIUS, color, material, false));
	}

	// Creating left Snake Eye
	m_renderer->spheres.push_back(Sphere(float3(0), EYES_RADIUS, float3(1, 1, 1), material, false));
	leftEyeIndex = (int)m_renderer->spheres.size() - 1; // Save index of left Snake Eye

	// Creating right Snake Eye
	m_renderer->spheres.push_back(Sphere(float3(0), EYES_RADIUS, float3(1, 1, 1), material, false));
	rightEyeIndex = (int)m_renderer->spheres.size() - 1; // Save index of right Snake Eye

	SnakeEyes();

	m_renderer->ResetAccumulator();
}

float Snake::GetGreen(int step)
{
	// Green gradient (each sphere will be 0.05 less green than the last sphere)
	float green = 1.0f - (step * 0.05f);

	// Ping Pong effect with green being between 0.95 and 0.05
	while (green < 0.0f || green > 1.0f)
	{
		if (green < 0.0f)
		{
			green = -green;
		}
		if (green > 1.0f)
		{
			green = 2.0f - green;
		}
	}

	// Prevent dark green and black colors
	if (green < 0.3f)
	{
		green = 0.3f;
	}

	return green;
}

void Snake::SnakeEyes()
{
	// Variables used for Snake Eyes
	const float3 headPos = worldBody[0];
	const float3 forward = float3((float)snakeDir.x, (float)snakeDir.y, 0.0f);
	float3 eyeForward = forward;

	// Invert eyeForward direction if moving down (so that Snake Eyes face towards the camera)
	if (eyeForward.y < 0)
	{
		eyeForward.y = -eyeForward.y;
	}

	const float3 up = { 0, 1, 0 };
	float3 right;

	// When snake moves up or down, cross product will be 0
	if (fabsf(dot(eyeForward, up)) > 0.99f)
	{
		right = { 1, 0, 0 };
	}
	else
	{
		right = normalize(cross(up, eyeForward)); // Calculates what direction right is with how the snake is moving
	}

	float3 localUp = normalize(cross(eyeForward, right)); // Calculates what direction up is with how the snake is moving

	// Build a 4x4 matrix for snake head
	mat4 headMatrix = mat4::FromAxes(right, localUp, forward, headPos);

	// Update Snake Eyes pos with offset so that the right pos is set
	float3 leftEyeLocal = float3(EYES_OFFSET, EYES_OFFSET * 0.5f, RADIUS * 0.7f);
	float3 rightEyeLocal = float3(-EYES_OFFSET, EYES_OFFSET * 0.5f, RADIUS * 0.7f);

	// Transform localPos to worldPos using matrix
	m_renderer->spheres[leftEyeIndex].pos = headMatrix.TransformPoint(leftEyeLocal);
	m_renderer->spheres[rightEyeIndex].pos = headMatrix.TransformPoint(rightEyeLocal);
}

bool Snake::InsideSnake(int gridX, int gridY)
{
	for (int i = 0; i < snakeSize; i++)
	{
		if (gridBody[i].x == gridX && gridBody[i].y == gridY)
		{
			return true;
		}
	}

	return false;
}

bool Snake::InsideFood(int gridX, int gridY)
{
	for (int2& food : foodPositions)
	{
		if (food.x == gridX && food.y == gridY)
		{
			return true;
		}
	}

	return false;
}

}