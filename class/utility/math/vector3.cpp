#ifndef _VECTOR3_CLASS

#include "./vector3.h"

#pragma region vector3
Vector3::Vector3(float x, float y, float z){
    this->x = x;
    this->y = y;
    this->z = z;
}
Vector3::Vector3(const Vector3Int& other){
    this->x = other.x;
    this->y = other.y;
    this->z = other.z;
}
Vector3::Vector3(){
    this->x = 0;
    this->y = 0;
    this->z = 0;
}

Vector3Int Vector3::round() {
    return Vector3Int(
        roundf(this->x),
        roundf(this->y),
        roundf(this->z)
    );
}
Vector3Int Vector3::floor() {
    return Vector3Int(
        floorf(this->x),
        floorf(this->y),
        floorf(this->z)
    );
}

Vector3 Vector3::operator+(const Vector3& other){
    return Vector3(
        this->x + other.x,
        this->y + other.y,
        this->z + other.z
    );
}
Vector3& Vector3::operator+=(const Vector3& other){
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;
    return *this;
}

Vector3 Vector3::operator-(const Vector3& other){
    return Vector3(
        this->x - other.x,
        this->y - other.y,
        this->z - other.z
    );
}
Vector3& Vector3::operator-=(const Vector3& other){
    this->x -= other.x;
    this->y -= other.y;
    this->z -= other.z;
    return *this;
}

// per component multiplication
Vector3 Vector3::operator*(const Vector3& other){
    return Vector3(
        this->x * other.x,
        this->y * other.y,
        this->z * other.z
    );
}
// per component multiplication
Vector3& Vector3::operator*=(const Vector3& other){
    this->x *= other.x;
    this->y *= other.y;
    this->z *= other.z;
    return *this;
}
Vector3 Vector3::operator*(float scalar){
    return Vector3(
        this->x * scalar,
        this->y * scalar,
        this->z * scalar
    );
}
Vector3& Vector3::operator*=(float scalar){
    this->x *= scalar;
    this->y *= scalar;
    this->z *= scalar;
    return *this;
}

// per component division
Vector3 Vector3::operator/(const Vector3& other){
    return Vector3(
        this->x / other.x,
        this->y / other.y,
        this->z / other.z
    );
}
// per component division
Vector3& Vector3::operator/=(const Vector3& other){
    this->x /= other.x;
    this->y /= other.y;
    this->z /= other.z;
    return *this;
}
Vector3 Vector3::operator/(float scalar){
    return Vector3(
        this->x / scalar,
        this->y / scalar,
        this->z / scalar
    );
}
Vector3& Vector3::operator/=(float scalar){
    this->x /= scalar;
    this->y /= scalar;
    this->z /= scalar;
    return *this;
}

// per component mod
Vector3 Vector3::operator%(float scalar) {
    return Vector3(
        this->x - floorf(this->x / scalar) * scalar,
        this->y - floorf(this->y / scalar) * scalar,
        this->z - floorf(this->z / scalar) * scalar
    );
}
// per component mod
Vector3& Vector3::operator%=(float scalar) {
    this->x -= floorf(this->x / scalar) * scalar;
    this->y -= floorf(this->y / scalar) * scalar;
    this->z -= floorf(this->z / scalar) * scalar;
    return *this;
}

// return the component at this index (0 => x, 1 => y, 2 => z)
// throws an error if index < 0 or index > 2
float& Vector3::operator[](int index){
    switch (index)
    {
    case 0:
        return this->x;
    case 1:
        return this->y;
    case 2:
        return this->z;
    
    default:
        std::cerr << "ERROR: trying to acces component " << index << " of a Vector3\n";
        exit(1);
    }
}
bool Vector3::operator==(const Vector3& other){
    return this->x == other.x && this->y == other.y && this->z == other.z;
}
bool Vector3::operator!=(const Vector3& other){
    return this->x != other.x || this->y != other.y || this->z != other.z;
}

// return a vector othogonal to the two vectors
Vector3 Vector3::cross(const Vector3& other){
    return Vector3(
        this->y * other.z - this->z * other.y,
        this->z * other.x - this->x * other.z,
        this->x * other.y - this->y * other.x
    );
}
// return the dot product of the two vectors
float Vector3::dot(const Vector3& other){
    return this->x * other.x + this->y * other.y + this->z * other.z;
}
// return the length of the vector
float Vector3::magnitude(){
    return sqrtf(this->sqrmagnitude());
}
// return the squared length of the vector
float Vector3::sqrmagnitude(){
    return this->x * this->x + this->y * this->y + this->z * this->z;
}
// normalize the vector in place and return itself
Vector3& Vector3::normalize(){
    *this /= this->magnitude();
    return *this;
}
// return a new vector with same direction and a norm of 1
Vector3 Vector3::normalized(){
    return *this / this->magnitude();
}

std::string Vector3::to_str() {
    return "(" + std::to_string(this->x) + ", " + std::to_string(this->y) + ", " + std::to_string(this->z) + ")";
}
#pragma endregion


#pragma region vector3Int
Vector3Int::Vector3Int(int x, int y, int z){
    this->x = x;
    this->y = y;
    this->z = z;
}
Vector3Int::Vector3Int(const Vector3& other){
    this->x = (int)other.x;
    this->y = (int)other.y;
    this->z = (int)other.z;
}
Vector3Int::Vector3Int(){
    this->x = 0;
    this->y = 0;
    this->z = 0;
}

Vector3Int Vector3Int::operator+(const Vector3& other){
    return Vector3Int(
        this->x + other.x,
        this->y + other.y,
        this->z + other.z
    );
}
Vector3Int& Vector3Int::operator+=(const Vector3& other){
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;
    return *this;
}

Vector3Int Vector3Int::operator-(const Vector3& other){
    return Vector3Int(
        this->x - other.x,
        this->y - other.y,
        this->z - other.z
    );
}
Vector3Int& Vector3Int::operator-=(const Vector3& other){
    this->x -= other.x;
    this->y -= other.y;
    this->z -= other.z;
    return *this;
}

// per component multiplication
Vector3Int Vector3Int::operator*(const Vector3& other){
    return Vector3Int(
        this->x * other.x,
        this->y * other.y,
        this->z * other.z
    );
}
// per component multiplication
Vector3Int& Vector3Int::operator*=(const Vector3& other){
    this->x *= other.x;
    this->y *= other.y;
    this->z *= other.z;
    return *this;
}
Vector3Int Vector3Int::operator*(float scalar){
    return Vector3(
        this->x * scalar,
        this->y * scalar,
        this->z * scalar
    );
}
Vector3Int& Vector3Int::operator*=(float scalar){
    this->x *= scalar;
    this->y *= scalar;
    this->z *= scalar;
    return *this;
}

// per component division
Vector3Int Vector3Int::operator/(const Vector3& other){
    return Vector3Int(
        this->x / other.x,
        this->y / other.y,
        this->z / other.z
    );
}
// per component division
Vector3Int& Vector3Int::operator/=(const Vector3& other){
    this->x /= other.x;
    this->y /= other.y;
    this->z /= other.z;
    return *this;
}
Vector3Int Vector3Int::operator/(float scalar){
    return Vector3Int(
        this->x / scalar,
        this->y / scalar,
        this->z / scalar
    );
}
Vector3Int& Vector3Int::operator/=(float scalar){
    this->x /= scalar;
    this->y /= scalar;
    this->z /= scalar;
    return *this;
}

// per component mod
Vector3Int Vector3Int::operator%(int scalar) {
    return Vector3Int(
        this->x % scalar,
        this->y % scalar,
        this->z % scalar
    );
}
// per component mod
Vector3Int& Vector3Int::operator%=(int scalar) {
    this->x %= scalar;
    this->y %= scalar;
    this->z %= scalar;
    return *this;
}

// return the component at this index (0 => x, 1 => y, 2 => z)
// throws an error if index < 0 or index > 2
int& Vector3Int::operator[](int index){
    switch (index)
    {
    case 0:
        return this->x;
    case 1:
        return this->y;
    case 2:
        return this->z;
    
    default:
        std::cerr << "ERROR: trying to acces component " << index << " of a Vector3Int\n";
        exit(1);
    }
}
bool Vector3Int::operator==(const Vector3Int& other){
    return this->x == other.x && this->y == other.y && this->z == other.z;
}
bool Vector3Int::operator!=(const Vector3Int& other){
    return this->x != other.x || this->y != other.y || this->z != other.z;
}

// return the dot product of the two vectors
float Vector3Int::dot(const Vector3& other){
    return this->x * other.x + this->y * other.y + this->z * other.z;
}
// return the length of the vector
float Vector3Int::magnitude(){
    return sqrt(this->sqrmagnitude());
}
// return the squared length of the vector
float Vector3Int::sqrmagnitude(){
    return this->x * this->x + this->y * this->y + this->z * this->z;
}

std::string Vector3Int::to_str() {
    return "(" + std::to_string(this->x) + ", " + std::to_string(this->y) + ", " + std::to_string(this->z) + ")";
}
#pragma endregion

#endif