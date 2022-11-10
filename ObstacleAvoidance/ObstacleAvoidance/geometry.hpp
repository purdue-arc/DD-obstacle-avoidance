#pragma once

#include <string>
#include <cmath>

#define MIN(a, b) (a < b) ? a : b
#define MAX(a, b) (a > b) ? a : b

// 3D floating-point geometry
namespace gmtry3 {
	const int basis_bivectors[3][2] = { {1, 2}, {2, 0}, {0, 1} };

	struct vector3 {
		float x, y, z;
		vector3() = default;
		vector3(float i, float j, float k) {
			x = i;
			y = j;
			z = k;
		}
		float& operator [](int i) {
			return (&x)[i];
		}
		const float& operator [](int i) const {
			return (&x)[i];
		}
		void operator +=(const vector3& v) {
			x += v.x;
			y += v.y;
			z += v.z;
		}
		void operator -=(const vector3& v) {
			x -= v.x;
			y -= v.y;
			z -= v.z;
		}
	};

	inline vector3 operator +(const vector3& a, const vector3& b) {
		return vector3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	inline vector3 operator -(const vector3& a, const vector3& b) {
		return vector3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	inline vector3 operator -(const vector3& v) {
		return vector3(-v.x, -v.y, -v.z);
	}

	inline vector3 operator *(const vector3& v, float s) {
		return vector3(v.x * s, v.y * s, v.z * s);
	}

	inline vector3 operator /(const vector3& v, float s) {
		return v * (1.0F / s);
	}

	inline float magnitude(const vector3& v) {
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	inline vector3 normalize(const vector3& v) {
		return v / magnitude(v);
	}

	inline float dot(const vector3& a, const vector3& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline vector3 unit_project(const vector3& v, const vector3& n) {
		return n * dot(v, n);
	}

	inline vector3 unit_reject(const vector3& v, const vector3& n) {
		return v - unit_project(v, n);
	}

	inline vector3 project(const vector3& a, const vector3& b) {
		return b * dot(a, b) / dot(b, b);
	}

	inline vector3 reject(const vector3& a, const vector3& b) {
		return a - project(a, b);
	}

	inline vector3 cross(const vector3& a, const vector3& b) {
		return vector3(a.y * b.z - a.z * b.y,
					   a.z * b.x - a.x * b.z,
					   a.x * b.y - a.y * b.x);
	}

	std::string to_string(const vector3& v) {
		return std::to_string(v.x) + std::string(", ") + std::to_string(v.y) + std::string(", ") + std::to_string(v.z);
	}

	struct matrix3 {
		float n[3][3];
		matrix3() {
			n[0][0] = 1; n[1][0] = 0; n[2][0] = 0;
			n[0][1] = 0; n[1][1] = 1; n[2][1] = 0;
			n[0][2] = 0; n[1][2] = 0; n[2][2] = 1;
		}
		matrix3(float n00, float n01, float n02,
				float n10, float n11, float n12,
				float n20, float n21, float n22) {
			n[0][0] = n00; n[1][0] = n10; n[2][0] = n20;
			n[0][1] = n01; n[1][1] = n11; n[2][1] = n21;
			n[0][2] = n02; n[1][2] = n12; n[2][2] = n22;
		}
		matrix3(const vector3& a, const vector3& b, const vector3& c) {
			n[0][0] = a.x; n[1][0] = b.x; n[2][0] = c.x;
			n[0][1] = a.y; n[1][1] = b.y; n[2][1] = c.y;
			n[0][2] = a.z; n[1][2] = b.z; n[2][2] = c.z;
		}
		float& operator ()(int i, int j) {
			return n[i][j];
		}
		const float& operator ()(int i, int j) const {
			return n[i][j];
		}
		vector3& operator ()(int i) {
			return *(reinterpret_cast<vector3*>(n[i]));
		}
		const vector3& operator ()(int i) const {
			return *(reinterpret_cast<const vector3*>(n[i]));
		}
		// Inversion if column vectors are normalized and orthogonal
		matrix3 T() {
			return matrix3(n[0][0], n[1][0], n[2][0], 
						   n[0][1], n[1][1], n[2][1], 
						   n[0][2], n[1][2], n[2][2]);
		}
	};

	matrix3 make_rotation(int axis_idx, float theta) {
		float sin_theta = std::sin(theta);
		float cos_theta = std::cos(theta);
		matrix3 mat(0, 0, 0, 0, 0, 0, 0, 0, 0);
		mat(axis_idx, axis_idx) = 1;
		mat(basis_bivectors[axis_idx][0], basis_bivectors[axis_idx][0]) = cos_theta;
		mat(basis_bivectors[axis_idx][0], basis_bivectors[axis_idx][1]) = sin_theta;
		mat(basis_bivectors[axis_idx][1], basis_bivectors[axis_idx][0]) = -sin_theta;
		mat(basis_bivectors[axis_idx][1], basis_bivectors[axis_idx][1]) = cos_theta;
		return mat;
	}

	matrix3 operator *(const matrix3& A, const matrix3& B) {
		return matrix3(A.n[0][0] * B.n[0][0] + A.n[1][0] * B.n[0][1] + A.n[2][0] * B.n[0][2],
					   A.n[0][1] * B.n[0][0] + A.n[1][1] * B.n[0][1] + A.n[2][1] * B.n[0][2],
					   A.n[0][2] * B.n[0][0] + A.n[1][2] * B.n[0][1] + A.n[2][2] * B.n[0][2],
					   A.n[0][0] * B.n[1][0] + A.n[1][0] * B.n[1][1] + A.n[2][0] * B.n[1][2],
					   A.n[0][1] * B.n[1][0] + A.n[1][1] * B.n[1][1] + A.n[2][1] * B.n[1][2],
					   A.n[0][2] * B.n[1][0] + A.n[1][2] * B.n[1][1] + A.n[2][2] * B.n[1][2],
					   A.n[0][0] * B.n[2][0] + A.n[1][0] * B.n[2][1] + A.n[2][0] * B.n[2][2],
					   A.n[0][1] * B.n[2][0] + A.n[1][1] * B.n[2][1] + A.n[2][1] * B.n[2][2],
					   A.n[0][2] * B.n[2][0] + A.n[1][2] * B.n[2][1] + A.n[2][2] * B.n[2][2]);
	}

	inline vector3 operator *(const matrix3& M, const vector3& v) {
		return vector3(M.n[0][0] * v.x + M.n[1][0] * v.y + M.n[2][0] * v.z,
					   M.n[0][1] * v.x + M.n[1][1] * v.y + M.n[2][1] * v.z,
					   M.n[0][2] * v.x + M.n[1][2] * v.y + M.n[2][2] * v.z);
	}

	inline vector3 dot(const matrix3& M, const vector3& v) {
		return vector3(M.n[0][0] * v.x + M.n[0][1] * v.y + M.n[0][2] * v.z,
					   M.n[1][0] * v.x + M.n[1][1] * v.y + M.n[1][2] * v.z,
					   M.n[2][0] * v.x + M.n[2][1] * v.y + M.n[2][2] * v.z);
	}

	struct transform3 {
		matrix3 R;
		vector3 t;
		transform3() {
			R = matrix3();
			t = vector3();
		}
		transform3(float n00, float n01, float n02,
				   float n10, float n11, float n12,
				   float n20, float n21, float n22,
				   float n30, float n31, float n32) {
			R = matrix3(n00, n01, n02, n10, n11, n12, n20, n21, n22);
			t = vector3(n30, n31, n32);
		}
		transform3(const vector3& i, const vector3& j, const vector3& k, const vector3& d) {
			R = matrix3(i, j, k);
			t = d;
		}
		transform3(const matrix3& M, const vector3& v) {
			R = M;
			t = v;
		}
		// Inversion if column vectors of rotation matrix are normalized and orthogonal
		transform3 T() {
			matrix3 RT = R.T();
			return transform3(RT, RT * -t);
		}
	};

	inline vector3 operator *(const transform3& T, const vector3& p) {
		return T.R * p + T.t;
	}

	inline transform3 operator *(const transform3& T, const matrix3& M) {
		return transform3(T.R * M, T.t);
	}

	inline transform3 operator *(const matrix3& M, const transform3& T) {
		return transform3(M * T.R, M * T.t);
	}

	inline transform3 operator *(const transform3& A, const transform3& B) {
		return transform3(A.R * B.R, A.R * B.t + A.t);
	}

	inline transform3 operator +(const transform3& T, const vector3& v) {
		return transform3(T.R, T.t + v);
	}

	inline transform3 operator -(const transform3& T, const vector3& v) {
		return transform3(T.R, T.t - v);
	}

	struct rotor3 {
		float a;
		vector3 B;
		rotor3(float inner, const vector3& outer) {
			a = inner;
			B = outer;
		}
	};
	
	// unit_bivector has to be normalized
	inline rotor3 unit_make_rotor(const vector3& unit_bivector, float theta) {
		return rotor3(std::cos(theta/2), -unit_bivector * std::sin(theta/2));
	}

	// bivector doesn't have to be normalized
	inline rotor3 make_rotor(const vector3& bivector, float theta) {
		return rotor3(std::cos(theta/2), -normalize(bivector) * std::sin(theta/2));
	}

	inline rotor3 normalize(const rotor3& r) {
		float inv_norm = 1.0F / std::sqrt(r.a * r.a + dot(r.B, r.B));
		return rotor3(r.a * inv_norm, r.B * inv_norm);
	}

	inline vector3 operator *(const rotor3& r, const vector3& v) {
		vector3 bcrossv = cross(r.B, v);
		return v * (r.a * r.a) - bcrossv * (2 * r.a) + cross(r.B, bcrossv) + r.B * dot(r.B, v);
	}

	inline rotor3 operator *(const rotor3& r1, const rotor3& r2) {
		return rotor3(r1.a * r2.a - dot(r1.B, r2.B), r2.B * r1.a + r1.B * r2.a - cross(r1.B, r2.B));
	}

	inline rotor3 invert(const rotor3& r) {
		return rotor3(r.a, -r.B);
	}
}

// 2D integer geometry
namespace gmtry2i {
	struct vector2i {
		long x, y;
		vector2i() = default;
		vector2i(long i, long j) {
			x = i;
			y = j;
		}
		long& operator [](int i) {
			return (&x)[i];
		}
		const long& operator [](int i) const {
			return (&x)[i];
		}
		void operator +=(const vector2i& v) {
			x += v.x;
			y += v.y;
		}
		void operator -=(const vector2i& v) {
			x -= v.x;
			y -= v.y;
		}
	};

	inline vector2i operator +(const vector2i& a, const vector2i& b) {
		return vector2i(a.x + b.x, a.y + b.y);
	}

	inline vector2i operator -(const vector2i& a, const vector2i& b) {
		return vector2i(a.x - b.x, a.y - b.y);
	}

	inline vector2i operator -(const vector2i& v) {
		return vector2i(-v.x, -v.y);
	}

	inline vector2i operator *(const vector2i& v, long s) {
		return vector2i(v.x * s, v.y * s);
	}

	inline vector2i operator /(const vector2i& v, long s) {
		return vector2i(v.x / s, v.y / s);
	}

	inline vector2i operator <<(const vector2i& v, unsigned int i) {
		return vector2i(v.x << i, v.y << i);
	}

	inline vector2i operator >>(const vector2i& v, unsigned int i) {
		return vector2i(v.x >> i, v.y >> i);
	}

	inline vector2i operator &(const vector2i& v, long s) {
		return vector2i(v.x & s, v.y & s);
	}

	inline vector2i operator |(const vector2i& v, long s) {
		return vector2i(v.x | s, v.y | s);
	}

	inline vector2i operator >(const vector2i& v, long s) {
		return vector2i(v.x > s, v.y > s);
	}

	inline vector2i operator <(const vector2i& v, long s) {
		return vector2i(v.x < s, v.y < s);
	}

	std::string to_string(const vector2i& v) {
		return std::to_string(v.x) + std::string(", ") + std::to_string(v.y);
	}

	struct aligned_box2i {
		vector2i min, max;
		aligned_box2i() = default;
		aligned_box2i(const vector2i& min_v, const vector2i& max_v) {
			min = min_v;
			max = max_v;
		}
		aligned_box2i(const vector2i& origin, long width) {
			min = origin;
			max = vector2i(origin.x + width, origin.y + width);
		}
	};

	// Assumes the boxes intersect. Do not use the result if intersects(b1, b2) == false
	inline aligned_box2i intersection(const aligned_box2i& b1, const aligned_box2i& b2) {
		return aligned_box2i(vector2i(MAX(b1.min.x, b2.min.x), MAX(b1.min.y, b2.min.y)),
							 vector2i(MIN(b1.max.x, b2.max.x), MIN(b1.max.y, b2.max.y)));
	}

	inline bool contains(const aligned_box2i& b, const vector2i& v) {
		return b.min.x <= v.x && b.min.y <= v.y && b.max.x > v.x && b.max.y > v.y;
	}

	inline bool contains(const aligned_box2i& b1, const aligned_box2i& b2) {
		return b1.min.x <= b2.min.x && b1.min.y <= b2.min.y && b1.max.x >= b2.max.x && b1.max.y >= b2.max.y;
	}

	inline vector2i center(const aligned_box2i& b1) {
		return (b1.min + b1.max) >> 1;
	}

	inline long area(const aligned_box2i& b) {
		return (b.max.x - b.min.x) * (b.max.y - b.min.y);
	}

	// Returns minkowski sum
	inline aligned_box2i operator +(const aligned_box2i& b1, const aligned_box2i& b2) {
		return aligned_box2i(b1.min + b2.min, b1.max + b2.max);
	}

	// Returns minkowski difference
	inline aligned_box2i operator -(const aligned_box2i& b1, const aligned_box2i& b2) {
		vector2i dif1 = b1.min - b2.max;
		vector2i dif2 = b1.max - b2.min;
		bool dif1_is_min = dif1.x < dif2.x || dif1.y < dif2.y;
		return aligned_box2i(dif1_is_min ? dif1 : dif2, dif1_is_min ? dif2 : dif1);
	}

	inline bool intersects(const aligned_box2i& b1, const aligned_box2i& b2) {
		return contains(b1 - b2, gmtry2i::vector2i());
	}

	inline aligned_box2i operator +(const aligned_box2i& b, const vector2i& v) {
		return aligned_box2i(b.min + v, b.max + v);
	}

	inline aligned_box2i operator -(const aligned_box2i& b, const vector2i& v) {
		return aligned_box2i(b.min - v, b.max - v);
	}

	std::string to_string(const aligned_box2i& b) {
		return to_string(b.min) + std::string("; ") + to_string(b.max);
	}
}