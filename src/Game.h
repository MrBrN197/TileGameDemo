#pragma once
#include <cstdint>

using bool32 = int32_t;
using uint32 = uint32_t;
using int32  = int32_t;
using real32 = float;
using uint8 = uint8_t;


#define internal static
#define local_persist static
#define global_variable static

#define PI 3.14159265358

#define Kilobytes(X) (X) * 1024
#define Megabytes(X) Kilobytes(X) * 1024
#define Gigabytes(X) Megabytes(X) * 1024

#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
// TODO: swap, min, max ... macros???

#define ASSERT(X) if(!(X)) { *(int8_t*)0 = 0; }


//TODO : services that the platform layer provides to the game


struct game_sound_buffer{
	int16_t *samples;
	int sampleCount;
};

struct game_back_buffer {
	//BITMAPINFO bitmapInfo;
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

struct game_button_state {
	bool endedDown;
	uint32 halfTransitionCount;
};

struct game_controller_input{

	bool isConnected;
	bool isAnalog;
	float stickAverageX;
	float stickAverageY;

	union {
		game_button_state buttons[12];
		struct {
			game_button_state moveUp;
			game_button_state moveDown;
			game_button_state moveRight;
			game_button_state moveLeft;

			game_button_state actionLeft;
			game_button_state actionRight;
			game_button_state actionUp;
			game_button_state actionDown;

			game_button_state right_shoulder;
			game_button_state left_shoulder;

			game_button_state start;
			game_button_state end;

			// NOTE: add buttons above this line
			game_button_state terminator;

		};
	};
};

struct game_input {
	game_controller_input controllers[5];
};

inline game_controller_input *GetController(game_input* input, uint32 ControllerIndex){

	ASSERT(ControllerIndex < ArrayCount(input->controllers));
	return &input->controllers[ControllerIndex];

}

struct debug_read_file_result
{
	uint32 size;
	void *contents;
};

#include "GameTile.h"
#include "GameIntrinsics.h"

struct world{
	tile_map *TileMap;
};

struct memory_arena{
	size_t Size;
	uint8* Base;
	size_t Used;
};

struct game_state {
	world *World;
	tile_map_position PlayerP;
	memory_arena MemoryArena;
	//int32_t xoffset;
	//int32_t yoffset;
	int32_t toneHz;

	float tSine;
};


struct thread_context{
	int placeholder;
};

struct game_memory
{
	bool isInitialized;
	size_t permanentStorageSize;
	void *permanentStorage;

	size_t transientStorageSize;
	void *transientStorage;

	void* (*DEBUGPlatformReadEntireFile)(thread_context *context, char *filename);
	void (*DEBUGPlatformFreeFileMemory)(thread_context *context, const debug_read_file_result* fileInfo);
	bool (*DEBUGPlatformWriteEntireFile)(thread_context *context, char *filename, uint32 size, void *memory);
};

#define PushStruct(arena, type) (type*)PushStruct_(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)PushStruct_(arena, sizeof(type) * count)
inline void*
PushStruct_(memory_arena &Arena, size_t Size){
	ASSERT(Arena.Used + Size <= Arena.Size);
	Arena.Used += Size;
	void* Result = (Arena.Base + Size);
	return Result;
}


//NOTE: services that the game provides to the platform layer

void* DEBUGPlatformReadEntireFile(thread_context *context, char *filename);
void DEBUGPlatformFreeFileMemory(thread_context *context, const debug_read_file_result* file);
bool DEBUGPlatformWriteEntireFile(thread_context *context, char *filename, uint32 size, void *memory);

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *memory, game_input *input, game_back_buffer *buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *memory, game_sound_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

//void FillBuffer(game_back_buffer *back_buffer, int32_t xoffset, int32_t yoffset);

///////////////////////////////////////////////////////////////////////////////////////