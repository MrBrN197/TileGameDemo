inline tile_chunk_position
GetChunkPosFor(world *World, uint32 AbsTileX, uint32 AbsTileY){
	tile_chunk_position Result;

	Result.TileChunkX = AbsTileX >> World->TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> World->TileMap->ChunkShift;
	Result.RelTileX = AbsTileX & World->TileMap->ChunkMask;
	Result.RelTileY = AbsTileY & World->TileMap->ChunkMask;

	return Result;
}

inline void 
RecanonicalizeCoord(world *World, int32 *Tile, real32 *TileRel)
{
	int32 Offset = FloorReal32ToInt32(*TileRel / World->TileMap->TileSideInPixels);
	*Tile += Offset;
	*TileRel -= Offset * World->TileMap->TileSideInPixels;	

	ASSERT(*TileRel < World->TileMap->TileSideInPixels);
}

inline tile_map_position
RecanonicalizePosition(world *World, tile_map_position Pos)
{
	tile_map_position Result = Pos;
	
	RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);
	return Result;
}

inline tile_chunk*
GetTileChunk(world *World, uint32 TileChunkX, uint32 TileChunkY)
{
	tile_chunk *TileChunk = nullptr;

	if((TileChunkY < World->TileMap->TileChunkCountY) && (TileChunkX < World->TileMap->TileChunkCountX))
		TileChunk = &World->TileMap->TileChunks[TileChunkY * World->TileMap->TileChunkCountX + TileChunkX];

	return TileChunk;
}

inline uint32
GetTileValueUnchecked(world *World, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
	ASSERT(TileX >= 0);
	ASSERT(TileY >= 0);
	ASSERT(TileX < World->TileMap->ChunkDim);
	ASSERT(TileY < World->TileMap->ChunkDim);

	int32 Result = TileChunk->Tiles[TileY * World->TileMap->ChunkDim + TileX];
	return Result;
}

inline void
SetTileValueUnchecked(world *World, tile_chunk *TileChunk, int32 TileX, int32 TileY, uint32 TileValue)
{
	ASSERT(TileX >= 0);
	ASSERT(TileY >= 0);
	ASSERT(TileX < World->TileMap->ChunkDim);
	ASSERT(TileY < World->TileMap->ChunkDim);

	TileChunk->Tiles[TileY * World->TileMap->ChunkDim + TileX] = TileValue;
}

inline bool32	
GetTileChunkTileValue(world *World, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
	uint32 Result = 0;
	if(TileChunk){
		Result = GetTileValueUnchecked(World, TileChunk, TileX, TileY);
	}
	return Result;
}

// inline void	
// SetTileChunkTileValue(world *World, tile_chunk *TileChunk, int32 TileX, int32 TileY, uint32 TileValue)
// {
// 	uint32 Result = 0;
// 	if(TileChunk){
// 		Result = SetTileValueUnchecked(World, TileChunk, TileX, TileY, TileValue);
// 	}
// 	return Result;
// }

internal bool32
GetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
	tile_chunk_position ChunkPos = GetChunkPosFor(World, AbsTileX, AbsTileY);
	tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	bool32 Result = GetTileChunkTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
	return Result;
}

internal void
SetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY, uint32 TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPosFor(World, AbsTileX, AbsTileY);
	tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);

	// TODO: Create TileChunks Dynamically
	ASSERT(TileChunk);

	SetTileValueUnchecked(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}

internal bool32
IsWorldPointEmpty(world *World, tile_map_position CanPos)
{
	bool32 Result = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
	return (Result == 0);
}