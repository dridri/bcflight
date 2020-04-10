#ifndef MINISTD_MUTEX_H
#define MINISTD_MUTEX_H

namespace ministd {

class mutex
{
public:
	mutex();
	mutex( const mutex& other );
	~mutex();
};

}

#endif // MINISTD_MUTEX_H
