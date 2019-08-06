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
	if (MaxX > buffer->width)
		MaxX = buffer->width;
	if (MaxY > buffer->height)
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

inline tile_chunk_position
GetChunkPosFor(world *World, uint32 AbsTileX, uint32 AbsTileY){
	tile_chunk_position Result;

	Result.TileChunkX = AbsTileX >> World->ChunkShift;
	Result.TileChunkY = AbsTileY >> World->ChunkShift;
	Result.RelTileX = AbsTileX & World->ChunkMask;
	Result.RelTileY = AbsTileY & World->ChunkMask;

	return Result;
}

inline void 
RecanonicalizeCoord(world *World, int32 *Tile, real32 *TileRel)
{
	int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInPixels);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInPixels;	

	ASSERT(*TileRel < World->TileSideInPixels);
}

inline world_position
RecanonicalizePosition(world *World, world_position Pos)
{
	world_position Result = Pos;
	
	RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);
	return Result;
}

inline tile_chunk*
GetTileMap(world *World, int32 TileChunkX, int32 TileChunkY)
{
	ASSERT(TileChunkX >= 0);
	ASSERT(TileChunkY >= 0);
	ASSERT(TileChunkX < World->ChunkDim);
	ASSERT(TileChunkY < World->ChunkDim);

	tile_chunk *TileMap = &World->TileChunks[TileChunkY * World->ChunkDim + TileChunkX];
	return TileMap;
}

inline uint32
GetTileValueUnchecked(world *World, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
	ASSERT(TileX >= 0);
	ASSERT(TileY >= 0);
	ASSERT(TileX < World->ChunkDim);
	ASSERT(TileY < World->ChunkDim);

	int32 Result = TileChunk->Tiles[TileY * World->ChunkDim + TileX];
	return Result;
}

#include <Windows.h>
inline bool32	
GetTileChunkTileValue(world *World, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
	uint32 Result = 0;
	if(TileChunk){
		Result = GetTileValueUnchecked(World, TileChunk, TileX, TileY);
	}
	return Result;
}

internal bool32
GetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
	tile_chunk_position ChunkPos = GetChunkPosFor(World, AbsTileX, AbsTileY);
	tile_chunk *TileChunk = GetTileMap(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	bool32 Result = GetTileChunkTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
	return Result;
}

internal bool32
IsWorldPointEmpty(world *World, world_position CanPos)
{
	bool32 Result = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
	return (Result == 0);
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{

#define CHUNK_DIM 256

	int32 TestTiles[CHUNK_DIM][CHUNK_DIM] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
		{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
		{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
		{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
		{1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};


	ASSERT(sizeof(game_state) <= memory->permanentStorageSize);

	game_state *GameState = (game_state *)memory->permanentStorage;

	real32 PlayerWidth = 0.75f * GameState->World.TileSideInPixels;
	real32 PlayerHeight = (real32)GameState->World.TileSideInPixels;


	if (!memory->isInitialized)
	{
		world World;
		World.ChunkShift = 8;
		World.ChunkDim = 256;
		World.ChunkMask = 0xFF;

		World.CountX = 19;
		World.CountY = 7;

		World.TileSideInPixels = 60;
		World.TileSideInMeters = 1.4f;

		World.UpperLeftX = -(real32)World.TileSideInPixels / 2;
		World.UpperLeftY = 0;

		GameState->World = World;

		char *filename = __FILE__;
		debug_read_file_result *file = (debug_read_file_result *)memory->DEBUGPlatformReadEntireFile(nullptr, filename);

		GameState->toneHz = 512;
		GameState->tSine = 0.0f;

		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 3;
		GameState->PlayerP.TileRelX = 5.f;
		GameState->PlayerP.TileRelY = 5.f;

		memory->isInitialized = true;
	}

	tile_chunk TileChunk;
	TileChunk.Tiles = (int32*)TestTiles;
	GameState->World.TileChunks = &TileChunk;

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

			world_position NewPlayerPos = GameState->PlayerP;
			NewPlayerPos.TileRelX += dx;
			NewPlayerPos.TileRelY += dy;

			NewPlayerPos = RecanonicalizePosition(&GameState->World, NewPlayerPos);

			char old_buffer[256];
			char new_buffer[256];

			// wsprintfA(old_buffer, "Old: TileMap: (%d, %d) Tile: (%d, %d) TileRel: (%d, %d) \n", GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY, GameState->PlayerP.TileX,  GameState->PlayerP.TileY, (int32)GameState->PlayerP.TileRelX, (int32)GameState->PlayerP.TileRelY);
			// wsprintfA(new_buffer, "New: TileMap: (%d, %d) Tile: (%d, %d) TileRel: (%d, %d) \n", NewPlayerPos.TileMapX, NewPlayerPos.TileMapY, NewPlayerPos.TileX,  NewPlayerPos.TileY, (int32)NewPlayerPos.TileRelX, (int32)NewPlayerPos.TileRelY);
			// OutputDebugStringA(old_buffer);
			// OutputDebugStringA(new_buffer);

			// COLLISION
			world_position LeftEdge = NewPlayerPos;
			world_position RightEdge = NewPlayerPos;
			LeftEdge.TileRelX += PlayerWidth/2;
			RightEdge.TileRelX -= PlayerWidth/2;
			LeftEdge = RecanonicalizePosition(&GameState->World, LeftEdge);
			RightEdge = RecanonicalizePosition(&GameState->World, RightEdge);
			LeftEdge.AbsTileY = GameState->World.CountY - 1 - LeftEdge.AbsTileY;
			RightEdge.AbsTileY = GameState->World.CountY - 1 - RightEdge.AbsTileY;


			if(IsWorldPointEmpty(&GameState->World, LeftEdge) && IsWorldPointEmpty(&GameState->World, RightEdge)){
				GameState->PlayerP = NewPlayerPos;
			}

			//GameState->PlayerP.TileMapX += (int)(4.0f * controller->stickAverageX);
			//GameState->PlayerP.TileMapY -= (int)(4.0f * controller->stickAverageY);

			//NewPlayerPos.TileRelX -= 0.5f * PlayerWidth;
			//NewPlayerPos.TileRelY -= 0.5f * PlayerWidth;
		}
	}


	DrawRectangle(buffer, 0.f, 0.f, 1280, 720, 0.2f, 0.3f, 0.8f);
	for (int Column = 0; Column < 19; Column++)
	{
		for (int Row = 0; Row < 7; Row++)
		{
			real32 Gray = 0.25f;
			uint32 TileValue = GetTileValue(&GameState->World, Column, Row);
			if (TileValue == 1)
				Gray = 1.0f;

			if ((GameState->PlayerP.AbsTileX == Column) && ((GameState->World.CountY - GameState->PlayerP.AbsTileY - 1) == Row))
				Gray = 0.f;

			int32 MinX = Column * GameState->World.TileSideInPixels;
			int32 MinY = Row * GameState->World.TileSideInPixels;
			int32 MaxX = MinX + GameState->World.TileSideInPixels;
			int32 MaxY = MinY + GameState->World.TileSideInPixels;

			DrawRectangle(buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
		}
	}

	int32 minx = (GameState->PlayerP.AbsTileX * GameState->World.TileSideInPixels + GameState->PlayerP.TileRelX) - PlayerWidth/2;
	int32 miny = (7 * GameState->World.TileSideInPixels) - (GameState->PlayerP.AbsTileY * GameState->World.TileSideInPixels + GameState->PlayerP.TileRelY) - PlayerHeight;
	int32 maxx = minx + PlayerWidth;
	int32 maxy = miny + PlayerHeight;
	DrawRectangle(buffer, minx, miny, maxx, maxy, 0.85f, 0.25f, 0.3f);
	//DrawRectangle(buffer, GameState->World.ChunkDim * GameState->World.TileSideInPixels, 0, 1280, 720, 0.f, 8.f, 0.f);
	//DrawRectangle(buffer, 0, GameState->World.ChunkDim * GameState->World.TileSideInPixels, 1280, 720, 0.f, 8.f, 0.f);

}
