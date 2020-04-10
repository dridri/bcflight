#ifndef MINISTD_FUNCTIONAL_H
#define MINISTD_FUNCTIONAL_H

namespace ministd {

template <class T1, class ...T>
struct first
{
	typedef T1 type;
};

template <typename T> class function;

template< typename Result, typename... Args > class function<Result(Args...)>
{
public:
	~function() {}
	function() {}
	function( const function& other ) {}
	template <typename T> function( T* t ) {
		fn = t;
	}
	template <typename T> function( Result (*p)(Args...) ) {
		fn = p;
	}
	Result operator()( Args... v ) {
		return fn( v... );
	}

private:
	Result (*fn)(Args...);
};

}

#endif // MINISTD_FUNCTIONAL_H
