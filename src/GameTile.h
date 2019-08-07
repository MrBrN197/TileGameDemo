#pragma once

struct tile_chunk_position{
	uint32 TileChunkX;
	uint32 TileChunkY;
	
	uint32 RelTileX;
	uint32 RelTileY;
};

struct tile_map_position{
	int32 AbsTileX;
	int32 AbsTileY;

	real32 TileRelX;
	real32 TileRelY;
};

struct tile_chunk{
	int32* Tiles;
};

struct tile_map{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	real32 TileSideInMeters;
	real32 TileSideInPixels;
	
	real32 UpperLeftX;
	real32 UpperLeftY;

	int32 TileChunkCountX;
	int32 TileChunkCountY;

	tile_chunk* TileChunks;
};