#ifndef MATH_INCLUDE_ONCE
#define MATH_INCLUDE_ONCE

#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========================================
template<typename T>
class Vec2
{
public:
	Vec2() { data[0] = data[1] = 0; }
	Vec2(T x, T y) { data[0] = x; data[1] = y; }
	T& x() { return data[0]; }
	T& y() { return data[1]; }
	const T& x() const { return data[0]; }
	const T& y() const { return data[1]; }
	T& operator[](unsigned i) { return data[i]; }
	const T& operator[](unsigned i) const { return data[i]; }
	Vec2 operator+(const Vec2& other) const { return Vec2(x() + other.x(), y() + other.y()); }
	Vec2 operator-(const Vec2& other) const { return Vec2(x() - other.x(), y() - other.y()); }
	Vec2 operator*(T val) const { return Vec2(x()*val, y()*val); }
	Vec2 operator/(T val) const { return Vec2(x() / val, y() / val); }
	Vec2& operator+=(const Vec2& other) { *this = Vec2(x() + other.x(), y() + other.y()); return *this; }
	Vec2& operator-=(const Vec2& other) { *this = Vec2(x() - other.x(), y() - other.y()); return *this; }
	Vec2& operator*=(T val) { *this = *this * val; return *this; }
	Vec2& operator/=(T val) { *this = *this / val; return *this; }
	Vec2& operator=(const Vec2& other) { x() = other.x(); y() = other.y(); return *this; }
	bool operator==(const Vec2& other) const { return x() == other.x() && y() == other.y(); }
	bool operator!=(const Vec2& other) const { return !operator==(other); }
	Vec2<float> asfloat() const { return Vec2<float>((float)x(), (float)y()); }
	Vec2<double> asdouble() const { return Vec2<double>((double)x(), (double)y()); }
	T* ptr() { return &data[0]; }
	const T* ptr() const { return &data[0]; }
	T length() const { return ::sqrt(x()*x() + y()*y()); }
	Vec2& normalize()
	{
		T l = length();
		if (l)
			*this *= (T)(1.0 / l);
		return *this;
	}
	Vec2 normalized()
	{
		T l = length();
		if (l)
			return Vec2(x(), y()) * (T)(1.0 / l);
		return Vec2(0, 0);
	}
	T lengthSquared() { return x()*x() + y()*y(); }
private:
	T data[2];
};
typedef Vec2<int> Vec2i;
typedef Vec2<float> Vec2f;
typedef Vec2<double> Vec2d;

inline float dot(const Vec2f& v1, const Vec2f& v2) { return v1.x()*v2.x() + v1.y()*v2.y(); }
inline double dot(const Vec2d& v1, const Vec2d& v2) { return v1.x()*v2.x() + v1.y()*v2.y(); }

// ========================================
template<typename T>
class Vec3
{
public:
	Vec3() { data[0] = data[1] = data[2] = 0; }
	Vec3(T x, T y, T z) { data[0] = x; data[1] = y; data[2] = z; }
	T& x() { return data[0]; }
	T& y() { return data[1]; }
	T& z() { return data[2]; }
	const T& x() const { return data[0]; }
	const T& y() const { return data[1]; }
	const T& z() const { return data[2]; }
	T& operator[](unsigned i) { return data[i]; }
	const T& operator[](unsigned i) const { return data[i]; }
	Vec3 operator+(const Vec3& other) const { return Vec3(x() + other.x(), y() + other.y(), z() + other.z()); }
	Vec3 operator-(const Vec3& other) const { return Vec3(x() - other.x(), y() - other.y(), z() - other.z()); }
	Vec3 operator*(T val) const { return Vec3(x()*val, y()*val, z()*val); }
	Vec3 operator/(T val) const { return Vec3(x() / val, y() / val, z() / val); }
	bool operator==(const Vec3& other) const { return x() == other.x() && y() == other.y() && z() == other.z(); }
	bool operator!=(const Vec3& other) const { return !operator==(other); }
	Vec3<float> asfloat() const { return Vec3<float>((float)x(), (float)y(), (float)z()); }
	Vec3<double> asdouble() const { return Vec3<double>((double)x(), (double)y(), (double)z()); }
	T* ptr() { return &data[0]; }
	const T* ptr() const { return &data[0]; }
	T gray() const { return (T)0.2989 * x() + (T)0.5870 * y() + (T)0.1140 * z(); }
	T length() const { return ::sqrt(x()*x() + y()*y() + z()*z()); }
	Vec3& normalize()
	{
		T l = length();
		if (l)
			*this *= (T)(1.0 / l);
		return *this;
	}
	Vec3 normalized()
	{
		T l = length();
		if (l)
			return Vec3(x(), y(), z()) * (T)(1.0 / l);
		return Vec3(0, 0, 0);
	}
private:
	T data[3];
};
typedef Vec3<int> Vec3i;
typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;

// ========================================
template<typename T>
class Vec4
{
public:
	Vec4() { data[0] = data[1] = data[2] = data[3] = 0; }
	Vec4(const Vec3<T> XYZ, const T& W) { data[0] = XYZ.x(); data[1] = XYZ.y(); data[2] = XYZ.z(); data[3] = W; }
	Vec4(T x, T y, T z, T w) { data[0] = x; data[1] = y; data[2] = z; data[3] = w; }
	T& x() { return data[0]; }
	T& y() { return data[1]; }
	T& z() { return data[2]; }
	T& w() { return data[3]; }
	const T& x() const { return data[0]; }
	const T& y() const { return data[1]; }
	const T& z() const { return data[2]; }
	const T& w() const { return data[3]; }
	Vec3<T> xyz() const { return Vec3<T>(x(), y(), z()); }
	T& operator[](unsigned i) { return data[i]; }
	const T& operator[](unsigned i) const { return data[i]; }
	T* ptr() { return &data[0]; }
	const T* ptr() const { return &data[0]; }
private:
	T data[4];
};
typedef Vec4<int> Vec4i;
typedef Vec4<unsigned int> Vec4u;
typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;

// ========================================
template<typename T>
class Mat4
{
public:
	Mat4() { setIdentity(); }
	void setIdentity() 
	{
		data[0] = Vec4<T>(1, 0, 0, 0);
		data[1] = Vec4<T>(0, 1, 0, 0);
		data[2] = Vec4<T>(0, 0, 1, 0);
		data[3] = Vec4<T>(0, 0, 0, 1);
	}
	static Mat4<T> getIdentity()
	{
		return Mat4<T>();
	}
	static Mat4<T> getOrtho(T left, T right, T bottom, T top, T pnear, T pfar)
	{
		Mat4<T> m;

		m.e(0, 0) = ((T)2) / (right - left);
		m.e(0, 1) = 0;
		m.e(0, 2) = 0;
		m.e(0, 3) = -(right + left) / (right - left);

		m.e(1, 0) = 0;
		m.e(1, 1) = ((T)2) / (top - bottom);
		m.e(1, 2) = 0;
		m.e(1, 3) = -(top + bottom) / (top - bottom);

		m.e(2, 0) = 0;
		m.e(2, 1) = 0;
		m.e(2, 2) = -((T)2) / (pfar - pnear);
		m.e(2, 3) = -(pfar + pnear) / (pfar - pnear);

		m.e(3, 0) = 0;
		m.e(3, 1) = 0;
		m.e(3, 2) = 0;
		m.e(3, 3) = ((T)1);

		return m;
	}
	
	static Mat4<T> getOrtho2D(T left, T right, T bottom, T top) {
		return getOrtho(left, right, bottom, top, -1, +1);
	}

	T& e(int i, int j) { return data[j][i]; }
	const T& e(int i, int j) const { return data[j][i]; }

	T* ptr() { return &e(0, 0); }
	const T* ptr() const { return &e(0, 0); }

private:
	Vec4<T> data[4];
};
typedef Mat4<float> Mat4x4f;

// ========================================
class BoundingBox2d
{
public:
	BoundingBox2d() : _Min(FLT_MAX, FLT_MAX), _Max(-FLT_MAX, -FLT_MAX) {}
	BoundingBox2d(const Vec2f& Pnt1, const Vec2f& Pnt2) {
		_Min = Vec2f(
			std::min(Pnt1.x(), Pnt2.x()),
			std::min(Pnt1.y(), Pnt2.y()));
		_Max = Vec2f(
			std::max(Pnt1.x(), Pnt2.x()),
			std::max(Pnt1.y(), Pnt2.y()));
	}
	const Vec2f& GetMin() const { return _Min; }
	const Vec2f& GetMax() const { return _Max; }

private:
	Vec2f _Min;
	Vec2f _Max;
};

// ========================================
class Camera2D
{
public:
	explicit Camera2D(const BoundingBox2d& domainBoundingBox)
	{
		Vec2f position = (domainBoundingBox.GetMax() + domainBoundingBox.GetMin()) / 2;
		float width = domainBoundingBox.GetMax().x() - domainBoundingBox.GetMin().x();
		float height = domainBoundingBox.GetMax().y() - domainBoundingBox.GetMin().y();
		float left = position.x() - width * 0.5f;
		float right = position.x() + width * 0.5f;
		float bottom = position.y() - height * 0.5f;
		float top = position.y() + height * 0.5f;
		_View = Mat4x4f::getIdentity();
		_Proj = Mat4x4f::getOrtho2D(left, right, bottom, top);
	}

	const Mat4x4f& GetView() const { return _View; }
	const Mat4x4f& GetProj() const { return _Proj; }

private:
	Mat4x4f _View;
	Mat4x4f _Proj;
};


#endif //MATH_INCLUDE_ONCE