#pragma once

inline uint32 truncateUint64(uint64_t value)
{
	ASSERT(value < 0xFFFFFFFF);
	return (uint32)value;
}

#include <math.h>
inline int32
FloorReal32ToInt32(real32 value)
{
	return (int32)floorf(value);
	// if(n < 0){
		// n -= 1;
	// }
	// return (int32)n;
}

inline int32
Round(real32 value){
	return roundf(value);
}

inline int32
Ciel(real32 value){
	return ceilf(value);
}

inline real32 
Square(real32 value){
	real32 Result = value * value;
	return Result;
}
