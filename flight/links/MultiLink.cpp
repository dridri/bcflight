#include <../links/Link.h>
#include <../links/Link.h>
#include <../links/Link.h>
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

#include "MultiLink.h"
#include <Main.h>
#include <Config.h>

int MultiLink::flight_register( Main* main )
{
	RegisterLink( "MultiLink", &MultiLink::Instanciate );
	return 0;
}


Link* MultiLink::Instanciate( Config* config, const std::string& lua_object )
{
	int senders_count = config->ArrayLength( lua_object + ".senders" );
	int receivers_count = config->ArrayLength( lua_object + ".receivers" );
	int read_timeout = config->integer( lua_object + ".read_timeout" );

	if ( senders_count < 1 and receivers_count < 0 ) {
		gDebug() << "WARNING : There should be at least 1 sender or receiver, cannot create Link !\n";
		return nullptr;
	}

	std::list<Link*> senders;
	for ( int i = 0; i < senders_count; i++ ) {
		senders.emplace_back( Link::Create( config, lua_object + ".senders[" + std::to_string(i+1) + "]" ) );
	}

	std::list<Link*> receivers;
	for ( int i = 0; i < receivers_count; i++ ) {
		receivers.emplace_back( Link::Create( config, lua_object + ".receivers[" + std::to_string(i+1) + "]" ) );
	}

	MultiLink* ret = new MultiLink( senders, receivers );
	ret->setReadTimeout( read_timeout );
	return ret;
}


MultiLink::MultiLink( std::list<Link*> senders, std::list<Link*> receivers )
	: Link()
	, mBlocking( true )
	, mReadTimeout( 2000 )
	, mSenders( senders )
	, mReceivers( receivers )
{
	for ( Link* link : mReceivers ) {
		link->setBlocking( true );
	}
}


MultiLink::MultiLink( std::initializer_list<Link*> senders, std::initializer_list<Link*> receivers )
	: MultiLink( static_cast<std::list<Link*>>(senders), static_cast<std::list<Link*>>(receivers) )
{
}


MultiLink::~MultiLink()
{
}


int MultiLink::Connect()
{
	int ret = 0;

	for ( Link* link : mSenders ) {
		if ( link->Connect() != 0 ) {
			ret = 1;
		}
	}
	for ( Link* link : mReceivers ) {
		if ( link->Connect() != 0 ) {
			ret = 1;
		}
	}

	mConnected = ( not ret );
	return ret;
}


int MultiLink::setBlocking( bool blocking )
{
	mBlocking = blocking;
	return 0;
}


void MultiLink::setReadTimeout( uint32_t timeout )
{
	mReadTimeout = timeout;
}


void MultiLink::setRetriesCount( int retries )
{
	for ( Link* link : mSenders ) {
		link->setRetriesCount( retries );
	}
}


int MultiLink::retriesCount() const
{
	return mSenders.front()->retriesCount();
}


int32_t MultiLink::Channel()
{
	return mReceivers.front()->Channel();
}


int32_t MultiLink::Frequency()
{
	return mReceivers.front()->Frequency();
}


int32_t MultiLink::RxQuality()
{
	return mReceivers.front()->RxQuality();
}


int32_t MultiLink::RxLevel()
{
	return mReceivers.front()->RxLevel();
}


uint32_t MultiLink::fullReadSpeed()
{
	return 0;
}


int MultiLink::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	int32_t ret = 0;

	for ( Link* link : mSenders ) {
		int32_t r = link->Write( buf, len, ack, timeout );
		if ( r > ret ) {
			ret = r;
		}
	}

	return ret;
}


int MultiLink::Read( void* buf, uint32_t len, int32_t timeout )
{
	int ret = 0;
	int32_t hopping_timeout = 50;
	uint32_t time_base = Board::GetTicks();

	if ( mReceivers.size() == 0 ) {
		return -1;
	}

// 	if ( timeout > 0 ) {
// 		hopping_timeout = timeout / mReceivers.size();
// 	}
	if ( timeout == 0 ) {
		timeout = mReadTimeout;
	}

	do  {
		// First, try to receive using the first receiver
		Link* front = mReceivers.front();
		ret = front->Read( buf, len, hopping_timeout );
		// If good, then return now
		if ( ret > 0 ) {
			return ret;
		} else if ( ret == LINK_ERROR_TIMEOUT ) {
// 			gDebug() << front->Frequency() << " front timed out\n";
		}

		// Else, try with the others until one is responding
		for ( std::list<Link*>::iterator it = mReceivers.begin(); it != mReceivers.end(); it++ ) {
			if ( (*it) != front ) {
				ret = (*it)->Read( buf, len, hopping_timeout );
				// If good, put this receiver in front, and return now
				if ( ret > 0 ) {
					Link* tmp = mReceivers.front();
					mReceivers.front() = *it;
					*it = tmp;
					return ret;
				} else if ( ret == LINK_ERROR_TIMEOUT ) {
// 					gDebug() << (*it)->Frequency() << " timed out\n";
				} else {
// 					gDebug() << (*it)->Frequency() << " error " << ret << "\n";
				}
			}
		}

		if ( Board::GetTicks() - time_base >= (uint64_t)timeout * 1000llu ) {
			return LINK_ERROR_TIMEOUT;
		}
	} while ( mBlocking );

	return ret;
}
