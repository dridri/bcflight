#ifndef PAGENETWORK_H
#define PAGENETWORK_H

#include <string>
#include <map>
#include <libbcui/TablePage.h>

class PageNetwork : public BC::TablePage
{
public:
	PageNetwork();
	~PageNetwork();

	void gotFocus();
	void click( float x, float y, float force );
	void touch( float x, float y, float force );
	bool update( float t, float dt );
	void background();

protected:
	void networkClicked( int inetwork );
	std::map< std::string, std::string > mKnownNetworks;
	std::vector< std::string > mScannedNetworks;
};

#endif // PAGENETWORK_H
