
#include "Game.h"
#include "GameTile.cpp"

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

internal void BoundValue(int32 &Value, int32 LowerBound, int32 UpperBound){
	if(Value < LowerBound)
		Value = LowerBound;
	if(Value > UpperBound){
		Value = UpperBound;
	}
}

internal void DrawRectangle(game_back_buffer *buffer, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY, float R, float G, float B)
{	
	uint8_t *row = (uint8_t *)buffer->memory;

	ASSERT(MinX < MaxX);
	ASSERT(MinY < MaxY);

	BoundValue(MinX, 0, buffer->width);
	BoundValue(MaxX, 0, buffer->width);
	BoundValue(MinY, 0, buffer->height);
	BoundValue(MaxY, 0, buffer->height);
	
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


inline void
InitializeMemoryArena(memory_arena &Arena, size_t Size, void* PermanentStorage){
	Arena.Base = (uint8*)PermanentStorage;
	Arena.Size = Size;
	Arena.Used = 0;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{

#define CHUNK_DIM 256

	ASSERT(sizeof(game_state) <= memory->permanentStorageSize);

	game_state *GameState = (game_state *)memory->permanentStorage;

	if (!memory->isInitialized)
	{
		InitializeMemoryArena(GameState->MemoryArena, memory->permanentStorageSize - sizeof(GameState), (void*)((uint8*)memory->permanentStorage + sizeof(game_state)));

		GameState->World = PushStruct(GameState->MemoryArena, world);
		world *World = GameState->World;
		tile_map *TileMap = PushStruct(GameState->MemoryArena, tile_map);
		World->TileMap = TileMap;

		TileMap->ChunkShift = 8;
		// TODO: Generate ChunkDim And ChunkMask From ChunkShift
		TileMap->ChunkDim = 256;
		TileMap->ChunkMask = 0xFF;

		TileMap->TileChunkCountX = 4;
		TileMap->TileChunkCountY = 4;
		TileMap->TileChunkCountZ = 4;

		TileMap->TileSideInPixels = 10;
		TileMap->TileSideInMeters = 1.4f;
		TileMap->MetersToPixels = TileMap->TileSideInPixels/TileMap->TileSideInMeters;

		TileMap->UpperLeftX = -(real32)TileMap->TileSideInPixels / 2;
		TileMap->UpperLeftY = 0;

		TileMap->TileChunks = PushArray(GameState->MemoryArena, TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ, tile_chunk);

		int32 TilesPerWidth = 19;
		int32 TilesPerHeight = 7;

		for(int ScreenY = 0; ScreenY < 32; ScreenY++){
			for(int ScreenX = 0; ScreenX < 32; ScreenX++){
				for(int TileY = 0; TileY < TilesPerHeight; TileY++){
					for(int TileX = 0; TileX < TilesPerWidth; TileX++){
						int32 AbsTileX = ScreenX * TilesPerWidth + TileX;
						int32 AbsTileY = ScreenY * TilesPerHeight + TileY;
						int32 TileValue = 1;
						if((TileX == 0) || (TileX == TilesPerWidth-1)){
							if(TileY == TilesPerHeight/2){
								TileValue = 1;
							}else{
								TileValue = 2;
							}
						}
						if((TileY == 0) || (TileY == TilesPerHeight-1)){
							if(TileX == TilesPerWidth/2){
								TileValue = 1;
							}
							else{
								TileValue = 2;
							}
						}
						uint32 AbsTileZ = 0;
						SetTileValue(GameState->MemoryArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
					}
				}
			}
		}

		// char *filename = __FILE__;
		// debug_read_file_result *file = (debug_read_file_result *)memory->DEBUGPlatformReadEntireFile(nullptr, filename);

		GameState->toneHz = 512;
		GameState->tSine = 0.0f;

		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 3;
		GameState->PlayerP.TileRelX = 0.f;
		GameState->PlayerP.TileRelY = 0.f;

		memory->isInitialized = true;
	}

	real32 PlayerWidth = 0.75f * GameState->World->TileMap->TileSideInMeters;
	real32 PlayerHeight = (real32)GameState->World->TileMap->TileSideInMeters;

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

			static real32 sensitivity = 0.1;
			static int wait = 1;
			if (wait) {
				wait--;
			}
			if (!wait) {
				if (controller->actionDown.endedDown){
					sensitivity *= 2;
					wait = 20;
				}
			}
			if (controller->moveLeft.endedDown)
				dx -= sensitivity;
			if (controller->moveRight.endedDown)
				dx += sensitivity;
			if (controller->moveDown.endedDown)
				dy -= sensitivity;
			if (controller->moveUp.endedDown)
				dy += sensitivity;

			tile_map_position NewPlayerPos = GameState->PlayerP;
			NewPlayerPos.TileRelX += dx;
			NewPlayerPos.TileRelY += dy;

			NewPlayerPos = RecanonicalizePosition(GameState->World->TileMap, NewPlayerPos);


			// COLLISION
			tile_map_position LeftEdge = NewPlayerPos;
			tile_map_position RightEdge = NewPlayerPos;
			LeftEdge.TileRelX += PlayerWidth/2;
			RightEdge.TileRelX -= PlayerWidth/2;
			LeftEdge = RecanonicalizePosition(GameState->World->TileMap, LeftEdge);
			RightEdge = RecanonicalizePosition(GameState->World->TileMap, RightEdge);


			if(IsWorldPointEmpty(GameState->World->TileMap, LeftEdge) && IsWorldPointEmpty(GameState->World->TileMap, RightEdge)){
				GameState->PlayerP = NewPlayerPos;
			}
		}
	}

	tile_map *TileMap = GameState->World->TileMap;
	tile_map_position PlayerP = GameState->PlayerP;

	real32 CenterX = buffer->width/2.f;
	real32 CenterY = buffer->height/2.f;
	DrawRectangle(buffer, 0, 0, buffer->width, buffer->height, 0.2f, 0.3f, 0.8f);
	for (int32 RelRow = -100; RelRow < 100; RelRow++)
	{
		for (int32 RelColumn = -100; RelColumn < 100; RelColumn++)
		{
			real32 Gray = 0.25f;

			uint32 AbsColumn = PlayerP.AbsTileX + RelColumn;
			uint32 AbsRow = PlayerP.AbsTileY + RelRow;

			uint32 TileValue = GetTileValue(GameState->World->TileMap, AbsColumn, AbsRow, 0);
			if(TileValue > 0){
				if (TileValue == 2)
					Gray = 1.0f;

				if((AbsColumn == PlayerP.AbsTileX) && (AbsRow == PlayerP.AbsTileY))
					Gray = 0.f;

				int32 MinX = CenterX + (RelColumn * TileMap->TileSideInPixels) - PlayerP.TileRelX * TileMap->MetersToPixels;
				int32 MaxY = CenterY - (RelRow * TileMap->TileSideInPixels) + PlayerP.TileRelY * TileMap->MetersToPixels;
				int32 MaxX = MinX + TileMap->TileSideInPixels;
				int32 MinY = MaxY - TileMap->TileSideInPixels;
				DrawRectangle(buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
			}
		}
	}

	int32 minx = CenterX - PlayerWidth/2 * TileMap->MetersToPixels;
	int32 miny = CenterY - PlayerHeight * TileMap->MetersToPixels;
	int32 maxx = minx + PlayerWidth * TileMap->MetersToPixels;
	int32 maxy = miny + PlayerHeight * TileMap->MetersToPixels;
	DrawRectangle(buffer, minx, miny, maxx, maxy, 0.85f, 0.25f, 0.3f);
	DrawRectangle(buffer, 0, buffer->height/2, buffer->width, buffer->height/2+1, 0.8f, 0.2, 0.3f);
	DrawRectangle(buffer, buffer->width/2, 0, buffer->width/2+1, buffer->height, 0.8f, 0.2, 0.3f);

}
