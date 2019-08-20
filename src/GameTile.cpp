inline tile_chunk_position
GetChunkPosFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ){
	tile_chunk_position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.RelTile.x = AbsTileX & TileMap->ChunkMask;
	Result.RelTile.y = AbsTileY & TileMap->ChunkMask;

	return Result;
}

inline void 
RecanonicalizeCoord(tile_map *TileMap, int32 *Tile, real32 *TileRel)
{
	int32 Offset = Round(*TileRel / TileMap->TileSideInMetres);
	*Tile += Offset;
	*TileRel -= Offset * TileMap->TileSideInMetres;	

	ASSERT(*TileRel <= TileMap->TileSideInMetres/2.0f);
	ASSERT(*TileRel >= -TileMap->TileSideInMetres/2.0f);
}

inline tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
	tile_map_position Result = Pos;
	
	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.x);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.y);
	return Result;
}

inline tile_chunk*
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
	tile_chunk *TileChunk = nullptr;

	if((TileChunkX < TileMap->TileChunkCountX)
	 && (TileChunkY < TileMap->TileChunkCountY)
	 && (TileChunkZ < TileMap->TileChunkCountZ)){

		// 3D indexing ->    [z * (y * width * height)] + [(y * width)] + [x]
		TileChunk = &TileMap->TileChunks[
			TileChunkZ * (TileChunkY * TileMap->TileChunkCountX * TileMap->TileChunkCountX)
			+ (TileChunkY * TileMap->TileChunkCountX)
			+ TileChunkX];
	 }

	return TileChunk;
}

inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
	ASSERT(TileX >= 0);
	ASSERT(TileY >= 0);
	ASSERT(TileX < TileMap->ChunkDim);
	ASSERT(TileY < TileMap->ChunkDim);

	int32 Result = TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX];
	return Result;
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, int32 TileX, int32 TileY, uint32 TileValue)
{
	ASSERT(TileX >= 0);
	ASSERT(TileY >= 0);
	ASSERT(TileX < TileMap->ChunkDim);
	ASSERT(TileY < TileMap->ChunkDim);

	TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX] = TileValue;
}

inline bool32	
GetTileChunkTileValue(tile_map *TileMap, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
	uint32 Result = 0;
	if(TileChunk && TileChunk->Tiles){
		Result = GetTileValueUnchecked(TileMap, TileChunk, TileX, TileY);
	}
	return Result;
}

// inline void	
// SetTileChunkTileValue(tile_map *TileMap, tile_chunk *TileChunk, int32 TileX, int32 TileY, uint32 TileValue)
// {
// 	uint32 Result = 0;
// 	if(TileChunk){
// 		Result = SetTileValueUnchecked(TileMap, TileChunk, TileX, TileY, TileValue);
// 	}
// 	return Result;
// }

internal bool32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position ChunkPos = GetChunkPosFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
	bool32 Result = GetTileChunkTileValue(TileMap, TileChunk, ChunkPos.RelTile.x, ChunkPos.RelTile.y);
	return Result;
}

internal void
SetTileValue(memory_arena &Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPosFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);

	// TODO: Create TileChunks Dynamically
	ASSERT(TileChunk);
	if(!TileChunk->Tiles){
		TileChunk->Tiles = PushArray(Arena, TileMap->ChunkDim * TileMap->ChunkDim, int32);
		for(int i =0; i < TileMap->ChunkDim * TileMap->ChunkDim; i++){
			TileChunk->Tiles[i] = 1;
		}
	}
	SetTileValueUnchecked(TileMap, TileChunk, ChunkPos.RelTile.x, ChunkPos.RelTile.y, TileValue);
}

internal bool32
IsWorldPointEmpty(tile_map *TileMap, tile_map_position CanPos)
{
	bool32 Result = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);
	return (Result == 1);
}