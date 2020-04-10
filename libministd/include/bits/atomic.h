#ifndef MINISTD_ATOMIC_H
#define MINISTD_ATOMIC_H

namespace ministd {

template< typename T > class atomic
{
public:
	atomic();
	atomic( const atomic& other );
	~atomic();

protected:
	T mValue;
};

typedef atomic<bool> atomic_bool;

}

#endif // MINISTD_ATOMIC_H
