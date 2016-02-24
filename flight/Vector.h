#ifndef VECTOR_H
#define VECTOR_H

#include <cmath>

#define VECTOR_INLINE inline
// #define VECTOR_INLINE

#define VEC_OP( r, a, op, b ) \
	r x = a x op b x; \
	if ( n > 1 ) { \
		r y = a y op b y; \
		if ( n > 2 ) { \
			r z = a z op b z; \
			if ( n > 3 ) { \
				r w = a w op b w; \
			} \
		} \
	}

#define VEC_IM( r, a, op, im ) \
	r x = a x op im; \
	if ( n > 1 ) { \
		r y = a y op im; \
		if ( n > 2 ) { \
			r z = a z op im; \
			if ( n > 3 ) { \
				r w = a w op im; \
			} \
		} \
	}
	
#define VEC_ADD( r, a, op, b ) \
	r += a x op b x; \
	if ( n > 1 ) { \
		r += a y op b y; \
		if ( n > 2 ) { \
			r += a z op b z; \
			if ( n > 3 ) { \
				r += a w op b w; \
			} \
		} \
	}

#define VEC_ADD_IM( r, a, op, im ) \
	r += a x op im; \
	if ( n > 1 ) { \
		r += a y op im; \
		if ( n > 2 ) { \
			r += a z op im; \
			if ( n > 3 ) { \
				r += a w op im; \
			} \
		} \
	}

template <typename T, int n> class Vector
{
public:
	Vector( T x = 0, T y = 0, T z = 0, T w = 0 ) : x( x ), y( y ), z( z ), w( w ) {}
	Vector( const Vector<T,1>& v, T a = 0, T b = 0, T c = 0 ) : x(v.x), y(a), z(b), w(c) {}
	Vector( T a, const Vector<T,1>& v, T b = 0, T c = 0 ) : x(a), y(v.x), z(b), w(c) {}
	Vector( T a, T b, const Vector<T,1>& v, T c = 0 ) : x(a), y(b), z(v.x), w(c) {}
	Vector( T a, T b, T c, const Vector<T,1>& v ) : x(a), y(b), z(c), w(v.x) {}
	Vector( const Vector<T,2>& v, T a = 0, T b = 0 ) : x(v.x), y(v.z), z(a), w(b) {}
	Vector( T a, const Vector<T,2>& v, T b = 0 ) : x(a), y(v.x), z(v.y), w(b) {}
	Vector( T a, T b, const Vector<T,2>& v ) : x(a), y(b), z(v.x), w(v.y) {}
	Vector( const Vector<T,3>& v, T a = 0 ) : x(v.x), y(v.y), z(v.z), w(a) {}
	Vector(T a, const Vector<T,3>& v ) : x(a), y(v.x), z(v.y), w(v.z) {}
	Vector( const Vector<T,4>& v ) : x(v.x), y(v.y), z(v.z), w(v.w) {}
	Vector( float* v ) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}

	VECTOR_INLINE Vector<T,3> xyz() const { return Vector<T,3>( x, y, z ); }
	VECTOR_INLINE Vector<T,3> zyx() const { return Vector<T,3>( z, y, x ); }
	VECTOR_INLINE Vector<T,2> xy() const { return Vector<T,2>( x, y ); }
	VECTOR_INLINE Vector<T,2> xz() const { return Vector<T,2>( x, z ); }
	VECTOR_INLINE Vector<T,2> yz() const { return Vector<T,2>( y, z ); }

	Vector<T,n>& operator=( const Vector< T, n >& other )
	{
		VEC_IM( this-> , other. , + , 0 );
		return *this;
	}

	void normalize() {
		T add = 0;
		VEC_ADD( add, this-> , * , this-> );
		T l = std::sqrt( add );
		if ( l > 0.00001f ) {
			T il = 1 / l;
			VEC_IM( this-> , this-> , * , il );
		}
	}

	T length() const {
		T add = 0;
		VEC_ADD( add, this-> , * , this-> );
		return std::sqrt( add );
	}

	VECTOR_INLINE T operator[]( int i ) const
	{
		return ( &x )[i];
	}

	VECTOR_INLINE T& operator[]( int i )
	{
		return ( &x )[i];
	}


	VECTOR_INLINE void operator+=( const Vector<T,n>& v ) {
		VEC_OP( this-> , this-> , + , v. );
	}

	VECTOR_INLINE void operator-=( const Vector<T,n>& v ) {
		VEC_OP( this-> , this-> , - , v. );
	}

	VECTOR_INLINE void operator*=( T v ) {
		VEC_IM( this-> , this-> , * , v );
	}

	VECTOR_INLINE void operator/=( T v ) {
		VEC_IM( this-> , this-> , / , v );
	}


	VECTOR_INLINE Vector<T,n> operator-() const
	{
		Vector<T, n> ret;
		VEC_IM( ret. , - this-> , + , 0.0f );
		return ret;
	}


	VECTOR_INLINE Vector<T,n> operator+( const Vector<T,n>& v ) const {
		Vector<T, n> ret;
		VEC_OP( ret. , this-> , + , v. );
		return ret;
	}

	VECTOR_INLINE Vector<T,n> operator-( const Vector<T,n>& v ) const {
		Vector<T, n> ret;
		VEC_OP( ret. , this-> , - , v. );
		return ret;
	}

	VECTOR_INLINE Vector<T,n> operator*( T im ) const {
		Vector<T, n> ret;
		VEC_IM( ret. , this-> , * , im );
		return ret;
	}

	VECTOR_INLINE Vector<T,n> operator/( T im ) const {
		Vector<T, n> ret;
		VEC_IM( ret. , this-> , / , im );
		return ret;
	}

	VECTOR_INLINE T operator*( const Vector<T,n>& v ) const {
		T ret = 0;
		VEC_ADD( ret, this-> , * , v. );
		return ret;
	}

	VECTOR_INLINE Vector<T,n> operator^( const Vector<T,n>& v ) const {
		Vector<T, n> ret;
		for ( int i = 0; i < n; i++ ) {
			T a = ( &x )[ ( i + 1 ) % n ];
			T b = ( &v.x )[ ( i + 2 ) % n ];
			T c = ( &x )[ ( i + 2 ) % n ];
			T d = ( &v.x )[ ( i + 1 ) % n ];
			( &ret.x )[i] = a * b - c * d;
		}
		return ret;
	}

public:
	T x;
	T y;
	T z;
	T w;
} __attribute__((packed));


template <typename T, int n> Vector<T, n> operator*( T im, const Vector<T, n>& v ) {
	Vector<T, n> ret;
	for ( int i = 0; i < n; i++ ) {
		( &ret.x )[i] = ( &v.x )[i] * im;
	}
	return ret;
}


template <typename T, int n> bool operator==( const Vector<T, n>& v1, const Vector<T,n>& v2 ) {
	bool ret = true;
	for ( int i = 0; i < n; i++ ) {
		ret = ret && ( ( &v1.x )[i] == ( &v2.x )[i] );
	}
	return ret;
}


template <typename T, int n> bool operator!=( const Vector<T, n>& v1, const Vector<T,n>& v2 ) {
	bool ret = true;
	for ( int i = 0; i < n; i++ ) {
		ret = ret && ( ( &v1.x )[i] != ( &v2.x )[i] );
	}
	return ret;
}


typedef Vector<int, 2> Vector2i;
typedef Vector<int, 3> Vector3i;
typedef Vector<int, 4> Vector4i;

typedef Vector<float, 2> Vector2f;
typedef Vector<float, 3> Vector3f;
typedef Vector<float, 4> Vector4f;

typedef Vector<double, 2> Vector2d;
typedef Vector<double, 3> Vector3d;
typedef Vector<double, 4> Vector4d;

#endif // VECTOR_H
