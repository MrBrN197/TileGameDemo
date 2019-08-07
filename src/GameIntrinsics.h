#pragma once

inline uint32 truncateUint64(uint64_t n)
{
	ASSERT(n < 0xFFFFFFFF);
	return (uint32)n;
}

inline int32
FloorReal32ToInt32(real32 n)
{
	if(n < 0){
		n -= 1;
	}
	return (int32)n;
}


