/*
 * BCFlight
 * Copyright (C) 2017 Adrien Aubry (drich)
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

#ifndef MULTILINK_H
#define MULTILINK_H

#include <list>
#include "../links/Link.h"

class Main;
class Config;

class MultiLink : public Link
{
public:
	MultiLink( std::list<Link*> senders, std::list<Link*> receivers );
	MultiLink( std::initializer_list<Link*> senders, std::initializer_list<Link*> receivers );
	~MultiLink();

	int Connect();
	int setBlocking( bool blocking );
	void setRetriesCount( int retries );
	int retriesCount() const;
	int32_t Channel();
	int32_t Frequency();
	int32_t RxQuality();
	int32_t RxLevel();
	uint32_t fullReadSpeed();

	int Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );
	int Read( void* buf, uint32_t len, int32_t timeout );

	static int flight_register( Main* main );

protected:
	static Link* Instanciate( Config* config, const std::string& lua_object );

	bool mBlocking;
	std::list< Link* > mSenders;
	std::list< Link* > mReceivers;
};

#endif // MULTILINK_H
