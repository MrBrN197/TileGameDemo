#include "Vec4.h"
#include "Vec3.h"

struct mat4{

    union {
        float elements[16];
        vec4 columns[4];
    };

    mat4(){
        for(int i = 0; i < 16; i++){
            elements[i] = 0;
        }
        elements[0 * 4 + 0] = 1;
        elements[1 * 4 + 1] = 1;
        elements[2 * 4 + 2] = 1;
        elements[3 * 4 + 3] = 1;
    }

    mat4 multiply(const mat4& other) const{
        mat4 Result = {};
        for(int i = 0; i < 16; i++){
            Result.elements[i] = 0;
        }
        for(int column  = 0; column < 4; column++){
            for(int row  = 0; row < 4; row++){
                for (int e = 0; e < 4; e++){
                    Result.columns[column][row] += this->columns[e][row] * other.columns[column][e];
                }                
            }
        }
        return Result;
    }
    vec3 multiply(const vec3& vec) const{
        vec3 Result = {};
        Result.x = this->columns[0].x * vec.x + this->columns[1].x * vec.y + this->columns[2].x * vec.z + this->columns[3].x;
        Result.y = this->columns[0].y * vec.x + this->columns[1].y * vec.y + this->columns[2].y * vec.z + this->columns[3].y;
        Result.z = this->columns[0].z * vec.x + this->columns[1].z * vec.y + this->columns[2].z * vec.z + this->columns[3].z;
        return Result;
    }

    mat4 operator*(const mat4& rhs) const{
        return this->multiply(rhs);
    }
    mat4& operator*=(const mat4& other){
        *this = this->multiply(other);
        return *this;
    }
    vec3 operator*(const vec3& other) const{
        return this->multiply(other);
    }

    static mat4 translation(const vec3& translate){
        mat4 matrix{};
        matrix.columns[3].x = translate.x;
        matrix.columns[3].y = translate.y;
        matrix.columns[3].z = translate.z;
        return matrix;        
    }
    static mat4 scale(const vec3 scale){
        mat4 matrix{};
        matrix.columns[0][0] = scale.x;
        matrix.columns[1][1] = scale.y;
        matrix.columns[2][2] = scale.z;
    }

    static mat4 RotationX(float angle){
        mat4 rotationMatrix{};

        float rad = DegreesToRadians(angle);
        double cosTheta = cosf(rad);
        double sinTheta = sinf(rad);

        rotationMatrix.columns[1][1] = cosTheta;
        rotationMatrix.columns[1][2] = sinTheta;
        rotationMatrix.columns[2][1] = -sinTheta;
        rotationMatrix.columns[2][2] = cosTheta;

        return rotationMatrix;
    }

    static mat4 RotationY(float angle){
        mat4 rotationMatrix{};
        
        float rad = DegreesToRadians(angle);
        double cosTheta = cosf(rad);
        double sinTheta = sinf(rad);

        rotationMatrix.columns[0][0] = cosTheta;
        rotationMatrix.columns[2][0] = sinTheta;
        rotationMatrix.columns[0][2] = -sinTheta;
        rotationMatrix.columns[2][2] = cosTheta;

        return rotationMatrix;
    }

    static mat4 RotationZ(float angle){

        mat4 rotationMatrix;

        float rad = DegreesToRadians(angle);
        double cosTheta = cosf(rad);
        double sinTheta = sinf(rad);

        rotationMatrix.columns[0][0] = cosTheta;
        rotationMatrix.columns[0][1] = sinTheta;
        rotationMatrix.columns[1][0] = -sinTheta;
        rotationMatrix.columns[1][1] = cosTheta;

        return rotationMatrix;
    }
};
