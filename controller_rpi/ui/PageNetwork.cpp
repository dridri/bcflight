#include <algorithm>
#include <iwlib.h>
#include <fstream>
#include <gammaengine/Debug.h>
#include <libbcui/Widget.h>
#include <libbcui/Button.h>
#include "PageNetwork.h"
#include "Globals.h"


static std::string remove_trailing( const std::string& s )
{
	size_t start = 0;
	size_t end = 0;

	for ( start = 0; start < s.length(); start++ ) {
		if ( s.at(start) != ' ' and s.at(start) != '\t' ) {
			break;
		}
	}
	for ( end = s.length() - 1; end >= 0; end-- ) {
		if ( s.at(end) != ' ' and s.at(end) != '\t' ) {
			end++;
			break;
		}
	}

	return s.substr( start, end - start );
}


static bool string_match( const std::string& s1, const std::string& s2 )
{
	bool last_equal = false;
	size_t adv = 0;

	for ( size_t i = 0; i < s2.length(); ) {
		while ( i < s2.length() and s2.at(i) == '*' ) {
			i++;
		}
		size_t next = i;
		while ( next < s2.length() and s2.at(next) != '*' ) {
			next++;
		}
		adv = s1.find( s2.substr( i, next - i ), adv );
		if ( adv == std::string::npos ) {
			return false;
		}
		last_equal = ( s1.substr(adv) == s2.substr( i, next - i ) );
		if ( i >= s2.length() - 1 and s2.at(i - 1) == '*' ) {
			last_equal = true;
		}
		adv += ( next - i );
		i = next;
	}

	return last_equal;
}


PageNetwork::PageNetwork() : BC::TablePage()
{
	mColumnsWidth.resize(1);
	mColumnsWidth[0] = 0.9f;

	std::ifstream file( "data/networks.txt" );
	std::string line;
	if ( file.is_open() ) {
		while ( std::getline( file, line, '\n' ) ) {
			if ( line.at(0) != '#' ) {
				std::string key = line.substr( 0, line.find( "=" ) );
				std::string value = line.substr( line.find( "=" ) + 1 );
				gDebug() << "Loading '" << key << "' = '" << value << "'\n";
				key = remove_trailing( key );
				value = remove_trailing( value );
				gDebug() << "Storing '" << key << "' = '" << value << "'\n";
				mKnownNetworks[ key ] = value;
			}
		}
		file.close();
	}
}


PageNetwork::~PageNetwork()
{
}


int wifi_compare( wireless_scan* a, wireless_scan* b ) {
	return a->stats.qual.qual > b->stats.qual.qual;
}


void PageNetwork::gotFocus()
{
	BC::TablePage::gotFocus();
	getGlobals()->window()->Clear( 0xFF303030 );
	getGlobals()->RenderDrawer();
	render();
	getGlobals()->mainRenderer()->DrawText( getGlobals()->icon( "selector" )->width() * 1.1 + 32, 32, getGlobals()->font(), 0xFFFFFFFF, "Scanning..." );

	std::vector< wireless_scan* > list;
	wireless_scan_head head;
	wireless_scan* result;
	iwrange range;
	int sock;
	int i;

	sock = iw_sockets_open();

	if ( iw_get_range_info( sock, (char*)"wlan0", &range ) < 0 ) {
		gDebug() << "Error during iw_get_range_info. Aborting.\n";
		goto gotfocus_end;
	}

	while ( iw_scan( sock, (char*)"wlan0", range.we_version_compiled, &head ) < 0 && errno == 16 ) {
		gDebug() << "Error during iw_scan (" << errno << "). Retrying.\n";
	}

	result = head.result;
	while ( nullptr != result ) {
		list.push_back( result );
		result = result->next;
	}
	std::sort( list.begin(), list.end(), wifi_compare );
	i = 0;
	mWidgets.clear();
	mScannedNetworks.clear();
	for ( auto w : list ) {
		std::string ssid = w->b.essid;
		bool found = false;
		for ( auto it : mKnownNetworks ) {
			if ( it.first == ssid ) {
				gDebug() << "found " << it.first << " == " << ssid << "\n";
				found = true;
				break;
			}
			if ( string_match( ssid, it.first ) ) {
				gDebug() << "found " << it.first << " == " << ssid << "\n";
// 				mKnownNetworks[ ssid ] = it.second;
				mKnownNetworks.emplace( std::make_pair( ssid, it.second ) );
				found = true;
				break;
			}
		}
		if ( found ) {
			printf( "%s  %.3f  %d  %d\n", w->b.essid, w->b.freq / 1000000000.0, w->stats.qual.qual, w->stats.qual.level );
			std::function<void()> fct = [this,i]() { this->networkClicked(i); };
			std::vector< BC::Widget* > row;
			row.push_back( new BC::Button( getGlobals()->font(), std::wstring( ssid.begin(), ssid.end() ), fct ) );
			mWidgets.push_back( row );
			mScannedNetworks.push_back( ssid );
			i++;
		}
	}

gotfocus_end:
	iw_sockets_close( sock );

	BC::TablePage::gotFocus();
	getGlobals()->window()->Clear( 0xFF303030 );
	getGlobals()->RenderDrawer();
	render();
}


void PageNetwork::click( float _x, float _y, float force )
{
	int x = _x * getGlobals()->window()->width();
	int y = _y * getGlobals()->window()->height();

	if ( getGlobals()->PageSwitcher( x, y ) ) {
		return;
	}

	BC::TablePage::click( _x, _y, force );
}


void PageNetwork::touch( float x, float y, float force )
{
	BC::TablePage::touch( x, y, force );
}


bool PageNetwork::update( float t, float dt )
{
	return BC::TablePage::update( t, dt );
}


void PageNetwork::background()
{
	getGlobals()->window()->ClearRegion( 0xFF303030, 16 + getGlobals()->icon( "selector" )->width() * 1.1, 0, getGlobals()->window()->width() - 16 - getGlobals()->icon( "selector" )->width() * 1.1, getGlobals()->window()->height() );
}


void PageNetwork::networkClicked( int inetwork )
{
	fDebug( inetwork );
	gDebug() << "SSID : " << mScannedNetworks[ inetwork ] << "\n";
	gDebug() << "Passphrase : " << mKnownNetworks[ mScannedNetworks[ inetwork ] ] << "\n";

	std::string conf = R"(ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
		ssid=")" + mScannedNetworks[ inetwork ] + R"("
		psk=")" + mKnownNetworks[ mScannedNetworks[ inetwork ] ] + R"("
})";

	std::ofstream file( "/etc/wpa_supplicant/wpa_supplicant.conf" );
	file.write( conf.data(), conf.length() );
	file.close();

	if ( system( "killall -HUP wpa_supplicant" ) < 0 ) {
		gDebug() << "WARNING : Cannot update wpa_supplicant !\n";
	}
}
