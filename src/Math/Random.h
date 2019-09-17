struct random_state{
    uint32 Index;
};

inline uint32 Rand(random_state* State){
    uint32 x = State->Index;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return State->Index = x;
}

float RandF(random_state* State){
    return Rand(State)/(float)0xFFFFFFFF;
}