#ifdef SYSTEM_NAME_Linux
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include <regex>
#include "Console.h"
#include "Config.h"


Console::Console( Config* config )
	: Thread( "console" )
	, mConfig( config )
{
#ifdef SYSTEM_NAME_Linux
	if ( getenv("TERM") ) {
		struct termios term;
		ioctl(0, TCGETS, &term );
		term.c_lflag &= (~ICANON);
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 0;
		ioctl( 0, TCSETS, &term );
		printf("ok\n");
	}
#endif
}


Console::~Console()
{
}

bool Console::alnum( char c )
{
	return ( c >= 'a' and c <= 'z' ) or ( c >= 'A' and c <= 'Z' ) or ( c >= '0' and c <= '9' ) or c == '_';
}

bool Console::luavar( char c )
{
	return ( c >= 'a' and c <= 'z' ) or ( c >= 'A' and c <= 'Z' ) or ( c >= '0' and c <= '9' ) or c == '_' or c == '.' or c == '[' or c == ']';
}


bool Console::run()
{
	uint32_t cursor = 0;

	vector<string> history;
	for ( const string& s : mFullHistory ) {
		history.emplace_back( string(s) );
	}
	history.emplace_back(string());
	uint32_t history_cursor = history.size() - 1;

	string* prompt = &history.back();

	printf("\33[2K\rflight> "); fflush(stdout);
	while ( true ) {
		char buf[256];
		memset( buf, 0, 256 );
		int32_t res = read( 0, buf, 255 );
		buf[res] = 0;
		// printf("line: %d %02x %02X %02X %02X %02X %02X\n", res, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		if ( buf[0] == 0x0a ) {
			break;
		} else if ( buf[0] == 0x09 ) {
			int32_t start = cursor;
			int32_t end = cursor;
			while ( start > 0 and luavar((*prompt)[start - 1]) ) {
				start--;
			}
			while ( end < prompt->length() and alnum((*prompt)[end]) ) {
				end++;
			}
			if ( start >= 0 and end <= prompt->length() ) {
				printf( "start, end : %d, %d\n", start, end );
				string query = "_G." + prompt->substr( start, end - start );
				string leftquery = query.substr( 0, query.rfind( "." ) );
				string rightquery = query.substr( query.rfind( "." ) + 1 );
				const vector<string> allKeys = mConfig->luaState()->valueKeys( leftquery );
				vector<string> keys;
				for ( const string& k : allKeys ) {
					if ( k.find(rightquery) == 0 ) {
						keys.push_back( k );
					}
				}
				string commonPart = "";
				if ( keys.size() > 0 ) {
					bool finished = false;
					while ( true ) {
						for ( const string& k : keys ) {
							if ( k.find(commonPart) == k.npos ) {
								commonPart = commonPart.substr( 0, commonPart.length() - 1 );
								finished = true;
								break;
							}
						}
						if ( finished or commonPart.length() >= keys[0].length() ) {
							break;
						}
						commonPart = keys[0].substr( 0, commonPart.length() + 1 );
					};
				}
				if ( commonPart.length() > rightquery.length() ) {
					string newValue = ( leftquery + "." ).substr( 3 ) + commonPart;
					*prompt = prompt->substr( 0, start ) + newValue + prompt->substr( end );
					cursor = start + newValue.length();
				} else if ( keys.size() > 1 ) {
					printf("\n"); fflush(stdout);
					for ( const string& k : keys ) {
						printf( "%s  ", k.c_str() );
					}
					printf("\n"); fflush(stdout);
				}
			}
		} else if ( buf[0] == 0x1b and buf[1] == 0x5b ) {
			if ( buf[2] == 0x41 ) {
				// up
				if ( history_cursor > 0 ) {
					history_cursor--;
					prompt = &history[history_cursor];
					cursor = prompt->length();
				}
			} else if ( buf[2] == 0x42 ) {
				// down
				if ( history_cursor < history.size() - 1 ) {
					history_cursor++;
					prompt = &history[history_cursor];
					cursor = prompt->length();
				}
			} else if ( buf[2] == 0x44 ) {
				// left
				if ( cursor > 0 ) {
					cursor--;
				}
			} else if ( buf[2] == 0x43 ) {
				// right
				cursor = std::min( (uint32_t)prompt->length(), cursor + 1 );
			} else if ( buf[2] == 0x31 and res >= 6 ) {
				// ctrl
				if ( buf[3] == 0x3B and buf[4] == 0x35 ) {
					if ( buf[5] == 0x44 ) {
						// ctrl + left
						bool base_type = alnum( (*prompt)[cursor - 1] );
						while ( cursor > 0 and alnum( (*prompt)[cursor - 1] ) == base_type ) {
							cursor--;
						}
					} else if ( buf[5] == 0x43 ) {
						// ctrl + right
						bool base_type = alnum( (*prompt)[cursor] );
						while ( cursor < prompt->length() and alnum( (*prompt)[cursor] ) == base_type ) {
							cursor++;
						}
					}
				}
			} else if ( buf[2] == 0x33 ) {
				// del
				if ( cursor < prompt->length() ) {
					prompt->erase( cursor, 1 );
				}
			} else if ( buf[2] == 0x48 ) {
				// home
				cursor = 0;
			} else if ( buf[2] == 0x46 ) {
				// end
				cursor = prompt->length();
			}
		} else if ( buf[0] == 0x1b and buf[1] == 0x7f ) {
			uint32_t cursor2 = cursor;
			bool base_type = alnum( (*prompt)[cursor2 - 1] );
			while ( cursor2 > 0 and alnum( (*prompt)[cursor2 - 1] ) == base_type ) {
				cursor2--;
			}
			prompt->erase( cursor2, cursor - cursor2 );
			cursor = cursor2;
		} else if ( buf[0] == 0x7f ) {
			if ( cursor > 0 ) {
				prompt->erase( cursor - 1, 1 );
				cursor--;
			}
		} else {
			prompt->insert( cursor, buf );
			cursor = std::min( (uint32_t)prompt->length(), cursor + (uint32_t)strlen(buf) );
		}
		string cursor_move;
		for ( uint32_t i = 0; i < prompt->length() - cursor; i++ ) {
			cursor_move += "\033[D";
		}
		printf("\33[2K\rflight> %s%s", prompt->c_str(), cursor_move.c_str()); fflush(stdout);
	}

	string ppt = string(*prompt);
	if ( mFullHistory.size() == 0 or ppt != mFullHistory.back() ) {
		mFullHistory.emplace_back( ppt );
	}

	mConfig->Execute( *prompt, true );
}
