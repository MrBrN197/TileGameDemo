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

vec2 operator+(vec2 v1, vec2 v2){
    vec2 Result;
    Result.x = v1.x + v2.x;
    Result.y = v1.y + v2.y;
    return Result;
}

vec2 operator-(vec2 v1, vec2 v2){
    vec2 Result;
    Result.x = v1.x - v2.x;
    Result.y = v1.y - v2.y;
    return Result;
}

vec2& operator+=(vec2 &v1, vec2 v2){
    v1 = v1 + v2;
    return v1;
}
vec2& operator-=(vec2 &v1, vec2 v2){
    v1 = v1 - v2;
    return v1;
}