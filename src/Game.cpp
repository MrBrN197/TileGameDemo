#include "Game.h"

void FillSoundBuffer(int16_t *samples, int32_t sampleCount)
{
	int32_t toneHz = 440;
	int32_t period = 48000 / 440;
	int32_t volume = 50;

	const float delta = (2 * PI) / (float)period;

	int32_t blocks = sampleCount / 2;
	int16_t *iter = samples;
	for (int i = 0; i < blocks; i++)
	{
		//t += delta;
		float sinT = 1; //sinf(t);
		int16_t sampleValue = (int16_t)(sinT * volume);
		*iter++ = sampleValue;
		*iter++ = sampleValue;
	}
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	//FillSoundBuffer(soundBuffer->samples, soundBuffer->sampleCount);
}

internal void RenderWierdGradient(game_back_buffer *buffer, int32_t xoffset, int32_t yoffset)
{
	uint8_t *row = (uint8_t *)buffer->memory;

	for (int x = 0; x < buffer->height; x++)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int y = 0; y < buffer->width; y++)
		{
			uint8_t green = x + yoffset;
			uint8_t blue = y + xoffset;
			*pixel++ = (green << 8) | blue;
		}
		row += buffer->pitch;
	}
}
internal void DrawRectangle(game_back_buffer *buffer, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY, float R, float G, float B)
{	
	uint8_t *row = (uint8_t *)buffer->memory;

	if (MinX < 0)
		MinX = 0;
	if (MinY < 0)
		MinY = 0;
	if (MaxX >= buffer->width)
		MaxX = buffer->width;
	if (MaxY >= buffer->height)
		MaxY = buffer->height;

	row += (buffer->pitch) * MinY;
	row += MinX * (buffer->bytesPerPixel);

	for (uint32_t y = 0; y < (MaxY - MinY); y++)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (uint32_t x = 0; x < (MaxX - MinX); x++)
		{
			int32 r = R * 255;
			int32 g = G * 255;
			int32 b = B * 255;

			*pixel++ = ((255 << 24) | (r << 16) | (g << 8) | b);
		}
		row += buffer->pitch;
	}
}

inline int32
FloorReal32ToInt32(real32 n)
{
	if(n < 0){
		n -= 1;
	}
	return (int32)n;
}

inline void RecanonicalizeCoord(world *World, int32 *TileMap, int32 *Tile, real32 *TileRel, int32 TileCount)
{
	int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInPixels);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInPixels;	

	Offset = FloorReal32ToInt32(*Tile/(real32)TileCount);
	*TileMap += Offset;
	*Tile -= Offset * TileCount;

	ASSERT(*Tile < TileCount);
	ASSERT(*TileMap < 2);
	ASSERT(*TileRel < World->TileSideInPixels);
}

inline canonical_position
RecanonicalizePosition(world *World, canonical_position Pos)
{

	canonical_position Result = Pos;

	RecanonicalizeCoord(World, &Result.TileMapX, &Result.TileX, &Result.TileRelX, World->CountX);
	RecanonicalizeCoord(World, &Result.TileMapY, &Result.TileY, &Result.TileRelY, World->CountY);

	return Result;
}

inline tile_map*
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
	ASSERT(TileMapX >= 0);
	ASSERT(TileMapY >= 0);
	ASSERT(TileMapX < World->TileMapCountX);
	ASSERT(TileMapY < World->TileMapCountY);

	tile_map *TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
	return TileMap;
}

#include <Windows.h>
inline bool32	
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	int32 Result = TileMap->Tiles[TileY * World->CountX + TileX];
	return (Result == 0);
}

internal bool32
IsWorldPointEmpty(world *World, canonical_position CanPos)
{
	tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
	bool32 Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, World->CountY - CanPos.TileY - 1);
	return Empty;
}

inline uint32
GetTileValueUnchecked(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	ASSERT(TileX >= 0);
	ASSERT(TileY >= 0);
	ASSERT(TileX < World->CountX);
	ASSERT(TileY < World->CountY);

	int32 Result = TileMap->Tiles[TileY * World->CountX + TileX];
	return Result;
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{

#define TILE_MAP_COUNT_X 19
#define TILE_MAP_COUNT_Y 7

	tile_map TileMaps[2][2];

	int32 TileMap00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0},
		{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	int32 TileMap01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	int32 TileMap10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}
	};
	int32 TileMap11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}
	};

	ASSERT(sizeof(game_state) <= memory->permanentStorageSize);

	game_state *GameState = (game_state *)memory->permanentStorage;

	real32 PlayerWidth = 0.75f * GameState->World.TileSideInPixels;
	real32 PlayerHeight = (real32)GameState->World.TileSideInPixels;

	TileMaps[0][0].Tiles = (int32*)TileMap10;
	TileMaps[0][1].Tiles = (int32*)TileMap11;
	TileMaps[1][0].Tiles = (int32*)TileMap00;
	TileMaps[1][1].Tiles = (int32*)TileMap01;
	
	if (!memory->isInitialized)
	{
		world World;
		World.TileMapCountX = 2;
		World.TileMapCountY = 2;
		World.CountX = TILE_MAP_COUNT_X;
		World.CountY = TILE_MAP_COUNT_Y;
		World.TileSideInPixels = 60;
		World.TileSideInMeters = 1.4f;

		World.UpperLeftX = -(real32)World.TileSideInPixels / 2;
		World.UpperLeftY = 0;

		GameState->World = World;

		char *filename = __FILE__;
		debug_read_file_result *file = (debug_read_file_result *)memory->DEBUGPlatformReadEntireFile(nullptr, filename);

		GameState->toneHz = 512;
		GameState->tSine = 0.0f;

		GameState->PlayerP.TileMapX = 0;
		GameState->PlayerP.TileMapY = 0;
		GameState->PlayerP.TileX = 3;
		GameState->PlayerP.TileY = 3;
		GameState->PlayerP.TileRelX = 30.f;
		GameState->PlayerP.TileRelY = 30.f;

		memory->isInitialized = true;
	}

	GameState->World.TileMaps = (tile_map*)TileMaps;

	for (int controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
	{
		const game_controller_input *controller = &input->controllers[controllerIndex];
		if (controller->isAnalog)
		{
			//GameState->xoffset += controller->stickAverageX * 4.0f;
			//GameState->yoffset += controller->stickAverageY * 4.0f;
		}
		else
		{
			real32 dx = 0.0f;
			real32 dy = 0.0f;

			int sensitivity = 4;
			if (controller->moveLeft.endedDown)
				dx -= sensitivity;
			if (controller->moveRight.endedDown)
				dx += sensitivity;
			if (controller->moveDown.endedDown)
				dy -= sensitivity;
			if (controller->moveUp.endedDown)
				dy += sensitivity;

			canonical_position NewPlayerPos = GameState->PlayerP;
			NewPlayerPos.TileRelX += dx;
			NewPlayerPos.TileRelY += dy;

			NewPlayerPos = RecanonicalizePosition(&GameState->World, NewPlayerPos);

			char old_buffer[256];
			char new_buffer[256];

			wsprintfA(old_buffer, "Old: TileMap: (%d, %d) Tile: (%d, %d) TileRel: (%d, %d) \n", GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY, GameState->PlayerP.TileX,  GameState->PlayerP.TileY, (int32)GameState->PlayerP.TileRelX, (int32)GameState->PlayerP.TileRelY);
			wsprintfA(new_buffer, "New: TileMap: (%d, %d) Tile: (%d, %d) TileRel: (%d, %d) \n", NewPlayerPos.TileMapX, NewPlayerPos.TileMapY, NewPlayerPos.TileX,  NewPlayerPos.TileY, (int32)NewPlayerPos.TileRelX, (int32)NewPlayerPos.TileRelY);
			OutputDebugStringA(old_buffer);
			OutputDebugStringA(new_buffer);

			// COLLISION
			canonical_position LeftEdge = NewPlayerPos;
			canonical_position RightEdge = NewPlayerPos;
			LeftEdge.TileRelX += PlayerWidth/2;
			RightEdge.TileRelX -= PlayerWidth/2;
			LeftEdge = RecanonicalizePosition(&GameState->World, LeftEdge);
			RightEdge = RecanonicalizePosition(&GameState->World, RightEdge);


			if(IsWorldPointEmpty(&GameState->World, LeftEdge) && IsWorldPointEmpty(&GameState->World, RightEdge)){
				GameState->PlayerP = NewPlayerPos;
			}

			//GameState->PlayerP.TileMapX += (int)(4.0f * controller->stickAverageX);
			//GameState->PlayerP.TileMapY -= (int)(4.0f * controller->stickAverageY);

			//NewPlayerPos.TileRelX -= 0.5f * PlayerWidth;
			//NewPlayerPos.TileRelY -= 0.5f * PlayerWidth;
		}
	}

	tile_map *TileMap = GetTileMap(&GameState->World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);

	DrawRectangle(buffer, 0.f, 0.f, 1280, 720, 0.2f, 0.3f, 0.8f);
	for (int x = 0; x < TILE_MAP_COUNT_X; x++)
	{
		for (int y = 0; y < TILE_MAP_COUNT_Y; y++)
		{
			real32 Gray = 0.25f;
			uint32 TileID = GetTileValueUnchecked(&GameState->World, TileMap, x, y);
			if (TileID == 1)
				Gray = 1.0f;

			if ((GameState->PlayerP.TileX == x) && ((GameState->World.CountY - GameState->PlayerP.TileY - 1) == y))
				Gray = 0.f;

			int32 MinX = x * GameState->World.TileSideInPixels;
			int32 MinY = y * GameState->World.TileSideInPixels;
			int32 MaxX = MinX + GameState->World.TileSideInPixels;
			int32 MaxY = MinY + GameState->World.TileSideInPixels;

			DrawRectangle(buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
		}
	}

	int32 minx = (GameState->PlayerP.TileX * GameState->World.TileSideInPixels + GameState->PlayerP.TileRelX) - PlayerWidth/2;
	int32 miny = (TILE_MAP_COUNT_Y * GameState->World.TileSideInPixels) - (GameState->PlayerP.TileY * GameState->World.TileSideInPixels + GameState->PlayerP.TileRelY) - PlayerHeight;
	int32 maxx = minx + PlayerWidth;
	int32 maxy = miny + PlayerHeight;
	DrawRectangle(buffer, minx, miny, maxx, maxy, 0.85f, 0.25f, 0.3f);
	DrawRectangle(buffer, GameState->World.CountX * GameState->World.TileSideInPixels, 0, 1280, 720, 0.f, 8.f, 0.f);
	DrawRectangle(buffer, 0, GameState->World.CountY * GameState->World.TileSideInPixels, 1280, 720, 0.f, 8.f, 0.f);

}
