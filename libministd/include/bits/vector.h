#ifndef MINISTD_VECTOR_H
#define MINISTD_VECTOR_H

#include <malloc.h>

namespace ministd {

template< typename T > class vector
{
public:
	vector() : mData(nullptr), mCapacity(0), mLength(0) {}
	vector( const vector& other ) : mData(nullptr), mCapacity(0), mLength(0) {
		if ( other.mData and other.mLength > 0 ) {
			mData = (T*)malloc( sizeof(T) * other.mLength );
			memcpy( mData, other.mData, sizeof(T) * other.mLength );
			mLength = other.mLength;
			mCapacity = other.mLength;
		}
	}
	~vector() {
		if ( mData ) {
			free( mData );
		}
	}

	size_t size() const {
		return mLength;
	}

	void resize( size_t n ) {
		mLength = n;
		mData = (T*)realloc( mData, sizeof(T) * n );
	}

	T& operator[]( size_t i ) {
		if ( i >= mLength ) {
			return *((T*)0);
		}
		return mData[i];
	}
	const T& operator[]( size_t i ) const {
		return mData[i];
	}
	T& at( size_t i ) {
		if ( i >= mLength ) {
			return *((T*)0);
		}
		return mData[i];
	}
	const T& at( size_t i ) const {
		return mData[i];
	}

	void push_back( const T& value ) {
		if ( mLength + 1 > mCapacity ) {
			mCapacity++;
			mData = (T*)realloc( mData, sizeof(T) * mCapacity );
		}
		mData[mLength] = value;
		mLength++;
	}

	inline void emplace_back( const T& value ) {
		push_back( value );
	}

	T* begin() const {
		return &mData[0];
	}

	T* end() const {
		return &mData[mLength];
	}


protected:
	T* mData;
	uint32_t mCapacity;
	uint32_t mLength;
};

}

#endif // MINISTD_VECTOR_H
