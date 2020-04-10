#ifndef MINISTD_LIST_H
#define MINISTD_LIST_H

#include <stdint.h>
#include <malloc.h>

namespace ministd {

template< typename T> class list
{
public:
	typedef struct s_link {
		T value;
		struct s_link* next;
	} _link;

	class iterator {
	public:
		iterator( _link* ptr ): ptr(ptr) {}
		iterator operator++() { ptr = ptr->next; return *this; }
		bool operator!=(const iterator & other) const { return ptr != other.ptr; }
		const T& operator*() const { return ptr->value; }
	private:
		_link* ptr;
	};

	list() : mFront(nullptr), mBack(nullptr) {}
	list( const list& other ) {
		link* curr = other.mFront;
		while ( curr ) {
			push_back( curr->value );
		}
	}
	~list() {
		link* curr = mFront;
		while ( curr ) {
			link* next = curr->next;
			free( curr );
			curr = next;
		}
	}


	void push_back( const T& value ) {
		link* lnk = (link*)malloc( sizeof(struct s_link) );
		lnk->value = value;
		lnk->next = nullptr;
		if ( mBack ) {
			mBack->next = lnk;
		}
		if ( not mFront ) {
			mFront = lnk;
		}
		mBack = lnk;
	}

	inline void emplace_back( const T& value ) {
		push_back( value );
	}

	iterator begin() const { return iterator(mFront); }
	iterator end() const { return iterator((link*)0); }

protected:
	typedef _link link;
	link* mFront;
	link* mBack;
};

}

#endif // MINISTD_LIST_H
