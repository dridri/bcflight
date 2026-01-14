/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef VECTOR_H
#define VECTOR_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <ostream>
#ifdef LUA_CLASS
#include <Lua.h>
#endif

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
	Vector() : x(0), y(0), z(0), w(0) {}
	Vector( T x ) : x(x), y(x), z(x), w(x) {}
	Vector( T x, T y ) : x(x), y(y), z(x), w(y) {}
	Vector( T x, T y, T z ) : x(x), y(y), z(z), w(0) {}
	Vector( T x, T y, T z, T w ) : x(x), y(y), z(z), w(w) {}
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
#ifdef LUA_CLASS
	explicit Vector( const LuaValue& v ) {
		if ( v.type() == LuaValue::Table ) {
			const std::map<std::string, LuaValue >& t = v.toTable();
			try {
				x = t.at("x").toNumber();
				y = t.at("y").toNumber();
				z = t.at("z").toNumber();
				w = t.at("w").toNumber();
			} catch ( std::exception& e ) {
			}
		}
	}
#endif

	VECTOR_INLINE Vector<T,3> xyz() const { return Vector<T,3>( x, y, z ); }
	VECTOR_INLINE Vector<T,3> zyx() const { return Vector<T,3>( z, y, x ); }
	VECTOR_INLINE Vector<T,2> xy() const { return Vector<T,2>( x, y ); }
	VECTOR_INLINE Vector<T,2> xz() const { return Vector<T,2>( x, z ); }
	VECTOR_INLINE Vector<T,2> yz() const { return Vector<T,2>( y, z ); }

	constexpr int size() const {
		return n;
	}

	std::vector<T> toStdVector() const {
		std::vector<T> ret;
		for ( int i = 0; i < n; i++ ) {
			ret.push_back( ptr[i] );
		}
		return ret;
	}


	Vector<T,n>& operator=( const Vector< T, n >& other )
	{
		VEC_IM( this-> , other. , + , 0 );
		return *this;
	}

	Vector<T,n>& operator=( const T& v )
	{
		for ( int i = 0; i < n; i++ ) {
			ptr[i] = v;
		}
		return *this;
	}

	void normalize() {
		T add = 0;
		VEC_ADD( add, this-> , * , this-> );
		T l = sqrt( add );
		if ( l > 0.00001f ) {
			T il = 1 / l;
			VEC_IM( this-> , this-> , * , il );
		}
	}

	Vector<T,n> normalized() {
		Vector<T,n> ret;
		T add = 0;
		VEC_ADD( add, this-> , * , this-> );
		T l = sqrt( add );
		if ( l > 0.00001f ) {
			T il = 1 / l;
			VEC_IM( ret. , this-> , * , il );
		}
		return ret;
	}

	Vector<T,n> clamped(const Vector<T,n>& min, const Vector<T,n>& max) {
		Vector<T,n> ret;
		ret.x = std::clamp( x, min.x, max.x );
		if ( n > 1 ) {
			ret.y = std::clamp( y, min.y, max.y );
			if ( n > 2 ) {
				ret.z = std::clamp( z, min.z, max.z );
				if ( n > 3 ) {
					ret.w = std::clamp( w, min.w, max.w );
				}
			}
		}
		return ret;
	}

	T length() const {
		T add = 0;
		VEC_ADD( add, this-> , * , this-> );
		return sqrt( add );
	}

	VECTOR_INLINE T operator[]( int i ) const
	{
		return ptr[i];
	}

	VECTOR_INLINE T& operator[]( int i )
	{
		return ptr[i];
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

	VECTOR_INLINE void operator/=( const Vector<T,n>& v ) {
		VEC_OP( this-> , this-> , / , v. );
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

	VECTOR_INLINE Vector<T,n> operator/( const Vector<T,n>& v ) const {
		Vector<T, n> ret;
		VEC_OP( ret. , this-> , / , v. );
		return ret;
	}

	VECTOR_INLINE T operator&( const Vector<T,n>& v ) const {
		T ret = 0;
		VEC_ADD( ret, this-> , * , v. );
		return ret;
	}

	VECTOR_INLINE Vector<T,n> operator*( const Vector<T,n>& v ) const {
		Vector<T,n> ret;
		VEC_OP( ret., this-> , * , v. );
		return ret;
	}

	VECTOR_INLINE Vector<T,n> operator^( const Vector<T,n>& v ) const {
		Vector<T, n> ret;
		for ( int i = 0; i < n; i++ ) {
			T a = ptr[ ( i + 1 ) % n ];
			T b = v.ptr[ ( i + 2 ) % n ];
			T c = ptr[ ( i + 2 ) % n ];
			T d = v.ptr[ ( i + 1 ) % n ];
			ret.ptr[i] = a * b - c * d;
		}
		return ret;
	}

    friend std::ostream& operator<<(std::ostream& os, const Vector<T, n>& v) {
		os << "[";
		for ( int i = 0; i < n; i++ ) {
			os << v.ptr[i];
			if ( i < n - 1 ) {
				os << ", ";
			}
		}
		os << "]";
		return os;
	}

public:
	union {
		struct {
			T x;
			T y;
			T z;
			T w;
		};
		T ptr[std::max(n, 4)];
	};
};

template <typename T, int n> Vector<T, n> operator*( T im, const Vector<T, n>& v ) {
	Vector<T, n> ret;
	for ( int i = 0; i < n; i++ ) {
		ret.ptr[i] = im * v.ptr[i];
	}
	return ret;
}


template <typename T, int n> Vector<T, n> operator/( T im, const Vector<T, n>& v ) {
	Vector<T, n> ret;
	for ( int i = 0; i < n; i++ ) {
		ret.ptr[i] = im / v.ptr[i];
	}
	return ret;
}


template <typename T, int n> Vector<T, n> operator/( const Vector<T, n>& a, const Vector<T, n>& b ) {
	Vector<T, n> ret;
	VEC_OP( ret. , a. , / , b. );
	return ret;
}


template <typename T, int n> bool operator==( const Vector<T, n>& v1, const Vector<T,n>& v2 ) {
	bool ret = true;
	for ( int i = 0; i < n; i++ ) {
		ret = ret && ( v1.ptr[i] == v2.ptr[i] );
	}
	return ret;
}


template <typename T, int n> bool operator!=( const Vector<T, n>& v1, const Vector<T,n>& v2 ) {
	bool ret = true;
	for ( int i = 0; i < n; i++ ) {
		ret = ret && ( v1.ptr[i] != v2.ptr[i] );
	}
	return ret;
}


namespace std {
	template <typename T, int n> Vector<T, n> cos( const Vector<T, n>& v ) {
		Vector<T, n> ret;
		for ( int i = 0; i < n; i++ ) {
			ret.ptr[i] = std::cos( v.ptr[i] );
		}
		return ret;
	}
	template <typename T, int n> Vector<T, n> sin( const Vector<T, n>& v ) {
		Vector<T, n> ret;
		for ( int i = 0; i < n; i++ ) {
			ret.ptr[i] = std::sin( v.ptr[i] );
		}
		return ret;
	}
};


typedef Vector<int, 2> Vector2i;
typedef Vector<int, 3> Vector3i;
typedef Vector<int, 4> Vector4i;

typedef Vector<float, 2> Vector2f;
typedef Vector<float, 3> Vector3f;
typedef Vector<float, 4> Vector4f;

typedef Vector<double, 2> Vector2d;
typedef Vector<double, 3> Vector3d;
typedef Vector<double, 4> Vector4d;


template<typename T> struct is_vector : std::false_type {};
template<typename T, int n> struct is_vector<Vector<T, n>> : std::true_type {};

#endif // VECTOR_H
