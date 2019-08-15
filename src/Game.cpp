
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

internal void DrawBitmap(game_back_buffer *buffer, const bitmap_image *Image, real32 X, real32 Y)
{	
	ASSERT(X <= buffer->width);
	ASSERT(Y <= buffer->height);

	uint32* src = Image->Pixels;
	uint32* dst = (uint32*)buffer->memory;

	// flooring float value to int
	uint32 xoffset = (uint32)(X + 0.5f);
	uint32 yoffset = (uint32)(Y + 0.5f);

	uint32 width = (Image->Width < buffer->width - X) ? Image->Width : buffer->width - X;
	uint32 height = (Image->Height < buffer->height - Y) ? Image->Height : buffer->height - Y;

	dst += (buffer->width * yoffset) + xoffset;
	src += Image->Width * (Image->Height - 1);

	for(int y = 0; y < height; y++){
		for(int x = 0; x < width; x++){
			uint32 Foreground = *src;
			uint32 Background = *dst;
			real32 Alpha = (real32)(Foreground >> 24) / 255.f;  // value between 0 - 1
			uint8 R = (1-Alpha) * (uint8)(Background >> 16) + Alpha * (uint8)(Foreground >> 16);
			uint8 G = (1-Alpha) * (uint8)(Background >>  8) + Alpha * (uint8)(Foreground >>  8);
			uint8 B = (1-Alpha) * (uint8)(Background >>  0) + Alpha * (uint8)(Foreground >>  0);

			*dst++ = (255 << 24) | (R << 16) | (G << 8) | (B << 0);
			*src++;
		}
		dst += (buffer->width - width);
		src -= (width + Image->Width);
	}
}

inline void
InitializeMemoryArena(memory_arena &Arena, size_t Size, void* PermanentStorage){
	Arena.Base = (uint8*)PermanentStorage;
	Arena.Size = Size;
	Arena.Used = 0;
}

#pragma pack(push, 1)
struct bitmap_header {
  uint16  bfType;
  uint32 bfSize;
  uint16  bfReserved1;
  uint16  bfReserved2;
  uint32 bfOffBits;
};

struct bitmap_info_header {
  	uint32 biSize;
  	int32  biWidth;
  	int32  biHeight;
  	uint16  biPlanes;
  	uint16  biBitCount;
  	uint32 biCompression;
  	uint32 biSizeImage;
  	int32  biXPelsPerMeter;
  	int32  biYPelsPerMeter;
  	uint32 biClrUsed;
  	uint32 biClrImportant;
  	uint32 RedMask;         /* Mask identifying bits of red component */
  	uint32 GreenMask;       /* Mask identifying bits of green component */
  	uint32 BlueMask;        /* Mask identifying bits of green component */
};
#pragma pack(pop, 1)

inline bitmap_image
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, const char* path){

	bitmap_image Result;
	debug_read_file_result FileContents;

	FileContents = ReadEntireFile(Thread, path);
	ASSERT(FileContents.contents);
	if(FileContents.contents){
		bitmap_header header = *(bitmap_header*)FileContents.contents;
		bitmap_info_header info_header = *(bitmap_info_header*)((bitmap_header*)FileContents.contents+1);
		Result.Width = info_header.biWidth;
		Result.Height = info_header.biHeight;
		Result.Pixels = (uint32*)((uint8*)FileContents.contents + header.bfOffBits);
	}

	return Result;
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
		GameState->BMPPixels = DEBUGLoadBMP(Thread, memory->DEBUGPlatformReadEntireFile, "./circle.bmp");

		TileMap->ChunkShift = 8;
		// TODO: Generate ChunkDim And ChunkMask From ChunkShift
		TileMap->ChunkDim = 256;
		TileMap->ChunkMask = 0xFF;

		TileMap->TileChunkCountX = 4;
		TileMap->TileChunkCountY = 4;
		TileMap->TileChunkCountZ = 4;

		TileMap->TileSideInPixels = 60;
		TileMap->TileSideInMeters = 1.4f;
		TileMap->MetersToPixels = TileMap->TileSideInPixels/TileMap->TileSideInMeters;

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
		GameState->PlayerP.Offset = {0.f, 0.f};

		GameState->CameraP.AbsTileX = 3;
		GameState->CameraP.AbsTileY = 3;
		GameState->CameraP.Offset = {0.f, 0.f};

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
			vec2 acceleration = {0.f, 0.f};
			static real32 sensitivity = 10.f;
			static int wait = 1;
			if (wait) {
				wait--;
			}
			if (!wait) {
				if (controller->actionDown.endedDown){
					sensitivity += 1.f;
					wait = 20;
				}
			}
			if (controller->moveLeft.endedDown)
				acceleration.x -= sensitivity;
			if (controller->moveRight.endedDown)
				acceleration.x += sensitivity;
			if (controller->moveDown.endedDown)
				acceleration.y -= sensitivity;
			if (controller->moveUp.endedDown)
				acceleration.y += sensitivity;

			acceleration-= 0.75f * GameState->PlayerVel;
			tile_map_position NewPlayerPos = GameState->PlayerP;
			NewPlayerPos.Offset += 0.5f * acceleration * Square(input->dt) + GameState->PlayerVel * input->dt;   // s = x + ut + 1/2at^2


			GameState->PlayerVel += acceleration * input->dt;

			NewPlayerPos = RecanonicalizePosition(GameState->World->TileMap, NewPlayerPos);


			// COLLISION
			tile_map_position CollisionDirection = {};
			tile_map_position LeftEdge = NewPlayerPos;
			tile_map_position RightEdge = NewPlayerPos;
			LeftEdge.Offset.x += PlayerWidth/2;
			RightEdge.Offset.x -= PlayerWidth/2;
			LeftEdge = RecanonicalizePosition(GameState->World->TileMap, LeftEdge);
			RightEdge = RecanonicalizePosition(GameState->World->TileMap, RightEdge);

			
			bool Collided = false;

			if(!IsWorldPointEmpty(GameState->World->TileMap, LeftEdge)){
				CollisionDirection = LeftEdge;
				Collided = true;
			}
			if(!IsWorldPointEmpty(GameState->World->TileMap, RightEdge)){
				CollisionDirection = RightEdge;
				Collided = true;
			}

			tile_map *TileMap = GameState->World->TileMap;
			tile_map_position CameraP = GameState->CameraP;
			if((NewPlayerPos.AbsTileX  * TileMap->TileSideInPixels + NewPlayerPos.Offset.x) - (CameraP.AbsTileX * TileMap->TileSideInPixels + CameraP.Offset.x) > buffer->width/2)
				GameState->CameraP.AbsTileX += 9;
			if((NewPlayerPos.AbsTileX  * TileMap->TileSideInPixels + NewPlayerPos.Offset.x) - (CameraP.AbsTileX * TileMap->TileSideInPixels + CameraP.Offset.x) < -buffer->width/2)
				GameState->CameraP.AbsTileX -= 9;
			if((NewPlayerPos.AbsTileY  * TileMap->TileSideInPixels + NewPlayerPos.Offset.y) - (CameraP.AbsTileY * TileMap->TileSideInPixels + CameraP.Offset.y) > buffer->height/2)
				GameState->CameraP.AbsTileY += 7;
			if((NewPlayerPos.AbsTileY  * TileMap->TileSideInPixels + NewPlayerPos.Offset.y) - (CameraP.AbsTileY * TileMap->TileSideInPixels + CameraP.Offset.y) < -buffer->height/2)
				GameState->CameraP.AbsTileY -= 7;

			if(!Collided){
				GameState->PlayerP = NewPlayerPos;
			}else{
				vec2 Normal = {0, 0};
				if(CollisionDirection.AbsTileX < GameState->PlayerP.AbsTileX){
					Normal = {1, 0};
				}if(CollisionDirection.AbsTileX > GameState->PlayerP.AbsTileX){
					Normal = {-1, 0};
				}
				if(CollisionDirection.AbsTileY < GameState->PlayerP.AbsTileY){
					Normal = {0, 1};
				}
				if(CollisionDirection.AbsTileY > GameState->PlayerP.AbsTileY){
					Normal = {0, -1};
				}
				ASSERT(!((Normal.x == 0) && (Normal.y == 0)));
				GameState->PlayerVel = GameState->PlayerVel - (2 * dot(GameState->PlayerVel, Normal)) * Normal;  // V + (2*(N*V))*N
			}
		}
	}

	tile_map *TileMap = GameState->World->TileMap;
	tile_map_position PlayerP = GameState->PlayerP;
	tile_map_position CameraP = GameState->CameraP;

	real32 CenterX = buffer->width/2.f;
	real32 CenterY = buffer->height/2.f;
	DrawRectangle(buffer, 0, 0, buffer->width, buffer->height, 0.2f, 0.3f, 0.8f);
	for (int32 RelRow = -100; RelRow < 100; RelRow++)
	{
		for (int32 RelColumn = -100; RelColumn < 100; RelColumn++)
		{
			real32 Gray = 0.25f;

			uint32 AbsColumn = CameraP.AbsTileX + RelColumn;
			uint32 AbsRow = CameraP.AbsTileY + RelRow;

			uint32 TileValue = GetTileValue(GameState->World->TileMap, AbsColumn, AbsRow, 0);
			if(TileValue > 0){
				if (TileValue == 2)
					Gray = 1.0f;

				//if((AbsColumn == CameraP.AbsTileX) && (AbsRow == CameraP.AbsTileY))
				//	Gray = 0.f;

				int32 MinX = CenterX + (RelColumn * TileMap->TileSideInPixels) - CameraP.Offset.x * TileMap->MetersToPixels;
				int32 MaxY = CenterY - (RelRow * TileMap->TileSideInPixels) + CameraP.Offset.y * TileMap->MetersToPixels;
				int32 MaxX = MinX + TileMap->TileSideInPixels;
				int32 MinY = MaxY - TileMap->TileSideInPixels;
				DrawRectangle(buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
			}
		}
	}

	tile_map_position PlayerCameraRelPosition = {};
	PlayerCameraRelPosition.AbsTileX = PlayerP.AbsTileX - CameraP.AbsTileX;
	PlayerCameraRelPosition.AbsTileY = PlayerP.AbsTileY - CameraP.AbsTileY;
	PlayerCameraRelPosition.Offset = PlayerP.Offset - CameraP.Offset;
	PlayerCameraRelPosition = RecanonicalizePosition(TileMap, PlayerCameraRelPosition);

	int32 minx = CenterX + (PlayerCameraRelPosition.AbsTileX * TileMap->TileSideInMeters + PlayerCameraRelPosition.Offset.x - PlayerWidth/2) * TileMap->MetersToPixels;
	int32 maxy = CenterY - (PlayerCameraRelPosition.AbsTileY * TileMap->TileSideInMeters + PlayerCameraRelPosition.Offset.y) * TileMap->MetersToPixels;
	int32 maxx = minx + PlayerWidth * TileMap->MetersToPixels;
	int32 miny = maxy - PlayerHeight * TileMap->MetersToPixels;
	DrawRectangle(buffer, minx, miny, maxx, maxy, 0.85f, 0.25f, 0.3f);
	DrawRectangle(buffer, 0, buffer->height/2, buffer->width, buffer->height/2+1, 0.8f, 0.2, 0.3f);
	DrawRectangle(buffer, buffer->width/2, 0, buffer->width/2+1, buffer->height, 0.8f, 0.2, 0.3f);


	// DrawBitmap(buffer, &GameState->BMPPixels, buffer->width/2.f - GameState->BMPPixels.Width/2, buffer->height/2.f - GameState->BMPPixels.Height/2);

}
