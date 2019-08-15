#pragma once

struct tile_chunk_position{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;
	
	vec2 RelTile;
};

struct tile_map_position{
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;

	vec2 Offset;
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
	real32 MetersToPixels;
	
	int32 TileChunkCountX;
	int32 TileChunkCountY;
	int32 TileChunkCountZ;

	tile_chunk* TileChunks;
};