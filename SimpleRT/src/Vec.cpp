
#include <cmath>

#include "Vec.hpp"


Vec::Vec(float a, float b, float c) : x(a), y(b), z(c) {

}


Vec Vec::operator+(const Vec & v) const {
	return Vec(x + v.x, y + v.y, z + v.z);
}

Vec Vec::operator-(const Vec & v) const {
	return Vec(x - v.x, y - v.y, z - v.z);
}

Vec Vec::operator*(float val) const {
	return Vec(x * val, y * val, z * val);
}

Vec Vec::mult(const Vec & v) const {
	return Vec(x * v.x, y * v.y, z * v.z);
}

Vec& Vec::norm() {
	return *this = *this * (1 / sqrt(x * x + y * y + z * z));
}

float Vec::dot(const Vec & v) const {
	return x * v.x + y * v.y + z * v.z;
}

Vec Vec::cross(const Vec & v) const {
	return Vec(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

void Vec::clear() {
	x = 0.f;
	y = 0.f;
	z = 0.f;
}

