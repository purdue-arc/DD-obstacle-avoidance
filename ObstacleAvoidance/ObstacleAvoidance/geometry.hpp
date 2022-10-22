
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

	struct aligned_box2i {
		vector2i max, min;
		aligned_box2i() = default;
	};
}