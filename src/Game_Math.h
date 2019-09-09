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


//////////////////////////////////////////////

struct vec3{
    real32 x, y, z;
};

vec3 operator*(real32 scale, vec3 v){
    vec3 Result = v;
    Result.x *= scale;
    Result.y *= scale;
    Result.z *= scale;
    return Result;
}

vec3 operator*(vec3 v, real32 scale){
    vec3 Result = v;
    Result.x *= scale;
    Result.y *= scale;
    Result.z *= scale;
    return Result;
}

vec3 operator+(vec3 a, vec3 b){
    vec3 Result;
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;
    Result.z = a.z + b.z;
    return Result;
}

vec3 operator-(vec3 a, vec3 b){
    vec3 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    Result.z = a.z - b.z;
    return Result;
}

vec3& operator+=(vec3 &a, vec3 b){
    a = a + b;
    return a;
}
vec3& operator-=(vec3 &a, vec3 b){
    a = a - b;
    return a;
}

real32 dot(vec3 a, vec3 b){
    real32 Result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return Result;
}

vec3 cross(vec3 a, vec3 b){
    real32 x = (a.y * b.z) - (a.z * b.y);
    real32 y = (a.x * b.z) - (a.z * b.x);
    real32 z = (a.x * b.y) - (a.y * b.x);
    return {x, y, z};
}

float DegreesToRadians(float angle){
    return angle/180.f * PI;
}