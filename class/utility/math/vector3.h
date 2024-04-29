#ifndef _VECTOR3_CLASS
#define _VECTOR3_CLASS

#include <iostream>
#include <string>
#include <cmath>

class Vector3Int;

class Vector3
{
private:
public:
    float x = 0, y = 0, z = 0;
    
    Vector3();
    Vector3(const Vector3Int&);
    Vector3(float x, float y, float z);

    Vector3Int round();
    Vector3Int floor();

    Vector3 operator+(const Vector3& other);
    Vector3& operator+=(const Vector3& other);

    Vector3 operator-(const Vector3& other);
    Vector3& operator-=(const Vector3& other);

    // per component multiplication
    Vector3 operator*(const Vector3& other);
    // per component multiplication
    Vector3& operator*=(const Vector3& other);
    Vector3 operator*(float scalar);
    Vector3& operator*=(float scalar);

    // per component division
    Vector3 operator/(const Vector3& other);
    // per component division
    Vector3& operator/=(const Vector3& other);
    Vector3 operator/(float scalar);
    Vector3& operator/=(float scalar);

    // per component mod
    Vector3 operator%(float scalar);
    // per component mod
    Vector3& operator%=(float scalar);

    // return the component at this index (0 => x, 1 => y, 2 => z)
    // throws an error if index < 0 or index > 2
    float& operator[](int index);
    bool operator==(const Vector3& other);
    bool operator!=(const Vector3& other);

    // return a vector othogonal to the two vectors
    Vector3 cross(const Vector3& other);
    // return the dot product of the two vectors
    float dot(const Vector3& other);
    // return the length of the vector
    float magnitude();
    // return the squared length of the vector
    float sqrmagnitude();
    // normalize the vector in place and return itself
    Vector3& normalize();
    // return a new vector with same direction and a norm of 1
    Vector3 normalized();

    std::string to_str();
};

class Vector3Int
{
private:
public:
    int x = 0, y = 0, z = 0;
    
    Vector3Int();
    Vector3Int(const Vector3&);
    Vector3Int(int x, int y, int z);

    Vector3Int operator+(const Vector3& other);
    Vector3Int& operator+=(const Vector3& other);

    Vector3Int operator-(const Vector3& other);
    Vector3Int& operator-=(const Vector3& other);

    // per component multiplication
    Vector3Int operator*(const Vector3& other);
    // per component multiplication
    Vector3Int& operator*=(const Vector3& other);
    Vector3Int operator*(float scalar);
    Vector3Int& operator*=(float scalar);

    // per component division
    Vector3Int operator/(const Vector3& other);
    // per component division
    Vector3Int& operator/=(const Vector3& other);
    Vector3Int operator/(float scalar);
    Vector3Int& operator/=(float scalar);

    // per component mod
    Vector3Int operator%(int scalar);
    // per component mod
    Vector3Int& operator%=(int scalar);

    // return the component at this index (0 => x, 1 => y, 2 => z)
    // throws an error if index < 0 or index > 2
    int& operator[](int index);
    bool operator==(const Vector3Int& other);
    bool operator!=(const Vector3Int& other);

    // return the dot product of the two vectors
    float dot(const Vector3& other);
    // return the length of the vector
    float magnitude();
    // return the squared length of the vector
    float sqrmagnitude();

    std::string to_str();
};

#endif