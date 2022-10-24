#pragma once


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
		void operator +=(vector2i v) {
			x += v.x;
			y += v.y;
		}
		void operator -=(vector2i v) {
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

	inline bool contains(const aligned_box2i& b, const vector2i& v) {
		return b.min.x <= v.x && b.min.y <= v.y && b.max.x > v.x && b.max.y > v.y;
	}

	inline bool contains(const aligned_box2i& b1, const aligned_box2i& b2) {
		return b1.min.x <= b2.min.x && b1.min.y <= b2.min.y && b1.max.x >= b2.max.x && b1.max.y >= b2.max.y;
	}

	inline vector2i center(const aligned_box2i& b1) {
		return (b1.min + b1.max) >> 1;
	}

	inline aligned_box2i operator +(const aligned_box2i& b1, const aligned_box2i& b2) {
		return aligned_box2i(b1.min + b2.min, b1.max + b2.max);
	}

	inline aligned_box2i operator -(const aligned_box2i& b1, const aligned_box2i& b2) {
		vector2i min1max2dif = b1.min - b2.max;
		vector2i max1min2dif = b1.max - b2.min;
		bool thingy = min1max2dif.x < max1min2dif.x || min1max2dif.y < max1min2dif.y;
		return aligned_box2i(thingy ? min1max2dif : max1min2dif, thingy ? max1min2dif : min1max2dif);
	}
}