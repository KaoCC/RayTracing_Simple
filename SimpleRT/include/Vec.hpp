
#ifndef _VEC_H_
#define	_VEC_H_

//typedef struct {
//	float x, y, z; // for position and color (r,g,b)
//} Vec;


class Vec {

public:

	Vec() = default;
	Vec(float a, float b, float c);

	Vec& operator=(const Vec & other) = default;

	Vec operator+(const Vec & v) const;
	Vec operator-(const Vec & v) const;
	Vec operator*(float val) const;

	Vec mult(const Vec & v) const;
	Vec& norm();
	float dot(const Vec & v) const;
	Vec cross(const Vec & v) const;

	void clear();


	float x = 0.f; 
	float y = 0.f; 
	float z = 0.f; // for position, or color (r,g,b)
};




#endif	/* _VEC_H */

