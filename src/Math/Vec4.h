#pragma once

struct vec4{
    real32 x, y, z, w;  // TODO: handle w properly

    float& operator[](size_t index){
        return (float&)*((real32*)&this->x + index);
    }
    float operator[](size_t index) const{
        return (float)*((real32*)&this->x + index);
    }
};

vec4 operator*(real32 scale, vec4 v){
    vec4 Result = v;
    Result.x *= scale;
    Result.y *= scale;
    Result.z *= scale;
    Result.w = v.w; // TODO: handle w properly
    return Result;
}

vec4 operator*(vec4 v, real32 scale){
    vec4 Result = v;
    Result.x *= scale;
    Result.y *= scale;
    Result.z *= scale;
    Result.w = v.w; // TODO: handle w properly
    return Result;
}

vec4 operator+(vec4 a, vec4 b){
    vec4 Result = {};
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;
    Result.z = a.z + b.z;
    Result.w = a.w; // TODO: handle w properly
    return Result;
}

vec4 operator-(vec4 a, vec4 b){
    vec4 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    Result.z = a.z - b.z;
    Result.w = a.w;   // TODO: handle w properly
    return Result;
}

vec4& operator+=(vec4 &a, vec4 b){
    a = a + b;
    return a;
}
vec4& operator-=(vec4 &a, vec4 b){
    a = a - b;
    return a;
}

// real32 dot(vec4 a, vec4 b){
//     real32 Result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
//     return Result;
// }

// vec4 cross(vec4 a, vec4 b){
//     real32 x = (a.y * b.z) - (a.z * b.y);
//     real32 y = (a.x * b.z) - (a.z * b.x);
//     real32 z = (a.x * b.y) - (a.y * b.x);
//     return {x, y, z};
// }