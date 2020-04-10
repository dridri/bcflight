#ifndef MINISTD_MAP_H
#define MINISTD_MAP_H

#include <malloc.h>
#include <initializer_list>

namespace ministd {


template< typename K, typename V > struct pair
{
	K first;
	V second;
};


template< typename K, typename V > class map
{
public:
	map() : mPairs(nullptr), mLength(0) {}
	map( const map& other ) {
		if ( other.mPairs and other.mLength > 0 ) {
			mLength = other.mLength;
			mPairs = (pair< K, V >*)malloc( sizeof(pair< K, V >) * mLength );
			memcpy( mPairs, other.mPairs, sizeof(pair< K, V >) * mLength );
		}
	}
	map( const std::initializer_list< pair<K, V> >& ini ) {
		// TODO
	}
	~map() {
		if ( mPairs ) {
			for ( uint32_t i = 0; i < mLength; i++ ) {
			mPairs[i].first = K();
			mPairs[i].second = V();
			}
			free( mPairs );
		}
	}

	V& operator[]( const K& key ) {
		for ( uint32_t i = 0; i < mLength; i++ ) {
			if ( mPairs[i].first == key ) {
				return mPairs[i].second;
			}
		}
		// If we still did not returned here, it means we did not find the key, so we create it
		mPairs = (pair< K, V >*)realloc( mPairs, sizeof(pair< K, V >) * ( mLength + 1 ) );
		memset( &mPairs[mLength].first, 0, sizeof(K) );
		memset( &mPairs[mLength].second, 0, sizeof(V) );
		mPairs[mLength].first = key;
		mPairs[mLength].second = V();
		mLength++;
		return mPairs[mLength - 1].second;
	}
	const V& operator[]( const K& key ) const {
		if ( mPairs and mLength > 0 ) {
			for ( uint32_t i = 0; i < mLength; i++ ) {
				if ( mPairs[i].first == key ) {
					return mPairs[i].second;
				}
			}
		}
		return *((V*)0);
	}

	pair< K, V >* begin() const {
		return &mPairs[0];
	}

	pair< K, V >* end() const {
		return &mPairs[mLength];
	}

	pair< K, V >* find( const K& key ) const {
		if ( mPairs and mLength > 0 ) {
			for ( uint32_t i = 0; i < mLength; i++ ) {
				if ( mPairs[i].first == key ) {
					return &mPairs[i];
				}
			}
			return &mPairs[mLength];
		}
	}


protected:
	pair< K, V >* mPairs;
	uint32_t mLength;
};

}

#endif // MINISTD_MAP_H
