#pragma once

#include <string>
#include <cmath>

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

const float PI = 3.14159265F;

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

	class point3_ostream {
	public:
		virtual void write(const gmtry3::vector3& p) = 0;
		virtual void flush() {}
	};

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
		matrix3 T() const {
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
		transform3 T() const {
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

	struct ray3 {
		vector3 p, d;
		ray3(const vector3& point, const vector3& direction) {
			p = point;
			d = direction;
		}
	};

	struct triangle3 {
		vector3 a, b, c;
		triangle3() = default;
		triangle3(const vector3& p, const vector3& q, const vector3& r) {
			a = p;
			b = q;
			c = r;
		}
		inline vector3& operator [](int i) {
			return (&a)[i];
		}
		inline const vector3& operator [](int i) const {
			return (&a)[i];
		}
	};

	struct ball3 {
		vector3 c;
		float r;
		ball3() = default;
		ball3(const vector3& center, float radius) {
			c = center;
			r = radius;
		}
	};

	inline ball3 operator *(const ball3& c, float s) {
		return { c.c, c.r * s };
	}

	inline ball3 operator /(const ball3& c, float s) {
		return { c.c, c.r / s };
	}

	inline ball3 operator +(const ball3& c, const vector3& v) {
		return { c.c + v, c.r };
	}

	inline ball3 operator -(const ball3& c, const vector3& v) {
		return { c.c - v, c.r };
	}
}

namespace gmtry2 {
	const float EPSILON = 0.00001; // one hundred-thousandth

	struct vector2 {
		float x, y;
		vector2() = default;
		vector2(float i, float j) {
			x = i;
			y = j;
		}
		vector2(const gmtry3::vector3& v) {
			x = v.x;
			y = v.y;
		}
		float& operator [](int i) {
			return (&x)[i];
		}
		const float& operator [](int i) const {
			return (&x)[i];
		}
		void operator +=(const vector2& v) {
			x += v.x;
			y += v.y;
		}
		void operator -=(const vector2& v) {
			x -= v.x;
			y -= v.y;
		}
		vector2& operator =(const gmtry3::vector3& v) {
			x = v.x;
			y = v.y;
			return *this;
		}
	};

	inline vector2 operator +(const vector2& a, const vector2& b) {
		return vector2(a.x + b.x, a.y + b.y);
	}

	inline vector2 operator -(const vector2& a, const vector2& b) {
		return vector2(a.x - b.x, a.y - b.y);
	}

	inline vector2 operator -(const vector2& v) {
		return vector2(-v.x, -v.y);
	}

	inline vector2 operator *(const vector2& v, float s) {
		return vector2(v.x * s, v.y * s);
	}

	inline vector2 operator /(const vector2& v, float s) {
		return v * (1.0F / s);
	}

	inline float magnitude(const vector2& v) {
		return std::sqrt(v.x * v.x + v.y * v.y);
	}

	inline vector2 normalize(const vector2& v) {
		return v / magnitude(v);
	}

	inline float dot(const vector2& a, const vector2& b) {
		return a.x * b.x + a.y * b.y;
	}

	inline float wedge(const vector2& a, const vector2& b) {
		return a.x * b.y - a.y * b.x;
	}

	inline float squared(const vector2& v) {
		return dot(v, v);
	}

	struct line_segment2 {
		vector2 a, b;
		line_segment2() = default;
		line_segment2(const vector2& pointA, const vector2& pointB) {
			a = pointA;
			b = pointB;
		}
	};

	bool intersects(const line_segment2& l1, const line_segment2& l2) {
		vector2 disp1 = l1.b - l1.a; // from l1.a to l1.b
		vector2 disp2 = l2.b - l2.a; // from l2.a to l2.b
		vector2 adisp = l1.a - l2.a; // from l2.a to l1.a
		float wedge1 = wedge(disp2, adisp);
		float wedge2 = wedge(disp1, adisp);
		float wedge3 = wedge(disp2, disp1);
		// test for parallelity
		if (abs(wedge3) < gmtry2::EPSILON) return false;
		// see intersection(line_segment2, line_segment2) for first half of following derivation
		// if t >= 0 and abs(t) <= 1, then l1 and l2 intersect
		//		[ t >= 0 ] = XOR(wedge(disp2, adisp) < 0, wedge(disp2, disp1) < 0)
		//		[ abs(t) <= 1 ] = [ abs(wedge(disp2, adisp)) / abs(wedge(disp2, disp1)) <= 1 ]
		//						= [ abs(wedge(disp2, adisp)) <= abs(wedge(disp2, disp1)) ]
		return ((wedge1 < 0) != (wedge3 < 0)) && ((wedge2 < 0) != (wedge3 < 0)) &&
		       (abs(wedge1) <= abs(wedge3))   && (abs(wedge2) <= abs(wedge3));
	}

	// result invalid if l1 and l2 do not intersect
	vector2 intersection(const line_segment2& l1, const line_segment2& l2) {
		vector2 disp1 = l1.b - l1.a; // from l1.a to l1.b
		vector2 disp2 = l2.b - l2.a; // from l2.a to l2.b
		vector2 adisp = l1.a - l2.a; // from l2.a to l1.a
		// wedge(disp2, adisp + t*disp1) = 0
		// = wedge(disp2, adisp) + t*wedge(disp2, disp1)
		// t = - wedge(disp2, adisp) / wedge(disp2, disp1)
		return l1.a - disp1 * (wedge(disp2, adisp) / wedge(disp2, disp1));
	}

	inline line_segment2 operator +(const line_segment2& l, const vector2& v) {
		return { l.a + v, l.b + v };
	}

	inline line_segment2 operator -(const line_segment2& l, const vector2& v) {
		return { l.a - v, l.b - v };
	}

	struct line_stepper2 {
		vector2 step_p, p;
		unsigned int waypoints;
		line_stepper2(const line_segment2& l, float step_size) {
			vector2 disp = l.b - l.a;
			float lengthf = std::sqrt(squared(disp));
			waypoints = (lengthf / step_size) + 1;
			step_p = disp * (step_size / lengthf);
			p = l.a;
		}
		inline void step() {
			p += step_p;
		}
	};

	struct ball2 {
		vector2 c;
		float r;
		ball2() = default;
		ball2(const vector2& center, float radius) {
			c = center;
			r = radius;
		}
		ball2& operator =(const gmtry3::ball3& sphere) {
			c = sphere.c;
			r = sphere.r;
		}
	};

	inline ball2 operator *(const ball2& b, float s) {
		return { b.c, b.r * s };
	}

	inline ball2 operator /(const ball2& b, float s) {
		return { b.c, b.r / s };
	}

	inline ball2 operator +(const ball2& b, const vector2& v) {
		return { b.c + v, b.r };
	}

	inline ball2 operator -(const ball2& b, const vector2& v) {
		return { b.c - v, b.r };
	}

	inline bool intersects(const vector2& p, const ball2& b) {
		return squared(p - b.c) <= b.r * b.r;
	}
}

// 2D integer geometry
namespace gmtry2i {
	inline long floor(float f) {
		long l = f;
		return l - (l > f);
	}

	inline long ceil(float f) {
		long l = f;
		return l + (l < f);
	}

	struct vector2i {
		long x, y;
		vector2i() = default;
		vector2i(long i, long j) {
			x = i;
			y = j;
		}
		vector2i(const gmtry2::vector2& p) {
			x = floor(p.x);
			y = floor(p.y);
		}
		inline long& operator [](int i) {
			return (&x)[i];
		}
		inline const long& operator [](int i) const {
			return (&x)[i];
		}
		vector2i& operator +=(const vector2i& v) {
			x += v.x;
			y += v.y;
			return *this;
		}
		vector2i& operator -=(const vector2i& v) {
			x -= v.x;
			y -= v.y;
			return *this;
		}
		vector2i& operator =(const gmtry2::vector2& p) {
			x = floor(p.x);
			y = floor(p.y);
			return *this;
		}
	};

	inline vector2i operator +(const vector2i& a, const vector2i& b) {
		return vector2i(a.x + b.x, a.y + b.y);
	}

	inline vector2i operator -(const vector2i& a, const vector2i& b) {
		return vector2i(a.x - b.x, a.y - b.y);
	}

	inline vector2i operator |(const vector2i& a, const vector2i& b) {
		return vector2i(a.x | b.x, a.y | b.y);
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

	inline vector2i operator >=(const vector2i& v, long s) {
		return vector2i(v.x >= s, v.y >= s);
	}

	inline vector2i operator <=(const vector2i& v, long s) {
		return vector2i(v.x <= s, v.y <= s);
	}

	inline bool operator ==(const vector2i& u, const vector2i& v) {
		return u.x == v.x && u.y == v.y;
	}

	inline long dot(const vector2i& u, const vector2i& v) {
		return u.x * v.x + u.y * v.y;
	}

	inline long squared(const vector2i& v) {
		return dot(v, v);
	}

	// returns pseudoscalar (<u v>_2) / (e_1 e_2)
	inline long wedge(const vector2i& u, const vector2i& v) {
		return u.x * v.y - u.y * v.x;
	}

	std::string to_string(const vector2i& v) {
		return std::to_string(v.x) + std::string(", ") + std::to_string(v.y);
	}

	gmtry2::vector2 to_vector2(const vector2i& v) {
		return gmtry2::vector2(v.x, v.y);
	}

	gmtry2::vector2 to_point2(const vector2i& p) {
		return { p.x + 0.5F, p.y + 0.5F };
	}

	class point2_ostream {
	public:
		virtual void write(const gmtry2i::vector2i& p) = 0;
		virtual void flush() {}
	};

	struct aligned_box2i {
		// inclusive min, exclusive max
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
		inline vector2i& operator [](int i) {
			return (&min)[i];
		}
		inline const vector2i& operator [](int i) const {
			return (&min)[i];
		}
	};

	inline vector2i center(const aligned_box2i& b1) {
		return (b1.min + b1.max) >> 1;
	}

	inline long area(const aligned_box2i& b) {
		return (b.max.x - b.min.x) * (b.max.y - b.min.y);
	}

	inline aligned_box2i boundsof(const vector2i& p) {
		return aligned_box2i(p, 1);
	}

	inline aligned_box2i boundsof(const aligned_box2i& b) {
		return b;
	}

	inline aligned_box2i boundsof(const vector2i& p1, const vector2i& p2) {
		return aligned_box2i({ MIN(p1.x, p2.x),     MIN(p1.y, p2.y) }, 
							 { MAX(p1.x, p2.x) + 1, MAX(p1.y, p2.y) + 1 });
	}

	inline aligned_box2i boundsof(const aligned_box2i& b, const vector2i& p) {
		return aligned_box2i({ MIN(b.min.x, p.x),     MIN(b.min.y, p.y) }, 
							 { MAX(b.max.x, p.x + 1), MAX(b.max.y, p.y + 1) });
	}

	inline aligned_box2i boundsof(const gmtry2::ball2& b) {
		gmtry2::vector2 corner_disp(b.r, b.r);
		return { b.c - corner_disp, b.c + corner_disp };
	}

	inline bool contains(const aligned_box2i& b, const vector2i& p) {
		return b.min.x <= p.x && b.min.y <= p.y && b.max.x > p.x && b.max.y > p.y;
	}

	inline bool contains(const aligned_box2i& b1, const aligned_box2i& b2) {
		return b1.min.x <= b2.min.x && b1.min.y <= b2.min.y && b1.max.x >= b2.max.x && b1.max.y >= b2.max.y;
	}

	inline bool intersects(const vector2i& p, const aligned_box2i& b) {
		return contains(b, p);
	}

	inline vector2i intersection(const vector2i& p, const aligned_box2i& b) {
		return p;
	}

	// Returns minkowski sum
	inline aligned_box2i operator +(const aligned_box2i& b1, const aligned_box2i& b2) {
		return aligned_box2i(b1.min + b2.min, b1.max + b2.max);
	}

	// Returns minkowski difference
	inline aligned_box2i operator -(const aligned_box2i& b1, const aligned_box2i& b2) {
		vector2i dif1 = b1.min - b2.max;
		vector2i dif2 = b1.max - b2.min;
		// sign(dif1.x - dif2.x) = sign(dif1.y - dif2.y) ? Sure hope so
		bool dif1_is_min = dif1.x < dif2.x || dif1.y < dif2.y;
		return aligned_box2i(dif1_is_min ? dif1 : dif2, dif1_is_min ? dif2 : dif1);
	}

	inline aligned_box2i operator +(const aligned_box2i& b, const vector2i& v) {
		return aligned_box2i(b.min + v, b.max + v);
	}

	inline aligned_box2i operator -(const aligned_box2i& b, const vector2i& v) {
		return aligned_box2i(b.min - v, b.max - v);
	}

	inline aligned_box2i operator -(const vector2i& v, const aligned_box2i& b) {
		return aligned_box2i(v - b.max, v - b.min);
	}

	inline bool intersects(const aligned_box2i& b1, const aligned_box2i& b2) {
		return contains(b1 - b2, vector2i());
	}

	inline aligned_box2i intersection(const aligned_box2i& b1, const aligned_box2i& b2) {
		aligned_box2i b3(vector2i(MAX(b1.min.x, b2.min.x), MAX(b1.min.y, b2.min.y)),
						 vector2i(MIN(b1.max.x, b2.max.x), MIN(b1.max.y, b2.max.y)));
		if ((b3.min.x < b3.max.x) && (b3.min.y < b3.max.y)) return b3;
		else return aligned_box2i();
	}

	inline bool intersects(const gmtry2::line_segment2 l, const aligned_box2i& box) {
		// test if box contains either endpoint
		if (contains(box, l.a) || contains(box, l.b)) return true;
		gmtry2::vector2 disp = l.b - l.a;
		const gmtry2::vector2 epsilon_disp(gmtry2::EPSILON, gmtry2::EPSILON);
		// Inclusive extrema of box
		gmtry2::vector2 box_pts[2] = { to_vector2(box.min) + epsilon_disp, to_vector2(box.max) - epsilon_disp };
		for (int dim = 0; dim < 2; dim++) if (abs(disp[dim]) > gmtry2::EPSILON)
			for (int extrema = 0; extrema < 2; extrema++) {
				// a[dim] + t*disp[dim] = box[extrema][dim]
				// t = (box[extrema][dim] - a[dim]) / disp[dim]
				// if 0 <= t <= 1 then they intersect
				float value1 = box_pts[extrema][dim] - l.a[dim];
				float value2 = disp[dim];
				// if t is not negative AND t is less than 1
				if (((value1 < 0) == (value2 < 0)) && (abs(value1) <= abs(value2))) {
					int other_dim = 1 & ~dim;
					float other_intersection = l.a[other_dim] + disp[other_dim] * (value1 / value2);
					if (box.min[other_dim] <= other_intersection && other_intersection < box.max[other_dim])
						return true;
				}
			}
		return false;
	}

	gmtry2::line_segment2 intersection(const gmtry2::line_segment2 l, const aligned_box2i& box, 
	                                   bool* no_intersection) {
		// new points will either be intersections with box edges or existing endpoints contained in the box
		gmtry2::vector2 new_pts[2];
		int pt_idx = 0;
		if (contains(box, l.a)) new_pts[pt_idx++] = l.a;
		if (contains(box, l.b)) new_pts[pt_idx++] = l.b;
		if (pt_idx > 1) return { new_pts[0], new_pts[1] };
		gmtry2::vector2 disp = l.b - l.a;
		const gmtry2::vector2 epsilon_disp(gmtry2::EPSILON, gmtry2::EPSILON);
		// Inclusive extrema of box
		gmtry2::vector2 box_pts[2] = { to_vector2(box.min) + epsilon_disp, to_vector2(box.max) - epsilon_disp };
		for (int dim = 0; dim < 2; dim++) if (abs(disp[dim]) > gmtry2::EPSILON) {
			float value2 = disp[dim];
			for (int extrema = 0; extrema < 2; extrema++) {
				// explanation for the following is in intersects(line_segment2i, aligned_box2i)
				float value1 = box_pts[extrema][dim] - l.a[dim];
				if (((value1 < 0) == (value2 < 0)) && (abs(value1) <= abs(value2))) {
					gmtry2::vector2 intersection = l.a + disp * (value1 / value2);
					if (contains(box, intersection)) new_pts[pt_idx++] = intersection;
					if (pt_idx > 1) return { new_pts[0], new_pts[1] };
				}
			}
		}
		if (no_intersection) *no_intersection = true;
		return {};
	}

	std::string to_string(const aligned_box2i& b) {
		return to_string(b.min) + std::string("; ") + to_string(b.max);
	}

	struct line_segment2i {
		vector2i a, b;
		line_segment2i() = default;
		line_segment2i(const vector2i& point_A, const vector2i& point_B) {
			a = point_A;
			b = point_B;
		}
		line_segment2i(const gmtry2::line_segment2& l) {
			a = l.a;
			b = l.b;
		}
		operator gmtry2::line_segment2() const {
			return { to_point2(a), to_point2(b) };
		}
	};

	inline aligned_box2i boundsof(const line_segment2i& l) {
		return boundsof(l.a, l.b);
	}

	inline bool contains(const gmtry2i::aligned_box2i& b, const line_segment2i& l) {
		return contains(b, l.a) && contains(b, l.b);
	}

	inline line_segment2i operator +(const line_segment2i& l, const vector2i& v) {
		return { l.a + v, l.b + v };
	}

	inline line_segment2i operator -(const line_segment2i& l, const vector2i& v) {
		return { l.a - v, l.b - v };
	}

	bool intersects(const line_segment2i& l1, const line_segment2i& l2) {
		vector2i disp1 = l1.b - l1.a; // from l1.a to l1.b
		vector2i disp2 = l2.b - l2.a; // from l2.a to l2.b
		vector2i adisp = l1.a - l2.a; // from l2.a to l1.a
		long wedge1 = wedge(disp2, adisp);
		long wedge2 = wedge(disp1, adisp);
		long wedge3 = wedge(disp2, disp1);
		// test for parallelity
		if (wedge3 == 0) return false;
		// see intersection(line_segment2i, line_segment2i) for first half of following derivation
		// if t >= 0 and abs(t) <= 1, then l1 and l2 intersect
		//		[ t >= 0 ] = XOR(wedge(disp2, adisp) < 0, wedge(disp2, disp1) < 0)
		//		[ abs(t) <= 1 ] = [ abs(wedge(disp2, adisp)) / abs(wedge(disp2, disp1)) <= 1 ]
		//						= [ abs(wedge(disp2, adisp)) <= abs(wedge(disp2, disp1)) ]
		return ((wedge1 < 0) != (wedge3 < 0)) && ((wedge2 < 0) != (wedge3 < 0)) && 
		       (abs(wedge1) <= abs(wedge3)) && (abs(wedge2) <= abs(wedge3));
	}

	// result invalid if l1 and l2 do not intersect
	vector2i intersection(const line_segment2i& l1, const line_segment2i& l2) {
		vector2i disp1 = l1.b - l1.a; // from l1.a to l1.b
		vector2i disp2 = l2.b - l2.a; // from l2.a to l2.b
		vector2i adisp = l1.a - l2.a; // from l2.a to l1.a
		// wedge(disp2, adisp + t*disp1) = 0
		// = wedge(disp2, adisp) + t*wedge(disp2, disp1)
		// t = - wedge(disp2, adisp) / wedge(disp2, disp1)
		float disp_scale = static_cast<float>(wedge(disp2, adisp)) / wedge(disp2, disp1);
		return l1.a + gmtry2i::vector2i(floor(0.5F - disp1.x * disp_scale), floor(0.5F - disp1.y * disp_scale));
	}

	bool intersects(const line_segment2i& l, const aligned_box2i& box) {
		// test if box contains either endpoint
		if (contains(box, l.a) || contains(box, l.b)) return true;
		vector2i disp = l.b - l.a;
		vector2i box_pts[2] = { box.min, box.max - vector2i(1, 1) };
		for (int dim = 0; dim < 2; dim++) if (disp[dim])
			for (int extrema = 0; extrema < 2; extrema++) {
				// a[dim] + t*disp[dim] = box[extrema][dim]
				// t = (box[extrema][dim] - a[dim]) / disp[dim]
				// if 0 <= t <= 1 then they intersect
				float value1 = box_pts[extrema][dim] - (l.a[dim] + 0.5F);
				int value2 = disp[dim];
				// if t is not negative AND t is less than 1
				if (((value1 < 0) == (value2 < 0)) && (abs(value1) <= abs(value2))) {
					int other_dim = 1 & ~dim;
					long other_intersection = l.a[other_dim] + floor(0.5F + 
					                          disp[other_dim] * (value1 / value2));
					if (box.min[other_dim] <= other_intersection && other_intersection < box.max[other_dim])
						return true;
				}
			}
		return false;
	}

	line_segment2i intersection(const line_segment2i& l, const aligned_box2i& box) {
		// new points will either be intersections with box edges or existing endpoints contained in the box
		vector2i new_pts[2];
		int pt_idx = 0;
		if (contains(box, l.a)) new_pts[pt_idx++] = l.a;
		if (contains(box, l.b)) new_pts[pt_idx++] = l.b;
		if (pt_idx > 1) return line_segment2i(new_pts[0], new_pts[1]);
		vector2i disp = l.b - l.a;
		vector2i box_pts[2] = { box.min, box.max - vector2i(1, 1) };
		for (int dim = 0; dim < 2; dim++) if (disp[dim]) {
			long value2 = disp[dim];
			for (int extrema = 0; extrema < 2; extrema++) {
				// explanation for the following is in intersects(line_segment2i, aligned_box2i)
				float value1 = box_pts[extrema][dim] - (l.a[dim] + 0.5F);
				if (((value1 < 0) == (value2 < 0)) && (abs(value1) <= abs(value2))) {
					float scale = static_cast<float>(value1) / value2;
					vector2i intersection = l.a + gmtry2i::vector2i(floor(0.5F + disp.x * scale), 
					                                                floor(0.5F + disp.y * scale));
					if (contains(box, intersection)) new_pts[pt_idx++] = intersection;
					if (pt_idx > 1) return line_segment2i(new_pts[0], new_pts[1]);
				}
			}
		}
		return line_segment2i();
	}

	// Current implementation is slow
	bool intersects(const line_segment2i& l, const vector2i& p) {
		gmtry2i::aligned_box2i box(boundsof(p));
		// test if box contains either endpoint
		if (contains(box, l.a) || contains(box, l.b)) return true;
		vector2i disp = l.b - l.a;
		for (int dim = 0; dim < 2; dim++) if (disp[dim])
			for (int extrema = 0; extrema < 2; extrema++) {
				// a[dim] + t*disp[dim] = box[extrema][dim]
				// t = (box[extrema][dim] - a[dim]) / disp[dim]
				// if 0 <= t <= 1 then they intersect
				float value1 = box[extrema][dim] - (l.a[dim] + 0.5F);
				int value2 = disp[dim];
				// if t is not negative AND t is less than 1
				if (((value1 < 0) == (value2 < 0)) && (abs(value1) <= abs(value2))) {
					int other_dim = 1 & ~dim;
					float other_intersection = l.a[other_dim] + 0.5F +
						disp[other_dim] * (static_cast<float>(value1) / (value2));
					if (box.min[other_dim] <= other_intersection && other_intersection <= box.max[other_dim])
						return true;
				}
			}
		return false;
	}

	struct line_stepper2i {
		float step_x, step_y;
		float x, y;
		unsigned int waypoints;
		vector2i p;
		line_stepper2i(const line_segment2i& l, float step_size) {
			vector2i disp = l.b - l.a;
			float lengthf = std::sqrt(squared(disp));
			waypoints = (lengthf / step_size) + 1;
			float step_scale = step_size / lengthf;
			step_x = disp.x * step_scale;
			step_y = disp.y * step_scale;
			x = static_cast<float>(l.a.x) + 0.5F;
			y = static_cast<float>(l.a.y) + 0.5F;
			p = l.a;
		}
		inline void step() {
			x += step_x; y += step_y;
			p = vector2i(floor(x), floor(y));
		}
	};

	template <typename T>
	concept intersectable2i = requires (T a, aligned_box2i b, bool c) {
		b = boundsof(a);
		c = intersects(a, b);
		//intersectable2i<typename decltype(intersection(a, b))>;
	};

	// Interface for object with the basic functionality of intersectable2i<aligned_box2i> without the proper functions
	class box_intersector2i {
	public:
		virtual bool intersects(const aligned_box2i& box) = 0;
		virtual box_intersector2i* intersection(const aligned_box2i& box) = 0;
		virtual box_intersector2i* clone() = 0;
		virtual aligned_box2i get_bounds() = 0;
	};

	// Used to create a box_intersector2i out of an intersectable2i<aligned_box2i>
	template <intersectable2i T>
	class intersectable_box_intersector2i : public box_intersector2i {
		T shape;
	public:
		intersectable_box_intersector2i(T new_shape) {
			shape = new_shape;
		}
		bool intersects(const aligned_box2i& box) {
			return gmtry2i::intersects(shape, box);
		}
		box_intersector2i* intersection(const aligned_box2i& box) {
			auto box_intersection = gmtry2i::intersection(shape, box);
			return new intersectable_box_intersector2i<decltype(box_intersection)>(box_intersection);
		}
		box_intersector2i* clone() {
			return new intersectable_box_intersector2i(shape);
		}
		aligned_box2i get_bounds() {
			return gmtry2i::boundsof(shape);
		}
	};

	//  Memory manager for a box_intersector2i*, through which it satisfies intersectable2i<aligned_box2i>
	class box_intersectable2i {
		box_intersector2i* intersector;
	public:
		box_intersectable2i() {
			intersector = new intersectable_box_intersector2i<aligned_box2i>(aligned_box2i());
		}
		box_intersectable2i(box_intersector2i* new_intersector) {
			intersector = new_intersector->clone();
		}
		box_intersectable2i(const box_intersectable2i& object) {
			intersector = object.intersector->clone();
		}
		inline bool intersects(const aligned_box2i& box) const {
			return intersector->intersects(box);
		}
		inline box_intersectable2i intersection(const aligned_box2i& box) const {
			return box_intersectable2i(intersector->intersection(box));
		}
		inline aligned_box2i get_bounds() const {
			return intersector->get_bounds();
		}
		box_intersectable2i operator =(const box_intersectable2i& object) {
			intersector = object.intersector->clone();
			return *this;
		}
		~box_intersectable2i() {
			delete intersector;
		}
	};

	// Creates a box_intersectable2i out of an intersectable
	// Practical for passing an intersectable2i<aligned_box> to a non-template class or function
	template <intersectable2i T>
	inline box_intersectable2i make_box_intersectable(T shape) {
		intersectable_box_intersector2i intersector(shape);
		return box_intersectable2i(&intersector);
	}

	inline bool intersects(const box_intersectable2i& object, const aligned_box2i& box) {
		return object.intersects(box);
	}

	inline box_intersectable2i intersection(const box_intersectable2i& object, const aligned_box2i& box) {
		return object.intersection(box);
	}

	inline aligned_box2i boundsof(const box_intersectable2i& object) {
		return object.get_bounds();
	}
}