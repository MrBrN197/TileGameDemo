#pragma once

struct vec2{
    real32 x, y;
};

vec2 operator*(real32 scale, vec2 v){
    vec2 Result = v;
    Result.x *= scale;
    Result.y *= scale;
    return Result;
}

vec2 operator*(vec2 v, real32 scale){
    vec2 Result = v;
    Result.x *= scale;
    Result.y *= scale;
    return Result;
}

vec2 operator+(vec2 a, vec2 b){
    vec2 Result;
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;
    return Result;
}

vec2 operator-(vec2 a, vec2 b){
    vec2 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    return Result;
}

vec2& operator+=(vec2 &a, vec2 b){
    a = a + b;
    return a;
}
vec2& operator-=(vec2 &a, vec2 b){
    a = a - b;
    return a;
}

real32 dot(vec2 a, vec2 b){
    real32 Result = (a.x * b.x) + (a.y * b.y);
    return Result;
}