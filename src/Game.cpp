
#include "Game.h"
#include "GameTile.cpp"

void FillSoundBuffer(int16 *samples, int32 sampleCount)
{
	int32 toneHz = 440;
	int32 period = 48000 / 440;
	int32 volume = 50;

	const float delta = (2 * PI) / (float)period;

	int32 blocks = sampleCount / 2;
	int16 *iter = samples;
	for (int i = 0; i < blocks; i++)
	{
		//t += delta;
		float sinT = 1; //sinf(t);
		int16 sampleValue = (int16)(sinT * volume);
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
	uint8 *row = (uint8 *)buffer->memory;

	ASSERT(MinX < MaxX);
	ASSERT(MinY < MaxY);

	BoundValue(MinX, 0, buffer->width);
	BoundValue(MaxX, 0, buffer->width);
	BoundValue(MinY, 0, buffer->height);
	BoundValue(MaxY, 0, buffer->height);
	
	row += (buffer->pitch) * MinY;
	row += MinX * (buffer->bytesPerPixel);

	for (uint32 y = 0; y < (MaxY - MinY); y++)
	{
		uint32 *pixel = (uint32 *)row;
		for (uint32 x = 0; x < (MaxX - MinX); x++)
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

inline void
InitializePlayer(game_state *GameState, entity *Entity){
	*Entity = {};
	Entity->Exists = true;
	Entity->Position.AbsTileX = 9;
	Entity->Position.AbsTileY = 3;
	Entity->Position.AbsTileZ = 0;
	Entity->PlayerWidth = 0.75f * GameState->World->TileMap->TileSideInMetres;
	Entity->PlayerHeight = 0.5f * (real32)GameState->World->TileMap->TileSideInMetres;
}

inline entity*
GetEntity(game_state *GameState, uint32 EntityIndex){
	entity *Result = nullptr;
	if((EntityIndex > 0) && (EntityIndex <= GameState->EntityCount)){
		Result = &GameState->Entities[EntityIndex];
	}
	return Result;
}

inline uint32
AddEntity(game_state *GameState){
	ASSERT(GameState->EntityCount < ArrayCount(GameState->Entities)-1);
	uint32 Result = ++GameState->EntityCount;
	InitializePlayer(GameState, GetEntity(GameState, Result));
	if(!GameState->EnitytCameraFollowsIndex){
		GameState->EnitytCameraFollowsIndex = Result; 
	}
	return Result;
}

inline bool 
TestWall(real32 WallX, real32 OriginX, real32 OriginY, real32 MinY, real32 MaxY, real32* tMin, real32 PlayerDeltaX, real32 PlayerDeltaY){
	bool Result = false;
	if(Abs(PlayerDeltaX) > 0.0f){
		real32 tResult = (WallX - OriginX)/ PlayerDeltaX;
		real32 yPos = OriginY + tResult*PlayerDeltaY;
		if((yPos < MaxY) && (yPos > MinY)){
			if((tResult > 0.0f) && (tResult < *tMin)){
				*tMin = Max(0.0f, tResult - 0.001f);
				Result = true;
			}
		}
			
	}
	return Result;
}

inline void
MovePlayer(game_state *GameState, entity *Entity, vec2 acceleration, real32 dt){
	acceleration = acceleration * 90.f;
	acceleration -= 8.f * Entity->PlayerVel;
	tile_map_position OldPlayerPos = Entity->Position;
	vec2 PlayerDelta = 0.5f * acceleration * Square(dt) + Entity->PlayerVel * dt;  // s = x + ut + 1/2at^2
	tile_map_position NewPlayerPos = OldPlayerPos;
	NewPlayerPos.Offset += PlayerDelta;
	NewPlayerPos = RecanonicalizePosition(GameState->World->TileMap, NewPlayerPos);

	Entity->PlayerVel += acceleration * dt;  // Note: When should this be done?

	real32 TileSideInMetres = GameState->World->TileMap->TileSideInMetres;

	uint32 MinTileX = Min(OldPlayerPos.AbsTileX, NewPlayerPos.AbsTileX);
	uint32 MinTileY = Min(OldPlayerPos.AbsTileY, NewPlayerPos.AbsTileY);
	uint32 MaxTileX = Max(OldPlayerPos.AbsTileX, NewPlayerPos.AbsTileX);
	uint32 MaxTileY = Max(OldPlayerPos.AbsTileY, NewPlayerPos.AbsTileY);

	uint32 EntityTileRelWidth = Ceil(0.5f*Entity->PlayerWidth/TileSideInMetres);
	uint32 EntityTileRelHeight = Ceil(0.5f*Entity->PlayerHeight/TileSideInMetres);
	
	MinTileX = Max(0.0f, MinTileX - EntityTileRelWidth);
	MinTileY = Max(0.0f, MinTileY - EntityTileRelHeight);
	MaxTileX = Min(0xFFFFFFFF, MaxTileX + EntityTileRelWidth);
	MaxTileY = Min(0xFFFFFFFF, MaxTileY + EntityTileRelHeight);

	real32 tRemaining = 1.0f;
	vec2 Normal = {};
	for(int i = 0; (i < 4) && (tRemaining > 0.0f); i++){
		real32 tMin = 1.0f;
		vec2 OldPlayerAbsolutePos = {};
		OldPlayerAbsolutePos.x = OldPlayerPos.AbsTileX * TileSideInMetres + TileSideInMetres/2.f + OldPlayerPos.Offset.x;
		OldPlayerAbsolutePos.y = OldPlayerPos.AbsTileY * TileSideInMetres + TileSideInMetres/2.f + OldPlayerPos.Offset.y;

		for(uint32 Y = MinTileY; Y <= MaxTileY; Y++){
			for(uint32 X = MinTileX; X <= MaxTileX; X++){

				// TODO: X and Y might go above the (2^32)-1

				vec2 MinCorner = {X * TileSideInMetres - Entity->PlayerWidth/2.f, Y * TileSideInMetres - Entity->PlayerHeight/2.f};
				vec2 MaxCorner = {X * TileSideInMetres + TileSideInMetres + Entity->PlayerWidth/2.f, Y * TileSideInMetres + TileSideInMetres + Entity->PlayerHeight/2.f};

				ASSERT(NewPlayerPos.AbsTileZ == OldPlayerPos.AbsTileZ);
				uint32 TileValue = GetTileValue(GameState->World->TileMap, X, Y, OldPlayerPos.AbsTileZ);
				if(TileValue == 2){
					if(TestWall(MinCorner.x, OldPlayerAbsolutePos.x, OldPlayerAbsolutePos.y, MinCorner.y, MaxCorner.y, &tMin, PlayerDelta.x, PlayerDelta.y))
						Normal = {-1, 0};
					if(TestWall(MaxCorner.x, OldPlayerAbsolutePos.x, OldPlayerAbsolutePos.y, MinCorner.y, MaxCorner.y, &tMin, PlayerDelta.x, PlayerDelta.y))
						Normal = {1, 0};
					if(TestWall(MinCorner.y, OldPlayerAbsolutePos.y, OldPlayerAbsolutePos.x, MinCorner.x, MaxCorner.x, &tMin, PlayerDelta.y, PlayerDelta.x))
						Normal = {0, -1};
					if(TestWall(MaxCorner.y, OldPlayerAbsolutePos.y, OldPlayerAbsolutePos.x, MinCorner.x, MaxCorner.x, &tMin, PlayerDelta.y, PlayerDelta.x))
						Normal = {0, 1};
				}
			}
		}
		OldPlayerPos.Offset += PlayerDelta * tMin;
		Entity->PlayerVel -= dot(Normal, Entity->PlayerVel) * Normal;
		OldPlayerPos = RecanonicalizePosition(GameState->World->TileMap, OldPlayerPos);
		Entity->Position = OldPlayerPos;  // Note: Defer

		PlayerDelta -= tMin * PlayerDelta;
		PlayerDelta -= dot(Normal, PlayerDelta) * Normal;
		RecanonicalizePosition(GameState->World->TileMap, Entity->Position);
		tRemaining -= (tMin * tRemaining);
	}
	ASSERT(tRemaining == 0.0f);
}

void DrawPoint(game_back_buffer* buffer, vec2& pos, int size){
	pos.y = buffer->height - pos.y;
	DrawRectangle(buffer, pos.x - size, pos.y - size, pos.x + size, pos.y + size, 1.f, 1.f, 1.f);
}

void DrawLine(game_back_buffer* buffer, vec2& pos1, vec2 pos2, float R, float G, float B){

	ASSERT(pos1.x >= 0);
	ASSERT(pos1.y >= 0);
	ASSERT(pos2.x >= 0);
	ASSERT(pos2.y >= 0);

	vec2 difference = pos2 - pos1;
	int32 dx = FloorReal32ToInt32(difference.x);
	int32 dy = FloorReal32ToInt32(difference.y);

	float gradient = dx/(float)dy;

 	uint32* row = ((uint32*)buffer->memory + buffer->width * FloorReal32ToInt32(pos1.y) + FloorReal32ToInt32(pos1.x));
	for(uint32 y = 0; y < Abs(dy); y++){
		uint32 lineWidth = 1;
		for(uint32 x = 0; x < lineWidth; x++){
			int32 r = R * 255;
			int32 g = G * 255;
			int32 b = B * 255;
			*row = ((255 << 24) | (r << 16) | (g << 8) | b);
		}
		if(dy >= 0){
			row += buffer->width;
		}else{
			row -= buffer->width;
		}
		if(dx >= 0){
			row += (Ceil(Abs(gradient)));
		}else{
			row -= (Ceil(Abs(gradient)));
		}
	}
}

void DrawTriangle(game_back_buffer* buffer, vec3 p1, vec3 p2, vec3 p3, const vec3& color){

	// TODO: Move Culling To ClipSpace Stage 
	vec3 normal = cross(p2 - p1, p3 - p1);
	if(dot(normal, {0, 0, 1}) > 0.0){
		return;
	}

	// TODO: Handle invalid triangles properly
	// ASSERT((p1.x != p2.x) && (p1.x != p3.x) && (p2.x != p3.x));
	// ASSERT((p1.y != p2.y) && (p1.y != p3.y) && (p2.y != p3.y));

	// triangle where all points lie on the same line (finds only vertical and horizantal lines)
	ASSERT(!((p1.y == p2.y) && (p1.y == p3.y)));
	ASSERT(!((p1.x == p2.x) && (p1.x == p3.x)));

	// triangle where two points are the same
	ASSERT(!((p1.x == p2.x) && (p1.y == p2.y)));
	ASSERT(!((p1.x == p3.x) && (p1.y == p3.y)));
	ASSERT(!((p2.x == p3.x) && (p2.y == p3.y)));

	vec3* v1 = &p1;
	vec3* v2 = &p2;
	vec3* v3 = &p3;

	if(v2->y < v1->y){
		vec3* temp = v2;
		v2 = v1;
		v1 = temp;
	}
	if(v3->y < v2->y){
		vec3* temp = v3;
		v3 = v2; 
		v2 = temp;
	}
	if(v2->y < v1->y){
		vec3* temp = v2;
		v2 = v1; 
		v1 = temp;
	}
	ASSERT((v1->y <= v2->y) && (v2->y <= v3->y));

	float m13;
	float m12;
	float dy13 = (v3->y - v1->y);
	if(dy13 == 0.f)
		ASSERT(false);
	m13 = (v3->x - v1->x) / (v3->y - v1->y);

	float dy12 = (v2->y - v1->y);
	if(dy12 == 0.f){
		m12 = 0.f;
	}else{
		m12 = (v2->x - v1->x) / (v2->y - v1->y) ;
	}

	float xIntersection = v1->x + (v2->y - v1->y) * m13;
	ASSERT(xIntersection != v2->x);

	int32 yStart = Ceil(v1->y - 0.5f);
	int32 yEnd = Ceil(v2->y - 0.5f);
	
	uint32* row = (uint32*)buffer->memory + buffer->width * yStart;

	for(int y = yStart; y< yEnd; y++){

		float deltaY = (y + 0.5f - v1->y);
		float point1 = v1->x + deltaY * m13;
		float point2 = v1->x + deltaY * m12;

		int start = Ceil(point1 - 0.5f);
		int end = Ceil(point2 - 0.5f);
		int xStart = start;
		int xEnd = end;
		if(xIntersection > v2->x){
			xStart = end;
			xEnd = start;
		}

		uint32* pixels = row + xStart;
		for (int x = xStart; x < xEnd; x++){
			int32 r = color.x * 255;
			int32 g = color.y * 255;
			int32 b = color.z * 255;
			*pixels++ = ((255 << 24) | (r << 16) | (g << 8) | b);
		}
		row += buffer->width;
	}

	// Second Half of Triangle

	float Y = v2->y;  // Optimize Get NewHalfPoint
	float d12 = (v2->y - v1->y);
	float X = v1->x + d12 * m13;
	vec3 secondHalfPoint = vec3{X, Y};

	yStart = Ceil(secondHalfPoint.y - 0.5f);
	yEnd = Ceil(v3->y -0.5f);
	float m23 = (v3->x - v2->x)/(v3->y - v2->y);

	for (int y = yStart; y < yEnd; y++)
	{
		float deltaY = (y + 0.5f - secondHalfPoint.y);
		float point1 = secondHalfPoint.x  + deltaY * m13;
		float point2 = v2->x + deltaY * m23;
		
		int start = Ceil(point1 - 0.5f);
		int end = Ceil(point2 - 0.5f);
		int xStart = start;
		int xEnd = end;
		if(xIntersection > v2->x){
			xStart = end;
			xEnd = start;
		}

		uint32* pixels = row + xStart;
		for (int x = xStart; x < xEnd; x++)
		{
			int32 r = color.x * 255;
			int32 g = color.y * 255;
			int32 b = color.z * 255;
			*pixels++ = ((255 << 24) | (r << 16) | (g << 8) | b);
		}
		row += buffer->width;
	}
}

vertex_buffer_3d* GetVertexBuffer(graphics_context *GraphicsContext, uint32 BufferID){
	ASSERT(BufferID < GraphicsContext->VertexBufferCount);
	return &GraphicsContext->VertexBuffers[BufferID];
}
index_buffer* GetIndexBuffer(graphics_context *GraphicsContext, uint32 BufferID){
	ASSERT(BufferID < GraphicsContext->IndexBufferCount);
	return &GraphicsContext->IndexBuffers[BufferID];
}

void FillRenderTarget(graphics_context *GraphicsContext, uint32 VertexBufferID){
	vertex_buffer_3d* VertexBuffer = GetVertexBuffer(GraphicsContext, VertexBufferID);
	// TODO: Clear RenderTarget
	vec3* RenderTarget = (vec3*)GraphicsContext->RenderTarget;
	for(int i = 0; i < VertexBuffer->Count / sizeof(vec3); i++){
		*RenderTarget++ = *((vec3*)VertexBuffer->Vertices + i);
	}
}

void NDCToScreen(game_back_buffer* buffer, vec3& point){
	ASSERT(point.x >= -1.f &&  point.x <= 1.0f);
	ASSERT(point.y >= -1.f &&  point.y <= 1.0f);

	point.x = (point.x + 1)/2.f *  buffer->width;
	point.y = (1 - point.y)/2.f * buffer->height;

	ASSERT(point.x >= 0)
	ASSERT(point.y >= 0)
	ASSERT(point.x <= buffer->width);
	ASSERT(point.y <= buffer->height);
}

void DrawMesh(graphics_context *GraphicsContext, game_back_buffer* Buffer, uint32 VertexBufferID, uint32 IndexBufferID, uint32 Count){
	// TODO: Get Attributes From Layout

	vec3 Colors[12] = {
		{0.8f, 0.2f, 0.3f},	
		{0.75f, 0.2f, 0.3f},	
		{0.39f, 0.7f, 0.57f},
		{0.29f, 0.6f, 0.47f},
		{0.7f, 0.4f, 0.92f},	
		{0.6f, 0.3f, 0.82f},	
		{0.1f, 0.23f, 0.72f},	
		{0.05f, 0.26f, 0.83f},
		{0.2f, 0.2f, 0.2f},
		{0.25f, 0.25f, 0.25f},
		{0.93f, 0.63f, 0.23f},
		{0.93f, 0.73f, 0.23f}
	};

	// TODO: Convert To Binding Mechanism
	// vertex_buffer_3d* VertexBuffer = GetVertexBuffer(GraphicsContext, VertexBufferID);
	vec3* RenderTarget = (vec3*)GraphicsContext->RenderTarget;
	index_buffer* IndexBuffer = GetIndexBuffer(GraphicsContext, IndexBufferID);

	ASSERT(Count <= IndexBuffer->Count/sizeof(uint32));
	ASSERT(Count % 3 == 0);
	for(uint32 i = 0; i < Count/3; i++){
		// const vec3* Point1 = (vec3*)VertexBuffer->Vertices[*(((uint32*)IndexBuffer->Indices) + (i*3+0))];

		int triplet = i*3;
		uint32 index1 = *((uint32*)IndexBuffer->Indices + (triplet + 0));
		uint32 index2 = *((uint32*)IndexBuffer->Indices + (triplet + 1));
		uint32 index3 = *((uint32*)IndexBuffer->Indices + (triplet + 2));

		// don't ovewrite render target vertices
		// they will be indexed to again by other triangles
		vec3 Point1 = *(RenderTarget + index1);
		vec3 Point2 = *(RenderTarget + index2);
		vec3 Point3 = *(RenderTarget + index3);

		NDCToScreen(Buffer, Point1);
		NDCToScreen(Buffer, Point2);
		NDCToScreen(Buffer, Point3);

		DrawTriangle(Buffer, Point1, Point2, Point3, Colors[i]);
	}
}

void ClearBuffer(game_back_buffer* buffer){
	uint32 *memory = (uint32*)buffer->memory;
	for(int i = 0; i < buffer->width * buffer->height; i++){
		uint8 r = 104;
		uint8 g = 89;
		uint8 b = 94;
		*memory++ = (255 << 24) | (r << 16) | (g << 8) | b;
	}
}


void PerspectiveProjection(graphics_context *GraphicsContext, uint32 VertexBufferID, float angle, float n, float f, float aspect_ratio = 1.f){

	// TODO: 
	float A = -(f+n)/(f-n);
	float B = -2*f*n/(f-n);
	float a = 1.f/(tan(DegreesToRadians(angle/2)) * aspect_ratio);
	float b = 1.f/tan(DegreesToRadians(angle/2));

	vertex_buffer_3d* VertexBuffer = GetVertexBuffer(GraphicsContext, VertexBufferID);
	vec3* RenderTarget = (vec3*)GraphicsContext->RenderTarget;

	for(int i = 0; i < VertexBuffer->Count/sizeof(vec3); i++){
		float w = -RenderTarget->z;
		RenderTarget->x = (RenderTarget->x * a)/w;
		RenderTarget->y = (RenderTarget->y * b)/w;
		RenderTarget->z = (RenderTarget->z * A + B)/w;
		RenderTarget++;
	}
}

void InputAssemble(graphics_context *GraphicsContext, uint32 VertexBufferID, uint32 IndexBufferID){

}
void VertexTransform(graphics_context *GraphicsContext, uint32 VertexBufferID, const mat4& transform){

	// TODO: buffer layout
	vec3 *RenderTarget = (vec3*)GraphicsContext->RenderTarget;
	vertex_buffer_3d *VertexBuffer = GetVertexBuffer(GraphicsContext, VertexBufferID);

	for(int i = 0;i < VertexBuffer->Count/sizeof(vec3);i++){
		vec3 &vertex = *((vec3*)RenderTarget + i);
		vertex = transform.multiply(vertex);
	}
}

uint32 CreateVertexBuffer(game_state* GameState, uint32 Size, void* Data){
	uint32_t VertexBufferID;
	graphics_context *GraphicsContext = GameState->GraphicsContext;
	memory_arena& MemoryArena = GameState->MemoryArena;

	ASSERT(GraphicsContext->Initialized);

	vertex_buffer_3d* VertexBuffer = (GraphicsContext->VertexBuffers + GraphicsContext->VertexBufferCount);
	VertexBufferID = GraphicsContext->VertexBufferCount++;
	VertexBuffer->Count = Size;

	uint8* Memory = PushArray(MemoryArena, Size, uint8);
	VertexBuffer->Vertices = Memory;
	if(Data){
		uint8* MemoryIterator = Memory;
		uint8* DataIterator = (uint8*)Data;
		for(int i = 0; i < Size; i++){
			*MemoryIterator++ = *DataIterator++;
		}
	}

	return VertexBufferID;
}
uint32 CreateIndexBuffer(game_state* GameState, uint32 Size, void* Data){
	uint32_t IndexBufferID;
	graphics_context *GraphicsContext = GameState->GraphicsContext;
	memory_arena& MemoryArena = GameState->MemoryArena;

	ASSERT(GraphicsContext->Initialized);

	index_buffer* IndexBuffer = (GraphicsContext->IndexBuffers + GraphicsContext->IndexBufferCount);
	IndexBufferID = GraphicsContext->IndexBufferCount++;
	IndexBuffer->Count = Size;

	uint8* Memory = PushArray(MemoryArena, Size, uint8);
	IndexBuffer->Indices = Memory;
	if(Data){
		uint8* MemoryIterator = Memory;
		uint8* DataIterator = (uint8*)Data;
		for(int i = 0; i < Size; i++){
			*MemoryIterator++ = *DataIterator++;
		}
	}

	return IndexBufferID;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	ClearBuffer(buffer);
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
		TileMap->TileSideInMetres = 1.4f;
		TileMap->MetresToPixels = TileMap->TileSideInPixels/TileMap->TileSideInMetres;

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

		GameState->CameraP.AbsTileX = 9;
		GameState->CameraP.AbsTileY = 3;
		GameState->CameraP.Offset = {0.f, 0.f};

		
		// Graphics Context
		graphics_context GraphicsContext = {};
		GraphicsContext.VertexBuffers = PushArray(GameState->MemoryArena, MAX_VERTEX_BUFFERS, vertex_buffer_3d);
		GraphicsContext.IndexBuffers = PushArray(GameState->MemoryArena, MAX_INDEX_BUFFERS, index_buffer);
		GameState->GraphicsContext = PushStruct(GameState->MemoryArena, graphics_context);
		GraphicsContext.RenderTarget = PushArray(GameState->MemoryArena, MAX_RENDER_TARGET_SIZE, uint8);
		GraphicsContext.Initialized = true;
		*GameState->GraphicsContext = GraphicsContext;

		float vertices[3 * 8] = {
			-1.f,-1.f, 1.f,
			 1.f,-1.f, 1.f,
			 1.f, 1.f, 1.f,
			-1.f, 1.f, 1.f,
			-1.f,-1.f,-1.f, 
			 1.f,-1.f,-1.f,
			 1.f, 1.f,-1.f,
			-1.f, 1.f,-1.f
		};
		unsigned int indices[36]{
			0,1,2,0,2,3,
			5,4,7,5,7,6,
			4,0,3,4,3,7,
			1,5,6,1,6,2,
			3,2,6,3,6,7,
			1,0,4,1,4,5
		};

		uint32 VertexBuffer = CreateVertexBuffer(GameState, sizeof(vertices), (void*)vertices);
		uint32 IndexBuffer = CreateIndexBuffer(GameState, sizeof(indices), (void*)indices);

		memory->isInitialized = true;
	}

	// ENTITY MOVEMENT
	for (int controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
	{
		const game_controller_input *controller = &input->controllers[controllerIndex];
		vec2 acceleration = {0.f, 0.f};

		entity *controllerEntity = GetEntity(GameState, GameState->EntityIndexForController[controllerIndex]);
		if(controllerEntity){
			if (controller->isAnalog){
				acceleration = vec2{controller->stickAverageX, controller->stickAverageY};
			}
			else{
				static real32 sensitivity = 1.f;
				static int wait = 1;
				if (wait) {
					wait--;
				}
				if (!wait) {
					if (controller->actionUp.endedDown){
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
			}
			MovePlayer(GameState, controllerEntity, acceleration, input->dt);
		}else{
			if(controller->start.endedDown){
				uint32 entityIndex = AddEntity(GameState);
				entity* Entity = GetEntity(GameState, entityIndex);
				GameState->EntityIndexForController[controllerIndex] = entityIndex;
			}
		}
	}

	// CAMERA MOVEMENT
	const tile_map *TileMap = GameState->World->TileMap;
	entity *CameraFollowingEntity = GetEntity(GameState, GameState->EnitytCameraFollowsIndex);
	if(CameraFollowingEntity){
		GameState->CameraP.AbsTileZ = CameraFollowingEntity->Position.AbsTileZ;

		tile_map_position CameraP = GameState->CameraP;
		vec2 Difference = {}; 
		Difference.x = (CameraFollowingEntity->Position.AbsTileX - CameraP.AbsTileX) * TileMap->TileSideInMetres;
		Difference.y = (CameraFollowingEntity->Position.AbsTileY - CameraP.AbsTileY) * TileMap->TileSideInMetres;
		Difference.x += CameraFollowingEntity->Position.Offset.x - CameraP.Offset.x;
		Difference.y += CameraFollowingEntity->Position.Offset.y - CameraP.Offset.y;

		uint32 TilesPerWidth = 19;
		uint32 TilesPerHeight = 7;

		if(Difference.x > TileMap->TileSideInMetres * TilesPerWidth/2.f){
			GameState->CameraP.AbsTileX += TilesPerWidth;
		}
		if(Difference.x < -TileMap->TileSideInMetres * TilesPerWidth/2.f){
			GameState->CameraP.AbsTileX -= TilesPerWidth;
		}
		if(Difference.y > TileMap->TileSideInMetres * TilesPerHeight/2.f){
			GameState->CameraP.AbsTileY += TilesPerHeight;
		}
		if(Difference.y < -TileMap->TileSideInMetres * TilesPerHeight/2.f){
			GameState->CameraP.AbsTileY -= TilesPerHeight;
		}
	}

	// DRAWING TILES
	real32 CenterX = buffer->width/2.f;
	real32 CenterY = buffer->height/2.f;
	DrawRectangle(buffer, 0, 0, buffer->width, buffer->height, 0.2f, 0.3f, 0.8f);
	for (int32 RelRow = -3; RelRow <= 3; RelRow++)
	{
		for (int32 RelColumn = -9; RelColumn <= 9; RelColumn++)
		{
			real32 Gray = 0.25f;

			uint32 AbsColumn = GameState->CameraP.AbsTileX + RelColumn;
			uint32 AbsRow = GameState->CameraP.AbsTileY + RelRow;

			uint32 TileValue = GetTileValue(GameState->World->TileMap, AbsColumn, AbsRow, 0);
			if(TileValue > 0){
				if (TileValue == 2)
					Gray = 1.0f;

				if((AbsColumn == GameState->CameraP.AbsTileX) && (AbsRow == GameState->CameraP.AbsTileY))
					Gray -= 0.1f;

				int32 MinX = CenterX - TileMap->TileSideInPixels/2.f + (RelColumn * TileMap->TileSideInPixels) - (GameState->CameraP.Offset.x * TileMap->MetresToPixels);
				int32 MinY = CenterY - TileMap->TileSideInPixels/2.f - (RelRow * TileMap->TileSideInPixels) + (GameState->CameraP.Offset.y * TileMap->MetresToPixels);
				int32 MaxX = MinX + TileMap->TileSideInPixels;
				int32 MaxY = MinY + TileMap->TileSideInPixels;
				DrawRectangle(buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
			}
		}
	}

	// DRAWING ENTITIES
	// Note: Start from index 1 because index 0 is always NULL entity
	// We could also check if Entity->Exists instead
	// In this case we are doing both
	entity *Entity = &GameState->Entities[1];
	for (uint32 EntityIndex = 0; EntityIndex < GameState->EntityCount; EntityIndex++, Entity++){
		if(Entity->Exists){
			vec2 PlayerCameraRelPosition = {};
			PlayerCameraRelPosition.x = (Entity->Position.AbsTileX - GameState->CameraP.AbsTileX) * TileMap->TileSideInMetres;
			PlayerCameraRelPosition.y = (Entity->Position.AbsTileY - GameState->CameraP.AbsTileY) * TileMap->TileSideInMetres;
			PlayerCameraRelPosition.x += (Entity->Position.Offset.x - GameState->CameraP.Offset.x);
			PlayerCameraRelPosition.y += (Entity->Position.Offset.y - GameState->CameraP.Offset.y);

			int32 minx = CenterX + (PlayerCameraRelPosition.x - Entity->PlayerWidth/2.f) * TileMap->MetresToPixels;
			int32 miny = CenterY - (PlayerCameraRelPosition.y + Entity->PlayerHeight/2.f) * TileMap->MetresToPixels;
			int32 maxx = minx + Entity->PlayerWidth * TileMap->MetresToPixels;
			int32 maxy = miny + Entity->PlayerHeight * TileMap->MetresToPixels;
			DrawRectangle(buffer, minx, miny, maxx, maxy, 0.85f, 0.25f, 0.3f);
			// DrawRectangle(buffer, 0, buffer->height/2, buffer->width, buffer->height/2+1, 0.8f, 0.2, 0.3f);
			// DrawRectangle(buffer, buffer->width/2, 0, buffer->width/2+1, buffer->height, 0.8f, 0.2, 0.3f);


			// DrawBitmap(buffer, &GameState->BMPPixels, buffer->width/2.f - GameState->BMPPixels.Width/2, buffer->height/2.f - GameState->BMPPixels.Height/2);
		}
	}

	ClearBuffer(buffer);
	uint32 IndexBuffer = 0;
	uint32 VertexBuffer = 0;
	static float angle = 0;
	angle += 5.01f;
	FillRenderTarget(GameState->GraphicsContext, VertexBuffer);
	mat4 rotX = mat4::RotationY(angle*0.666f);
	mat4 rotY = mat4::RotationX(angle);
	VertexTransform(GameState->GraphicsContext, VertexBuffer, rotX.multiply(rotY));
	mat4 transform = mat4::translation({0, 0, -4.5f});
	VertexTransform(GameState->GraphicsContext, VertexBuffer, transform);
	InputAssemble(GameState->GraphicsContext, VertexBuffer, IndexBuffer);
	PerspectiveProjection(GameState->GraphicsContext, VertexBuffer, 80 + sin(angle*0.015f) * 10, 1, 10, buffer->width/float(buffer->height));

	// for (int i = 0; i < vertexBuffer.Count; i++){
	// 	NDCToScreen(buffer, vertexBuffer.Vertices[i]);
	// }
	DrawMesh(GameState->GraphicsContext, buffer, VertexBuffer, IndexBuffer, 36);
}
